/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#include <string>

#include "base/file_utilities.h"
#include "base/string_utilities.h"
#include "base/trackable.h"
#include "base/notifications.h"
#include "base/log.h"

#include "grts/structs.db.mgmt.h"
#include "wb_command_ui.h"
#include "wb_context_ui.h"

#include "wb_module.h"
#include "grt/clipboard.h"
#include "grt/plugin_manager.h"

#include "wb_overview.h"

#include "model/wb_component.h"

#include <mforms/toolbar.h>
#include <mforms/menubar.h>

using namespace bec;
using namespace wb;
using namespace base;

using std::max;
using std::min;

DEFAULT_LOG_DOMAIN(DOMAIN_COMMAND_HANDLING)

struct wb::ParsedCommand {
  std::string type;
  std::string name;
  std::string args;

  ParsedCommand(const ParsedCommand &other) : type(other.type), name(other.name), args(other.args) {
  }

  ParsedCommand(const std::string &command) {
    std::string::size_type p, q;

    p = command.find(':');
    if (p != std::string::npos) {
      type = command.substr(0, p);
      q = command.find(':', p + 1);
      if (q != std::string::npos) {
        name = command.substr(p + 1, q - (p + 1));
        args = command.substr(q + 1);
      } else
        name = command.substr(p + 1);
    } else
      type = command;
  }

  inline bool valid() const {
    return !type.empty() && !name.empty();
  }
  inline bool has_args() const {
    return !args.empty();
  }
};

CommandUI::CommandUI(WBContext *wb) : _wb(wb), _include_se(false) {
}

void CommandUI::load_data() {
  _include_se = _wb->is_commercial();

  _shortcuts = grt::ListRef<app_ShortcutItem>::cast_from(
    grt::GRT::get()->unserialize(base::makePath(_wb->get_datadir(), "data/shortcuts.xml")));
}

static bool match_context(const std::string &item_context, const std::string &current_context) {
  if (item_context == "" || item_context == WB_CONTEXT_GLOBAL)
    return true;

  if (item_context == current_context)
    return true;

  if (item_context == "*query") {
    if (current_context == WB_CONTEXT_QUERY)
      return true;
  } else if (item_context == "*model") {
    if (current_context == WB_CONTEXT_MODEL || current_context == WB_CONTEXT_EDITOR ||
        current_context == WB_CONTEXT_PHYSICAL_OVERVIEW)
      return true;
  }

  return false;
}

static bool filter_context_and_platform(const app_CommandItemRef &item, const std::string &context) {
  std::vector<std::string> plats(base::split(item->platform(), ","));

  if (!plats.empty()) {
    std::string platform;
#ifdef _MSC_VER
    platform = "windows";
#elif defined(__APPLE__)
    platform = "macosx";
#else
    platform = "linux";
#endif

    if (std::find(plats.begin(), plats.end(), platform) == plats.end())
      return false;
  }

  return match_context(item->context(), context);
}

void CommandUI::update_item_state(const app_CommandItemRef &item, const wb::ParsedCommand &cmd,
                                  mforms::MenuItem *menu_item) {
  bool state = validate_command_item(item, cmd);

  if (state)
    menu_item->set_enabled(true);
  else
    menu_item->set_enabled(false);
}

void CommandUI::update_item_state(const app_ToolbarItemRef &item, const ParsedCommand &cmd,
                                  mforms::ToolBarItem *tb_item) {
  bool state = validate_command_item(item, cmd);

  if (state)
    tb_item->set_enabled(true);
  else
    tb_item->set_enabled(false);
}

static void add_option_value_to_list(WBComponent *comp, const std::string &name, std::list<std::string> *result) {
  result->push_back(comp->get_command_option_value(name));
}

bool CommandUI::validate_command_item(const app_CommandItemRef &item, const wb::ParsedCommand &cmd) {
  std::string name(item->name());

  if (name == "exit_application")
    return true;

  if (!cmd.valid())
    return true;

  if (cmd.type == "builtin")
    return validate_builtin_command(cmd.name);
  else if (cmd.type == "option") {
    std::list<std::string> results;

    _wb->foreach_component(std::bind(add_option_value_to_list, std::placeholders::_1, cmd.name, &results));

    return true;
  }

  if (cmd.type == "plugin") {
    app_PluginRef plugin(_wb->get_plugin_manager()->get_plugin(cmd.name));

    if (plugin.is_valid()) {
      bec::ArgumentPool argpool;
      _wb->update_plugin_arguments_pool(argpool);
      argpool["app.PluginInputDefinition:string"] = grt::StringRef(cmd.args);

      return bec::GRTManager::get()->check_plugin_runnable(plugin, argpool);
    }
  } else if (cmd.type == "call") {
    std::string module, function;
    if (base::partition(cmd.name, ".", module, function)) {
      grt::Module *m = grt::GRT::get()->get_module(module);
      if (m && m->has_function(function))
        return true;
      logInfo("Invalid function %s.%s\n", module.c_str(), function.c_str());
      return false;
    }
  }

  return true;
}

//--------------------------------------------------------------------------------------------------

bool CommandUI::validate_plugin_command(app_PluginRef plugin) {
  bool result = false;
  if (plugin.is_valid()) {
    if (bec::GRTManager::get()->check_plugin_runnable(plugin, _argpool))
      result = true;
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

// Keyboard and Shortcut Handling

static bool parse_key(const std::string &key, mdc::KeyInfo &info) {
  static struct {
    mdc::KeyCode code;
    std::string name;
  } keys[] = {{mdc::KShift, "Shift"},
              {mdc::KAlt, "Alt"},
              {mdc::KControl, "Control"},
              {mdc::KOption, "Option"},
              {mdc::KCommand, "Command"},
#ifdef __APPLE__
              {mdc::KCommand, "Modifier"},
#else
              {mdc::KControl, "Modifier"},
#endif

              {mdc::KF1, "F1"},
              {mdc::KF2, "F2"},
              {mdc::KF3, "F3"},
              {mdc::KF4, "F4"},
              {mdc::KF5, "F5"},
              {mdc::KF6, "F6"},
              {mdc::KF7, "F7"},
              {mdc::KF8, "F8"},
              {mdc::KF9, "F9"},
              {mdc::KF10, "F10"},
              {mdc::KF11, "F11"},
              {mdc::KF12, "F12"},

              {mdc::KLeft, "Left"},
              {mdc::KRight, "Right"},
              {mdc::KUp, "Up"},
              {mdc::KDown, "Down"},
              {mdc::KHome, "Home"},
              {mdc::KEnd, "End"},
              {mdc::KPageUp, "PageUp"},
              {mdc::KPageDown, "PageDown"},

              {mdc::KInsert, "Insert"},
              {mdc::KDelete, "Delete"},
              {mdc::KBackspace, "Backspace"},
              {mdc::KBackspace, "BackSpace"},

              {mdc::KEscape, "Escape"},
              {mdc::KTab, "Tab"},
              {mdc::KEnter, "Enter"},
              {mdc::KSpace, "Space"},
              {mdc::KPlus, "Plus"},
              {mdc::KMinus, "Minus"},
              {mdc::KNone, ""}};

  info.keycode = mdc::KNone;

  if (key.empty())
    return false;

  for (size_t i = 0; !keys[i].name.empty(); ++i)
    if (keys[i].name == key) {
      info.keycode = keys[i].code;
      break;
    }

  info.string = key;
#ifndef _MSC_VER
  if (info.string.length() == 1)
    info.string = tolower(info.string[0]);
#endif

  return true;
}

static bool parse_shortcut(const std::string &shortcut, mdc::KeyInfo &key, mdc::EventState &mods) {
  if (shortcut.empty())
    return false;

  std::vector<std::string> parts = base::split(shortcut, "+");
  mods = mdc::SNone;

  for (ssize_t c = parts.size() - 1, i = 0; i < c; i++) {
    std::string mod = parts[i];
    if (mod == "control")
      mods = mods | mdc::SControlMask;
    else if (mod == "alt")
      mods = mods | mdc::SAltMask;
    else if (mod == "shift")
      mods = mods | mdc::SShiftMask;
    else if (mod == "option")
      mods = mods | mdc::SOptionMask;
    else if (mod == "command")
      mods = mods | mdc::SCommandMask;
#ifdef __APPLE__
    else if (mod == "modifier")
      mods = mods | mdc::SCommandMask;
#else
    else if (mod == "modifier")
      mods = mods | mdc::SControlMask;
#endif
    else
      return false;
  }

  if (!parse_key(parts.back(), key))
    return false;

  return true;
}

void CommandUI::append_shortcut_items(const grt::ListRef<app_ShortcutItem> &plist, const std::string &context,
                                      std::vector<WBShortcut> *items) {
  std::string platform;

  if (!plist.is_valid())
    return;

  for (size_t c = plist.count(), i = 0; i < c; i++) {
    app_ShortcutItemRef shortcut(plist[i]);
    std::string item_context;

    if (!filter_context_and_platform(shortcut, context))
      continue;

    // filter by context
    if (shortcut->context().is_valid())
      item_context = shortcut->context();
    if (item_context.empty())
      item_context = WB_CONTEXT_GLOBAL;

    if (item_context == WB_CONTEXT_GLOBAL) {
      // if item is global, skip if there's another item with same name for current context
      if (context != WB_CONTEXT_GLOBAL) {
        bool dupe = false;

        for (size_t j = 0; j < c; j++) {
          if (j != i && *shortcut->platform() == *plist[j]->platform() && *shortcut->name() != "" &&
              *plist[j]->name() == *shortcut->name()) {
            dupe = true;
            break;
          }
        }
        if (dupe)
          continue;
      }
    }

    WBShortcut item;

    item.name = shortcut->name();
    item.command = shortcut->command();
    item.shortcut = shortcut->shortcut();

    if (!parse_shortcut(shortcut->shortcut(), item.key, item.modifiers)) {
      item.key.keycode = mdc::KNone;
      item.modifiers = mdc::SNone;
    }

    items->push_back(item);
  }
}

std::vector<WBShortcut> CommandUI::get_shortcuts_for_context(const std::string &context) {
  std::vector<WBShortcut> shortcuts;

  append_shortcut_items(_shortcuts, context, &shortcuts);

  // view specific contextual shortcuts
  if (context == WB_CONTEXT_MODEL) {
    grt::ListRef<app_ShortcutItem> model_items;

    _wb->foreach_component(std::bind(&CommandUI::append_shortcut_items, this,
                                     std::bind(&WBComponent::get_shortcut_items, std::placeholders::_1), context,
                                     &shortcuts));
  }

  return shortcuts;
}

//--------------------------------------------------------------------------------
// Menu Management

void CommandUI::add_recent_menu(mforms::MenuItem *parent) {
  grt::StringListRef strlist(_wb->get_root()->options()->recentFiles());

  mforms::MenuItem *item;
  if (strlist.count() == 0) {
    // We need at least one entry in the Open Recent menu
    // so the menu_will_show function gets called.
    item = mforms::manage(new mforms::MenuItem("", mforms::SeparatorMenuItem));
    item->set_name("Separator");
    parent->add_item(item);
    return;
  }
  for (size_t c = min(strlist.count(), (size_t)10), i = 0; i < c; i++) {
    std::string caption;
    if (i < 9) {
      caption = strfmt("%li %s", (long)i + 1, strlist.get(i).c_str());

#if !defined(_MSC_VER) && !defined(__APPLE__)
      caption = "_" + base::replaceString(caption, "_", "__");
#endif
    } else if (i == 9) {
      caption = strfmt("0 %s", strlist.get(i).c_str());
#if !defined(_MSC_VER) && !defined(__APPLE__)
      caption = "_" + base::replaceString(caption, "_", "__");
#endif
    } else {
      caption = strfmt(" %s", strlist.get(i).c_str());
#if !defined(_MSC_VER) && !defined(__APPLE__)
      caption = base::replaceString(caption, "_", "__");
#endif
    }

    item = mforms::manage(new mforms::MenuItem(caption));

    item->set_name("Open Recent File " + std::to_string(i + 1));
    item->setInternalName("wb.file.openRecentModel:" + std::to_string(i + 1));

    scoped_connect(item->signal_clicked(), std::bind(&WBContext::open_recent_document, _wb, (int)i + 1));
    parent->add_item(item);
  }
}

void CommandUI::add_plugins_menu_items(mforms::MenuItem *parent, const std::string &group) {
  std::vector<app_PluginRef> plugins(_wb->get_plugin_manager()->get_plugins_for_group(group));

  for (std::vector<app_PluginRef>::const_iterator iter = plugins.begin(); iter != plugins.end(); ++iter) {
    mforms::MenuItem *item = mforms::manage(new mforms::MenuItem((*iter)->caption()));
    item->set_name((*iter)->accessibilityName());
    item->setInternalName(std::string("plugin:") + (*iter)->name().c_str());
    item->add_validator(std::bind(&CommandUI::validate_plugin_command, this, *iter));
    item->validate();
    scoped_connect(
      item->signal_clicked(),
      std::bind((void (CommandUI::*)(const std::string &)) & CommandUI::activate_command, this, item->getInternalName()));
    parent->add_item(item);
  }
}

void CommandUI::add_plugins_menu(mforms::MenuItem *parent, const std::string &context) {
  // get sub-groups for the plugins menu

  grt::ListRef<app_PluginGroup> groups(_wb->get_root()->registry()->pluginGroups());

  for (size_t c = groups.count(), i = 0; i < c; i++) {
    app_PluginGroupRef group(groups.get(i));
    std::string category = group->category();

    if (context != category && category != "Others") {
      if (category == "SQLEditor" && context != WB_CONTEXT_QUERY)
        continue;
      else if ((category == "Model" || category == "Catalog" || category == "Database") &&
               context != WB_CONTEXT_PHYSICAL_OVERVIEW && context != WB_CONTEXT_MODEL)
        continue;
    }

    if (g_str_has_prefix(group->name().c_str(), "Menu/") && group->plugins().count() > 0) {
      std::string name = group->name();
      size_t pos = name.rfind('/');
      if (pos != std::string::npos)
          name = name.substr(pos + 1);
      
      mforms::MenuItem *item = mforms::manage(new mforms::MenuItem(name));
      item->set_name(group->accessibilityName());
      item->setInternalName(std::string("plugin:") + group->name().c_str());
      parent->add_item(item);

      add_plugins_menu_items(item, group->name());
    }
  }

  add_plugins_menu_items(parent, "Others/Menu/Ungrouped");

  if (parent->get_subitems().empty()) {
    mforms::MenuItem *item = mforms::manage(new mforms::MenuItem("No Extra Plugins"));
    item->set_enabled(false);
    parent->add_item(item);
  }
}

void CommandUI::add_scripts_menu(mforms::MenuItem *parent) {
  try {
    parent->add_validator([parent]() {
      return !parent->get_subitems().empty();
    });
    std::list<std::string> pyfiles =
      base::scan_for_files_matching(base::makePath(bec::GRTManager::get()->get_user_script_path(), "*.py"));
    std::vector<std::string> files;

    std::copy(pyfiles.begin(), pyfiles.end(), std::back_inserter(files));
    std::sort(files.begin(), files.end());

    for (std::vector<std::string>::const_iterator f = files.begin(); f != files.end(); ++f)
      parent->add_item_with_title(base::basename(*f), std::bind(&WBContext::run_script_file, _wb, *f), "", "");
  } catch (...) {
  }
}

void CommandUI::add_menu_items_for_context(const std::string &context, mforms::MenuItem *parent,
                                           const app_MenuItemRef &menu) {
  // special menu handling
  if (menu->name() == "open_recent")
    add_recent_menu(parent);
  else if (menu->name() == "run_script_list")
    add_scripts_menu(parent);
  else {
    std::set<std::string> added_menu_items;

    grt::ListRef<app_MenuItem> plist(menu->subItems());
    auto activate_slot = [this](const std::string &str) { activate_command(str); };

    for (size_t c = plist.count(), i = 0; i < c; i++) {
      app_MenuItemRef mitem(plist[i]);
      std::string item_context;

      if (mitem->itemType() != "separator" && added_menu_items.find(mitem->name()) != added_menu_items.end())
        continue;

      if (!filter_context_and_platform(mitem, context))
        continue;

      // filter by context
      if (mitem->context().is_valid())
        item_context = mitem->context();
      if (item_context.empty())
        item_context = WB_CONTEXT_GLOBAL;

      const mforms::MenuItemType type = (mitem->itemType() != "check" && mitem->itemType() != "radio")
                                          ? mforms::NormalMenuItem
                                          : mforms::CheckedMenuItem;

      if (item_context == WB_CONTEXT_GLOBAL) {
        // if item is global, skip if there's another item with same name for current context
        if (context != WB_CONTEXT_GLOBAL && mitem->itemType() != "separator") {
          bool dupe = false;

          for (size_t j = 0; j < c; j++) {
            if (!filter_context_and_platform(plist[j], context))
              continue;

            app_MenuItemRef jitem(plist[j]);

            if (j != i && jitem->name() == mitem->name() && // filter_context_platform(plist[j], context))
                (*jitem->context() == context || *jitem->context() == WB_CONTEXT_GLOBAL)) {
              dupe = true;
              break;
            }
          }
          if (dupe)
            continue;
        }
      }
      // redundant else if (!match_context(item_context, context))
      //  continue; // skip items in different context

      mforms::MenuItem *item = 0;

      ParsedCommand cmd(mitem->command());

      // When getting a caption for the menu entry make sure access letter markup is properly adjusted for each
      // platform.
      // get_command_item_caption constructs a caption from a command and an item name. It has to take
      // care itself to properly handle this. All other cases are handled here.
      if (mitem->itemType() == "cascade") {
        if (base::hasSuffix(mitem.id(), "/SE") && !_include_se)
          continue;
        item = mforms::manage(new mforms::MenuItem(mitem->caption(), type));
        item->set_name(mitem->accessibilityName());
        item->setInternalName(mitem->name());
        parent->add_item(item);
        update_item_state(mitem, cmd, item);

        add_menu_items_for_context(context, item, mitem);
      } else if (mitem->itemType() == "separator") {
        if (base::hasSuffix(mitem.id(), "/SE") && !_include_se)
          continue;
        item = mforms::manage(new mforms::MenuItem("", mforms::SeparatorMenuItem));
        item->set_name(mitem->accessibilityName());
        item->setInternalName(mitem->name());
        parent->add_item(item);
      } else {
        std::string caption = mitem->caption();

        bool enabled = true;
        std::function<bool()> validator;
        if (cmd.type == "plugin") {
          app_PluginRef plugin(_wb->get_plugin_manager()->get_plugin(cmd.name));
          if (caption.empty() && plugin.is_valid())
            caption = plugin->caption();
          if (caption.empty())
            caption = mitem->command();

          // skip if the plugin is invalid
          if (!plugin.is_valid()) {
            if (!base::hasSuffix(mitem.id(), "/SE") || _include_se)
              logWarning("Plugin item %s was not found\n", cmd.name.c_str());
            // if plugin is missing, check if it's supposed to be there
            if (!_include_se || !base::hasSuffix(mitem.id(), "/SE"))
              continue;
            enabled = false;
#ifdef ENABLE_DEBUG
            caption.append(" (invalid)");
#endif
          } else
            validator = std::bind(&CommandUI::validate_plugin_command, this, plugin);
        } else if (cmd.type == "call") {
          std::string module, function;
          if (base::partition(cmd.name, ".", module, function)) {
            grt::Module *m = grt::GRT::get()->get_module(module);
            if (m && m->has_function(function))
              enabled = true;
            else {
              if (!base::hasSuffix(mitem.id(), "/SE") || _include_se)
                logWarning("Module function %s was not found\n", cmd.name.c_str());
              // if plugin is missing, check if it's supposed to be there
              if (!_include_se || !base::hasSuffix(mitem.id(), "/SE"))
                continue;

              enabled = false;
#ifdef ENABLE_DEBUG
              caption.append(" (invalid)");
#endif
            }
          }
        } else if (cmd.type == "builtin") {
          if (_builtin_commands.find(cmd.name) != _builtin_commands.end()) {
            if (_builtin_commands[cmd.name].validate)
              validator = _builtin_commands[cmd.name].validate;
          }
        } else {
          grt::GRT::get()->send_warning("Invalid menu item command: " + *mitem->command() + "\n");
          continue;
        }

        if (base::hasSuffix(mitem.id(), "/SE") && !_include_se)
          continue;

        item = mforms::manage(new mforms::MenuItem(caption, type));
        parent->add_item(item);
        item->set_name(mitem->accessibilityName());
        item->setInternalName(cmd.args.empty() ? cmd.name : cmd.name + ":" + cmd.args);
        if (!mitem->shortcut().empty())
          item->set_shortcut(mitem->shortcut());
        scoped_connect(item->signal_clicked(), std::bind(activate_slot, mitem->command()));
        // update_item_state(mitem, cmd, item);
        if (!enabled)
          item->set_enabled(false);
        else {
          item->set_enabled(true);
          if (validator)
            item->add_validator(validator);
        }
      }

#ifdef _MSC_VER
      item->set_title(base::replaceString(item->get_title(), "_", "&"));
#elif defined(__APPLE__)
      // remove _ in osx
      item->set_title(base::replaceString(item->get_title(), "_", ""));
#endif

      added_menu_items.insert(mitem->name());
    }
  }

  // Add plugin items after other items defined in the XML
  if (menu->name() == "plugins")
    add_plugins_menu(parent, context);
}

static void connect_validate_to_signal(boost::signals2::signal<void()> &validate_edit_menu_items,
                                       std::vector<mforms::MenuItem *> &items) {
  for (std::vector<mforms::MenuItem *>::iterator i = items.begin(); i != items.end(); ++i) {
    (*i)->scoped_connect(&validate_edit_menu_items, std::bind(&mforms::MenuItem::validate, *i));
    connect_validate_to_signal(validate_edit_menu_items, (*i)->get_subitems());
  }
}

void CommandUI::menu_will_show(mforms::MenuItem *item) {
  if (item->getInternalName() == "open_recent") {
    item->remove_all();
    add_recent_menu(item);
  } else if (item->getInternalName() == "edit")
    revalidate_edit_menu_items();
}

mforms::MenuBar *CommandUI::create_menubar_for_context(const std::string &context) {
  mforms::MenuBar *menubar = new mforms::MenuBar();

  menubar->signal_will_show()->connect(std::bind(&CommandUI::menu_will_show, this, std::placeholders::_1));

  grt::ListRef<app_MenuItem> main_menu(grt::ListRef<app_MenuItem>::cast_from(
    grt::GRT::get()->unserialize(base::makePath(_wb->get_datadir(), "data/main_menu.xml"))));

  for (size_t c = main_menu.count(), i = 0; i < c; i++) {
    app_MenuItemRef mitem(main_menu[i]);
    std::string item_context;

    if (mitem->context().is_valid())
      item_context = mitem->context();

    if (!match_context(item_context, context))
      continue;

    std::string caption(mitem->caption());
#ifdef _MSC_VER
    // turn mnemonic indicator from _ into & for windows
    caption = base::replaceString(caption, "_", "&");
#elif defined(__APPLE__)
    // remove _ in osx
    caption = base::replaceString(caption, "_", "");
#endif

    mforms::MenuItem *item = mforms::manage(new mforms::MenuItem(caption));
    item->set_name(mitem->accessibilityName());
    item->setInternalName(mitem->name());
    menubar->add_item(item);

    add_menu_items_for_context(context, item, mitem);

    if (mitem->name() == "edit")
      connect_validate_to_signal(_validate_edit_menu_items, item->get_subitems());
  }
  main_menu->reset_references();

  return menubar;
}

//--------------------------------------------------------------------------------
// Toolbar Management

mforms::ToolBar *CommandUI::create_toolbar(const std::string &toolbar_file) {
  return create_toolbar(toolbar_file, [this](const std::string &str) { activate_command(str); });
}

mforms::ToolBar *CommandUI::create_toolbar(const std::string &toolbar_file,
                                           const std::function<void(std::string)> &activate_slot) {
  app_ToolbarRef toolbar(
    app_ToolbarRef::cast_from(grt::GRT::get()->unserialize(bec::GRTManager::get()->get_data_file_path(toolbar_file))));

  grt::ListRef<app_ToolbarItem> plist(toolbar->items());
  mforms::ToolBar *tbar = new mforms::ToolBar();

  // std::vector<WBShortcut> shortcuts(get_shortcuts_for_context(context));

  for (size_t c = plist.count(), i = 0; i < c; i++) {
    app_ToolbarItemRef titem(plist[i]);

    if (!filter_context_and_platform(titem, ""))
      continue;

    // filter by context
    if (titem->itemType() != "separator") {
      bool dupe = false;

      for (size_t j = 0; j < c; j++) {
        if (j != i && *titem->name() != "" && *plist[j]->name() == *titem->name()) {
          dupe = true;
          break;
        }
      }
      if (dupe)
        continue;
    }

    mforms::ToolBarItem *tbitem = 0;

    if (titem->itemType() == "action") {
      tbitem = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
      tbitem->set_name(titem->accessibilityName());
      tbitem->setInternalName(titem->name());
      std::string s = IconManager::get_instance()->get_icon_path(titem->icon());
      tbitem->set_icon(s);

      scoped_connect(tbitem->signal_activated(), std::bind(activate_slot, titem->command()));
    } else if (titem->itemType() == "toggle") {
      tbitem = mforms::manage(new mforms::ToolBarItem(mforms::ToggleItem));
      tbitem->set_name(titem->accessibilityName());
      tbitem->setInternalName(titem->name());
      std::string s = IconManager::get_instance()->get_icon_path(titem->icon());
      tbitem->set_icon(s);

      if (!(titem->altIcon()).empty())
        s = IconManager::get_instance()->get_icon_path(titem->altIcon());
      tbitem->set_alt_icon(s);

      scoped_connect(tbitem->signal_activated(), std::bind(activate_slot, titem->command()));
    } else if (titem->itemType() == "segmentedToggle") {
      tbitem = mforms::manage(new mforms::ToolBarItem(mforms::SegmentedToggleItem));
      tbitem->set_name(titem->accessibilityName());
      tbitem->setInternalName(titem->name());
      tbitem->set_icon(IconManager::get_instance()->get_icon_path(titem->icon()));
      if (strlen(titem->altIcon().c_str()) > 0)
        tbitem->set_alt_icon(IconManager::get_instance()->get_icon_path(titem->altIcon()));
      tbitem->set_checked(titem->initialState() != 0);
      tbitem->signal_activated()->connect(std::bind(activate_slot, titem->command()));
      tbar->add_item(tbitem);
      continue;
    } else if (titem->itemType() == "label") {
      tbitem = mforms::manage(new mforms::ToolBarItem(mforms::LabelItem));
      tbitem->set_name(titem->accessibilityName());
      tbitem->setInternalName(titem->name());
      tbitem->set_text(titem->tooltip());
      tbar->add_item(tbitem);
      continue;
    } else if (titem->itemType() == "selector") {
      tbitem = mforms::manage(new mforms::ToolBarItem(mforms::SelectorItem));
      tbitem->set_name(titem->accessibilityName());
      tbitem->setInternalName(titem->name());

      scoped_connect(tbitem->signal_activated(), std::bind(activate_slot, titem->command()));
    } else if (titem->itemType() == "imagebox") {
      tbitem = mforms::manage(new mforms::ToolBarItem(mforms::ImageBoxItem));
      tbitem->set_icon(IconManager::get_instance()->get_icon_path(titem->icon()));
      tbar->add_item(tbitem);
      continue;
    } else if (titem->itemType() == "search") {
      tbitem = mforms::manage(new mforms::ToolBarItem(mforms::SearchFieldItem));

      scoped_connect(tbitem->signal_activated(), std::bind(activate_slot, titem->command()));
    } else if (titem->itemType() == "expander") {
      tbitem = mforms::manage(new mforms::ToolBarItem(mforms::ExpanderItem));
      tbar->add_item(tbitem);
      continue;
    } else if (titem->itemType() == "separator") {
      tbitem = mforms::manage(new mforms::ToolBarItem(mforms::SeparatorItem));
      tbar->add_item(tbitem);
      continue;
    } else {
      logWarning("invalid toolbar item type %s", titem->itemType().c_str());
      continue;
    }
    /*
    std::string shortcut;
    for (std::vector<WBShortcut>::const_iterator iter= shortcuts.begin(); iter != shortcuts.end(); ++iter)
    {
      if (iter->command == item.command)
      {
        shortcut= iter->shortcut;
        break;
      }
    }*/

    std::string tooltip;
    tooltip = titem->tooltip();
    //    if (shortcut.empty())
    tbitem->set_tooltip(tooltip);
    //    else
    //      item.tooltip= strfmt("%s (Quick key - press %s)", tooltip.c_str(), shortcut.c_str());

    ParsedCommand cmd(titem->command());
    tbitem->set_name(titem->accessibilityName());
    tbitem->setInternalName(titem->name());

    if (cmd.type == "plugin") {
      app_PluginRef plugin(_wb->get_plugin_manager()->get_plugin(cmd.name));

      // skip if the plugin is invalid
      if (!plugin.is_valid()) {
        // continue;
        tbitem->set_enabled(false);
      } else {
        // tbitem->set_enabled(validate_command_item(titem, cmd) != ItemDisabled);

        if (tooltip.empty())
          tbitem->set_tooltip(plugin->description());
      }
    } else if (cmd.type == "call") {
    } else if (cmd.type == "builtin") {
      // tbitem->set_enabled(validate_command_item(titem, cmd) != ItemDisabled);
    } else if (cmd.type == "option")
      tbitem->set_enabled(validate_command_item(titem, cmd));
    else {
      grt::GRT::get()->send_warning("Invalid toolbar item command: " + *titem->command() + "\n");
      delete tbitem;
      continue;
    }

    tbar->add_item(tbitem);
  }

  toolbar->reset_references();

  return tbar;
}

//--------------------------------------------------------------------------------

#include "model/wb_model_diagram_form.h"
static bool has_active_view(WBContext *wb) {
  return (dynamic_cast<ModelDiagramForm *>(wb->get_active_form()) != 0);
}

//--------------------------------------------------------------------------------------------------

void CommandUI::add_frontend_commands(const std::list<std::string> &commands) {
  for (std::list<std::string>::const_iterator iter = commands.begin(); iter != commands.end(); ++iter) {
    // hack
    if (iter->compare("diagram_size") == 0 || iter->compare("wb.page_setup") == 0)
      add_builtin_command(*iter, std::bind(_wb->_frontendCallbacks->perform_command, *iter),
                          std::bind(has_active_view, _wb));
    else
      add_builtin_command(*iter, std::bind(_wb->_frontendCallbacks->perform_command, *iter));
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Convenience method to remove registered front end commands.
 */
void wb::CommandUI::remove_frontend_commands(const std::list<std::string> &commands) {
  for (std::list<std::string>::const_iterator iter = commands.begin(); iter != commands.end(); ++iter)
    remove_builtin_command(*iter);
}

//--------------------------------------------------------------------------------------------------

void CommandUI::remove_builtin_command(const std::string &name) {
  std::map<std::string, BuiltinCommand>::iterator iter = _builtin_commands.find(name);
  if (iter != _builtin_commands.end())
    _builtin_commands.erase(iter);
}

void CommandUI::add_builtin_command(const std::string &name, const std::function<void()> &slot,
                                    const std::function<bool()> &validate) {
  BuiltinCommand cmd;

  cmd.execute = slot;
  cmd.validate = validate;

  if (_builtin_commands.find(name) != _builtin_commands.end())
    logWarning("%s built-in command is being overwritten", name.c_str());

  _builtin_commands[name] = cmd;
}

bool CommandUI::execute_builtin_command(const std::string &name) {
  if (_builtin_commands.find(name) != _builtin_commands.end()) {
    _builtin_commands[name].execute();
    return true;
  }
  return false;
}

bool CommandUI::validate_builtin_command(const std::string &name) {
  if (_builtin_commands.find(name) != _builtin_commands.end()) {
    if (_builtin_commands[name].validate)
      return _builtin_commands[name].validate();
    return true;
  }
  return false;
}

//--------------------------------------------------------------------------------

bool CommandUI::activate_command(const std::string &command, bec::ArgumentPool argpool) {
  try {
    ParsedCommand cmdparts(command);

    if (cmdparts.type == "plugin") {
      _wb->execute_plugin(cmdparts.name, argpool);
      return true;
    } else if (cmdparts.type == "call") {
      std::string module, function;
      if (base::partition(cmdparts.name, ".", module, function)) {
        grt::GRT::get()->call_module_function(module, function, grt::BaseListRef(true));
        return true;
      }
    } else if (cmdparts.type == "builtin") {
      execute_builtin_command(cmdparts.name);
      return true;
    } else
      throw std::runtime_error("Unhandled command type " + cmdparts.type);
  } catch (grt::grt_runtime_error &error) {
    _wb->show_exception(command, error);
  } catch (const std::exception &e) {
    _wb->show_exception(command, e);
  }
  return false;
}

static const std::vector<std::string> clipboardCommands = {"builtin:paste", "builtin:copy", "builtin:delete"};

void CommandUI::activate_command(const std::string &command) {
  if (command.empty() || !_wb->user_interaction_allowed())
    return;

  // Finish any ongoing editing task before starting a new one.
  
  if (std::find(clipboardCommands.begin(), clipboardCommands.end(), command) == clipboardCommands.end()) {
    _wb->request_refresh(RefreshType::RefreshFinishEdits, "");
    _wb->flush_idle_tasks(true);
  }

  ParsedCommand cmdparts(command);

  if (!cmdparts.valid())
    return;

  try {
    //    bec::UIForm *active_form= get_active_form();

    // XXX: allow parameters also for built-in commands (e.g. for search).
    if (cmdparts.type == "builtin") {
      if (!execute_builtin_command(cmdparts.name))
        throw std::runtime_error(strfmt("Unrecognized command %s", cmdparts.name.c_str()));
    } else if (cmdparts.type == "plugin") {
      bec::ArgumentPool args;

      if (cmdparts.has_args()) {
        args["app.PluginInputDefinition:string"] = grt::StringRef(cmdparts.args);
        _wb->execute_plugin(cmdparts.name, args);
      } else {
        _wb->execute_plugin(cmdparts.name, args);
      }
    } else if (cmdparts.type == "call") {
      std::string module, function;
      if (base::partition(cmdparts.name, ".", module, function))
        grt::GRT::get()->call_module_function(module, function, grt::BaseListRef(true));
    }
  } catch (grt::grt_runtime_error &error) {
    std::string caption(command);
    app_PluginRef plugin = _wb->get_plugin_manager()->get_plugin(cmdparts.name);
    if (plugin.is_valid())
      caption = plugin->caption();
    _wb->show_exception(caption, error);
  } catch (const std::exception &e) {
    _wb->show_exception(command, e);
  }
}

//--------------------------------------------------------------------------------------------------

void CommandUI::revalidate_menu_bar(mforms::MenuBar *menu) {
  // XXX: the arg pool can hold a reference to a grt value representing an editor and what not.
  //      So keeping that around can prevent freeing such an editor.
  //      Hence this must be redesigned not to rely on a global argpool.
  _argpool.clear();
  _wb->update_plugin_arguments_pool(_argpool);
  _argpool["app.PluginInputDefinition:string"] = grt::StringRef("");
  menu->validate();
}

void CommandUI::revalidate_edit_menu_items() {
  _argpool.clear();
  _wb->update_plugin_arguments_pool(_argpool);
  _argpool["app.PluginInputDefinition:string"] = grt::StringRef("");

  if (mforms::Utilities::in_main_thread())
    _validate_edit_menu_items();
  else
    bec::GRTManager::get()->run_once_when_idle(std::bind(&CommandUI::revalidate_edit_menu_items, this));
  // mforms::Utilities::perform_from_main_thread((std::bind(&CommandUI::revalidate_edit_menu_items, this), (void*)0));

  // NOTE : using perform_from_main_thread causes a _grtm reference on the BaseEditor to to get lost in the process,
  //        this causes the application to crash while attempting to create AutoUndoUpdate objects.
  //        Changed to run_once_when_idle to fix this issue
}

void CommandUI::clearBuildInCommands(){
  _builtin_commands.clear();
}
