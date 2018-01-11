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

#pragma once

#include "grt/common.h"
#include "grtui/grt_wizard_form.h"
#include "grtui/wizard_finished_page.h"
#include "grtui/wizard_progress_page.h"
#include "grtui/grtdb_connect_panel.h"

#include "workbench/wb_context.h"
#include "workbench/wb_module.h"

#include "cppdbc.h"

#include "mforms/fs_object_selector.h"
#include "mforms/radiobutton.h"
#include "mforms/checkbox.h"

/**
 * Implementation of a wizard to set up remote management for a given connection.
 */

using namespace grtui;
using namespace mforms;

class NewServerInstanceWizard;

class NewServerInstancePage : public WizardPage {
public:
  NewServerInstancePage(WizardForm* form, const std::string& pageid);

protected:
  NewServerInstanceWizard* wizard();
};

class IntroductionPage : public WizardPage {
public:
  IntroductionPage(WizardForm* form);
};

class TestDatabaseSettingsPage : public WizardProgressPage {
public:
  TestDatabaseSettingsPage(WizardForm* host);
  virtual void enter(bool advancing);

protected:
  sql::ConnectionWrapper _dbc_conn;
  std::string _message;
  bool open_connection();
  virtual void tasks_finished(bool success);
  bool get_server_version();
  bool get_server_platform();

  NewServerInstanceWizard* wizard();
};

class HostAndRemoteTypePage : public NewServerInstancePage {
public:
  HostAndRemoteTypePage(WizardForm* host);

protected:
  virtual void enter(bool advancing);
  virtual bool advance();
  virtual bool skip_page();

  void refresh_profile_list();
  void toggle_remote_admin();

private:
  Panel _management_type_panel; // Border.
  Box _management_type_box;     // Content.
  Panel _os_panel;              // Border.
  Box _os_box;                  // Content.

  Label _os_description;

  Table _params;
  Label _os_label;
  Selector _os_selector;

  Label _type_label;
  Selector _type_selector;

  mforms::RadioButton _win_remote_admin;
  mforms::RadioButton _ssh_remote_admin;

  std::map<std::string, std::vector<std::pair<std::string, std::string> > > _presets;
};

class SSHConfigurationPage : public NewServerInstancePage {
public:
  SSHConfigurationPage(WizardForm* host);

protected:
  void use_ssh_key_changed();

  virtual void enter(bool advancing);
  virtual bool advance();
  virtual void leave(bool advancing);
  virtual bool skip_page();

private:
  Label _main_description1;
  Label _main_description2;

  Table _ssh_settings_table;

  Box _indent;
  Label _host_name_label;
  TextEntry _host_name;
  Label _port_label;
  TextEntry _port;

  Label _username_label;
  TextEntry _username;

  CheckBox _use_ssh_key;
  Label _ssh_path_label;
  TextEntry _ssh_key_path;
  Button _ssh_key_browse_button;
  FsObjectSelector* _file_selector;
};

class WindowsManagementPage : public NewServerInstancePage {
public:
  WindowsManagementPage(WizardForm* host, wb::WBContext* context);

protected:
  void refresh_config_path();

  virtual void enter(bool advancing);
  virtual void leave(bool advancing);

  virtual bool advance();
  virtual bool skip_page();

private:
  wb::WBContext* _context;
  std::vector<std::string> _config_paths;
  std::vector<std::string> _service_names;

  Table _layout_table;
  Box _indent;

  Label _main_description1;
  Label _main_description2;

  Label _service_label;
  TextEntry _service_name;
  Selector _service_selector;
  Label _progress_label;

  Label _config_path_label;
  TextEntry _config_path;
  Button _browse_button;
  FsObjectSelector* _file_selector;
};

class TestHostMachineSettingsPage : public WizardProgressPage {
public:
  TestHostMachineSettingsPage(WizardForm* host);

  virtual void enter(bool advance);
  virtual void leave(bool advancing);

protected:
  bool connect_to_host();
  bool find_config_file();
  bool find_error_files();
  bool check_admin_commands();
  virtual void tasks_finished(bool success);
  virtual bool skip_page();

  NewServerInstanceWizard* wizard();

private:
  TaskRow* _connect_task;
  TaskRow* _commands_task;
};

class ReviewPage : public NewServerInstancePage {
public:
  ReviewPage(WizardForm* host);

protected:
  virtual void enter(bool advancing);
  virtual void leave(bool advancing);
  virtual bool skip_page();
  virtual bool next_closes_wizard();
  virtual std::string close_caption() const {
    return finish_caption();
  }

  void customize_changed();

private:
  Label _description;
  Table _content;
  Label _label;

  TextBox _text;

  CheckBox _customize_check;
};

class PathsPage : public NewServerInstancePage {
public:
  PathsPage(WizardForm* host, wb::WBContext* context);

protected:
  virtual void enter(bool advancing);
  virtual bool advance();
  virtual bool skip_page();
  void browse_remote_config_file();
  void test_path();
  void test_section();

private:
  wb::WBContext* _context;

  Label _description;
  Table _content;

  Label _version_label;
  TextEntry _version;

  Label _config_path_label;
  TextEntry _config_path;
  Button _browse_button;
  FsObjectSelector* _file_selector;
  Button _test_config_path_button;
  Label _test_config_path_description;

  Label _section_name_label;
  TextEntry _section_name;
  Button _test_section_button;
  Label _test_section_description;
};

class CommandsPage : public NewServerInstancePage {
public:
  CommandsPage(WizardForm* host);

protected:
  virtual void enter(bool advancing);
  virtual void leave(bool advancing);
  virtual bool advance();
  virtual bool skip_page();
  virtual bool next_closes_wizard() {
    return true;
  }
  virtual std::string close_caption() const {
    return finish_caption();
  }

private:
  Label _description;
  Table _content;

  Label _start_label;
  TextEntry _start_command;
  Label _stop_label;
  TextEntry _stop_command;

  CheckBox _use_sudo;
};

class NewServerInstanceWizard : public WizardForm {
public:
  NewServerInstanceWizard(wb::WBContext* context, db_mgmt_ConnectionRef connection);
  ~NewServerInstanceWizard();

  db_mgmt_ServerInstanceRef assemble_server_instance();
  grt::ValueRef test_setting_grt(const std::string& name);

  void load_defaults();
  std::string get_server_info(const std::string& key);

  wb::WBContext* wb() {
    return _context;
  }

  bool is_admin_enabled();
  bool is_local();
  bool test_setting(const std::string& name, std::string& detail);

  void create_instance();

protected:
  wb::WBContext* _context;

  db_mgmt_ConnectionRef _connection;   // The connection for which we are configuring the server instance.
  db_mgmt_ServerInstanceRef _instance; // The server instance we are working on.

private:
  IntroductionPage* _introduction_page;
  TestDatabaseSettingsPage* _test_database_settings_page;
  HostAndRemoteTypePage* _os_page;
  SSHConfigurationPage* _ssh_configuration_page;
  WindowsManagementPage* _windows_connection_page;
  TestHostMachineSettingsPage* _test_host_machine_settings_page;
  ReviewPage* _review_page;
  PathsPage* _paths_page;
  CommandsPage* _commands_page;
};
