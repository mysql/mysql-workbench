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

#include "grtdb/db_helpers.h"
#include "base/string_utilities.h"
#include "new_server_instance_wizard.h"
#include "grt/grt_manager.h"
#include "grtui/grtdb_connection_editor.h"

#include "mforms/uistyle.h"
#include "base/log.h"
DEFAULT_LOG_DOMAIN(DOMAIN_WB_CONTEXT_UI)

#define INTRO_TEXT                                                                                 \
  "This wizard will guide you through the creation of a Server Profile to manage a MySQL server. " \
  "To fully support management of a remote MySQL server, an SSH daemon must be running "           \
  "on the target machine. Alternatively, if you are going to manage a Windows server from a "      \
  "Windows computer, you can also use native Windows management tools. "                           \
  "Remote management is used to start and stop a server and do server configuration. "             \
  "You may create a Profile without remote management if you do not need that functionality."

static struct {
  const char *pattern;
  const char *os_name;
} platform_strings[] = {
  {"apple-darwin", "macOS"}, // For macOS there's an additional check.
  {"-linux", "Linux"},
  {"win64", "Windows"}, {"win32", "Windows"},
  {NULL, NULL},
};

using namespace base;
using namespace mforms;

//----------------- NewServerInstancePage ----------------------------------------------------------

NewServerInstancePage::NewServerInstancePage(WizardForm *form, const std::string &pageid) : WizardPage(form, pageid) {
}

//--------------------------------------------------------------------------------------------------

NewServerInstanceWizard *NewServerInstancePage::wizard() {
  return dynamic_cast<NewServerInstanceWizard *>(_form);
}

//----------------- IntroductionPage ---------------------------------------------------------------

IntroductionPage::IntroductionPage(WizardForm *form) : WizardPage(form, "introduction-page") {
  set_short_title(_("Introduction"));
  set_title(_("Introduction"));

  mforms::Label *text = mforms::manage(new mforms::Label());
  text->set_text(
    _("This dialog will help you to set up remote management for your connection. At the start "
      "a connection attempt is made to determine server version and operating system of the target "
      "machine. This allows you to validate the connection settings and allows the wizard to pick "
      "a meaningful configuration preset. If this attempt fails you can still continue, however.\n\n"
      "Continue to the next page to start the connection. This might take a few moments."));
  text->set_wrap_text(true);
  add(text, false, true);
}

//----------------- TestDatabaseSettingsPage -------------------------------------------------------

TestDatabaseSettingsPage::TestDatabaseSettingsPage(WizardForm *host)
  : WizardProgressPage(host, "test database settings page", true) {
  set_short_title(_("Test DB Connection"));
  set_title(_("Testing the Database Connection"));

  set_heading(
    _("The database connection information is being tested. This might take a few "
      "moments depending on your network connection."));

  add_task(_("Open Database Connection"), std::bind(&TestDatabaseSettingsPage::open_connection, this),
           _("Connecting to database server..."));

  add_task(_("Get Server Version"), std::bind(&TestDatabaseSettingsPage::get_server_version, this),
           _("Querying server version..."));

  add_task(_("Get Server OS"), std::bind(&TestDatabaseSettingsPage::get_server_platform, this),
           _("Querying server OS type..."));

  /*XXX Improve rights checks.
   add_task(_("Check Account Privileges"),
   std::bind(&TestDatabaseSettingsPage::get_server_platform, this),
   _("Checking if account being used has enough privileges..."));
   - check the privs the account has and warn if it can't do something:
   - creating and modifying user accts
   - querying for tables
   - check if it has global schema rights and if not, warn dumps will only work for databases it has access to
   - other stuff
   */

  end_adding_tasks(_("Database connection tested successfully."));

  set_status_text("");
}

//--------------------------------------------------------------------------------------------------

bool TestDatabaseSettingsPage::open_connection() {
  try {
    db_mgmt_ConnectionRef conn(db_mgmt_ConnectionRef::cast_from(values().get("connection")));
    add_log_text(strfmt("Connecting to MySQL server %s...", conn->name().c_str()));
    _dbc_conn = sql::DriverManager::getDriverManager()->getConnection(conn);
    add_log_text("Connected.");
  } catch (std::exception &exc) {
    _message = exc.what();
    add_log_text(_message.c_str());
    throw;
  }

  return true;
}

//--------------------------------------------------------------------------------------------------

void TestDatabaseSettingsPage::tasks_finished(bool success) {
  if (!success)
    set_status_text(
      strfmt("Could not connect to MySQL server:\n  %s\nYou may continue if the server is simply not running.",
             _message.c_str()),
      true);
}

//--------------------------------------------------------------------------------------------------

bool TestDatabaseSettingsPage::get_server_version() {
  sql::Statement *pstmt = _dbc_conn->createStatement();
  sql::ResultSet *res = pstmt->executeQuery("SELECT VERSION() as VERSION");
  std::string version;

  if (res && res->next()) {
    version = res->getString("VERSION");
  }
  delete res;
  delete pstmt;

  if (version.empty()) {
    current_task()->label.set_text("Server Version: unknown");
    throw std::runtime_error("Error querying version of MySQL server");
  }

  values().gset("server_version", version);

  current_task()->label.set_text("Server Version: " + version);
  add_log_text(strfmt("MySQL server version is %s", version.c_str()));

  // check that we're connecting to a known and supported version of the server
  if (!bec::is_supported_mysql_version(version)) {
    current_task()->label.set_text("Get Server Version: Unsupported Server Version");
    std::string msg = strfmt(
      "Unknown/unsupported server version or connection protocol detected (%s).\nMySQL Workbench is developed and "
      "tested for MySQL Server versions 5.6 and newer.\nA connection can be established but some MySQL Workbench "
      "features may not work properly.\nFor MySQL Server version older than 5.6, please use MySQL Workbench 6.3.",
      version.c_str());
    add_log_text(msg);
    throw std::runtime_error(msg);
  }

  return true;
}

//--------------------------------------------------------------------------------------------------

/**
 * This functions attempts to find a clue on which OS this server is running by examining
 * on which is was built. There's usually a good correlation, even though it may not be very precise.
 */
bool TestDatabaseSettingsPage::get_server_platform() {
  sql::Statement *pstmt = _dbc_conn->createStatement();
  sql::ResultSet *res = pstmt->executeQuery("SHOW VARIABLES LIKE 'version_compile_%'");
  std::string name, value;
  std::string machine, os;

  while (res && res->next()) {
    name = res->getString("Variable_name");
    value = res->getString("Value");

    if (name == "version_compile_machine")
      machine = value;
    if (name == "version_compile_os")
      os = value;
  }
  delete res;
  delete pstmt;

  _dbc_conn.reset();

  os = base::tolower(os);
  std::string os_type = "";
  if (base::hasPrefix(os, "macos"))
    os_type = "macOS";

  if (os.empty()) {
    for (int i = 0; platform_strings[i].pattern; i++) {
      if (strstr(os.c_str(), platform_strings[i].pattern)) {
        os_type = platform_strings[i].os_name;
        values().gset("detected_os_type", os_type);
        break;
      }
    }
  }

  if (os_type.empty())
    os_type = "unknown";
  current_task()->label.set_text("Server OS: " + os_type);

  add_log_text(strfmt("MySQL server architecture is %s", machine.empty() ? "unknown" : machine.c_str()));
  add_log_text(strfmt("MySQL server OS is %s", os.empty() ? "unknown" : os.c_str()));

  return true;
}

//--------------------------------------------------------------------------------------------------

void TestDatabaseSettingsPage::enter(bool advancing) {
  if (advancing) {
    values().remove("server_version");
    values().remove("detected_os_type");
  }
  WizardProgressPage::enter(advancing);
}

//--------------------------------------------------------------------------------------------------

NewServerInstanceWizard *TestDatabaseSettingsPage::wizard() {
  return dynamic_cast<NewServerInstanceWizard *>(_form);
}

//----------------- HostAndRemoteTypePage ----------------------------------------------------------

HostAndRemoteTypePage::HostAndRemoteTypePage(WizardForm *host)
  : NewServerInstancePage(host, "os + remote page"),
    _management_type_panel(TitledBoxPanel),
    _management_type_box(false),
    _os_panel(TitledBoxPanel),
    _os_box(false),
    _win_remote_admin(RadioButton::new_id()),
    _ssh_remote_admin(_win_remote_admin.group_id()) {
  set_short_title(_("Management and OS"));

  // Remote management.
  _management_type_panel.set_title(_("Select the type of remote management you want to use:"));
  _management_type_panel.add(&_management_type_box);

  _win_remote_admin.set_text(_("Native Windows remote management (only available on Windows)"));
  scoped_connect(_win_remote_admin.signal_clicked(), std::bind(&HostAndRemoteTypePage::toggle_remote_admin, this));
#ifndef _MSC_VER
  _win_remote_admin.set_enabled(false);
#endif
  _ssh_remote_admin.set_text(_("SSH login based management"));
  scoped_connect(_ssh_remote_admin.signal_clicked(), std::bind(&HostAndRemoteTypePage::toggle_remote_admin, this));

  _management_type_box.add(&_win_remote_admin, false, true);
  _management_type_box.add(&_ssh_remote_admin, false, true);

#ifdef _MSC_VER
  _win_remote_admin.set_active(true);
#else
  _ssh_remote_admin.set_active(true);
#endif

  _management_type_box.set_spacing(8);
  _management_type_box.set_padding(10);
  add(&_management_type_panel, false, true);

  // OS selection.
  _os_panel.set_title(_("Operating System Selection"));
  _os_panel.add(&_os_box);

  _os_description.set_wrap_text(true);
  _os_description.set_text(
    _("Select the operating system and the type of database installation "
      "on the target machine. If you configure a Linux target and you are unsure about the type of "
      "database installation select the (Vendor Package) variant. If your specific operating system is not in this "
      "list, select "
      "a related variant. It can later be customized, if needed."));
  _os_box.add(&_os_description, false, true);

  _params.set_row_count(2);
  _params.set_column_count(2);
  _params.set_row_spacing(MF_TABLE_ROW_SPACING);
  _params.set_column_spacing(MF_TABLE_COLUMN_SPACING);

  _os_label.set_text_align(mforms::MiddleRight);
  _os_label.set_text(_("Operating System:"));
  _params.add(&_os_label, 0, 1, 0, 1, mforms::HFillFlag);
  _params.add(&_os_selector, 1, 2, 0, 1, mforms::HFillFlag | mforms::HExpandFlag);
  scoped_connect(_os_selector.signal_changed(), std::bind(&HostAndRemoteTypePage::refresh_profile_list, this));

  _type_label.set_text_align(mforms::MiddleRight);
  _type_label.set_text(_("MySQL Installation Type:"));
  _params.add(&_type_label, 0, 1, 1, 2, mforms::HFillFlag);
  _params.add(&_type_selector, 1, 2, 1, 2, mforms::HFillFlag | mforms::HExpandFlag);

  _os_box.add(&_params, true, true);
  _os_box.set_spacing(10);
  _os_panel.set_padding(8);

  add(&_os_panel, false, true);
}

//--------------------------------------------------------------------------------------------------

void HostAndRemoteTypePage::enter(bool advancing) {
  if (!advancing)
    return;

  // force check of admin type
  toggle_remote_admin();

  if (wizard()->is_local())
    set_title(_("Specify the installation type for your target operation system"));
  else
    set_title(_("Specify remote management type and target operation system"));

  int last_selected = _os_selector.get_selected_index();

  suspend_layout();
  _os_selector.suspend_layout();

  // Refresh platform list on each enter. This way a change on disk can be reflected without
  // restarting the wizard.
  std::string path = bec::GRTManager::get()->get_data_file_path("mysql.profiles");
  GDir *dir = g_dir_open(path.c_str(), 0, NULL);
  if (dir) {
    const gchar *file;

    _presets.clear();
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
        _presets[dict.get_string("sys.system")].push_back(std::make_pair(label, path + "/" + file));
      }
    }
    g_dir_close(dir);
  } else {
    logError("Opening profiles folder failed.");
  }

  // we need to sort the preset list
  for (std::map<std::string, std::vector<std::pair<std::string, std::string> > >::const_iterator it = _presets.begin();
       it != _presets.end(); it++) {
    std::sort(_presets[it->first].begin(), _presets[it->first].end());
  }

  std::string detected_os_type = values().get_string("detected_os_type");
  if (wizard()->is_local()) {
    _management_type_panel.show(false);
    if (detected_os_type.empty()) {
#ifdef _MSC_VER
      detected_os_type = "Windows";
#elif defined(__APPLE__)
      detected_os_type = "macOS";
#else
      detected_os_type = "Linux";
#endif
    }
  } else
    _management_type_panel.show(true);

  _os_selector.clear();
  int i = 0;
  for (std::map<std::string, std::vector<std::pair<std::string, std::string> > >::const_iterator it = _presets.begin();
       it != _presets.end(); ++it, ++i) {
    _os_selector.add_item(it->first);
    if (advancing) {
      if (detected_os_type == it->first)
        last_selected = i;
    }
  }

  if (last_selected < 0)
    last_selected = 0;
  _os_selector.set_selected(last_selected);

  _os_selector.resume_layout();
  resume_layout();

  refresh_profile_list();
}

//--------------------------------------------------------------------------------------------------

void HostAndRemoteTypePage::refresh_profile_list() {
  wizard()->clear_problem();

  std::string system = _os_selector.get_string_value();

  _type_selector.clear();
  std::list<std::string> profiles;
  for (std::vector<std::pair<std::string, std::string> >::const_iterator iter = _presets[system].begin();
       iter != _presets[system].end(); ++iter)
    profiles.push_back(iter->first);

  _type_selector.add_items(profiles);
}

//--------------------------------------------------------------------------------------------------

void HostAndRemoteTypePage::toggle_remote_admin() {
  wizard()->clear_problem();

  std::string detected_os_type = values().get_string("detected_os_type");
  bool refresh_profiles = false;

  // Hard code Windows for remote Windows management. There is also no profile to choose from
  // as we base the config file location on the managed service.
  if (_win_remote_admin.get_active() && !wizard()->is_local()) {
    detected_os_type = "Windows";
    _os_panel.show(false);
    _type_selector.set_selected(-1);
  } else {
    refresh_profiles = true;
    _os_panel.show(true);
    _os_panel.relayout();
    if (detected_os_type.empty() && wizard()->is_local()) {
#ifdef _MSC_VER
      detected_os_type = "Windows";
#elif defined(__APPLE__)
      detected_os_type = "macOS";
#else
      detected_os_type = "Linux";
#endif
    }

    int i = 0;
    for (std::map<std::string, std::vector<std::pair<std::string, std::string> > >::const_iterator
           it = _presets.begin();
         it != _presets.end(); ++it, ++i) {
      if (detected_os_type == it->first) {
        if (_os_selector.get_selected_index() != i) {
          _os_selector.set_selected(i);
          if (refresh_profiles)
            refresh_profile_list();
        }
        break;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

bool HostAndRemoteTypePage::advance() {
  std::string system = _os_selector.get_string_value();
  values().gset("os", system);

  bool need_templates = false;
  if (wizard()->is_local()) {
    values().gset("remoteAdmin", 0);
#ifdef _MSC_VER
    values().gset("windowsAdmin", 1);
#else
    need_templates = true;
    values().remove("windowsAdmin");
#endif

  } else {
    if (_ssh_remote_admin.get_active()) {
      need_templates = true;
      values().remove("windowsAdmin");
      values().gset("remoteAdmin", 1);
    } else {
      values().gset("windowsAdmin", 1);
      values().gset("remoteAdmin", 0);
    }
  }

  if (need_templates) {
    int selected_index = _type_selector.get_selected_index();
    if (selected_index == -1) {
      wizard()->set_problem(_("MySQL installation type not selected"));
      return false;
    }
    values().gset("template_path", _presets[system][selected_index].second);
    values().gset("template", _presets[system][selected_index].first);
  }

  wizard()->load_defaults();

  return true;
}

//--------------------------------------------------------------------------------------------------

bool HostAndRemoteTypePage::skip_page() {
// Skip this page if this is a local Windows installation.
#ifdef _MSC_VER
  if (wizard()->is_local()) {
    values().gset("remoteAdmin", 0);
    values().gset("windowsAdmin", 1);
    values().remove("template_path");
    values().remove("template");

    values().gset("os", "Windows");

    return true;
  }

  return false;
#else
  return false;
#endif
}

//----------------- SSHManagementPage --------------------------------------------------------------

SSHConfigurationPage::SSHConfigurationPage(WizardForm *host)
  : NewServerInstancePage(host, "ssh configuration page"), _indent(false) {
  set_short_title(_("SSH Configuration"));
  set_title(_("Set remote SSH configuration parameters"));

  set_spacing(10);
  _main_description1.set_wrap_text(true);
  _main_description1.set_text(
    _("In order to remotely configure this database instance an SSH account on this "
      "host with appropriate privileges is required. This account needs write access "
      "to the database configuration file, read access to the database logs and "
      "privileges to start/stop the database service/daemon."));
  add(&_main_description1, false, true);

  _ssh_settings_table.set_row_count(6);
  _ssh_settings_table.set_row_spacing(5);
  _ssh_settings_table.set_column_count(5);
  _ssh_settings_table.set_column_spacing(5);

  _indent.set_size(20, -1);
  _ssh_settings_table.add(&_indent, 0, 1, 0, 1, HFillFlag);

  _host_name_label.set_text(_("Host Name:"));
  _ssh_settings_table.add(&_host_name_label, 1, 2, 0, 1, HFillFlag);
  _ssh_settings_table.add(&_host_name, 2, 3, 0, 1, HFillFlag | HExpandFlag);
  _port_label.set_text(_("Port:"));
  _ssh_settings_table.add(&_port_label, 3, 4, 0, 1, HFillFlag);
  _port.set_size(50, -1);
  _port.set_value("22");
  _ssh_settings_table.add(&_port, 4, 5, 0, 1, HFillFlag);

  _username_label.set_text(_("User Name:"));
  _ssh_settings_table.add(&_username_label, 1, 2, 1, 2, HFillFlag);
  _ssh_settings_table.add(&_username, 2, 3, 1, 2, HFillFlag | HExpandFlag);

  _use_ssh_key.set_text(_("Authenticate Using SSH Key"));
  scoped_connect(_use_ssh_key.signal_clicked(), std::bind(&SSHConfigurationPage::use_ssh_key_changed, this));
  _ssh_settings_table.add(&_use_ssh_key, 1, 5, 4, 5, HFillFlag);

  _ssh_path_label.set_text(_("SSH Private Key Path:"));
  _ssh_settings_table.add(&_ssh_path_label, 1, 2, 5, 6, HFillFlag);
  _ssh_settings_table.add(&_ssh_key_path, 2, 3, 5, 6, HFillFlag | HExpandFlag);
  _ssh_settings_table.add(&_ssh_key_browse_button, 3, 4, 5, 6, 0);

  _file_selector = mforms::manage(new FsObjectSelector(&_ssh_key_browse_button, &_ssh_key_path));
  std::string homedir =
#ifdef _MSC_VER
    mforms::Utilities::get_special_folder(mforms::ApplicationData);
#else
    "~";
#endif
  _file_selector->initialize(homedir + "/.ssh/id_rsa", mforms::OpenFile, "", true,
                             std::bind(&WizardPage::validate, this));
  use_ssh_key_changed();

  add(&_ssh_settings_table, false, true);
}

//--------------------------------------------------------------------------------------------------

void SSHConfigurationPage::use_ssh_key_changed() {
  bool value = _use_ssh_key.get_active();
  _ssh_path_label.set_enabled(value);
  _ssh_key_path.set_enabled(value);
  _ssh_key_browse_button.set_enabled(value);
}

//--------------------------------------------------------------------------------------------------

void SSHConfigurationPage::enter(bool advancing) {
  if (advancing) {
    _host_name.set_value(values().get_string("host_name"));

    std::string value = values().get_string("ssh_user_name");
    if (value.empty() && g_get_user_name() != NULL)
      value = g_get_user_name();
    _username.set_value(value.empty() ? "" : value);

    value = values().get_string("ssh_port");
    if (!value.empty())
      _port.set_value(value);

    value = values().get_string("ssh_key_path");
    if (!value.empty()) {
      _use_ssh_key.set_active(true);
      use_ssh_key_changed();
      _file_selector->set_filename(value);
    }
  }
}

//--------------------------------------------------------------------------------------------------

bool SSHConfigurationPage::advance() {
  db_mgmt_ServerInstanceRef instance(wizard()->assemble_server_instance());

  // Check if we have valid SSH credentials.
  std::string value = base::trim(_host_name.get_string_value());
  if (value.empty()) {
    Utilities::show_error(_("SSH Host Needed"), _("Please specify the host name or address."), _("OK"));
    return false;
  }

  value = base::trim(_username.get_string_value());
  if (value.empty()) {
    Utilities::show_error(_("SSH User Name Needed"), _("Please specify the user name for the SSH account to be used."),
                          _("OK"));
    return false;
  }

  return true;
}

//--------------------------------------------------------------------------------------------------

void SSHConfigurationPage::leave(bool advancing) {
  if (advancing) {
    values().gset("host_name", _host_name.get_string_value());
    values().gset("ssh_port", _port.get_string_value());
    values().gset("ssh_user_name", _username.get_string_value());
    if (_use_ssh_key.get_active())
      values().gset("ssh_key_path", _ssh_key_path.get_string_value());
    else
      values().remove("ssh_key_path");
  }
}

//--------------------------------------------------------------------------------------------------

bool SSHConfigurationPage::skip_page() {
  return values().get_int("remoteAdmin", 0) != 1;
}

//----------------- WindowsManagementPage ----------------------------------------------------------

WindowsManagementPage::WindowsManagementPage(WizardForm *host, wb::WBContext *context)
  : NewServerInstancePage(host, "windows management page"), _indent(false) {
  _context = context;

  set_short_title(_("Windows Management"));

  // set_spacing(10);
  _layout_table.set_row_count(6);
  _layout_table.set_column_count(5);
  _layout_table.set_row_spacing(6);
  _layout_table.set_column_spacing(4);

  _indent.set_size(10, -1);
  _layout_table.add(&_indent, 0, 1, 1, 2, 0);

  _main_description1.set_wrap_text(true);
  _main_description1.set_text(
    _("Remote Windows management requires a user account on the remote machine "
      "which is allowed to connect remotely and also has the required privileges to query system status and "
      "to control services. For configuration file manipulation read/write access is needed to the file. "
      "Depending on your environment several ways of accessing that file are possible.\n\nExamples are mapped drives, "
      "network shares and administrative shares:"));
  _layout_table.add(&_main_description1, 0, 4, 0, 1, HFillFlag | VFillFlag);

  _main_description2.set_wrap_text(true);
  _main_description2.set_style(BoldStyle);
  _main_description2.set_text(
    _("M:\\<path to file>\\my.ini\n"
      "\\\\<server>\\<share>\\<path to file>\\my.ini\n"
      "\\\\<server>\\C$\\Program Files\\MySQL\\MySQL Server 5.1\\my.ini\n"));
  _layout_table.add(&_main_description2, 1, 4, 1, 2, HFillFlag | VFillFlag);

  _progress_label.set_text(_("Initializing WMI, please wait..."));
  _layout_table.add(&_progress_label, 0, 4, 2, 3, HFillFlag | VFillFlag);

  _service_label.set_wrap_text(true);
  _service_label.set_text(
    _("Select the service to manage from the list below. It will also help to "
      "find the configuration file."));
  _layout_table.add(&_service_label, 0, 4, 3, 4, HFillFlag | VFillFlag);

  scoped_connect(_service_selector.signal_changed(), std::bind(&WindowsManagementPage::refresh_config_path, this));
  _layout_table.add(&_service_selector, 1, 4, 4, 5, HFillFlag);

  _config_path_label.set_text(_("Path to Configuration File:"));
  _config_path_label.set_text_align(MiddleRight);
  _layout_table.add(&_config_path_label, 1, 2, 5, 6, HFillFlag);
  _layout_table.add(&_config_path, 2, 4, 5, 6, HFillFlag | HExpandFlag);
  _layout_table.add(&_browse_button, 4, 5, 5, 6, HFillFlag);

  // Setup for configuration file browsing.
  _file_selector = mforms::manage(new FsObjectSelector(&_browse_button, &_config_path));
  _file_selector->initialize("", mforms::OpenFile, "", true, std::bind(&WizardPage::validate, this));

  add(&_layout_table, false, true);
}

//--------------------------------------------------------------------------------------------------

void WindowsManagementPage::refresh_config_path() {
  if (_service_selector.get_selected_index() >= 0 && _service_selector.get_selected_index() < (int)_config_paths.size())
    _config_path.set_value(_config_paths[_service_selector.get_selected_index()]);
  else
    _config_path.set_value("");
}

//--------------------------------------------------------------------------------------------------

void WindowsManagementPage::leave(bool advancing) {
  // If we're going back, reset the progress label. We can't set it right before
  // performing the slow operation, because it blocks the UI.
  if (!advancing) {
    _progress_label.set_text(_("Initializing WMI, please wait..."));
  }
}

//--------------------------------------------------------------------------------------------------

void WindowsManagementPage::enter(bool advancing) {
  if (advancing) {
    wizard()->clear_problem();
    _config_paths.clear();
    _service_names.clear();
    _service_selector.clear();

    std::string host = values().get_string("host_name");

    std::string user;
    std::string password;

    bool local_connection = wizard()->is_local();
    if (!local_connection) {
      set_title(_("Set remote Windows configuration parameters for host " + host));

      _main_description1.set_text(
        _("Remote Windows management requires a user account on the remote machine "
          "which is allowed to connect remotely and also has the required privileges to query system status and "
          "to control services. For configuration file manipulation read/write access is needed to the file. "
          "Depending on your environment several ways of accessing that file are possible.\n\nExamples are mapped "
          "drives, "
          "network shares and administrative shares:"));
      _main_description2.show(true);

      user = values().get_string("wmi_user_name", "");

      std::string title = strfmt(_("Remote Windows Login (%s)"), host.c_str()) + "|";
      title += _("Please enter a Windows user login and password for the remote server with rights to WMI");

      bool result = false;
      try {
        result = Utilities::credentials_for_service(title, "wmi@" + host, user, false, password);
      } catch (std::exception &exc) {
        logWarning("Exception caught when clearning the password: %s\n", exc.what());
        mforms::Utilities::show_error("Clear Password", base::strfmt("Could not clear password: %s", exc.what()), "OK");
      }

      if (!result) {
        _progress_label.set_text(_("Need valid user credentials to connect to server."));
        wizard()->set_problem(_("Need valid user credentials to connect to server."));
        return;
      }

      values().gset("wmi_user_name", user);
    } else {
      host = ""; // Empty host name for local machine.

      _main_description1.set_text(
        _("Windows management requires a user account on this machine "
          "which has the required privileges to query system status and to control services. "
          "For configuration file manipulation read/write access to the file is needed. "));
      _main_description2.show(false);

      set_title(_("Set Windows configuration parameters for this machine"));
    }

    grt::Module *module = grt::GRT::get()->get_module("Workbench");

    try {
      grt::ValueRef wmi_session;

      {
        grt::StringListRef arguments(grt::Initialized);
        arguments.ginsert(grt::StringRef(host));
        arguments.ginsert(grt::StringRef(user));
        arguments.ginsert(grt::StringRef(password));

        // This can take a few seconds.
        wmi_session = module->call_function("wmiOpenSession", arguments);
      }

      grt::ValueRef wmi_result;
      {
        grt::BaseListRef arguments(true);
        arguments.ginsert(wmi_session);
        arguments.ginsert(
          grt::StringRef("select * from Win32_Service where (Name like \"%mysql%\" or DisplayName like \"%mysql%\")"));

        wmi_result = module->call_function("wmiQuery", arguments);

        module->call_function("wmiCloseSession", arguments); // Only the first parameter of arguments is used here.
      }
      grt::DictListRef entries = grt::DictListRef::cast_from(wmi_result);

      size_t count = entries->count();
      if (count > 0) {
        for (size_t i = 0; i < count; i++) {
          grt::DictRef entry(entries[i]);

          std::string service_name = entry.get_string("Name", "invalid");
          std::string service_display_name = entry.get_string("DisplayName", "invalid");
          std::string path = entry.get_string("PathName", "invalid");
          std::string state = entry.get_string("State", "unknown");
          std::string start_mode = entry.get_string("StartMode", "unknown");

          std::string config_file = base::extract_option_from_command_line("--defaults-file", path);
          if (!local_connection) {
            if (config_file.empty())
              config_file = "C$\\";
            config_file = "\\\\" + host + "\\" + config_file;
            base::replaceStringInplace(config_file, ":", "$");
          }
          _config_paths.push_back(config_file);
          _service_names.push_back(service_name);
          std::string selector_text = service_display_name + " (" + state + ", Start mode: " + start_mode + ")";
          _service_selector.add_item(selector_text);
        }
        _progress_label.set_text("");
      } else {
        _progress_label.set_text("No MySQL service found.");
        wizard()->set_problem(
          _("In order to manage a MySQL server it must be installed as a service. "
            "The wizard could not find any MySQL service on the target machine, hence the server instance "
            "cannot be created."));
      }

      refresh_config_path();
    } catch (std::runtime_error &e) {
      _progress_label.set_text(base::strfmt(_("Could not set up connection: %s"), e.what()));
      wizard()->set_problem(e.what());

      // In case this error was caused by wrong credentials (we cannot be sure from the error message)
      // we remove the already stored password to make it possible to (re) enter the current one.
      try {
        Utilities::forget_password("wmi@" + host, user);
      } catch (std::exception &exc) {
        logWarning("Exception caught when clearning the password: %s\n", exc.what());
        mforms::Utilities::show_error("Clear Password", base::strfmt("Could not clear password: %s", exc.what()), "OK");
      }

      values().gset("wmi_user_name", "");
    }
  }
}

//--------------------------------------------------------------------------------------------------

bool WindowsManagementPage::advance() {
  if (_service_names.size() == 0 || _service_selector.get_selected_index() < 0)
    return false;

  values().gset("ini_path", _config_path.get_string_value());
  values().gset("ini_section", "mysqld");
  values().gset("service_name", _service_names[_service_selector.get_selected_index()]);

  return true;
}

//--------------------------------------------------------------------------------------------------

bool WindowsManagementPage::skip_page() {
  // Provide native Windows (WMI) management for local and remote Windows boxes.
  // Remote Windows boxes which are managed via SSH use the SSH config page instead, though.

  bool local_wmi;
#ifdef _MSC_VER
  local_wmi = true;
#else
  local_wmi = false;
#endif
  bool remote_wmi = values().get_int("windowsAdmin", 0) != 0;

  if (dynamic_cast<NewServerInstanceWizard *>(_form)->is_local())
    return !local_wmi;
  else
    return !remote_wmi;
}

//----------------- TestHostMachineSettingsPage ----------------------------------------------------

TestHostMachineSettingsPage::TestHostMachineSettingsPage(WizardForm *host)
  : WizardProgressPage(host, "test host machine settings page", true) {
  set_short_title(_("Test Settings"));
  set_title(_("Testing Host Machine Settings"));

  set_heading(
    _("The connection to the host machine is being tested. This might take a few "
      "moments depending on your network connection."));
  _connect_task = add_task(_("Connect to host machine"), std::bind(&TestHostMachineSettingsPage::connect_to_host, this),
                           _("Trying to find host machine and connecting to it..."));

  _commands_task = add_async_task(_("Check location of start/stop commands"),
                                  std::bind(&TestHostMachineSettingsPage::check_admin_commands, this),
                                  _("Checking if commands to start and stop server are in the expected location..."));

  add_async_task(_("Check MySQL configuration file"), std::bind(&TestHostMachineSettingsPage::find_config_file, this),
                 _("Looking for the configuration file of the database server..."));

  end_adding_tasks(_("Testing host machine settings is done."));

  set_status_text("");
}

//--------------------------------------------------------------------------------------------------

void TestHostMachineSettingsPage::enter(bool advance) {
  reset_tasks();

  db_mgmt_ServerInstanceRef instance(wizard()->assemble_server_instance());
  _connect_task->set_enabled(values().get_int("remoteAdmin", 0) == 1);
  _commands_task->set_enabled(values().get_int("windowsAdmin", 0) == 0);

  WizardProgressPage::enter(advance);
}

//--------------------------------------------------------------------------------------------------

bool TestHostMachineSettingsPage::connect_to_host() {
  // This will require the ssh or SSH key password, so it needs to be called from main thread.
  wizard()->test_setting_grt("connect_to_host");

  return true;
}

//--------------------------------------------------------------------------------------------------

bool TestHostMachineSettingsPage::find_config_file() {
  // Native remote Windows management uses a direct URI for the files.
  bool use_local = wizard()->is_local() || values().get_int("windowsAdmin", 0) == 1;
  execute_grt_task(std::bind(&NewServerInstanceWizard::test_setting_grt, wizard(),
                             use_local ? "find_config_file/local" : "find_config_file"),
                   false);
  return true;
}

//--------------------------------------------------------------------------------------------------

bool TestHostMachineSettingsPage::find_error_files() {
  bool use_local = wizard()->is_local() || values().get_int("windowsAdmin", 0) == 1;
  execute_grt_task(std::bind(&NewServerInstanceWizard::test_setting_grt, wizard(),
                             use_local ? "find_error_files/local" : "find_error_files"),
                   false);
  return true;
}

//--------------------------------------------------------------------------------------------------

bool TestHostMachineSettingsPage::check_admin_commands() {
  execute_grt_task(std::bind(&NewServerInstanceWizard::test_setting_grt, wizard(),
                             wizard()->is_local() ? "check_admin_commands/local" : "check_admin_commands"),
                   false);
  return true;
}

//--------------------------------------------------------------------------------------------------

bool TestHostMachineSettingsPage::skip_page() {
  return !(wizard()->is_admin_enabled());
}

//--------------------------------------------------------------------------------------------------

void TestHostMachineSettingsPage::tasks_finished(bool success) {
  values().gset("host_tests_succeeded", success);
}

//--------------------------------------------------------------------------------------------------

void TestHostMachineSettingsPage::leave(bool advancing) {
  if (advancing) {
    bool require_review = false;
    if (values().get_int("host_tests_succeeded") == 1) {
      require_review = Utilities::show_message(
                         _("Review settings"),
                         _("Checks succeeded for Connection and Configuration Settings for this new Server Instance."),
                         _("Continue"), "", _("I'd like to review the settings again")) == mforms::ResultOther;
    } else
      require_review = true;
    values().gset("review_required", require_review);

    if (!require_review)
      wizard()->create_instance();
  }
}

//--------------------------------------------------------------------------------------------------

NewServerInstanceWizard *TestHostMachineSettingsPage::wizard() {
  return dynamic_cast<NewServerInstanceWizard *>(_form);
}

//----------------- ReviewPage ----------------------------------------------------------------------

ReviewPage::ReviewPage(WizardForm *host) : NewServerInstancePage(host, "review"), _text(VerticalScrollBar) {
  set_short_title(_("Review Settings"));
  set_title(_("Review Remote Management Settings"));

  _label.set_text(
    _("Below is a list of all settings collected so far. This includes also values taken "
      "from templates or default values. Check if they match your actual settings and toggle 'Change Parameters' "
      "if you need to make any changes to default values. For any other change go back to the appropriate wizard "
      "page.\n\n"
      "Pay special attention if you run more than one instance of MySQL on the same machine."));
  _label.set_wrap_text(true);

  _text.set_read_only(true);

  add(&_label, false, true);
  add(&_text, true, true);
  _customize_check.set_text("Change Parameters");
  scoped_connect(_customize_check.signal_clicked(), std::bind(&ReviewPage::customize_changed, this));
  add(&_customize_check, false, true);
}

//--------------------------------------------------------------------------------------------------

void ReviewPage::customize_changed() {
  values().gset("customize", _customize_check.get_active());
  wizard()->update_buttons();
}

//--------------------------------------------------------------------------------------------------

void ReviewPage::enter(bool advancing) {
  if (advancing) {
    std::string summary;

    grt::DictRef serverInfo(wizard()->assemble_server_instance()->serverInfo());

    bool ssh_management = values().get_int("remoteAdmin") != 0;
    bool wmi_management = values().get_int("windowsAdmin") != 0;

    std::string host_name = values().get_string("host_name", "localhost");
    if (ssh_management) {
      std::string ssh_port = values().get_string("ssh_port", "22");
      std::string ssh_user_name = values().get_string("ssh_user_name");
      std::string ssh_key_path = values().get_string("ssh_key_path");

      summary.append(_("SSH Based Adminstration enabled\n"));
      summary.append(strfmt(_("    SSH host:  %s:%s\n"), host_name.c_str(), ssh_port.c_str()));
      summary.append(strfmt(_("    SSH user:  %s\n"), ssh_user_name.c_str()));
      summary.append(strfmt(_("    SSH key file:  %s\n"), ssh_key_path.empty() ? "not set" : ssh_key_path.c_str()));
    } else if (wmi_management) {
      std::string user_name = values().get_string("wmi_user_name");
      std::string service_name = values().get_string("service_name");

      summary.append(_("Native Windows Adminstration enabled\n"));
      summary.append(strfmt(_("    Windows host:  %s\n"), host_name.c_str()));
      if (!wizard()->is_local())
        summary.append(strfmt(_("    Windows user name:  %s\n"), user_name.c_str()));
      summary.append(strfmt(_("    MySQL service name:  %s\n"), service_name.c_str()));
    }

    summary.append("\n");
    std::string os_title = serverInfo.get_string("sys.system", "Unknown");
    std::string ini_path = serverInfo.get_string("sys.config.path");
    std::string ini_section = serverInfo.get_string("sys.config.section");
    std::string mysql_version = serverInfo.get_string("serverVersion");
    summary.append(_("MySQL Configuration\n"));
    summary.append(strfmt(_("    MySQL Version:  %s\n"), mysql_version.empty() ? "Unknown" : mysql_version.c_str()));
    summary.append(strfmt(_("    Settings Template:  %s\n"), serverInfo.get_string("sys.preset").c_str()));
    summary.append(strfmt(_("    Path to Configuration File:  %s\n"), ini_path.c_str()));
    summary.append(strfmt(_("    Instance Name in Configuration File:  %s\n"), ini_section.c_str()));
    summary.append("\n");

    if (!wmi_management) {
      std::string start = serverInfo.get_string("sys.mysqld.start");
      std::string stop = serverInfo.get_string("sys.mysqld.stop");
      bool use_sudo = serverInfo.get_int("sys.usesudo") != 0;
      summary.append(_("Commands for MySQL Management\n"));
      summary.append(strfmt(_("    Start MySQL:  %s\n"), start.c_str()));
      summary.append(strfmt(_("    Stop MySQL:  %s\n"), stop.c_str()));
      if (os_title != "Windows")
        summary.append(strfmt(_("    Use sudo:  %s\n"), use_sudo ? _("Yes") : _("No")));
    }

    _text.set_value(summary);
  }
}

//--------------------------------------------------------------------------------------------------

bool ReviewPage::skip_page() {
  return values().get_int("review_required", 0) == 0;
}

//--------------------------------------------------------------------------------------------------

bool ReviewPage::next_closes_wizard() {
  return !_customize_check.get_active();
}

//--------------------------------------------------------------------------------------------------

void ReviewPage::leave(bool advancing) {
  if (advancing && !_customize_check.get_active())
    wizard()->create_instance();
}

//----------------- PathsPage ----------------------------------------------------------------------

PathsPage::PathsPage(WizardForm *host, wb::WBContext *context) : NewServerInstancePage(host, "paths page") {
  _context = context;

  set_short_title(_("MySQL Config File"));
  set_title(_("Information about MySQL configuration"));

  set_padding(10);
  set_spacing(20);
  _description.set_text(
    _("In order to manage the settings of the MySQL Server it is necessary to "
      "know where its configuration file resides.\n\n"
      "The configuration file may consist of several sections, each of them "
      "belonging to a different tool or server instance. Hence it is also "
      "necessary to know which section belongs to the server we are managing.\n\n"
      "Please specify this information below."));
  _description.set_wrap_text(true);
  add(&_description, false, true);

  _content.set_column_count(4);
  _content.set_column_spacing(8);
  _content.set_row_count(5);
  _content.set_row_spacing(8);

  _version_label.set_text(_("MySQL Server Version:"));
  _version_label.set_text_align(MiddleRight);
  _content.add(&_version_label, 0, 1, 0, 1, HFillFlag);
  _content.add(&_version, 1, 2, 0, 1, HFillFlag);

  _config_path_label.set_text(_("Path to Configuration File:"));
  _config_path_label.set_text_align(MiddleRight);
  _content.add(&_config_path_label, 0, 1, 1, 2, HFillFlag);
  _content.add(&_config_path, 1, 3, 1, 2, HFillFlag);
  _content.add(&_browse_button, 3, 4, 1, 2, HFillFlag);

  // Setup for local config file browsing. This will be adjusted if we are at a remote location.
  _file_selector = mforms::manage(new FsObjectSelector(&_browse_button, &_config_path));
  _file_selector->initialize("", mforms::OpenFile, "", true, std::bind(&WizardPage::validate, this));

  _test_config_path_button.set_text(_("Check Path"));
  scoped_connect(_test_config_path_button.signal_clicked(), std::bind(&PathsPage::test_path, this));
  _content.add(&_test_config_path_button, 1, 2, 2, 3, HFillFlag);
  _test_config_path_description.set_text(_("Click to test if your path is correct."));
  _content.add(&_test_config_path_description, 2, 3, 2, 3, HFillFlag | HExpandFlag);

  _section_name_label.set_text(_("Section of the Server Instance:"));
  _section_name_label.set_text_align(MiddleRight);
  _content.add(&_section_name_label, 0, 1, 3, 4, HFillFlag);
  _content.add(&_section_name, 1, 3, 3, 4, HFillFlag);

  _test_section_button.set_text(_("Check Name"));
  scoped_connect(_test_section_button.signal_clicked(), std::bind(&PathsPage::test_section, this));
  _content.add(&_test_section_button, 1, 2, 4, 5, HFillFlag);
  _test_section_description.set_text(_("Click to test if your instance name is correct."));
  _content.add(&_test_section_description, 2, 3, 4, 5, HFillFlag | HExpandFlag);

  add(&_content, true, true);
}

//--------------------------------------------------------------------------------------------------

bool PathsPage::skip_page() {
  return !(wizard()->is_admin_enabled()) || !values().get_int("customize");
}

//--------------------------------------------------------------------------------------------------

void PathsPage::enter(bool advancing) {
  _test_config_path_description.set_color(base::Color::getSystemColor(base::TextColor).to_html());
  _test_config_path_description.set_text(_("Click to test if your path is correct."));
  _test_section_description.set_color(base::Color::getSystemColor(base::TextColor).to_html());
  _test_section_description.set_text(_("Click to test if your section is correct."));

  if (advancing) {
    // Prefill values from defaults.
    _version.set_value(wizard()->get_server_info("serverVersion"));
    _config_path.set_value(wizard()->get_server_info("sys.config.path"));
    _section_name.set_value(wizard()->get_server_info("sys.config.section"));
  }

  bool ssh_management = values().get_int("remoteAdmin", 0) != 0;
  if (ssh_management) {
    // Setup for remote browsing.
    _file_selector->set_browse_callback(std::bind(&PathsPage::browse_remote_config_file, this));
  }
}

//--------------------------------------------------------------------------------------------------

bool PathsPage::advance() {
  std::string version = base::trim(_version.get_string_value());
  int a, b, c;
  if (version.empty() || sscanf(version.c_str(), "%i.%i.%i", &a, &b, &c) < 2 || a < 4) {
    Utilities::show_error(_("Invalid version"), _("The MySQL server version number provided appears to be invalid."),
                          _("OK"));
    return false;
  }

  std::string path = base::trim(_config_path.get_string_value());
  if (path.empty()) {
    Utilities::show_error(_("Empty path"), _("The path to the configuration must not be empty."), _("OK"));
    return false;
  }
  std::string section = base::trim(_section_name.get_string_value());
  if (section.empty()) {
    Utilities::show_error(_("Empty section"), _("A section must be given which belongs to the given server."), _("OK"));
    return false;
  }
  values().gset("server_version", version);
  values().gset("ini_path", path);
  values().gset("ini_section", section);
  return true;
}

//--------------------------------------------------------------------------------------------------

/**
 * Triggers the remote file open dialog.
 */
void PathsPage::browse_remote_config_file() {
  db_mgmt_ServerInstanceRef instance(wizard()->assemble_server_instance());

  grt::BaseListRef args(true);
  args.ginsert(values().get("connection"));
  args.ginsert(instance);

  try {
    grt::StringRef selection =
      grt::StringRef::cast_from(grt::GRT::get()->call_module_function("WbAdmin", "openRemoteFileSelector", args));
    if (selection.is_valid() && !selection.empty())
      _config_path.set_value(selection);
  } catch (const std::exception &exc) {
    grt::GRT::get()->send_error("Error in remote file browser", exc.what());
  }
}

//--------------------------------------------------------------------------------------------------

void PathsPage::test_path() {
  std::string detail;

  values().gset("ini_path", _config_path.get_string_value());
  bool success;
  try {
    if (values().get_int("windowsAdmin", 0) != 0 || wizard()->is_local())
      success = wizard()->test_setting("check_config_path/local", detail);
    else
      success = wizard()->test_setting("check_config_path", detail);
  } catch (std::exception &) {
    success = false;
  }
  if (success) {
    _test_config_path_description.set_color("#00A000");
    _test_config_path_description.set_text(_("The config file path is valid."));
  } else {
    _test_config_path_description.set_color("#A00000");
    _test_config_path_description.set_text(_("The config file could not be found."));
  }
}

//--------------------------------------------------------------------------------------------------

void PathsPage::test_section() {
  std::string detail;

  values().gset("ini_path", _config_path.get_string_value());
  values().gset("ini_section", _section_name.get_string_value());
  bool success;
  try {
    if (values().get_int("windowsAdmin", 0) != 0 || wizard()->is_local())
      success = wizard()->test_setting("check_config_section/local", detail);
    else
      success = wizard()->test_setting("check_config_section", detail);
  } catch (std::exception &) {
    success = false;
  }

  if (success) {
    _test_section_description.set_color("#00A000");
    _test_section_description.set_text(_("The config file section is valid."));
  } else {
    _test_section_description.set_color("#A00000");
    _test_section_description.set_text(_("The config file section is invalid."));
  }
}

//----------------- CommandsPage ----------------------------------------------------------------------

CommandsPage::CommandsPage(WizardForm *host) : NewServerInstancePage(host, "commands page") {
  set_short_title(_("Specify Commands"));
  set_title(_("Specify commands to be used to manage the MySQL server."));

  set_spacing(20);
  set_padding(8);
  _description.set_text(
    _("The values on this page comprise rather low level commands, which are used "
      "to control the MySQL server instance (start or stop it) and others.\n\n"
      "If you are unsure what these values mean leave them untouched. The defaults "
      "are usually a good choice already (for single server machines)."));
  _description.set_wrap_text(true);
  add(&_description, false, true);

  _content.set_column_count(2);
  _content.set_column_spacing(8);
  _content.set_row_count(4);
  _content.set_row_spacing(5);

  _start_label.set_text(_("Command to start the MySQL server:"));
  _start_label.set_text_align(MiddleRight);
  _content.add(&_start_label, 0, 1, 0, 1, HFillFlag);
  _content.add(&_start_command, 1, 2, 0, 1, HFillFlag | HExpandFlag);

  _stop_label.set_text(_("Command to stop the MySQL server:"));
  _stop_label.set_text_align(MiddleRight);
  _content.add(&_stop_label, 0, 1, 1, 2, HFillFlag);
  _content.add(&_stop_command, 1, 2, 1, 2, HFillFlag | HExpandFlag);

  _use_sudo.set_text(
    _("Check this box if you want or need the above commands \n"
      "to be executed with elevated Operating System Privileges."));
  _content.add(&_use_sudo, 1, 2, 3, 4, HFillFlag);

  add(&_content, false, true);
}

//--------------------------------------------------------------------------------------------------

bool CommandsPage::skip_page() {
  return !(wizard()->is_admin_enabled()) || !values().get_int("customize");
}

//--------------------------------------------------------------------------------------------------

void CommandsPage::enter(bool advancing) {
  if (advancing) {
    // Prefill values from defaults.
    _start_command.set_value(wizard()->get_server_info("sys.mysqld.start"));
    _stop_command.set_value(wizard()->get_server_info("sys.mysqld.stop"));
    _use_sudo.set_active(wizard()->get_server_info("sys.usesudo") != "0");
  }
}

//--------------------------------------------------------------------------------------------------

bool CommandsPage::advance() {
  values().gset("command_start", base::trim(_start_command.get_string_value()));
  values().gset("command_stop", base::trim(_stop_command.get_string_value()));
  values().gset("use_sudo", _use_sudo.get_active());

  return true;
}

//--------------------------------------------------------------------------------------------------

void CommandsPage::leave(bool advancing) {
  if (advancing)
    wizard()->create_instance();
}

//----------------- NewServerInstanceWizard ---------------------------------------------------------

NewServerInstanceWizard::NewServerInstanceWizard(wb::WBContext *context, db_mgmt_ConnectionRef connection)
  : WizardForm(), _instance(grt::Initialized) {
  set_name("New Instance Wizard");
  setInternalName("new_instance_wizard");
  _context = context;
  _connection = connection;
  values().set("connection", connection);

  if (is_local())
    set_title(_("Configure Local Management"));
  else
    set_title(_("Configure Remote Management"));

  // Fill in some values from the connection that are used by the wizard pages.
  grt::DictRef parameter_values = _connection->parameterValues();
  std::string host = parameter_values.get_string("sshHost"); // SSH tunnel host name.
  if (host.empty())
    host = parameter_values.get_string("hostName");             // MySQL server host name.
  std::vector<std::string> host_parts = base::split(host, ":"); // Separate host name and port.

  if (host_parts.size() > 1) {
    // We come here usually only for SSH connection.
    values().gset("host_name", host_parts[0]);
    values().gset("ssh_port", host_parts[1]);
    values().gset("ssh_user_name", parameter_values.get_string("sshUserName"));
    std::string key_path = parameter_values.get_string("sshKeyFile");
    if (!key_path.empty())
      values().gset("ssh_key_path", key_path);
  } else
    values().gset("host_name", host);

  // Set up page structure of the wizard.
  _introduction_page = new IntroductionPage(this);
  add_page(manage(_introduction_page));

  _test_database_settings_page = new TestDatabaseSettingsPage(this);
  add_page(manage(_test_database_settings_page));

  _os_page = new HostAndRemoteTypePage(this);
  add_page(manage(_os_page));

  _ssh_configuration_page = new SSHConfigurationPage(this);
  add_page(manage(_ssh_configuration_page));

  _windows_connection_page = new WindowsManagementPage(this, _context);
  add_page(manage(_windows_connection_page));

  _test_host_machine_settings_page = new TestHostMachineSettingsPage(this);
  add_page(manage(_test_host_machine_settings_page));

  _review_page = new ReviewPage(this);
  add_page(manage(_review_page));

  _paths_page = new PathsPage(this, _context);
  add_page(manage(_paths_page));

  _commands_page = new CommandsPage(this);
  add_page(manage(_commands_page));
}

//--------------------------------------------------------------------------------------------------

NewServerInstanceWizard::~NewServerInstanceWizard() {
  // Pages are freed by the WizardForm ancestor.
  // disconnect the test SSH session.
  std::string s;
  test_setting("disconnect", s);
}

//--------------------------------------------------------------------------------------------------

/**
 * Creates a server instance object from the current values.
 */
db_mgmt_ServerInstanceRef NewServerInstanceWizard::assemble_server_instance() {
  db_mgmt_ConnectionRef conn(db_mgmt_ConnectionRef::cast_from(values().get("connection")));

  _instance->owner(_context->get_root()->rdbmsMgmt());

  std::string os = values().get_string("os");
  _instance->serverInfo().gset("sys.system", os);

  bool ssh_management = values().get_int("remoteAdmin", 0) != 0;
  _instance->serverInfo().gset("remoteAdmin", ssh_management);
  if (ssh_management) {
    _instance->loginInfo().gset("ssh.userName", values().get_string("ssh_user_name", ""));
    _instance->loginInfo().gset("ssh.hostName", values().get_string("host_name", "localhost"));
  }

  bool win_management = values().get_int("windowsAdmin", 0) != 0;
  _instance->serverInfo().gset("windowsAdmin", win_management);
  if (win_management) {
    _instance->loginInfo().gset("wmi.userName", values().get_string("wmi_user_name", ""));
    _instance->loginInfo().gset("wmi.hostName", values().get_string("host_name", "localhost"));
  }

  std::string instance_name = *conn->name() + " instance";
  std::string ssh_port = values().get_string("ssh_port", "22");
  std::string ssh_key_path = values().get_string("ssh_key_path");

  _instance->name(instance_name);
  _instance->loginInfo().gset("ssh.port", ssh_port);
  _instance->loginInfo().gset("ssh.useKey", !ssh_key_path.empty());
  if (!ssh_key_path.empty())
    _instance->loginInfo().gset("ssh.key", ssh_key_path);

  std::string version = values().get_string("server_version", "");
  if (!version.empty())
    _instance->serverInfo().gset("serverVersion", version);

  if (values().get_int("customize", 0))
    _instance->serverInfo().gset("sys.preset", "Custom");
  else
    _instance->serverInfo().gset("sys.preset", values().get_string("template"));

  if (win_management ||
      (get_active_page_number() >= 7 &&
       values().get_int("customize"))) // don't overwrite defaults unless user has had time to enter them
  {
    std::string ini_path = values().get_string("ini_path");
    std::string ini_section = values().get_string("ini_section");
    std::string service_name = values().get_string("service_name");

    _instance->serverInfo().gset("sys.config.path", ini_path);
    _instance->serverInfo().gset("sys.config.section", ini_section);
    _instance->serverInfo().gset("sys.mysqld.service_name", service_name);
  }
  if (get_active_page_number() >= 8 &&
      values().get_int("customize")) // don't overwrite defaults unless user has had time to enter them
  {
    std::string start = values().get_string("command_start");
    std::string stop = values().get_string("command_stop");
    bool use_sudo = values().get_int("use_sudo") != 0;

    _instance->serverInfo().gset("sys.mysqld.start", start);
    _instance->serverInfo().gset("sys.mysqld.stop", stop);
    _instance->serverInfo().gset("sys.usesudo", use_sudo);
  }

  _instance->connection(db_mgmt_ConnectionRef::cast_from(values().get("connection")));

  return _instance;
}

//--------------------------------------------------------------------------------------------------

grt::ValueRef NewServerInstanceWizard::test_setting_grt(const std::string &name) {
  std::string detail;
  if (!test_setting(name, detail))
    throw std::runtime_error(detail);
  return grt::ValueRef();
}

//--------------------------------------------------------------------------------------------------

bool NewServerInstanceWizard::test_setting(const std::string &name, std::string &detail) {
  grt::Module *module = grt::GRT::get()->get_module("WbAdmin");
  if (module) {
    grt::BaseListRef args(true);
    grt::ValueRef ret;
    args.ginsert(grt::StringRef(name));
    args.ginsert(values().get("connection"));
    args.ginsert(assemble_server_instance());

    try {
      ret = module->call_function("testInstanceSettingByName", args);
      if (ret.is_valid() && grt::StringRef::can_wrap(ret)) {
        std::string s = *grt::StringRef::cast_from(ret);

        if (g_str_has_prefix(s.c_str(), "OK")) {
          if (s.size() > 3 && s[2] == ' ')
            detail = s.substr(3);
          return true;
        }

        // ERROR
        if (s.size() > 6 && s[5] == ' ')
          detail = s.substr(6);

        return false;
      }
    } catch (std::exception &exc) {
      detail = exc.what();
      return false;
    }
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

/**
 * Loads all default values for a given instance.
 */
void NewServerInstanceWizard::load_defaults() {
  std::string template_file = values().get_string("template_path");
  if (!template_file.empty()) {
    grt::DictRef dict;
    try {
      dict = grt::DictRef::cast_from(grt::GRT::get()->unserialize(template_file));
    } catch (std::exception &exc) {
      logWarning("Instance %s contains invalid data: %s\n", template_file.c_str(), exc.what());
      return;
    }
    grt::merge_contents(_instance->serverInfo(), dict, true);
    _instance->serverInfo().gset("sys.preset", values().get_string("template"));
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the current value for the server info at the specified key. Might be the default
 * value or one set by assemble_server_instance().
 */
std::string NewServerInstanceWizard::get_server_info(const std::string &key) {
  grt::ValueRef value = _instance->serverInfo().get(key);

  if (!value.is_valid())
    return "";
  if (grt::StringRef::can_wrap(value))
    return grt::StringRef::cast_from(value);
  return value.debugDescription();
}

//--------------------------------------------------------------------------------------------------

bool NewServerInstanceWizard::is_admin_enabled() {
  return (values().get_int("remoteAdmin", 0) == 1) || (values().get_int("windowsAdmin", 0) == 1) || is_local();
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if the currently selected host is the local machine.
 */
bool NewServerInstanceWizard::is_local() {
  std::string driver = _connection->driver().is_valid() ? _connection->driver()->name() : "";
  if (driver != "MysqlNativeSSH") {
    std::string hostname = _connection->parameterValues().get_string("hostName");
    if (hostname == "localhost" || hostname.empty() || hostname == "127.0.0.1")
      return true;
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

void NewServerInstanceWizard::create_instance() {
  db_mgmt_ManagementRef rdbms(_context->get_root()->rdbmsMgmt());
  grt::ListRef<db_mgmt_ServerInstance> instances = rdbms->storedInstances();

  // Remove any previously defined instance for the given connection and set the new one.
  db_mgmt_ServerInstanceRef instance = assemble_server_instance();
  for (grt::ListRef<db_mgmt_ServerInstance>::const_iterator iterator = instances.begin(); iterator != instances.end();
       iterator++) {
    if ((*iterator)->connection() == _connection) {
      instances->remove(*iterator);
      break;
    }
  }

  instances.insert(instance);
}

//--------------------------------------------------------------------------------------------------
