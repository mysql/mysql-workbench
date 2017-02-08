/*
 * Copyright (c) 2010, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef _PLUGIN_MANAGER_WINDOW_H_
#define _PLUGIN_MANAGER_WINDOW_H_

#include "grt/grt_manager.h"

#include "mforms/form.h"
#include "mforms/treeview.h"
#include "mforms/imagebox.h"
#include "mforms/label.h"
#include "mforms/checkbox.h"
#include "mforms/panel.h"
#include "mforms/box.h"

namespace wb {
  class WBContext;
};

class PluginManagerWindow : public mforms::Form {
public:
  PluginManagerWindow(wb::WBContext *wb);
  void run();

private:
  wb::WBContext *_wb;
  mforms::TreeView plugin_list;
  mforms::ImageBox plugin_icon;
  mforms::Label plugin_caption;
  mforms::Label plugin_version;
  mforms::Label plugin_type;
  mforms::Label plugin_author;
  mforms::Label plugin_name;
  mforms::Label plugin_description;
  mforms::Label plugin_path;
  mforms::Label plugin_function;
  mforms::CheckBox plugin_enabled;
  mforms::Button plugin_uninstall;
  mforms::Button plugin_close;
  mforms::CheckBox plugin_show_details;
  mforms::Panel plugin_info_panel;
  mforms::Box plugin_info_box;
  mforms::Box plugin_details_box;

  std::map<std::string, std::list<std::string> > _module_plugins;

  void uninstall();
  void toggle_enable();
  void toggle_show_details();
  void refresh_plugin_list();
  void list_selection_changed();
};

#endif
