/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "base/log.h"
#include "base/string_utilities.h"

#include "plugin_manager.h"
#include "editor_base.h"
#include "grt_manager.h"

#include "interfaces/plugin.h"

DEFAULT_LOG_DOMAIN("plugins")

using namespace bec;
using namespace grt;

static std::string get_args_hash(const grt::BaseListRef &list) {
  std::string hash;

  for (size_t c = list.count(), i = 0; i < c; i++) {
    switch (list.get(i).type()) {
      case ObjectType:
        hash += grt::ObjectRef::cast_from(list.get(i)).id();
        break;
      case ListType:
        hash += get_args_hash(grt::BaseListRef::cast_from(list.get(i)));
        break;
      default:
        hash += list.get(i).toString();
        break;
    }
  }

  return hash;
}

PluginManagerImpl::PluginManagerImpl(grt::CPPModuleLoader *loader) : superclass(loader) {
  InterfaceImplBase::Register<PluginInterfaceImpl>();
}

/**
 ****************************************************************************
 * @brief Sets the GRT tree path where the plugin registry is located
 *
 * @param plugins_path
 * @param groups_path
 ****************************************************************************
 */
void PluginManagerImpl::set_registry_paths(const std::string &plugins_path, const std::string &groups_path) {
  _registry_path = plugins_path;
  _group_registry_path = groups_path;
}

bool PluginManagerImpl::check_plugin_validity(const app_PluginRef &plugin, grt::Module *module) {
  if (plugin->pluginType() == GUI_PLUGIN_TYPE) {
    // not much that can be tested here, maybe check if the dll actually exists
    return true;
  } else if (plugin->pluginType() == STANDALONE_GUI_PLUGIN_TYPE || plugin->pluginType() == NORMAL_PLUGIN_TYPE) {
    // check if the module matches and the function exists
    if (plugin->moduleName() != module->name()) {
      logWarning("Plugin '%s' from module %s declares moduleName() as '%s', which doesn't match the module it belongs to.\n",
        plugin->name().c_str(), module->name().c_str(), plugin->moduleName().c_str());
      return false;
    }

    {
      std::string f = plugin->moduleFunctionName();
      if (!module->has_function(f)) {
        logWarning("Plugin '%s' from module %s has invalid moduleFunctionName '%s'.\n",
          plugin->name().c_str(),
          module->name().c_str(), f.c_str());
        return false;
      }
    }
    return true;
  } else if (plugin->pluginType() == INTERNAL_PLUGIN_TYPE) {
    return true;
  } else if (0 == (*plugin->pluginType()).find(CUSTOM_PLUGIN_TYPE)) {
    return true;
  } else {
    logWarning("Plugin '%s' from module %s has invalid type '%s'.\n",
      plugin->name().c_str(), module->name().c_str(),
      plugin->pluginType().c_str());
  }
  return false;
}

void PluginManagerImpl::set_plugin_enabled(const app_PluginRef &plugin, bool flag) {
  grt::StringListRef disabled_list(get_disabled_plugin_names());
  size_t idx = disabled_list.get_index(plugin->name());

  if (flag && idx != grt::BaseListRef::npos) {
    disabled_list.remove(idx);
    if (plugin->groups().count() == 0)
      add_plugin_to_group(plugin, "Others/Menu/Ungrouped");
    else {
      for (size_t d = plugin->groups().count(), j = 0; j < d; j++)
        add_plugin_to_group(plugin, plugin->groups()[j]);
    }
  } else if (!flag && idx == grt::BaseListRef::npos) {
    disabled_list.insert(plugin->name());
    // remove the plugin from all groups
    grt::ListRef<app_PluginGroup> groups(get_plugin_groups());
    for (size_t c = groups.count(), i = 0; i < c; i++)
      groups[i]->plugins().remove_value(plugin);
  }
}

bool PluginManagerImpl::plugin_enabled(const std::string &plugin_name) {
  grt::StringListRef names(get_disabled_plugin_names());
  if (names.get_index(plugin_name) == grt::BaseListRef::npos)
    return true;
  return false;
}

grt::StringListRef PluginManagerImpl::get_disabled_plugin_names() {
  std::string disabled_path(_registry_path);
  base::pop_path_back(disabled_path);
  base::pop_path_back(disabled_path);
  disabled_path.append("/options/disabledPlugins");
  return grt::StringListRef::cast_from(grt::GRT::get()->get(disabled_path));
}

/**
 ****************************************************************************
 * @brief Refreshes the list of plugins from the list of modules and DLLs
 *
 * Scans the list of registered modules for plugins.
 * Modules exporting plugins must implement PluginInterface.
 * The list of scanned plugins is stored in the GRT tree.
 *
 ****************************************************************************
 */
void PluginManagerImpl::rescan_plugins() {
  grt::ListRef<app_Plugin> plugin_list = get_plugin_list();
  std::set<std::string> disabled_plugins;

  // make a set of disabled plugin names
  {
    grt::StringListRef list = get_disabled_plugin_names();
    if (list.is_valid())
      for (grt::StringListRef::const_iterator i = list.begin(); i != list.end(); ++i) {
        disabled_plugins.insert(*i);
      }
  }

  // clear plugin list
  while (plugin_list.count() > 0) {
    // this should only be done on shutdown, the actual plugin objects are still used after this
    // plugin_list[0]->reset_references();
    plugin_list.remove(0);
  }

  // clear group contents
  {
    grt::ListRef<app_PluginGroup> groups;
    app_PluginGroupRef group;

    groups = get_plugin_groups();
    for (size_t c = groups.count(), i = 0; i < c; i++) {
      group = groups[i];
      while (group->plugins().count() > 0)
        group->plugins().remove(0);
    }
  }

  // get list of modules that implement the plugin interface
  std::vector<Module *> plugin_modules = grt::GRT::get()->find_modules_matching("PluginInterface", "");

  _plugin_source_module.clear();

  // add all modules to the plugins list
  for (std::vector<Module *>::const_iterator pm = plugin_modules.begin(); pm != plugin_modules.end(); ++pm) {
    grt::ListRef<app_Plugin> plist;
    try {
      grt::ValueRef result = (*pm)->call_function("getPluginInfo", grt::BaseListRef());

      plist = grt::ListRef<app_Plugin>::cast_from(result);
      if (!plist.is_valid() || plist.count() == 0) {
        grt::GRT::get()->send_warning(
          "Module " + (*pm)->name() + " implements PluginInterface but does not export any plugins", "");
        continue;
      }
    } catch (std::exception &exc) {
      grt::GRT::get()->send_error(
        "Module " + (*pm)->name() + " had an error while executing getPluginInfo: " + exc.what(),
        "Location: " + (*pm)->path());
      continue;
    }

    size_t i, c = plist.count();
    for (i = 0; i < c; i++) {
      app_PluginRef plugin = plist[i];

      if (_plugin_source_module.find(plugin->name()) != _plugin_source_module.end()) {
        grt::GRT::get()->send_warning(
          "Duplicate plugin name " + *plugin->name(),
          base::strfmt("There is more than one plugin with the name %s (in %s and %s).", plugin->name().c_str(),
                       (*pm)->name().c_str(), _plugin_source_module[plugin->name()].c_str()));
        // must reset internal references in the object or we get a leak because of the cycles
        plugin->reset_references();
        continue;
      }

      if (!check_plugin_validity(plugin, *pm)) {
        plugin->reset_references();
        continue;
      }
      _plugin_source_module[plugin->name()] = (*pm)->name();

      // add to the plugin list
      if (plugin_list.is_valid())
        plugin_list.insert(plugin);

      if (disabled_plugins.find(*plugin->name()) != disabled_plugins.end()) {
        if (grt::GRT::get()->verbose())
          grt::GRT::get()->send_info("Plugin " + *plugin->name() + " is disabled, skipping...", "");
        plugin->reset_references();
        continue;
      }

      if (plugin->groups().count() == 0)
        add_plugin_to_group(plugin, "Others/Menu/Ungrouped");
      else {
        for (size_t d = plugin->groups().count(), j = 0; j < d; j++)
          add_plugin_to_group(plugin, plugin->groups()[j]);
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

app_PluginGroupRef PluginManagerImpl::get_group(const std::string &group_name) {
  grt::ListRef<app_PluginGroup> groups;
  app_PluginGroupRef group;

  groups = get_plugin_groups();

  for (size_t c = groups.count(), i = 0; i < c; i++) {
    if (groups[i]->name() == group_name) {
      group = groups[i];
      break;
    }
  }

  return group;
}

void PluginManagerImpl::add_plugin_to_group(const app_PluginRef &plugin, const std::string &group_name) {
  app_PluginGroupRef group = get_group(group_name);

  if (group.is_valid())
    group->plugins().insert(plugin);
}

/**
 ****************************************************************************
 * @brief Sets callbacks for handling of GUI plugins
 *
 * @param open callback for opening a plugin
 * @param show callback for showing a plugin
 * @param close callback for closing a plugin
 *
 ****************************************************************************
 */
void PluginManagerImpl::set_gui_plugin_callbacks(const OpenGUIPluginSlot &open, const ShowGUIPluginSlot &show,
                                                 const CloseGUIPluginSlot &close) {
  _open_gui_plugin_slot = open;
  _show_gui_plugin_slot = show;
  _close_gui_plugin_slot = close;
}

/**
 ****************************************************************************
 * @brief Adds a list of plugins to the plugin registry
 *
 * @param list of plugins
 *
 ****************************************************************************
 */
void PluginManagerImpl::register_plugins(grt::ListRef<app_Plugin> plugins) {
  grt::ListRef<app_Plugin> list = get_plugin_list();

  for (size_t c = plugins.count(), i = 0; i < c; i++)
    list.insert(plugins[i]);
}

/**
 ****************************************************************************
 * @brief Return list of registered plugin groups
 *
 * @return List of plugin groups
 ****************************************************************************
 */
grt::ListRef<app_PluginGroup> PluginManagerImpl::get_plugin_groups() {
  return grt::ListRef<app_PluginGroup>::cast_from(grt::GRT::get()->get(_group_registry_path));
}

/**
 ****************************************************************************
 * @brief Returns list of all registered plugins.
 *
 * The list of plugins is looked up at the GRT tree path set with
 * set_registry_path()
 *
 * @param group optional name of plugin group (allows * wildcards)
 *
 * @return List of plugins.
 ****************************************************************************
 */
grt::ListRef<app_Plugin> PluginManagerImpl::get_plugin_list(const std::string &group) {
  if (group.empty())
    return grt::ListRef<app_Plugin>::cast_from(grt::GRT::get()->get(_registry_path));
  else {
    grt::ListRef<app_Plugin> rlist(true), list;
    std::string left, right;

    // groups are expected to be either in group/subgroup format or group (which will be interpreted as group/*)

    if (group.find('/') != std::string::npos) {
      left = group.substr(0, group.find('/'));
      right = group.substr(group.find('/') + 1);
    } else {
      left = group;
      right = "*";
    }

    list = grt::ListRef<app_Plugin>::cast_from(grt::GRT::get()->get(_registry_path));

    for (size_t c = list.count(), i = 0; i < c; i++) {
      app_PluginRef plugin(list[i]);
      grt::StringListRef groups(plugin->groups());
      bool found = false;

      if (!plugin_enabled(plugin->name()))
        continue;

      for (size_t gc = groups.count(), g = 0; g < gc; g++) {
        std::string gstr(groups[g]);
        std::string gleft, gright;
        std::string::size_type pos;

        if ((pos = gstr.find('/'))) {
          gleft = gstr.substr(0, pos);
          gright = gstr.substr(pos + 1);
        } else {
          gleft = gstr;
          gright = "";
        }

        if ((left == "*" || left == gleft) && (right == "*" || right == gright)) {
          found = true;
          break;
        }
      }
      if (found)
        rlist.insert(plugin);
    }

    return rlist;
  }
}

bool PluginManagerImpl::check_plugin_input(const app_PluginInputDefinitionRef &def, const grt::ValueRef &value) {
  if (def.is_instance(app_PluginFileInput::static_class_name())) {
    if (value.is_valid() && value.type() != StringType)
      return false;
  } else if (def.is_instance(app_PluginSelectionInput::static_class_name())) {
    if (!value.is_valid() || value.type() != ListType)
      return false;

    app_PluginSelectionInputRef sdef(app_PluginSelectionInputRef::cast_from(def));
    grt::ObjectListRef olist(grt::ObjectListRef::cast_from(value));

    for (size_t d = olist.count(), j = 0; j < d; j++) {
      grt::ObjectRef value(olist.get(j));
      bool ok = false;

      for (size_t c = sdef->objectStructNames().count(), i = 0; i < c; i++) {
        if (value.is_instance(sdef->objectStructNames()[i])) {
          ok = true;
          break;
        }
      }
      if (!ok)
        return false;
    }

    std::string card = *sdef->argumentCardinality();
    if (card == "1") {
      if (olist.count() != 1)
        return false;
    } else if (card == "?") {
      if (olist.count() > 1)
        return false;
    } else if (card == "+") {
      if (olist.count() == 0)
        return false;
    } else if (card == "*")
      ;
    else {
      if (olist.count() != 1)
        return false;
    }
  } else if (def.is_instance(app_PluginObjectInput::static_class_name())) {
    if (!value.is_valid() || value.type() != ObjectType)
      return false;

    app_PluginObjectInputRef odef(app_PluginObjectInputRef::cast_from(def));

    if (!grt::ObjectRef::cast_from(value).is_instance(odef->objectStructName()))
      return false;
  } else
    return false;

  return true;
}

bool PluginManagerImpl::check_input_for_plugin(const app_PluginRef &plugin, const grt::BaseListRef &args) {
  if (args.count() != plugin->inputValues().count())
    return false;

  for (size_t c = plugin->inputValues().count(), i = 0; i < c; i++) {
    if (!check_plugin_input(plugin->inputValues()[i], args[i]))
      return false;
  }
  return true;
}

app_PluginRef PluginManagerImpl::select_plugin_for_input(const std::string &group, const grt::BaseListRef &args) {
  ListRef<app_Plugin> plugins = get_plugin_list(group);
  app_PluginRef best_match;
  ssize_t rating = -1;

  for (size_t c = plugins.count(), i = 0; i < c; i++) {
    app_PluginRef plugin = plugins[i];

    if (check_input_for_plugin(plugin, args) && *plugin->rating() > rating) {
      best_match = plugin;
      rating = *plugin->rating();
    }
  }

  return best_match;
}

/**
 ****************************************************************************
 * @brief Returns list of plugins that can be called on the objects
 *
 *
 * @param objects list of objects
 * @param group optional name of group for filtering (eg: catalog/Editors)
 *
 * @return list of plugins
 ****************************************************************************
 */
// TODO: delete?
std::vector<app_PluginRef> PluginManagerImpl::get_plugins_for_objects(const grt::ObjectListRef &objects,
                                                                      const std::string &group) {
  std::vector<app_PluginRef> plist;
  grt::ListRef<app_Plugin> plugins;

  plugins = get_plugin_list(group);
  size_t i, c = plugins.count();

  // look for plugins that take 1 ObjectInput arg that is compatible with the obj list
  if (objects.count() == 1)
    for (i = 0; i < c; i++) {
      app_PluginRef plugin(plugins[i]);

      if (plugin->inputValues().count() == 1 &&
          plugin->inputValues()[0]->is_instance(app_PluginObjectInput::static_class_name())) {
        app_PluginObjectInputRef oinput(app_PluginObjectInputRef::cast_from(plugin->inputValues()[0]));
        std::string struct_name = oinput->objectStructName();
        bool ok = true;

        for (size_t oc = objects.count(), oi = 0; oi < oc; oi++) {
          if (!objects[oi].is_instance(struct_name)) {
            ok = false;
            break;
          }
        }

        if (ok)
          plist.push_back(plugin);
      }
    }

  // look for plugins that take selection as input and are compatible with the obj list
  for (i = 0; i < c; i++) {
    app_PluginRef plugin(plugins[i]);

    if (plugin->inputValues().count() == 1 &&
        plugin->inputValues()[0]->is_instance(app_PluginSelectionInput::static_class_name())) {
      app_PluginSelectionInputRef oinput(app_PluginSelectionInputRef::cast_from(plugin->inputValues()[0]));
      std::string card = *oinput->argumentCardinality();
      grt::StringListRef struct_names(oinput->objectStructNames());
      bool ok = true;

      if (card == "1") {
        if (objects.count() != 1)
          continue;
      } else if (card == "?") {
        if (objects.count() > 1)
          continue;
      } else if (card == "+") {
        if (objects.count() == 0)
          continue;
      } else if (card == "*")
        ;
      else {
        if (objects.count() != 1)
          continue;
      }

      for (size_t oc = objects.count(), oi = 0; oi < oc && ok; oi++) {
        for (size_t sc = struct_names.count(), si = 0; si < sc && ok; si++) {
          if (!objects[oi].is_instance(struct_names[si])) {
            ok = false;
            break;
          }
        }
      }

      if (ok)
        plist.push_back(plugin);
    }
  }

  return plist;
}

/**
 ****************************************************************************
 * @brief Returns list of plugins in the named group.
 *
 * @param group
 *
 * @return List of plugins
 *
 ****************************************************************************
 */
std::vector<app_PluginRef> PluginManagerImpl::get_plugins_for_group(const std::string &group) {
  std::vector<app_PluginRef> rlist;
  grt::ListRef<app_Plugin> list = get_plugin_list(group);

  for (size_t c = list.count(), i = 0; i < c; i++)
    rlist.push_back(list[i]);

  return rlist;
}

/**
 ****************************************************************************
 * @brief Return named plugin
 *
 * @param name
 *
 * @return plugin
 ****************************************************************************
 */
app_PluginRef PluginManagerImpl::get_plugin(const std::string &name) {
  grt::ListRef<app_Plugin> plugins = get_plugin_list();

  for (size_t c = plugins.count(), i = 0; i < c; i++) {
    if (*plugins[i]->name() == name)
      return plugins[i];
  }
  return app_PluginRef();
}

static std::string make_open_plugin_id(const grt::Module *module, const std::string &class_name,
                                       const grt::BaseListRef &args) {
  std::string argshash = get_args_hash(args);

  return module->name() + "/" + class_name + "//" + argshash;
}

std::vector<NativeHandle> PluginManagerImpl::get_similar_open_plugins(grt::Module *module,
                                                                      const std::string &class_name, grt::BaseListRef) {
  std::vector<NativeHandle> handles;

  std::string prefix = module->name() + "/" + class_name + "//";

  for (std::map<std::string, NativeHandle>::const_iterator iter = _open_gui_plugins.begin();
       iter != _open_gui_plugins.end(); ++iter) {
    if (iter->first.substr(0, prefix.size()) == prefix)
      handles.push_back(iter->second);
  }

  return handles;
}

//--------------------------------------------------------------------------------------------------

std::string PluginManagerImpl::open_gui_plugin(const app_PluginRef &plugin, const grt::BaseListRef &args,
                                               GUIPluginFlags flags) {
  if (!plugin.is_valid())
    throw std::invalid_argument("Attempt to open an invalid plugin");

  GRTDispatcher::Ref dispatcher = bec::GRTManager::get()->get_dispatcher();
  if (*plugin->pluginType() == GUI_PLUGIN_TYPE) {
    if (bec::GRTManager::get()->in_main_thread())
      return open_gui_plugin_main(plugin, args, flags);
    else {
      // Request the plugin to be executed and opened by the frontend in the main thread.
      DispatcherCallback<std::string>::Ref cb = DispatcherCallback<std::string>::create_callback(
        std::bind(&PluginManagerImpl::open_gui_plugin_main, this, plugin, args, flags));

      dispatcher->call_from_main_thread(cb, false, false);

      grt::Module *module = grt::GRT::get()->get_module(_plugin_source_module[plugin->name()]);

      // Build the handle name ourselves.
      return make_open_plugin_id(module, plugin->moduleFunctionName(), args);
    }
  } else if (*plugin->pluginType() == STANDALONE_GUI_PLUGIN_TYPE) {
    if (bec::GRTManager::get()->in_main_thread())
      open_standalone_plugin_main(plugin, args);
    else {
      // Request the plugin to be executed and opened by the frontend in the main thread.
      DispatcherCallback<void>::Ref cb = DispatcherCallback<void>::create_callback(
        std::bind(&PluginManagerImpl::open_standalone_plugin_main, this, plugin, args));
      dispatcher->call_from_main_thread(cb, false, false);
    }
  } else if (*plugin->pluginType() == INTERNAL_PLUGIN_TYPE) {
    if (bec::GRTManager::get()->in_main_thread())
      open_normal_plugin_grt(plugin, args);
    else {
      // Request the plugin to be executed and opened by the frontend in the main thread.
      DispatcherCallback<grt::ValueRef>::Ref cb = DispatcherCallback<grt::ValueRef>::create_callback(
        std::bind(&PluginManagerImpl::open_normal_plugin_grt, this, plugin, args));

      dispatcher->call_from_main_thread(cb, false, false);
    }
  } else // A normal plugin implemented by a GRT module.
  {
    // Opening a normal plugin is usually done in the context of the grt thread and we want to
    // continue that way. But if we are currently in the main thread switch here to the grt thread
    // for opening the plugin.
    if (bec::GRTManager::get()->in_main_thread()) {
      bec::GRTManager::get()->get_dispatcher()->execute_sync_function(
        "Open normal plugin", std::bind(&PluginManagerImpl::open_normal_plugin_grt, this, plugin, args));
    } else
      open_normal_plugin_grt(plugin, args);
  }
  return "";
}

//--------------------------------------------------------------------------------------------------

/**
 ****************************************************************************
 * @brief Executes the plugin with the given list of arguments
 *
 * If the plugin is a GUI type plugin, the GUI plugin callbacks are executed
 * in the main thread and may return a handle to pass back to the frontend
 * and be processed.
 *
 * If the plugin is a normal type plugin, it will be just executed in the
 * GRT thread, as usual.
 *
 * @param plugin object
 * @param args list of
 *
 * @return An identifier/handle for the opened plugin, if it's a GUI plugin
 * or "" for other types of plugins.
 ****************************************************************************
 */
std::string PluginManagerImpl::open_plugin(const app_PluginRef &plugin, const grt::BaseListRef &args) {
  return open_gui_plugin(plugin, args, NoFlags);
}

grt::ValueRef PluginManagerImpl::execute_plugin_function(const app_PluginRef &plugin, const grt::BaseListRef &args) {
  grt::Module *module = grt::GRT::get()->get_module(plugin->moduleName());

  if (!module)
    throw grt::grt_runtime_error("Cannot execute plugin " + *plugin->name(),
                                 "Called module " + *plugin->moduleName() + " not found");

  return module->call_function(*plugin->moduleFunctionName(), args);
}

grt::ValueRef PluginManagerImpl::open_normal_plugin_grt(const app_PluginRef &plugin, const grt::BaseListRef &args) {
  grt::Module *module = grt::GRT::get()->get_module(plugin->moduleName());

  if (!module)
    throw grt::grt_runtime_error("Cannot execute plugin " + *plugin->name(),
                                 "Called module " + *plugin->moduleName() + " not found");

  return module->call_function(*plugin->moduleFunctionName(), args);
}

void PluginManagerImpl::open_standalone_plugin_main(const app_PluginRef &plugin, const grt::BaseListRef &args) {
  grt::Module *module = grt::GRT::get()->get_module(plugin->moduleName());

  if (!module)
    throw grt::grt_runtime_error("Cannot execute plugin " + *plugin->name(),
                                 "Called module " + *plugin->moduleName() + " not found");

  module->call_function(*plugin->moduleFunctionName(), args);
}

std::string PluginManagerImpl::open_gui_plugin_main(const app_PluginRef &plugin, const grt::BaseListRef &args,
                                                    GUIPluginFlags flags) {
  NativeHandle handle;
  grt::Module *module = grt::GRT::get()->get_module(_plugin_source_module[plugin->name()]);
  std::string open_plugin_id = make_open_plugin_id(module, plugin->moduleFunctionName(), args);

  if (_open_gui_plugins.find(open_plugin_id) != _open_gui_plugins.end()) {
    handle = _open_gui_plugins[open_plugin_id];
    _show_gui_plugin_slot(handle);
  } else {
    grt::Module *module = grt::GRT::get()->get_module(_plugin_source_module[plugin->name()]);

    // open the editor and get a handle for the GUI object to pass to the frontend
    NativeHandle handle =
      _open_gui_plugin_slot(module, *plugin->moduleName(), *plugin->moduleFunctionName(), args, flags);
    if (handle) {
      _open_gui_plugins[open_plugin_id] = handle;
      _show_gui_plugin_slot(handle);
    }
  }

  return open_plugin_id;
}

/**
 ****************************************************************************
 * @brief Shows a previously opened (GUI) plugin.
 *
 * @param handle returned by open_plugin
 *
 * @return always 0 at the moment
 ****************************************************************************
 */
int PluginManagerImpl::show_plugin(const std::string &handle) {
  if (bec::GRTManager::get()->in_main_thread())
    return show_gui_plugin_main(handle);
  else {
    GRTDispatcher::Ref dispatcher = bec::GRTManager::get()->get_dispatcher();

    // Request the plugin to be executed and opened by the frontend in the main thread.
    DispatcherCallback<int>::Ref cb =
      DispatcherCallback<int>::create_callback(std::bind(&PluginManagerImpl::show_gui_plugin_main, this, handle));
    dispatcher->call_from_main_thread(cb, false, false);

    // Return value is ignored atm.
    return 0;
  }
}

int PluginManagerImpl::show_gui_plugin_main(const std::string &handle) {
  if (_open_gui_plugins.find(handle) != _open_gui_plugins.end()) {
    NativeHandle hdl = _open_gui_plugins[handle];
    _show_gui_plugin_slot(hdl);
  }

  return 1;
}

/**
 ****************************************************************************
 * @brief Closes a previously opened (GUI) plugin.
 *
 * @param handle return by open_plugin
 *
 * @return always 0 at the moment
 ****************************************************************************
 */
int PluginManagerImpl::close_plugin(const std::string &handle) {
  if (bec::GRTManager::get()->in_main_thread())
    return close_gui_plugin_main(handle);
  else {
    GRTDispatcher::Ref dispatcher = bec::GRTManager::get()->get_dispatcher();

    // Request the plugin to be executed and opened by the frontend in the main thread.
    DispatcherCallback<int>::Ref cb =
      DispatcherCallback<int>::create_callback(std::bind(&PluginManagerImpl::close_gui_plugin_main, this, handle));
    dispatcher->call_from_main_thread(cb, false, false);

    // Return value is ignored atm.
    return 0;
  }
}

//--------------------------------------------------------------------------------------------------

int PluginManagerImpl::close_gui_plugin_main(const std::string &handle) {
  if (_open_gui_plugins.find(handle) != _open_gui_plugins.end()) {
    NativeHandle hdl = _open_gui_plugins[handle];
    _close_gui_plugin_slot(hdl);
  }

  return 1;
}

//--------------------------------------------------------------------------------------------------

void PluginManagerImpl::forget_gui_plugin_handle(NativeHandle handle) {
  for (std::map<std::string, NativeHandle>::iterator iter = _open_gui_plugins.begin(); iter != _open_gui_plugins.end();
       ++iter) {
    if (iter->second == handle) {
      _open_gui_plugins.erase(iter);
      break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

void PluginManagerImpl::close_and_forget_gui_plugin(NativeHandle handle) {
  for (std::map<std::string, NativeHandle>::iterator iter = _open_gui_plugins.begin(); iter != _open_gui_plugins.end();
       ++iter) {
    if (iter->second == handle) {
      _close_gui_plugin_slot(handle);
      _open_gui_plugins.erase(iter);
      break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

grt::ValueRef ArgumentPool::find_match(const app_PluginInputDefinitionRef &pdef, std::string &searched_key_name_ret,
                                       bool strict) const {
  std::string key = pdef.class_name();

  if (pdef.class_name() == app_PluginSelectionInput::static_class_name()) {
    app_PluginSelectionInputRef sdef(app_PluginSelectionInputRef::cast_from(pdef));

    // add source
    key.append(":").append(*pdef->name()).append(":");
    searched_key_name_ret = key;

    // find entry
    const_iterator iter;
    if ((iter = find(key)) == end())
      return grt::ValueRef();

    if (!iter->second.is_valid() || !grt::ObjectListRef::can_wrap(iter->second))
      return grt::ValueRef();

    grt::ObjectListRef olist(grt::ObjectListRef::cast_from(iter->second));

    bool matches = true;

    // check if value matches the requirements
    for (size_t d = olist.count(), j = 0; j < d; j++) {
      grt::ObjectRef value(olist.get(j));
      bool ok = false;

      for (size_t c = sdef->objectStructNames().count(), i = 0; i < c; i++) {
        if (value.is_instance(sdef->objectStructNames()[i])) {
          ok = true;
          break;
        }
      }
      if (!ok) {
        matches = false;
        break;
      }
    }

    std::string card = *sdef->argumentCardinality();
    if (card == "1") {
      if (olist.count() != 1)
        matches = false;
    } else if (card == "?") {
      if (olist.count() > 1)
        matches = false;
    } else if (card == "+") {
      if (olist.count() == 0)
        matches = false;
    } else if (card == "*")
      ;
    else {
      if (olist.count() != 1)
        matches = false;
    }

    if (matches)
      return olist;
  } else {
    if (pdef.class_name() == app_PluginObjectInput::static_class_name())
      key.append(":")
        .append(*pdef->name())
        .append(":")
        .append(*app_PluginObjectInputRef::cast_from(pdef)->objectStructName());
    else if (pdef.class_name() == app_PluginFileInput::static_class_name()) {
      if (strict)
        key.append(":").append(*pdef->name()).append(":").append(app_PluginFileInputRef::cast_from(pdef)->dialogType());
      else
        key.append(":").append(":").append(app_PluginFileInputRef::cast_from(pdef)->dialogType());
    } else if (pdef.class_name() == app_PluginInputDefinition::static_class_name())
      key.append(":").append(*pdef->name());

    searched_key_name_ret = key;

    const_iterator iter;
    if ((iter = find(key)) != end())
      return iter->second;
  }

  return grt::ValueRef();
}

void ArgumentPool::dump_keys(const std::function<void(std::string)> &dump_function) const {
  for (ArgumentPool::const_iterator i = begin(); i != end(); ++i) {
    if (dump_function)
      dump_function(i->first + "\n");
    else
      g_message("%s", i->first.c_str());
  }
}

void ArgumentPool::add_simple_value(const std::string &name, const grt::ValueRef &value) {
  std::string prefix = "app.PluginInputDefinition:" + name;

  (*this)[prefix] = value;
}

void ArgumentPool::add_list_for_selection(const std::string &source_name, const grt::ObjectListRef &list) {
  std::string prefix = "app.PluginSelectionInput:" + source_name + ":";

  (*this)[prefix] = list;
}

void ArgumentPool::add_entries_for_object(const std::string &name, const grt::ObjectRef &object,
                                          const std::string &topmost_class_name) {
  if (object.is_valid()) {
    std::string prefix = "app.PluginObjectInput:" + name + ":";
    std::string class_name = object.class_name();
    bool done = false;
    for (;;) {
      grt::MetaClass *mc = grt::GRT::get()->get_metaclass(class_name);
      (*this)[prefix + mc->name()] = object;

      class_name = mc->parent() ? mc->parent()->name() : "";
      if (topmost_class_name.empty() || class_name.empty() || done)
        break;
      if (topmost_class_name == class_name)
        done = true;
    }
  }
}

bool ArgumentPool::needs_simple_input(const app_PluginRef &plugin, const std::string &name) {
  const size_t c = plugin->inputValues().count();
  for (size_t i = 0; i < c; i++) {
    app_PluginInputDefinitionRef pdef(plugin->inputValues().get(i));

    if (pdef.class_name() == app_PluginInputDefinition::static_class_name()) {
      if (pdef->name() == name)
        return true;
    }
  }
  return false;
}

app_PluginFileInputRef ArgumentPool::needs_file_input(const app_PluginRef &plugin) {
  const size_t c = plugin->inputValues().count();
  for (size_t i = 0; i < c; i++) {
    app_PluginInputDefinitionRef pdef(plugin->inputValues().get(i));

    if (pdef.is_instance(app_PluginFileInput::static_class_name()))
      return app_PluginFileInputRef::cast_from(pdef);
  }
  return app_PluginFileInputRef();
}

void ArgumentPool::add_file_input(const app_PluginFileInputRef &pdef, const std::string &value) {
  std::string key = app_PluginFileInput::static_class_name();
  key.append(":").append(*pdef->name()).append(":").append(pdef->dialogType());

  (*this)[key] = grt::StringRef(value);
}

grt::BaseListRef ArgumentPool::build_argument_list(const app_PluginRef &plugin) {
  // build the argument list
  grt::BaseListRef fargs(true);
  const size_t c = plugin->inputValues().count();
  for (size_t i = 0; i < c; i++) {
    app_PluginInputDefinitionRef pdef(plugin->inputValues().get(i));
    std::string searched_key;
    grt::ValueRef argument = find_match(pdef, searched_key);
    if (!argument.is_valid()) {
      logWarning("Cannot satisfy plugin input for %s: %s\n", plugin->name().c_str(), searched_key.c_str());
      logWarning("Missing input: %s\n", pdef.debugDescription().c_str());

      throw grt::grt_runtime_error("Cannot execute " + *plugin->name(), "Plugin requires unavailable argument value.");
    }
    fargs.ginsert(argument);
  }
  return fargs;
}
