/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "server_instance_editor.h"
#include "grtui/grtdb_connection_editor.h"

#include "grtpp_util.h"
#include "base/string_utilities.h"

#include "mforms/uistyle.h"
#include "mforms/panel.h"
#include "mforms/filechooser.h"

DEFAULT_LOG_DOMAIN(DOMAIN_WB_CONTEXT_UI)

#define SYSTEM_STAT_HEADER                                     \
  "# Customize this script to suit your system if necessary\n" \
  "# Shell function names and output format must be maintained\n\n"

#define WINDOWS_CONFIG_HINT_TEXT                                \
  "\nWindows Hint:\n\n"                                         \
  "When a connection is configured for\n"                       \
  "\"Windows Remote Management\", the \"Configuration File\"\n" \
  "must be specified using a mapped drive,\n"                   \
  "network share or administrative share.\n\n"                  \
  "Examples of these are\n"                                     \
  "M:\\<path to file>\\my.ini\n"                                \
  "\\\\<server>\\<share>\\<path to file>\\my.ini\n"             \
  "\\\\<server>\\C$\\Program Fiels\\MySQL\\MySQL Server 5.5\\my.ini"

using namespace mforms;
using namespace base;

inline Label *RLabel(const std::string &text) {
  Label *l = new Label(text);
  l->set_text_align(MiddleRight);
  return l;
}

inline Table *NewTable(int rows, int cols) {
  Table *table = new Table();
  table->set_row_count(rows);
  table->set_column_count(cols);
  table->set_row_spacing(8);
  table->set_column_spacing(8);
  table->set_padding(8);
  return table;
}

//--------------------------------------------------------------------------------------------------

/**
 * Determines if the given connection is an SSH connection and returns true if so.
 */
static bool is_ssh_connection(const db_mgmt_ConnectionRef &connection) {
  if (connection.is_valid()) {
    std::string driver = connection->driver().is_valid() ? connection->driver()->name() : "";
    return (driver == "MysqlNativeSSH");
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

/**
 * Determines if the given connection is a local connection (i.e. to the current box).
 */
static bool is_local_connection(const db_mgmt_ConnectionRef &connection) {
  if (connection.is_valid()) {
    std::string hostname = connection->parameterValues().get_string("hostName");

    if (!is_ssh_connection(connection) && (hostname == "localhost" || hostname.empty() || hostname == "127.0.0.1"))
      return true;
  }
  return false;
}

//----------------- ServerInstanceEditor -----------------------------------------------------------

ServerInstanceEditor::ServerInstanceEditor(const db_mgmt_ManagementRef &mgmt)
  : Form(0, FormResizable),
    _top_vbox(false),
    _top_hbox(true),
    _content_box(false),
    _inst_list_buttons_hbox(true),
    _stored_connection_list(TreeDefault | TreeFlatList),
    _tabview(mforms::TabViewSystemStandard),
    _no_remote_admin(RadioButton::new_id()),
    _win_remote_admin(_no_remote_admin.group_id()),
    _ssh_remote_admin(_no_remote_admin.group_id()),
    _remote_param_box(false),
    _password_box(true),
    _sys_box(false),
    _details_panel(mforms::TitledBoxPanel),
    _custom_sudo_box(true),
    _connect_panel(new grtui::DbConnectPanel(grtui::DbConnectPanelHideConnectionName)),
    _contains_group(false),
    _bottom_hbox(true),
    _remote_admin_box(false) {
  set_name("Instance Editor");
  setInternalName("instance_editor");
  _mgmt = mgmt;
  _connections = mgmt->storedConns();
  _instances = mgmt->storedInstances();

  {
    // do a quick validation, removing any instance objects that don't have a matching connection
    for (ssize_t i = _instances.count() - 1; i >= 0; --i) {
      if (!_instances[i]->connection().is_valid()) {
        logWarning("Server instance %s has no attached connection, deleting it\n", _instances[i]->name().c_str());
        _instances.remove(i);
      }
    }
  }

  set_title(_("Manage Server Connections"));
  _top_vbox.suspend_layout();

  _top_vbox.set_padding(MF_WINDOW_PADDING);
  _top_vbox.set_spacing(12);
  _top_hbox.set_spacing(8);
  _top_vbox.add(&_top_hbox, true, true);
  _top_vbox.add(&_bottom_hbox, false, true);

  _bottom_hbox.set_spacing(12);

  _stored_connection_list.set_name("Connection List");
  _top_hbox.add(&_stored_connection_list, false, true);

  _content_box.set_spacing(8);
  _content_box.set_name("Connection Specification");

  {
    mforms::Box *hbox = mforms::manage(new mforms::Box(true));
    hbox->add(mforms::manage(new mforms::Label("Connection Name:")), false, true);
    hbox->add(&_name_entry, true, true);
    hbox->set_spacing(8);
    _content_box.add(hbox, false, true);
  }
  _name_entry.set_name("Connection Name");
  scoped_connect(_name_entry.signal_changed(), std::bind(&ServerInstanceEditor::entry_changed, this, &_name_entry));

  _content_box.add(&_tabview, true, true);
  _top_hbox.add(&_content_box, true, true);

  _connect_panel->set_driver_changed_cb(
    std::bind(&ServerInstanceEditor::driver_changed_cb, this, std::placeholders::_1));
  scoped_connect(_tabview.signal_tab_changed(), std::bind(&ServerInstanceEditor::tab_changed, this));
  scoped_connect(_stored_connection_list.signal_changed(), std::bind(&ServerInstanceEditor::show_connection, this));

  _remote_param_box.set_padding(MF_PANEL_PADDING);
  _sys_box.set_padding(MF_PANEL_PADDING);
  _sys_box.set_spacing(10);

  _connect_panel->init(_mgmt);
  _connect_panel->set_padding(12);
  _tabview.set_name("Options");
  _tabview.add_page(_connect_panel, _("Connection"));


  // Remote management
  {
    _remote_admin_box.set_padding(MF_PANEL_PADDING);
    _remote_admin_box.set_spacing(4);
    mforms::Table *remote_param_table = NewTable(6, 2);

    _no_remote_admin.set_text(_("Do not use remote management"));
    scoped_connect(_no_remote_admin.signal_clicked(), std::bind(&ServerInstanceEditor::toggle_administration, this));
    _win_remote_admin.set_text(_("Native Windows remote management (only available on Windows)"));
    scoped_connect(_win_remote_admin.signal_clicked(), std::bind(&ServerInstanceEditor::toggle_administration, this));
#ifndef _MSC_VER
    _win_remote_admin.set_enabled(false);
#endif
    _ssh_remote_admin.set_text(_("SSH login based management"));
    scoped_connect(_ssh_remote_admin.signal_clicked(), std::bind(&ServerInstanceEditor::toggle_administration, this));

    _remote_admin_box.add(&_no_remote_admin, false, true);
    _remote_admin_box.add(&_win_remote_admin, false, true);
    _remote_admin_box.add(&_ssh_remote_admin, false, true);

    _remote_param_box.set_spacing(12);
    _remote_param_box.add(manage(remote_param_table), true, true);

    remote_param_table->add(RLabel(_("Hostname:")), 0, 1, 0, 1, HFillFlag);
    Box *box = manage(new Box(true));
    box->set_spacing(MF_TABLE_COLUMN_SPACING);

    _remote_host.set_size(200, -1);
    box->add(&_remote_host, true, true);
    scoped_connect(_remote_host.signal_changed(), std::bind(&ServerInstanceEditor::entry_changed, this, &_remote_host));

    box->add(manage(RLabel(_("Port:"))), false, true);
    _ssh_port.set_size(50, -1);
    _ssh_port.set_placeholder_text("22");
    box->add(&_ssh_port, true, true);
    scoped_connect(_ssh_port.signal_changed(), std::bind(&ServerInstanceEditor::entry_changed, this, &_ssh_port));
    remote_param_table->add(box, 1, 2, 0, 1, HFillFlag | HExpandFlag | mforms::VFillFlag);

    remote_param_table->add(manage(RLabel(_("Username:"))), 0, 1, 1, 2, HFillFlag);
    remote_param_table->add(&_remote_user, 1, 2, 1, 2, HExpandFlag | HFillFlag);
    scoped_connect(_remote_user.signal_changed(), std::bind(&ServerInstanceEditor::entry_changed, this, &_remote_user));
    remote_param_table->add(manage(RLabel(_("Password:"))), 0, 1, 2, 3, HFillFlag);
    remote_param_table->add(&_password_box, 1, 2, 2, 3, HExpandFlag | HFillFlag | mforms::VFillFlag);

#ifdef _MSC_VER
    _password_set.set_text(_("Store in Vault ..."));
    _password_set.set_tooltip(_("Store the password for this connection in the secured vault."));
    _password_clear.set_text(_("Remove from Vault"));
    _password_clear.set_tooltip(_("Remove previously stored password from the secured vault."));
#else
    _password_set.set_text(_("Store in Keychain ..."));
    _password_set.set_tooltip(_("Store the password for this connection in the system's keychain."));
    _password_clear.set_text(_("Remove from Keychain"));
    _password_clear.set_tooltip(_("Remove previously stored password from the system's keychain"));
#endif

    scoped_connect(_password_set.signal_clicked(), std::bind(&ServerInstanceEditor::set_password, this, false));
    scoped_connect(_password_clear.signal_clicked(), std::bind(&ServerInstanceEditor::set_password, this, true));

    _password_box.set_spacing(8);
    _password_box.add(&_password_set, true, true);
    _password_box.add(&_password_clear, true, true);

    _ssh_usekey.set_text(_("Authenticate Using SSH Key"));
    scoped_connect(_ssh_usekey.signal_clicked(), std::bind(&ServerInstanceEditor::check_changed, this, &_ssh_usekey));
    remote_param_table->add(&_ssh_usekey, 1, 2, 3, 4, HFillFlag);

    mforms::Label *l;
    remote_param_table->add(l = RLabel(_("SSH Key Path:")), 0, 1, 4, 5, HFillFlag);
    box = manage(new Box(true));
    box->set_spacing(MF_TABLE_COLUMN_SPACING);
    box->add(&_ssh_keypath, true, true);
    l->set_tooltip(
      _("Path to the SSH private key to use for connecting to the remote SSH server.\n"
        "The key must be in the format used by OpenSSH. If you do not use OpenSSH to\n"
        "generate the key (ssh-keygen), you may need to convert your key to that format."));
    _ssh_keypath.set_tooltip(
      _("Path to the SSH private key to use for connecting to the remote SSH server.\n"
        "The key must be in the format used by OpenSSH. If you do not use OpenSSH to\n"
        "generate the key (ssh-keygen), you may need to convert your key to that format."));
    scoped_connect(_ssh_keypath.signal_changed(), std::bind(&ServerInstanceEditor::entry_changed, this, &_ssh_keypath));
    Button *b = manage(new Button());
    b->set_text(_("Browse"));
    box->add(b, false, true);
    scoped_connect(b->signal_clicked(), std::bind(&ServerInstanceEditor::browse_file, this));
    remote_param_table->add(box, 1, 2, 4, 5, HFillFlag | mforms::VFillFlag);

    _remote_admin_box.add(&_remote_param_box, true, true);
  }

  {
    Box *box = manage(new Box(true));
    box->set_spacing(8);

    _autodetect_button.set_enabled(grt::GRT::get()->get_module("WbAdmin") != 0);
    _autodetect_button.set_text(_("Detect Server Configuration..."));
    _autodetect_button.set_tooltip(
      _("Attempt to automatically detect server configuration parameters for the system,\n"
        "such as operating system type, path to configuration file, how to start/stop MySQL etc"));

    box->add(&_autodetect_button, true, false);
    scoped_connect(_autodetect_button.signal_clicked(), std::bind(&ServerInstanceEditor::autodetect_system, this));
  }

  // Sys
  {
    Label *label = manage(
      new Label(_("Information about the server and MySQL configuration, such as path to the configuration file,\n"
                  "command to start or stop it etc. You may pick a preset configuration profile or customize one for "
                  "your needs.")));
    label->set_style(SmallStyle);

    _sys_box.add(label, false, true);

    Table *table = manage(NewTable(6, 2));
    _sys_box.add(table, false, true);

    {
      table->add(manage(RLabel(_("System Type:"))), 0, 1, 0, 1, HFillFlag);
      table->add(&_os_type, 1, 2, 0, 1, HFillFlag | HExpandFlag);
      scoped_connect(_os_type.signal_changed(), std::bind(&ServerInstanceEditor::system_type_changed, this));
    }
    {
      label = manage(RLabel(_("Installation Type:")));
      table->add(label, 0, 1, 1, 2, HFillFlag);

      scoped_connect(_sys_profile_type.signal_changed(), std::bind(&ServerInstanceEditor::profile_changed, this));
      table->add(&_sys_profile_type, 1, 2, 1, 2, HFillFlag | HExpandFlag);
    }

    { // use a custom file browsing field because the files can be local or remote
      Box *box = manage(new Box(true));
      box->set_spacing(8);
      table->add(manage(RLabel(_("Configuration File:"))), 0, 1, 2, 3, HFillFlag);

      box->add(&_sys_config_path, true, true);
      _sys_config_path_browse.enable_internal_padding(false);
      _sys_config_path_browse.set_text("...");
      box->add(&_sys_config_path_browse, false, true);
      scoped_connect(_sys_config_path_browse.signal_clicked(),
                     std::bind(&ServerInstanceEditor::run_filechooser_wrapper, this, &_sys_config_path));
      table->add(box, 1, 2, 2, 3, HFillFlag | HExpandFlag | mforms::VFillFlag);
    }

    scoped_connect(_sys_config_path.signal_changed(),
                   std::bind(&ServerInstanceEditor::entry_changed, this, &_sys_config_path));
    table->add(manage(RLabel(_("Configuration File Section:"))), 0, 1, 3, 4, HFillFlag);
    table->add(&_sys_myini_section, 1, 2, 3, 4, HFillFlag | HExpandFlag);
    scoped_connect(_sys_myini_section.signal_changed(),
                   std::bind(&ServerInstanceEditor::entry_changed, this, &_sys_myini_section));

    table->add(_sys_win_service_name_label = manage(RLabel(_("Windows Service Name:"))), 0, 1, 4, 5, HFillFlag);
    table->add(&_sys_win_service_name, 1, 2, 4, 5, HFillFlag | HExpandFlag);
    scoped_connect(_sys_win_service_name.signal_changed(),
                   std::bind(&ServerInstanceEditor::entry_changed, this, &_sys_win_service_name));

    _sys_win_hint_label.set_text(WINDOWS_CONFIG_HINT_TEXT);
    table->add(&_sys_win_hint_label, 1, 2, 5, 6, HFillFlag | HExpandFlag);

    _details_description.set_text(
      _("The following options specify how Workbench should perform certain management actions.\n"
        "Commands given here are the same as if they would be run in a system shell."));
    _details_description.set_style(SmallHelpTextStyle);
    _sys_box.add(&_details_description, false, true);

    _sys_box.add(&_details_panel, false, true);

    table = NewTable(3, 2);
    _details_panel.set_title(_("MySQL Management"));
#if defined(_MSC_VER) || defined(__APPLE__)
    _details_panel.add(manage(table));
#else
    // XXX tmp workaround for crash caused by changed destruction routine
    _details_panel.add(table);
#endif

    table->add(manage(RLabel(_("Start MySQL:"))), 0, 1, 0, 1, HFillFlag);
    table->add(&_start_cmd, 1, 2, 0, 1, HFillFlag | HExpandFlag);
    scoped_connect(_start_cmd.signal_changed(), std::bind(&ServerInstanceEditor::entry_changed, this, &_start_cmd));
    table->add(manage(RLabel(_("Stop MySQL:"))), 0, 1, 1, 2, HFillFlag);
    table->add(&_stop_cmd, 1, 2, 1, 2, HFillFlag | HExpandFlag);
    scoped_connect(_stop_cmd.signal_changed(), std::bind(&ServerInstanceEditor::entry_changed, this, &_stop_cmd));

    table->add(&_sudo_check, 1, 2, 2, 3, HFillFlag | HExpandFlag);
#ifndef _MSC_VER
    _sudo_check.set_text(_("Elevate privileges to execute start/stop commands\nand write configuration data"));
#else
    _sudo_check.set_text(
      _("Acquire administrator rights to execute start/stop commands\nand write configuration data"));
#endif

    _sudo_description.set_text(
      _("When sudo is used in Linux like systems, a certain set of parameters are passed to it.\n"
        "In certain environments, it may be necessary to override these parameters.\n"
        "Look at the Workbench log file to determine the parameters being currently used. Leave it blank if unsure."));
    _sudo_description.set_style(SmallHelpTextStyle);
    _sys_box.add(&_sudo_description, false, true);
    _custom_sudo_box.add(manage(RLabel(_("Override sudo command line:"))), false, true);
    _custom_sudo_box.add(&_sudo_prefix, true, true);

    scoped_connect(_sudo_prefix.signal_changed(), std::bind(&ServerInstanceEditor::entry_changed, this, &_sudo_prefix));
    scoped_connect(_sudo_check.signal_clicked(), std::bind(&ServerInstanceEditor::check_changed, this, &_sudo_check));

    _sys_box.add(&_custom_sudo_box, false, true);
  }

  _dup_inst_button.set_text(_("Duplicate"));
  scoped_connect(_dup_inst_button.signal_clicked(), std::bind(&ServerInstanceEditor::duplicate_instance, this));

  _del_inst_button.set_text(_("Delete"));
  scoped_connect(_del_inst_button.signal_clicked(), std::bind(&ServerInstanceEditor::delete_instance, this));
  _add_inst_button.set_text(_("New"));
  scoped_connect(_add_inst_button.signal_clicked(), std::bind(&ServerInstanceEditor::add_instance, this));

  _move_up_button.set_text(_("Move Up"));
  scoped_connect(_move_up_button.signal_clicked(), std::bind(&ServerInstanceEditor::reorder_instance, this, true));
  _move_down_button.set_text(_("Move Down"));
  scoped_connect(_move_down_button.signal_clicked(), std::bind(&ServerInstanceEditor::reorder_instance, this, false));

  _bottom_hbox.set_name("Button Bar");
  _bottom_hbox.add(&_add_inst_button, false, true);
  _bottom_hbox.add(&_del_inst_button, false, true);
  _bottom_hbox.add(&_dup_inst_button, false, true);
  _bottom_hbox.add(&_move_up_button, false, true);
  _bottom_hbox.add(&_move_down_button, false, true);

  _bottom_hbox.add_end(&_close_button, false, true);
  _bottom_hbox.add_end(&_test_button, false, true);
  //  _bottom_hbox.add_end(&_cancel_button, false, true);

  _close_button.set_text(_("Close"));

  _test_button.set_text(_("Test Connection"));
  //  _test_button.set_enabled(grt::GRT::get()->get_module("WbAdmin")!=0);
  scoped_connect(_test_button.signal_clicked(), std::bind(&ServerInstanceEditor::test_settings, this));

  _add_inst_button.enable_internal_padding(true);
  _del_inst_button.enable_internal_padding(true);
  _close_button.enable_internal_padding(true);
  _test_button.enable_internal_padding(true);

  _stored_connection_list.set_size(180, -1);

  set_content(&_top_vbox);

  _stored_connection_list.add_column(::mforms::StringColumnType, _("MySQL Connections"), 150, false);
  _stored_connection_list.end_columns();

  set_size(900, 600);
  center();

  _top_vbox.resume_layout();

  ///
  std::set<std::string> sys_types;

  std::string path = bec::GRTManager::get()->get_data_file_path("mysql.profiles");
  GDir *dir = g_dir_open(path.c_str(), 0, NULL);
  if (dir) {
    const gchar *file;
    while ((file = g_dir_read_name(dir))) {
      if (g_str_has_suffix(file, ".xml")) {
        std::string fname = std::string(file, strlen(file) - 4);
        std::string label = base::replaceString(fname, "_", " ");
        grt::DictRef dict;
        try {
          dict = grt::DictRef::cast_from(grt::GRT::get()->unserialize(path + "/" + file));
        } catch (std::exception &exc) {
          logWarning("Profile %s contains invalid data: %s\n", path.c_str(), exc.what());
          continue;
        }
        _presets[dict.get_string("sys.system")].push_back(std::make_pair(label, dict));

        sys_types.insert(dict.get_string("sys.system"));
      }
    }
    g_dir_close(dir);
  }

  // Fill in the system type selector
  for (std::set<std::string>::const_iterator iter = sys_types.begin(); iter != sys_types.end(); ++iter) {
    _os_type.add_item(*iter);
    std::sort(_presets[*iter].begin(), _presets[*iter].end());
  }
}

ServerInstanceEditor::~ServerInstanceEditor() {
  disconnect_scoped_connects();
  delete _connect_panel;
}

void ServerInstanceEditor::set_password(bool clear) {
  std::string port = _ssh_port.get_string_value();

  std::string storageKey;
  if (_ssh_remote_admin.get_active()) {
    // WBA stores password with key ssh@host:port
    storageKey = strfmt("ssh@%s:%s", _remote_host.get_string_value().c_str(), port.empty() ? "22" : port.c_str());
  } else
    storageKey = "wmi@" + _remote_host.get_string_value();
  std::string userName = _remote_user.get_string_value();

  if (userName.empty()) {
    mforms::Utilities::show_warning("Cannot Set Password", "Please fill the username to be used.", "OK");
    return;
  }

  if (clear) {
    try {
      mforms::Utilities::forget_password(storageKey, userName);
    } catch (std::exception &exc) {
      mforms::Utilities::show_error("Clear Password", base::strfmt("Could not clear password: %s", exc.what()), "OK");
    }
  } else {
    std::string password;

    try {
      if (mforms::Utilities::ask_for_password(_("Store Password For Server"), storageKey, userName, password))
        mforms::Utilities::store_password(storageKey, userName, password);
    } catch (std::exception &exc) {
      mforms::Utilities::show_error("Store Password", base::strfmt("Could not store password: %s", exc.what()), "OK");
    }
  }
  show_connection();
}

db_mgmt_ServerInstanceRef ServerInstanceEditor::run(db_mgmt_ConnectionRef select_connection, bool show_admin) {
  _top_vbox.suspend_layout();

  refresh_connection_list();

  int i = -1;
  if (select_connection.is_valid())
    i = (int)_connections.get_index(select_connection);

  if (i >= _stored_connection_list.count())
    i = 0;

  if (i != -1) {
    _stored_connection_list.select_node(_stored_connection_list.node_at_row((int)i));
    show_connection();
  }

  if (show_admin)
    _tabview.set_active_tab(2);

  _top_vbox.resume_layout();
  run_modal(NULL, &_close_button);

  grt::GRT::get()->call_module_function("Workbench", "saveConnections", grt::BaseListRef());
  grt::GRT::get()->call_module_function("Workbench", "saveInstances", grt::BaseListRef());

  return selected_instance();
}

void ServerInstanceEditor::run_filechooser_wrapper(
  mforms::TextEntry *entry) // Allows to run local or remote file selector
{
  db_mgmt_ServerInstanceRef instance(selected_instance());
  bool is_local = false;

  if (instance.is_valid())
    is_local = is_local_connection(instance->connection());

  if (is_local)
    run_filechooser(entry);
  else {
    grt::Module *module = grt::GRT::get()->get_module("WbAdmin");
    if (module) {
      grt::BaseListRef args(true);
      args.ginsert(instance->connection());
      args.ginsert(instance);

      try {
        grt::StringRef selection = grt::StringRef::cast_from(module->call_function("openRemoteFileSelector", args));
        if (!selection.empty()) {
          entry->set_value(selection.c_str());
          entry_changed(entry);
        }
      } catch (const std::exception &exc) {
        grt::GRT::get()->send_error("Error in remote file browser", exc.what());
      }
    }
  }
}

void ServerInstanceEditor::run_filechooser(mforms::TextEntry *entry) {
  mforms::FileChooser fc(mforms::OpenFile, true);
  // TODO: Add set directory
  if (fc.run_modal()) {
    const std::string path = fc.get_path();
    if (!path.empty() || path != "")
      entry->set_value(path);
    (*entry->signal_changed())();
  }
}

void ServerInstanceEditor::system_type_changed() {
  db_mgmt_ServerInstanceRef instance(selected_instance());

  if (instance.is_valid()) {
    std::string system = _os_type.get_string_value();
    if (!system.empty()) {
      instance->serverInfo().gset("sys.system", system);
      refresh_profile_list();
      profile_changed();
    }
  }
}

void ServerInstanceEditor::refresh_profile_list() {
  std::string system = _os_type.get_string_value();
  if (!system.empty()) {
    _sys_profile_type.clear();
    std::list<std::string> profiles;
    std::vector<std::pair<std::string, grt::DictRef> >::const_iterator iter = _presets[system].begin();
    for (; iter != _presets[system].end(); ++iter)
      profiles.push_back(iter->first);

    _sys_profile_type.add_items(profiles);
    _sys_profile_type.add_item(_("Custom"));
  }
}

void ServerInstanceEditor::refresh_connection_list() {
  _stored_connection_list.clear();

  GRTLIST_FOREACH(db_mgmt_Connection, _connections, conn) {
    TreeNodeRef node = _stored_connection_list.root_node()->add_child();
    node->set_string(0, *(*conn)->name());
  }
}

db_mgmt_ConnectionRef ServerInstanceEditor::selected_connection() {
  TreeNodeRef node = _stored_connection_list.get_selected_node();
  int row = _stored_connection_list.row_for_node(node);
  if (row >= 0)
    return _connections[row];

  return db_mgmt_ConnectionRef();
}

db_mgmt_ServerInstanceRef ServerInstanceEditor::selected_instance() {
  db_mgmt_ConnectionRef conn(selected_connection());
  if (conn.is_valid()) {
    GRTLIST_FOREACH(db_mgmt_ServerInstance, _instances, inst) {
      if ((*inst)->connection() == conn)
        return *inst;
    }
  }
  return db_mgmt_ServerInstanceRef();
}

void ServerInstanceEditor::autodetect_system() {
  grt::Module *module = grt::GRT::get()->get_module("WbAdmin");
  if (module) {
    grt::BaseListRef args(true);
    args.ginsert(selected_instance());

    module->call_function("detectInstanceSettings", args);
  }
}

void ServerInstanceEditor::test_settings() {
  if (_ssh_remote_admin.get_active()) {
    grt::Module *module = grt::GRT::get()->get_module("WbAdmin");
    if (module) {
      grt::BaseListRef args(true);
      grt::ValueRef ret;
      args.ginsert(selected_instance());

      ret = module->call_function("testInstanceSettings", args);
      grt::StringRef str(grt::StringRef::cast_from(ret));
      if (str.is_valid() && *str != "OK") {
        mforms::Utilities::show_error("Test Connection", *str, "OK", "", "");
        return;
      }
    } else
      logError("module WbAdmin not found\n");
  }
  _connect_panel->test_connection();
}

//--------------------------------------------------------------------------------------------------

void ServerInstanceEditor::toggle_administration() {
  db_mgmt_ServerInstanceRef instance(selected_instance());
  bool local_connection = false;
  bool ssh_administration = _ssh_remote_admin.get_active();
  bool win_administration = _win_remote_admin.get_active();

  if (instance.is_valid()) {
    local_connection = is_local_connection(instance->connection());

    // Note: remoteAdmin means actually SSH based remote administration.
    if (ssh_administration)
      instance->serverInfo().gset("remoteAdmin", 1);
    else
      instance->serverInfo().remove("remoteAdmin");

// Win admin and ssh admin are mutual exclusive. This semantic is enforced by radio buttons in the UI.
#ifdef _MSC_VER
    if (local_connection || win_administration)
#else
    if (win_administration)
#endif
      instance->serverInfo().gset("windowsAdmin", 1);
    else
      instance->serverInfo().remove("windowsAdmin");

    if (_remote_user.get_string_value().empty()) {
      const char *user = g_get_user_name();
      if (user)
        _remote_user.set_value(user);
    }

    if (_remote_host.get_string_value().empty()) {
      std::string value = instance->connection()->parameterValues().get_string("sshHost");
      if (value.empty() || win_administration)
        value = instance->connection()->parameterValues().get_string("hostName");
      std::size_t char_pos = value.rfind(":");
      if (std::string::npos != char_pos) {
        _remote_host.set_value(value.substr(0, char_pos));
        char_pos++;
        if (char_pos <= value.size())
          _ssh_port.set_value(value.substr(char_pos, std::string::npos));
      } else
        _remote_host.set_value(value);
    }
    if (ssh_administration) {
      instance->loginInfo().gset("ssh.hostName", _remote_host.get_string_value());
      instance->loginInfo().gset("ssh.userName", _remote_user.get_string_value());
    } else if (win_administration) {
      instance->loginInfo().gset("wmi.hostName", _remote_host.get_string_value());
      instance->loginInfo().gset("wmi.userName", _remote_user.get_string_value());
    }
    reset_setup_pending();
  }

  // Switch on server parameters if we have a remote connection.
  _remote_param_box.set_enabled(!_no_remote_admin.get_active());

  // Enable only fields relevant to that connection.
  _ssh_port.set_enabled(ssh_administration);
  _ssh_keypath.set_enabled(ssh_administration);
  _ssh_usekey.set_enabled(ssh_administration);

  //_sys_config_path_browse.set_enabled(is_local);

  // If this is a local connection or a remote one with remote management abilities then
  // enable the system commands section.
  bool can_administer = local_connection || ssh_administration || win_administration;
  _sys_box.set_enabled(can_administer);
}

void ServerInstanceEditor::tab_changed() {
  db_mgmt_ServerInstanceRef instance(selected_instance());
  if (!instance.is_valid()) {
    db_mgmt_ConnectionRef connection(selected_connection());
    if (connection.is_valid()) {
      grt::BaseListRef args(true);
      args.ginsert(connection);
      try {
        if (is_local_connection(connection))
          instance = db_mgmt_ServerInstanceRef::cast_from(
            grt::GRT::get()->call_module_function("WbAdmin", "autoDetectLocalInstance", args));
        else
          instance = db_mgmt_ServerInstanceRef::cast_from(
            grt::GRT::get()->call_module_function("WbAdmin", "autoDetectRemoteInstance", args));
      } catch (grt::module_error &exc) {
        logError("Error calling autoDetectRemoteInstance: %s, %s\n", exc.what(), exc.inner.c_str());
        mforms::Utilities::show_error("Auto Detect Server Configuration", exc.inner, "OK");
      } catch (std::exception &exc) {
        logError("Error calling autoDetectRemoteInstance: %s\n", exc.what());
        mforms::Utilities::show_error("Auto Detect Server Configuration", exc.what(), "OK");
      }
    }
  }

  if (instance.is_valid())
    show_instance_info(instance->connection(), instance);
}
//--------------------------------------------------------------------------------------------------
void ServerInstanceEditor::driver_changed_cb(const db_mgmt_DriverRef &driver) {
  db_mgmt_ConnectionRef connection(selected_connection());
  {
    if (_tabview.get_page_index(&_remote_admin_box) == -1)
      _tabview.add_page(&_remote_admin_box, _("Remote Management"));

    if (_tabview.get_page_index(&_sys_box) == -1)
      _tabview.add_page(&_sys_box, _("System Profile"));
  }
}

void ServerInstanceEditor::add_instance() {
  db_mgmt_ConnectionRef connection(grt::Initialized);
  std::string name = "new connection";
  TreeNodeRef node;
  bool dupe;
  int i = 1;
  do {
    dupe = false;
    GRTLIST_FOREACH(db_mgmt_Connection, _connections, conn) {
      if ((*conn)->name() == name) {
        name = strfmt("new connection %i", i++);
        dupe = true;
        break;
      }
    }
  } while (dupe);

  node = _stored_connection_list.root_node()->add_child();
  if (node) {
    node->set_string(0, name);
    _stored_connection_list.select_node(node);

    connection->owner(_mgmt);
    connection->name(name);
    connection->driver(_connect_panel->selected_rdbms()->defaultDriver());
    if (find_named_object_in_list(connection->driver()->parameters(), "useSSL").is_valid()) {
      // prefer SSL if possible by default
      connection->parameterValues().set("useSSL", grt::IntegerRef(1));
    }
    _connections.insert(connection);
    _connect_panel->set_connection(connection);
  }
  show_connection();
}

void ServerInstanceEditor::delete_instance() {
  TreeNodeRef node(_stored_connection_list.get_selected_node());

  if (node) {
    int row = _stored_connection_list.row_for_node(node);

    if (row >= 0 && row < (int)_connections.count()) {
      db_mgmt_ConnectionRef conn(_connections[row]);
      for (ssize_t i = _instances.count() - 1; i >= 0; --i) {
        if (_instances[i]->connection() == conn)
          _instances.remove(i);
      }

      _connections.remove(row);
      node->remove_from_parent();
      _stored_connection_list.select_node(_stored_connection_list.node_at_row(row - 1));

      show_connection();
    } else
      refresh_connection_list(); // shouldn't happen
  }
}

void ServerInstanceEditor::duplicate_instance() {
  db_mgmt_ConnectionRef orig_conn(selected_connection());
  db_mgmt_ConnectionRef copy_conn(grt::Initialized);
  db_mgmt_ServerInstanceRef orig_inst(selected_instance());
  db_mgmt_ServerInstanceRef copy_inst(grt::Initialized);

  if (!orig_conn.is_valid())
    return;

  std::string name = orig_conn->name();

  name = grt::get_name_suggestion_for_list_object(_connections, name);

  copy_conn->owner(orig_conn->owner());
  copy_conn->name(name);
  copy_conn->driver(orig_conn->driver());
  grt::merge_contents(copy_conn->parameterValues(), orig_conn->parameterValues(), true);
  copy_conn->hostIdentifier(orig_conn->hostIdentifier());

  if (orig_inst.is_valid()) {
    copy_inst->owner(orig_inst->owner());
    copy_inst->name(copy_conn->name());
    copy_inst->connection(copy_conn);
    grt::merge_contents(copy_inst->loginInfo(), orig_inst->loginInfo(), true);
    grt::merge_contents(copy_inst->serverInfo(), orig_inst->serverInfo(), true);
  }
  _connections.insert(copy_conn);
  if (orig_inst.is_valid())
    _instances.insert(copy_inst);

  TreeNodeRef node(_stored_connection_list.root_node()->add_child());
  if (node) {
    node->set_string(0, name);
    _stored_connection_list.select_node(node);
  }
  show_connection();
}

void ServerInstanceEditor::reorder_instance(bool up) {
  int row = _stored_connection_list.get_selected_row();

  if (row < 0)
    return;

  if (up) {
    if (row > 0) {
      _connections.reorder(row, row - 1);
      _stored_connection_list.select_node(_stored_connection_list.node_at_row(row - 1));
    }
  } else {
    if (row < _stored_connection_list.root_node()->count() - 1) {
      _connections.reorder(row, row + 1);
      _stored_connection_list.select_node(_stored_connection_list.node_at_row(row + 1));
    }
  }

  row = 0;
  GRTLIST_FOREACH(db_mgmt_Connection, _connections, conn) {
    _stored_connection_list.root_node()->get_child(row++)->set_string(0, (*conn)->name().c_str());
  }
}

void ServerInstanceEditor::browse_file() {
  FileChooser fsel(mforms::OpenFile, true);

  fsel.set_title(_("Pick SSH Private Key"));

  if (fsel.run_modal()) {
    _ssh_keypath.set_value(fsel.get_path());
    entry_changed(&_ssh_keypath);
  }
}

grt::DictRef ServerInstanceEditor::get_preset(const std::string &system, const std::string &preset_name) {
  grt::DictRef result;

  for (std::vector<std::pair<std::string, grt::DictRef> >::const_iterator iter = _presets[system].begin();
       iter != _presets[system].end(); ++iter) {
    if (iter->first == preset_name) {
      result = iter->second;
      break;
    }
  }

  return result;
}

void ServerInstanceEditor::entry_changed(mforms::TextEntry *sender) {
  const std::string value = base::trim(sender->get_string_value());
  db_mgmt_ConnectionRef connection(selected_connection());
  db_mgmt_ServerInstanceRef instance(selected_instance());

  if (connection.is_valid()) {
    if (&_name_entry == sender) {
      std::string text = value;
      if (!_contains_group) {
        _connect_panel->connection_user_input(text, _contains_group, false);
        _name_entry.set_value(text);
      }
      connection->name(text);
      mforms::TreeNodeRef node(_stored_connection_list.get_selected_node());
      if (node)
        node->set_string(0, text);
    }
  }

  if (instance.is_valid()) {
    if (&_name_entry == sender)
      instance->name(value);
    else if (&_remote_host == sender) {
      if (_ssh_remote_admin.get_active())
        instance->loginInfo().gset("ssh.hostName", value);
      else
        instance->loginInfo().gset("wmi.hostName", value);
    } else if (&_ssh_port == sender) {
      instance->loginInfo().gset("ssh.tunnelPort", value);
      instance->loginInfo().gset("ssh.port", value);
    } else if (&_remote_user == sender) {
      if (_ssh_remote_admin.get_active())
        instance->loginInfo().gset("ssh.userName", value);
      else
        instance->loginInfo().gset("wmi.userName", value);
    } else if (&_ssh_keypath == sender) {
      instance->loginInfo().gset("ssh.key", value);
      instance->loginInfo().gset("ssh.useKey", 1);
      _ssh_usekey.set_active(true);
    } else if (&_sys_config_path == sender) {
      instance->serverInfo().gset("sys.config.path", value);
      _sys_profile_type.set_selected(_sys_profile_type.get_item_count() - 1);
      instance->serverInfo().gset("sys.preset", "");
    } else if (&_sys_myini_section == sender) {
      instance->serverInfo().gset("sys.config.section", value);
      _sys_profile_type.set_selected(_sys_profile_type.get_item_count() - 1);
      instance->serverInfo().gset("sys.preset", "");
    } else if (&_sys_win_service_name == sender) {
      instance->serverInfo().gset("sys.mysqld.service_name", value);
      instance->serverInfo().gset("sys.preset", "");
    } else if (&_start_cmd == sender) {
      instance->serverInfo().gset("sys.mysqld.start", value);
      _sys_profile_type.set_selected(_sys_profile_type.get_item_count() - 1);
      instance->serverInfo().gset("sys.preset", "");
    } else if (&_stop_cmd == sender) {
      instance->serverInfo().gset("sys.mysqld.stop", value);
      _sys_profile_type.set_selected(_sys_profile_type.get_item_count() - 1);
      instance->serverInfo().gset("sys.preset", "");
    } else if (&_sudo_prefix == sender) {
      instance->serverInfo().gset("sys.mysqld.sudo_override", value);
      _sys_profile_type.set_selected(_sys_profile_type.get_item_count() - 1);
      instance->serverInfo().gset("sys.preset", "");
    }
    reset_setup_pending();
  }
}

void ServerInstanceEditor::check_changed(mforms::CheckBox *sender) {
  const bool value = sender->get_active();
  db_mgmt_ServerInstanceRef instance(selected_instance());

  if (instance.is_valid()) {
    grt::DictRef info(instance->serverInfo());

    if (&_ssh_usekey == sender)
      instance->loginInfo().gset("ssh.useKey", value ? 1 : 0);
    else if (&_sudo_check == sender) {
      if (_os_type.get_string_value() != "Windows") {
        _sudo_description.show(value);
        _custom_sudo_box.show(value);
      }
      info.gset("sys.usesudo", value ? 1 : 0);
    }
  }
}

/*
void ServerInstanceEditor::connection_changed()
{
  const int conn= 0;//_connection_selector.get_selected_index();
  db_mgmt_ServerInstanceRef instance(selected_instance());

  if (instance.is_valid() && conn >= 0 && conn < (int)_mgmt->storedConns().count())
  {
    db_mgmt_ConnectionRef connection(_mgmt->storedConns()[conn]);
    if (is_local_connection(connection))
    {
      // If the MySQL connection is to a local server then remote administration makes no sense.
      // Disable it in this case.
      _no_remote_admin.set_active(true);
      _win_remote_admin.set_enabled(false);
      _ssh_remote_admin.set_enabled(false);

      instance->loginInfo().gset("ssh.hostName", "");
      instance->loginInfo().gset("wmi.hostName", "");
      _remote_host.set_value("");
    }
    else
    {
#ifdef _MSC_VER
      _win_remote_admin.set_enabled(true);
#else
      _win_remote_admin.set_enabled(false);
#endif
      _ssh_remote_admin.set_enabled(true);

      const std::string driver= connection->driver().is_valid() ? connection->driver()->name() : "";

      // If the MySQL connection is SSH based then copy its settings to the management settings
      // as an easier starting point.
      if (driver == "MysqlNativeSSH")
      {
        std::string host = connection->parameterValues().get_string("sshHost");
        std::string port;

        if (host.find(':') != std::string::npos)
        {
          port = host.substr(host.find(':')+1);
          host = host.substr(0, host.find(':'));
        }
        else
          port = "22";

        _remote_host.set_value(host);
        _ssh_port.set_value(port);
        _remote_user.set_value(connection->parameterValues().get_string("sshUserName"));
        _ssh_keypath.set_value(connection->parameterValues().get_string("sshKeyFile"));

        instance->loginInfo().gset("ssh.hostName", host);
        instance->loginInfo().gset("ssh.port", port);
        instance->loginInfo().gset("ssh.userName", _remote_user.get_string_value());
        instance->loginInfo().gset("ssh.useKey", _ssh_keypath.get_string_value() != "");
        instance->loginInfo().gset("ssh.key", _ssh_keypath.get_string_value());
      }
      else
      {
        const std::string hostname= connection->parameterValues().get_string("hostName");

        if (!hostname.empty())
          instance->loginInfo().gset("wmi.hostName", hostname);
        _remote_host.set_value(hostname);
      }
    }
    instance->connection(connection);
  }

  toggle_administration();
}*/

void ServerInstanceEditor::profile_changed() {
  db_mgmt_ServerInstanceRef instance(selected_instance());
  const int systype = _sys_profile_type.get_selected_index();
  if (systype >= 0 && instance.is_valid()) {
    const std::string system = instance->serverInfo().get_string("sys.system");
    if (systype < (int)_presets[system].size()) {
      const std::string name = _presets[system][systype].first;
      grt::DictRef dict = _presets[system][systype].second;

      grt::merge_contents(instance->serverInfo(), dict, true);
      instance->serverInfo().gset("sys.preset", name);
      reset_setup_pending();
      show_connection();
    }
  }
}

void ServerInstanceEditor::reset_setup_pending() {
  db_mgmt_ServerInstanceRef instance(selected_instance());
  if (instance.is_valid())
    instance->serverInfo().gset("setupPending", 0);
}

//--------------------------------------------------------------------------------------------------

void ServerInstanceEditor::show_connection() {
  db_mgmt_ConnectionRef connection = selected_connection();
  db_mgmt_ServerInstanceRef instance = selected_instance();

  _connect_panel->set_active_stored_conn(connection);

  bool valid_connection = connection.is_valid();

  {
    if (_tabview.get_page_index(&_remote_admin_box) == -1)
      _tabview.add_page(&_remote_admin_box, _("Remote Management"));

    if (_tabview.get_page_index(&_sys_box) == -1)
      _tabview.add_page(&_sys_box, _("System Profile"));
  }

  _content_box.set_enabled(valid_connection);
  _move_up_button.set_enabled(valid_connection);
  _move_down_button.set_enabled(valid_connection);
  _del_inst_button.set_enabled(valid_connection);
  _dup_inst_button.set_enabled(valid_connection);
  _test_button.set_enabled(valid_connection);

  _contains_group = false;
  if (valid_connection) {
    std::string text = connection->name();
    std::size_t pos = text.find_first_of("/");
    if (pos != std::string::npos)
      _contains_group = true;
  }
  _name_entry.set_value(valid_connection ? connection->name() : "");

  show_instance_info(connection, instance);
}

//--------------------------------------------------------------------------------------------------

void ServerInstanceEditor::show_instance_info(db_mgmt_ConnectionRef connection, db_mgmt_ServerInstanceRef instance) {
  grt::DictRef serverInfo(instance.is_valid() ? instance->serverInfo() : grt::DictRef(true));
  grt::DictRef defaults;

  int j;
  const std::string system = serverInfo.get_string("sys.system");
  _os_type.set_value(system);

  refresh_profile_list();

  bool found = false;
  const std::string preset = serverInfo.get_string("sys.preset");
  j = 0;
  for (std::vector<std::pair<std::string, grt::DictRef> >::const_iterator iter = _presets[system].begin();
       iter != _presets[system].end(); ++iter, ++j) {
    if (iter->first == preset) {
      defaults = iter->second;
      found = true;
      _sys_profile_type.set_selected(j);
      break;
    }
  }

  if (!found)
    _sys_profile_type.set_selected(_sys_profile_type.get_item_count() - 1);

  bool local_connection = is_local_connection(connection);

  // Remote administration settings.

  // Admin settings are adjusted implicitly if we are not running on Windows but the profile is set for
  // remote Windows administration. Similar for enabled remote management but a local MySQL connection.
  if (serverInfo.get_int("remoteAdmin") != 0)
    _ssh_remote_admin.set_active(true);
  else
#ifdef _MSC_VER
    if (serverInfo.get_int("windowsAdmin") != 0)
    _win_remote_admin.set_active(true);
  else
#endif
    _no_remote_admin.set_active(true);

  grt::DictRef loginInfo(instance.is_valid() ? instance->loginInfo() : grt::DictRef(true));

  std::string storageKey;
  std::string port = _ssh_port.get_string_value();
  std::string userName;
  if (_ssh_remote_admin.get_active()) {
    _remote_host.set_value(loginInfo.get_string("ssh.hostName"));
    _remote_user.set_value(loginInfo.get_string("ssh.userName"));
    userName = _remote_user.get_string_value();
    // WBA stores password key as "ssh@<host>:<port>"
    storageKey = strfmt("ssh@%s:%s", _remote_host.get_string_value().c_str(), port.empty() ? "22" : port.c_str());
  } else {
    _remote_host.set_value(loginInfo.get_string("wmi.hostName"));
    _remote_user.set_value(loginInfo.get_string("wmi.userName"));
    userName = _remote_user.get_string_value();
    storageKey = "wmi@" + _remote_host.get_string_value();
  }
  _ssh_port.set_value(loginInfo.get_string("ssh.port"));
  _ssh_usekey.set_active(loginInfo.get_int("ssh.useKey") != 0);
  _ssh_keypath.set_value(loginInfo.get_string("ssh.key"));

  std::string dummy;

  if (instance.is_valid() && !userName.empty() && mforms::Utilities::find_password(storageKey, userName, dummy))
    _password_clear.set_enabled(true);
  else
    _password_clear.set_enabled(false);

  // Hide system specifics if the target is Windows with wmi.
  if (system == "Windows" && serverInfo.get_int("windowsAdmin")) {
    _details_description.show(false);
    _details_panel.show(false);
    _sys_win_service_name.show(true);
    _sys_win_service_name_label->show(true);
    _sys_win_hint_label.show(true);
  } else {
    _details_description.show(true);
    _details_panel.show(true);
    _sys_win_service_name.show(false);
    _sys_win_service_name_label->show(false);
    _sys_win_hint_label.show(false);
  }

  // If the MySQL connection is to a local server then remote administration makes no sense.
  // Disable it in this case.
  if (local_connection) {
    _no_remote_admin.set_active(true);
    _win_remote_admin.set_enabled(false);
    _ssh_remote_admin.set_enabled(false);
  } else {
#ifdef _MSC_VER
    _win_remote_admin.set_enabled(true);
#else
    _win_remote_admin.set_enabled(false);
#endif
    _ssh_remote_admin.set_enabled(true);
  }
  toggle_administration();

  // Disables the config path controls if locked: means it was found automatically
  // i.e. on the linux command call for ths server or on the windows service parameter
  bool enable_config_path = serverInfo.get_int("sys.config.path.lock", 0) != 1;
  _sys_config_path.set_value(serverInfo.get_string("sys.config.path"));
  _sys_config_path.set_enabled(enable_config_path);
  _sys_config_path_browse.set_enabled(enable_config_path);

  _sys_myini_section.set_value(serverInfo.get_string("sys.config.section"));
  if (system == "Windows")
    _sys_win_service_name.set_value(serverInfo.get_string("sys.mysqld.service_name"));

  _start_cmd.set_value(serverInfo.get_string("sys.mysqld.start"));
  _stop_cmd.set_value(serverInfo.get_string("sys.mysqld.stop"));
  _sudo_prefix.set_value(serverInfo.get_string("sys.mysqld.sudo_override"));

  _sudo_check.set_active(serverInfo.get_int("sys.usesudo", 1) != 0);

  bool show_custom_sudo = system != "Windows" && _sudo_check.get_active();

  _sudo_description.show(show_custom_sudo);
  _custom_sudo_box.show(show_custom_sudo);

  if (serverInfo.has_key("sys.sudo")) {
    if (serverInfo.get_string("sys.sudo") == "" && defaults.is_valid() && defaults.has_key("sys.sudo"))
      serverInfo.gset("sys.sudo", defaults.get_string("sys.sudo"));
  } else {
    if (defaults.is_valid() && defaults.has_key("sys.sudo"))
      serverInfo.gset("sys.sudo", defaults.get_string("sys.sudo"));
    else
      serverInfo.gset("sys.sudo", "");
  }
}
