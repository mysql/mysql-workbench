/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "plugin_manager_window.h"
#include "grt/icon_manager.h"
#include "base/file_utilities.h"
#include "base/string_utilities.h"
#include "workbench/wb_context.h"

using namespace mforms;
using namespace grt;

PluginManagerWindow::PluginManagerWindow(wb::WBContext *wb)
  : Form(0),
    _wb(wb),
    plugin_list(TreeDefault | TreeFlatList),
    plugin_info_panel(LineBorderPanel),
    plugin_info_box(false),
    plugin_details_box(false) {
  set_name("Plugin Manager");
  setInternalName("plugin_manager");
  Box *top = manage(new Box(false));
  Box *middle = manage(new Box(true));

  set_content(top);

  top->set_padding(12);
  top->set_spacing(12);
  top->add(middle, true, true);

  middle->set_spacing(8);
  middle->add(&plugin_list, false, true);
  plugin_list.set_size(200, -1);
  middle->add(&plugin_info_panel, true, true);

  scoped_connect(plugin_list.signal_changed(), std::bind(&PluginManagerWindow::list_selection_changed, this));

  plugin_info_panel.set_back_color("#ffffff");
  plugin_info_panel.add(&plugin_info_box);

  plugin_info_box.set_padding(12);
  plugin_info_box.set_spacing(12);
  plugin_info_box.add(&plugin_caption, false, true);

  {
    Box *hbox = manage(new Box(true));
    Box *vbox = manage(new Box(false));
    plugin_info_box.add(hbox, false, true);

    hbox->set_spacing(8);
    vbox->set_spacing(8);

    plugin_name.set_color("#999999");
    vbox->add(&plugin_name, false, true);

    plugin_icon.set_image_align(TopCenter);
#ifdef _MSC_VER
    plugin_icon.set_scale_contents(false);
#else
    plugin_icon.set_scale_contents(true);
#endif
    hbox->add(&plugin_icon, false, true);
    plugin_icon.set_size(48, -1);
    hbox->add(vbox, true, true);

    {
      Box *ibox = manage(new Box(true));
      ibox->set_spacing(8);
      vbox->add(ibox, false, true);
      plugin_version.set_size(100, -1);
      // ibox->add(&plugin_type, false, true);
      ibox->add(&plugin_version, false, true);
      ibox->add(&plugin_author, true, true);
      plugin_type.set_style(SmallStyle);
      plugin_author.set_style(SmallStyle);
      plugin_version.set_style(SmallStyle);
      plugin_author.set_color("#999999");
      plugin_version.set_color("#999999");
    }
    plugin_description.set_size(-1, 50);
    vbox->add(&plugin_description, false, true);

    plugin_caption.set_style(BoldStyle);
    plugin_description.set_wrap_text(true);

    hbox = manage(new Box(true));
    vbox->add_end(hbox, false, true);

    plugin_enabled.set_text("Enable Plugin");
    scoped_connect(plugin_enabled.signal_clicked(), std::bind(&PluginManagerWindow::toggle_enable, this));
    plugin_uninstall.set_text("Uninstall");
    scoped_connect(plugin_uninstall.signal_clicked(), std::bind(&PluginManagerWindow::uninstall, this));

    hbox->add(&plugin_enabled, true, true);
    hbox->add(&plugin_uninstall, false, true);
  }

  {
    plugin_details_box.set_spacing(4);
    plugin_details_box.add(&plugin_path, false, true);
    plugin_details_box.add(&plugin_function, false, true);

    plugin_info_box.add(&plugin_details_box, true, true);
  }

  plugin_list.add_column(IconColumnType, "", 20, false);
  plugin_list.add_column(StringColumnType, "Plugins", 400, false);
  plugin_list.end_columns();

  plugin_list.set_row_height(24);

  scoped_connect(plugin_show_details.signal_clicked(), std::bind(&PluginManagerWindow::toggle_show_details, this));
  toggle_show_details();

  list_selection_changed();

  {
    Box *button_box = manage(new Box(true));

    plugin_show_details.set_text("Show plugin details");
    button_box->add(&plugin_show_details, false, true);

    plugin_close.set_text("Close");
    button_box->add_end(&plugin_close, false, true);
    top->add(button_box, false, true);
  }

  set_title("Plugin Manager");
  set_size(650, 400);
}

void PluginManagerWindow::run() {
  refresh_plugin_list();
  run_modal(&plugin_close, 0);
}

void PluginManagerWindow::refresh_plugin_list() {
  std::string user_plugin_dir = bec::GRTManager::get()->get_user_module_path();
  ListRef<app_Plugin> plugins(ListRef<app_Plugin>::cast_from(grt::GRT::get()->get("/wb/registry/plugins")));

  plugin_list.clear();
  for (ListRef<app_Plugin>::const_iterator p = plugins.begin(); p != plugins.end(); ++p) {
    _module_plugins[(*p)->moduleName()].push_back((*p)->name());

    grt::Module *module = grt::GRT::get()->get_module(*(*p)->moduleName());
    if (module) {
      std::string path = module->path();
      if (path.compare(0, user_plugin_dir.size(), user_plugin_dir) == 0) {
        TreeNodeRef node = plugin_list.add_node();

        std::string icon_path;
        if (module->is_bundle())
          icon_path = module->default_icon_path();

        if (icon_path.empty() || !g_file_test(icon_path.c_str(), G_FILE_TEST_IS_REGULAR))
          icon_path = bec::IconManager::get_instance()->get_icon_path("MySQLPlugin-48.png");

        // disabled for now b/c TreeView in windows doesn't scale down images to fit the row
        // node->set_icon_path(0, icon_path);
        node->set_string(1, (*p)->caption());
        node->set_tag((*p)->name());
      }
    }
  }
}

void PluginManagerWindow::list_selection_changed() {
  TreeNodeRef node = plugin_list.get_selected_node();
  app_PluginRef plugin;
  if (node) {
    std::string plugin_name;
    plugin_name = node->get_tag();
    plugin = bec::GRTManager::get()->get_plugin_manager()->get_plugin(plugin_name);
  }

  if (plugin.is_valid()) {
    plugin_caption.set_text(plugin->caption());
    plugin_description.set_text(plugin->description());
    grt::Module *module = grt::GRT::get()->get_module(*plugin->moduleName());
    plugin_name.set_text("Name: " + *plugin->name());
    // type info
    if (module) {
      std::string path = module->is_bundle() ? module->bundle_path() : module->path();
      plugin_path.set_text(std::string("File Name: ").append(base::basename(path)));
      plugin_path.set_tooltip(path);
      plugin_function.set_text(
        std::string("Implemented by: ").append(plugin->moduleName()).append(".").append(plugin->moduleFunctionName()));

      plugin_author.set_text("by " + module->author());
      plugin_version.set_text("version " + module->version());

      std::string icon_path;
      if (module->is_bundle())
        icon_path = module->default_icon_path();

      if (icon_path.empty() || !g_file_test(icon_path.c_str(), G_FILE_TEST_IS_REGULAR))
        plugin_icon.set_image(bec::IconManager::get_instance()->get_icon_path("MySQLPlugin-48.png"));
      else
        plugin_icon.set_image(icon_path);

      // input/output details

    } else {
      plugin_path.set_text("File Name: ?");
      plugin_author.set_text("");
      plugin_version.set_text("");
      plugin_function.set_text("");
      plugin_icon.set_image(bec::IconManager::get_instance()->get_icon_path("MySQLPlugin-48.png"));
    }

    plugin_type.set_text("");
    plugin_enabled.set_active(bec::GRTManager::get()->get_plugin_manager()->plugin_enabled(plugin->name()));

    plugin_info_box.show(true);
  } else
    plugin_info_box.show(false);
}

void PluginManagerWindow::toggle_show_details() {
  plugin_details_box.show(plugin_show_details.get_active());
}

void PluginManagerWindow::uninstall() {
  TreeNodeRef node = plugin_list.get_selected_node();
  app_PluginRef plugin;
  if (node) {
    std::string plugin_name;
    plugin_name = node->get_tag();
    plugin = bec::GRTManager::get()->get_plugin_manager()->get_plugin(plugin_name);
    if (plugin.is_valid()) {
      std::list<std::string> plugins(_module_plugins[plugin->moduleName()]);
      grt::Module *module = grt::GRT::get()->get_module(plugin->moduleName());

      if (!module)
        return;

      bool confirmed = false;
      if (plugins.size() == 1) {
        if (Utilities::show_message("Uninstall Plugin",
                                    base::strfmt("Are you sure you want to uninstall the plugin %s?\n"
                                                 "This action cannot be undone.",
                                                 plugin->name().c_str()),
                                    "Uninstall", "Cancel", "") == ResultOk)
          confirmed = true;
      } else {
        std::string plugin_names;
        for (std::list<std::string>::const_iterator p = plugins.begin(); p != plugins.end(); ++p) {
          if (*p != plugin_name) {
            if (!plugin_names.empty())
              plugin_names.append(", ");
            plugin_names.append(*p);
          }
        }

        if (Utilities::show_message("Uninstall Plugins",
                                    base::strfmt("The file containing '%s' also contains the following other plugins:\n"
                                                 "%s\n"
                                                 "Are you sure you want to permanently uninstall them?\n"
                                                 "This action cannot be undone.",
                                                 plugin->name().c_str(), plugin_names.c_str()),
                                    "Uninstall", "Cancel", "") == ResultOk)
          confirmed = true;
      }

      if (confirmed) {
        _wb->uninstall_module(module);
        refresh_plugin_list();
      }
    }
  }
}

void PluginManagerWindow::toggle_enable() {
  TreeNodeRef node = plugin_list.get_selected_node();
  if (node) {
    std::string plugin_name;
    plugin_name = node->get_tag();
    app_PluginRef plugin(bec::GRTManager::get()->get_plugin_manager()->get_plugin(plugin_name));
    if (plugin.is_valid() &&
        bec::GRTManager::get()->get_plugin_manager()->plugin_enabled(plugin_name) != plugin_enabled.get_active()) {
      bec::GRTManager::get()->get_plugin_manager()->set_plugin_enabled(plugin, plugin_enabled.get_active());
      bec::GRTManager::get()->get_plugin_manager()->rescan_plugins();
    }
  }
}
