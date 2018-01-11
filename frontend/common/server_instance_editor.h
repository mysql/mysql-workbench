/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _SERVER_INSTANCE_EDITOR_H_
#define _SERVER_INSTANCE_EDITOR_H_

#include "workbench/wb_backend_public_interface.h"
#include "grts/structs.db.mgmt.h"
#include "grtui/grtdb_connect_panel.h"

#include "mforms/form.h"
#include "mforms/box.h"
#include "mforms/textentry.h"
#include "mforms/treeview.h"
#include "mforms/selector.h"
#include "mforms/button.h"
#include "mforms/tabview.h"
#include "mforms/radiobutton.h"
#include "mforms/checkbox.h"
#include "mforms/label.h"
#include "mforms/textbox.h"

class MYSQLWBBACKEND_PUBLIC_FUNC ServerInstanceEditor : public mforms::Form {
  db_mgmt_ManagementRef _mgmt;
  grt::ListRef<db_mgmt_Connection> _connections;
  grt::ListRef<db_mgmt_ServerInstance> _instances;

  mforms::Box _top_vbox;
  mforms::Box _top_hbox;

  mforms::TextEntry _name_entry;

  mforms::Box _content_box;

  mforms::Box _inst_list_buttons_hbox;
  mforms::Button _add_inst_button;
  mforms::Button _del_inst_button;
  mforms::Button _dup_inst_button;
  mforms::Button _move_up_button;
  mforms::Button _move_down_button;
  mforms::TreeView _stored_connection_list;

  mforms::TabView _tabview;

  mforms::RadioButton _no_remote_admin;
  mforms::RadioButton _win_remote_admin;
  mforms::RadioButton _ssh_remote_admin;

  mforms::Box _remote_param_box;
  mforms::TextEntry _remote_host;
  mforms::TextEntry _ssh_port;
  mforms::TextEntry _remote_user;
  mforms::Box _password_box;
  mforms::Button _password_set;
  mforms::Button _password_clear;
  mforms::CheckBox _ssh_usekey;
  mforms::TextEntry _ssh_keypath;

  mforms::Button _autodetect_button;

  mforms::Box _sys_box;
  mforms::Selector _os_type;
  mforms::Selector _sys_profile_type;
  mforms::TextEntry _sys_config_path;
  mforms::TextEntry _sys_myini_section;
  mforms::Label *_sys_win_service_name_label;
  mforms::TextEntry _sys_win_service_name;
  mforms::Button _sys_config_path_browse;
  mforms::Label _sys_win_hint_label;

  mforms::Label _details_description;
  mforms::Panel _details_panel;
  mforms::TextEntry _start_cmd;
  mforms::TextEntry _stop_cmd;
  mforms::CheckBox _sudo_check;
  mforms::Label _sudo_description;
  mforms::Box _custom_sudo_box;
  mforms::TextEntry _sudo_prefix;

  grtui::DbConnectPanel *_connect_panel;

  bool _contains_group;

  //  mforms::Button _save_preset_button;
  //  mforms::Button _delete_preset_button;

  mforms::Box _bottom_hbox;
  mforms::Box _remote_admin_box;
  mforms::Button _close_button;
  mforms::Button _test_button;

  std::map<std::string, std::vector<std::pair<std::string, grt::DictRef> > > _presets;

  db_mgmt_ConnectionRef selected_connection();
  db_mgmt_ServerInstanceRef selected_instance();

  void autodetect_system();
  void test_settings();

  void toggle_administration();

  grt::DictRef get_preset(const std::string &system, const std::string &preset_name);

  void entry_changed(mforms::TextEntry *sender);
  void check_changed(mforms::CheckBox *check);

  //  void button_clicked(mforms::Button *button);
  void browse_file();
  void show_connection();
  void show_instance_info(db_mgmt_ConnectionRef connection, db_mgmt_ServerInstanceRef instance);
  void add_instance();
  void delete_instance();
  void duplicate_instance();
  void reorder_instance(bool up);

  void system_type_changed();
  void profile_changed();

  void refresh_profile_list();
  void refresh_connection_list();

  void tab_changed();

  void driver_changed_cb(const db_mgmt_DriverRef &driver);

  void set_password(bool clear);

  void run_filechooser(mforms::TextEntry *entry);
  void run_filechooser_wrapper(mforms::TextEntry *entry); // Allows to run local or remote file selector

  void reset_setup_pending();

public:
  ServerInstanceEditor(const db_mgmt_ManagementRef &mgmt);
  virtual ~ServerInstanceEditor();

  db_mgmt_ServerInstanceRef run(db_mgmt_ConnectionRef select_connection = db_mgmt_ConnectionRef(),
                                bool select_admin = false);
};

#endif /* _SERVER_INSTANCE_EDITOR_H_ */
