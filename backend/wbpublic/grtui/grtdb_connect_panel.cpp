/*
 * Copyright (c) 2007, 2022, Oracle and/or its affiliates. All rights reserved.
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

#include "grtdb_connect_panel.h"
#include "grtdb_connection_editor.h"
#include "mforms/fs_object_selector.h"
#include "grtdb/db_helpers.h"

#include "base/string_utilities.h"
#include "base/log.h"
#include "base/file_utilities.h"

#include "mforms/uistyle.h"
#include "mforms/utilities.h"
#include "mforms/checkbox.h"
#include "mforms/textbox.h"
#include "mforms/app.h"
#include <mforms/radiobutton.h>

#include "objimpl/wrapper/mforms_ObjectReference_impl.h"

#define MYSQL_RDBMS_ID "com.mysql.rdbms.mysql"

DEFAULT_LOG_DOMAIN("DbConnectPanel");

using namespace base;
using namespace grtui;
using namespace mforms;

DbConnectPanel::DbConnectPanel(DbConnectPanelFlags flags)
  : Box(false),
    _connection(nullptr),
    _tab(mforms::TabViewSystemStandard),
    _content(false),
    _params_panel(mforms::TransparentPanel),
    _params_table(nullptr),
    _ssl_panel(mforms::TransparentPanel),
    _ssl_table(nullptr),
    _advanced_panel(mforms::TransparentPanel),
    _advanced_table(nullptr),
    _options_panel(mforms::TransparentPanel),
    _options_table(nullptr),
    _create_group(false),
    _show_connection_combo((flags & DbConnectPanelShowConnectionCombo) != 0),
    _show_manage_connections((flags & DbConnectPanelShowManageConnections) != 0),
    _dont_set_default_connection((flags & DbConnectPanelDontSetDefaultConnection) != 0),
    _last_active_tab(-1) {
  _allow_edit_connections = false;
  _initialized = false;
  _updating = false;

  _skip_schema_name = false;
  _delete_connection_be = false;

  set_spacing(4);

  if (_show_connection_combo) {
    _allow_edit_connections = false;
    _label1.set_text(_("Stored Connection:"));
  } else {
    _allow_edit_connections = true;
    _label1.set_text(_("Connection Name:"));
  }
  _label2.set_text(_("Database System:"));
  _label3.set_text(_("Connection Method:"));

  _label1.set_text_align(MiddleRight);
  _label2.set_text_align(MiddleRight);
  _label3.set_text_align(MiddleRight);

  if (_show_connection_combo)
    _desc1.set_text(_("Select from saved connection settings"));
  else
    _desc1.set_text(_("Type a name for the connection"));
  _desc1.set_style(mforms::SmallHelpTextStyle);
  _desc2.set_text(_("Select a RDBMS from the list of supported systems"));
  _desc2.set_style(mforms::SmallHelpTextStyle);
  _desc3.set_text(_("Method to use to connect to the RDBMS"));
  _desc3.set_style(mforms::SmallHelpTextStyle);

  _rdbms_sel.set_name("Database System");
  _stored_connection_sel.set_name("Connection List");
  _driver_sel.set_name("Connection Method");
  if (_show_connection_combo)
    scoped_connect(_stored_connection_sel.signal_changed(),
                   std::bind(&DbConnectPanel::change_active_stored_conn, this));
  scoped_connect(_rdbms_sel.signal_changed(), std::bind(&DbConnectPanel::change_active_rdbms, this));
  scoped_connect(_driver_sel.signal_changed(), std::bind(&DbConnectPanel::change_active_driver, this));
  scoped_connect(_name_entry.signal_changed(), std::bind(&DbConnectPanel::change_connection_name, this));

  _name_entry.set_name("Connection Name");
  _name_entry.setInternalName("connect_panel:table");
  _table.set_name("Connection information");
  _table.set_row_count(flags & DbConnectPanelShowRDBMSCombo ? 4 : 2);
  _table.set_column_count(3);

  _table.set_column_spacing(4);
  _table.set_row_spacing(4);

  int row = 0;
  if (flags & DbConnectPanelShowRDBMSCombo) {
    _table.add(&_label2, 0, 1, row, row + 1, mforms::HFillFlag);
    _table.add(&_rdbms_sel, 1, 2, row, row + 1, mforms::HExpandFlag | mforms::HFillFlag | mforms::VFillFlag);
    _table.add(&_desc2, 2, 3, row, row + 1, mforms::HFillFlag);
    row++;
    _table.add(mforms::manage(new mforms::Label()), 0, 1, row, row + 1, mforms::HFillFlag);
    row++;
  }

  if (!(flags & DbConnectPanelHideConnectionName)) {
    if (_show_connection_combo) {
      _table.add(&_label1, 0, 1, row, row + 1, mforms::HFillFlag);
      _table.add(&_stored_connection_sel, 1, 2, row, row + 1,
                 mforms::HExpandFlag | mforms::HFillFlag | mforms::VFillFlag);
      _table.add(&_desc1, 2, 3, row, row + 1, mforms::HFillFlag);
    } else {
      _table.add(&_label1, 0, 1, row, row + 1, mforms::HFillFlag);
      _table.add(&_name_entry, 1, 2, row, row + 1, mforms::HExpandFlag | mforms::HFillFlag | mforms::VFillFlag);
      _table.add(&_desc1, 2, 3, row, row + 1, mforms::HFillFlag);
    }
    row++;
  }

  _label3.set_size(-1, 30);
  _table.add(&_label3, 0, 1, row, row + 1, mforms::HFillFlag);
  _table.add(&_driver_sel, 1, 2, row, row + 1, mforms::HExpandFlag | mforms::HFillFlag | mforms::VFillFlag);
  _table.add(&_desc3, 2, 3, row, row + 1, mforms::HFillFlag);

  _tab.set_name("Connection Details");
  _params_panel.set_name("Parameters");
  _ssl_panel.set_name("SSL");
  _advanced_panel.set_name("Advanced");
  _options_panel.set_name("Options");

  set_name("Connection");
  setInternalName("connect_panel");

  add(&_content, true, true);
  _content.add(&_table, false, true);
  _content.add(&_tab, true, true);
  _warning.set_style(mforms::SmallHelpTextStyle);
  _warning.set_front_color("#FF0000");
  _content.add(&_warning, false, true);
}

DbConnectPanel::~DbConnectPanel() {
  if (_delete_connection_be)
    delete _connection;
}

void DbConnectPanel::connection_user_input(std::string &text_entry, bool &create_group, bool new_entry /*= true*/) {
  std::size_t pos = text_entry.find_first_of("/");
  if (pos == std::string::npos)
    return;
  create_group = false;
  std::string group = text_entry.substr(0, pos);
  std::string message = (new_entry) ? "Do you want to create connection inside the group"
                                    : "Do you want to split the name and move the connection to the group";
  int ret = mforms::Utilities::show_message(
    "Place Connection in a Group.",
    base::strfmt("You have used a forward slash in your connection name, which is used to separate a group from the "
                 "real connection name.\n"
                 "%s '%s'? If you select 'No' all forward slashes in the name will be replaced by underscores.",
                 message.c_str(), group.c_str()),
    _("Yes"), _("No"));
  if (ret == mforms::ResultOk) {
    create_group = true;
    return;
  }
  while (pos != std::string::npos) {
    text_entry[pos] = '_';
    pos = text_entry.find_first_of("/", pos + 1);
  }
}

void DbConnectPanel::change_connection_name() {
  if (_create_group)
    return;
  std::string text = _name_entry.get_string_value();
  connection_user_input(text, _create_group);
  _name_entry.set_value(text);
}

void DbConnectPanel::set_skip_schema_name(bool flag) {
  _skip_schema_name = flag;
}

void DbConnectPanel::suspend_view_layout(bool flag) {
  if (flag)
    suspend_layout();
  else
    resume_layout();
}

void DbConnectPanel::init(DbConnection *conn, const db_mgmt_ConnectionRef &default_conn) {
  _connection = conn;
  _delete_connection_be = false;

  _connection->set_control_callbacks(std::bind(&DbConnectPanel::suspend_view_layout, this, std::placeholders::_1),
                                     std::bind(&DbConnectPanel::begin_layout, this),
                                     std::bind(&DbConnectPanel::create_control, this, std::placeholders::_1,
                                               std::placeholders::_2, std::placeholders::_3, std::placeholders::_4),
                                     std::bind(&DbConnectPanel::end_layout, this));

  if (default_conn.is_valid())
    _anonymous_connection = default_conn;
  else {
    _anonymous_connection = db_mgmt_ConnectionRef(grt::Initialized);
    _anonymous_connection->owner(_connection->get_db_mgmt());
  }

  if (!_allowed_rdbms.is_valid()) {
    _allowed_rdbms = grt::ListRef<db_mgmt_Rdbms>(true);
    _allowed_rdbms.ginsert(_connection->get_db_mgmt()->rdbms()[0]);
  }

  _rdbms_sel.clear();
  for (grt::ListRef<db_mgmt_Rdbms>::const_iterator iter = _allowed_rdbms.begin(); iter != _allowed_rdbms.end(); ++iter)
    _rdbms_sel.add_item((*iter)->caption());
  _rdbms_sel.set_selected(0);

  _initialized = true;
  change_active_rdbms();

  if (!_anonymous_connection->driver().is_valid())
    _anonymous_connection->driver(selected_driver());

  if (_stored_connection_sel.get_selected_index() == 0) {
    if (default_conn.is_valid())
      _connection->set_connection_and_update(_anonymous_connection);
    else
      _connection->set_connection_keeping_parameters(_anonymous_connection);
  }
}

void DbConnectPanel::init(const db_mgmt_ManagementRef &mgmt, const grt::ListRef<db_mgmt_Rdbms> &allowed_rdbms,
                          const db_mgmt_ConnectionRef &default_conn) {
  if (!mgmt.is_valid())
    throw std::invalid_argument("DbConnectPanel::init() called with invalid db mgmt object");

  _allowed_rdbms = allowed_rdbms;

  DbConnection *conn = new DbConnection(
    mgmt, default_conn.is_valid() ? default_conn->driver() : _allowed_rdbms[0]->defaultDriver(), _skip_schema_name);

  init(conn, default_conn);
  _delete_connection_be = true;
}

void DbConnectPanel::init(const db_mgmt_ManagementRef &mgmt, const db_mgmt_ConnectionRef &default_conn) {
  if (!mgmt.is_valid())
    throw std::invalid_argument("DbConnectPanel::init() called with invalid db mgmt object");

  init(mgmt, mgmt->rdbms(), default_conn);
}

db_mgmt_ConnectionRef DbConnectPanel::get_connection(bool initInvalid) {
  if (!_connection->get_connection().is_valid() && initInvalid) {
    db_mgmt_ConnectionRef connection(grt::Initialized);
    connection->owner(get_be()->get_db_mgmt());
    connection->driver(selected_driver());
    set_connection(connection);
    change_active_stored_conn();
  }
  return _connection->get_connection();
}

grt::ListRef<db_mgmt_Connection> DbConnectPanel::connection_list() {
  if (_rdbms_sel.get_item_count() > 0) {
    int i = _rdbms_sel.get_selected_index();
    if (i >= 0 && i < (int)_allowed_rdbms->count()) {
      if (_allowed_rdbms[i]->id() == MYSQL_RDBMS_ID)
        return _connection->get_db_mgmt()->storedConns();
      else
        return _connection->get_db_mgmt()->otherStoredConns();
    }
  }

  db_mgmt_ConnectionRef conn(get_connection());
  if (conn.is_valid() && conn->driver().is_valid() && conn->driver()->owner().is_valid() &&
      conn->driver()->owner().id() == MYSQL_RDBMS_ID)
    return _connection->get_db_mgmt()->storedConns();
  else
    return _connection->get_db_mgmt()->otherStoredConns();
}

void DbConnectPanel::set_connection(const db_mgmt_ConnectionRef &conn) {
  const grt::ListRef<db_mgmt_Connection> list(connection_list());

  int count = 0;
  grt::ListRef<db_mgmt_Connection>::const_iterator iter = list.begin();
  grt::StringRef conn_host = conn->hostIdentifier();
  for (; iter != list.end(); ++iter) {
    if (conn == (*iter)) {
      _stored_connection_sel.set_selected(count + 1);
      change_active_stored_conn();
      break;
    }
    ++count;
  }
}

void DbConnectPanel::set_enabled(bool flag) {
  _name_entry.set_enabled(flag);
  _stored_connection_sel.set_enabled(flag);
  _rdbms_sel.set_enabled(flag);
  _driver_sel.set_enabled(flag);

  for (std::list<mforms::View *>::const_iterator iter = _views.begin(); iter != _views.end(); ++iter)
    (*iter)->set_enabled(flag);
}

void DbConnectPanel::set_default_host_name(const std::string &host, bool update) {
  _default_host_name = host;
  /*
 if (update)
 {
   for (std::map<std::string, db_mgmt_ConnectionRef>::iterator iter= _anonymous_connections.begin();
        iter != _anonymous_connections.end(); ++iter)
   {
     if (is_ssh_driver(iter->first))
       iter->second->parameterValues().gset("sshHost", _default_host_name);
     else
       iter->second->parameterValues().gset("hostName", _default_host_name);
   }

   // force UI update
   change_active_driver();
 }*/
}

void DbConnectPanel::param_value_changed(mforms::View *sender, bool trim_whitespace) {
  std::string param_name = sender->getInternalName();

  if (!_allow_edit_connections && !_updating) {
    // if stored connections combo is shown, copy the current connection params to the
    // to anonymous connection and select it
    // since stored connections are not editable in this case
    _connection->set_connection_keeping_parameters(_anonymous_connection);
    if (_stored_connection_sel.get_selected_index() != 0)
      _stored_connection_sel.set_selected(0);
  }

  DbDriverParam *param = _connection->get_db_driver_param_handles()->get(param_name);

  if (trim_whitespace)
    param->set_value(grt::StringRef(base::trim(sender->get_string_value())));
  else
    param->set_value(grt::StringRef(sender->get_string_value()));

  _connection->save_changes();

  std::string error = _connection->validate_driver_params();
  if (error != _last_validation)
    _signal_validation_state_changed(error, error.empty());
  _last_validation = error;
}

void DbConnectPanel::enum_param_value_changed(mforms::Selector *sender, std::vector<std::string> options) {
  std::string param_name = sender->getInternalName();

  if (!_allow_edit_connections && !_updating) {
    // if stored connections combo is shown, copy the current connection params to the
    // to anonymous connection and select it
    // since stored connections are not editable in this case
    _connection->set_connection_keeping_parameters(_anonymous_connection);
    if (_stored_connection_sel.get_selected_index() != 0)
      _stored_connection_sel.set_selected(0);
  }

  DbDriverParam *param = _connection->get_db_driver_param_handles()->get(param_name);
  int i = sender->get_selected_index();
  if (i >= 0)
    param->set_value(grt::StringRef(options[i]));
  else
    param->set_value(grt::StringRef(""));

  if (_connection) {
    _connection->save_changes();

    std::string error = _connection->validate_driver_params();
    if (error != _last_validation)
      _signal_validation_state_changed(error, error.empty());
    _last_validation = error;
  }
}

void DbConnectPanel::change_active_rdbms() {
  if (_initialized && !_updating) {
    if (!_allow_edit_connections) {
      _connection->set_connection_keeping_parameters(_anonymous_connection);
      if (_stored_connection_sel.get_selected_index() != 0)
        _stored_connection_sel.set_selected(0);
    }
    db_mgmt_RdbmsRef active_rdbms(selected_rdbms());
    if (active_rdbms.is_valid()) {
      int i = 0;
      int default_driver = -1;
      _updating = true;
      // refresh list of drivers
      grt::ListRef<db_mgmt_Driver> drivers(active_rdbms->drivers());
      _driver_sel.clear();
      for (grt::ListRef<db_mgmt_Driver>::const_iterator iter = drivers.begin(); iter != drivers.end(); ++iter, ++i) {
        _driver_sel.add_item((*iter)->caption());
        if ((*iter) == active_rdbms->defaultDriver())
          default_driver = i;
      }

      if (_show_connection_combo) {
        // refresh list of stored connections
        // this will select the driver for the default connection
        refresh_stored_connections();

        if (_stored_connection_sel.get_selected_index() > 0)
          change_active_stored_conn();
        else
          _connection->set_driver_and_update(selected_driver());
      } else {
        // select the default driver for the rdbms
        if (default_driver >= 0)
          _driver_sel.set_selected(default_driver);
        _connection->set_driver_and_update(selected_driver());
      }

      _updating = false;
    } else
      logWarning("DbConnectPanel: no active rdbms\n");
  }
}

db_mgmt_RdbmsRef DbConnectPanel::selected_rdbms() {
  int i = _rdbms_sel.get_selected_index();
  if (i >= 0 && i < (int)_allowed_rdbms.count())
    return _allowed_rdbms[i];
  return db_mgmt_RdbmsRef();
}

db_mgmt_DriverRef DbConnectPanel::selected_driver() {
  int i = _driver_sel.get_selected_index();
  if (i >= 0 && i < (int)selected_rdbms()->drivers().count())
    return selected_rdbms()->drivers()[i];
  return db_mgmt_DriverRef();
}

void DbConnectPanel::change_active_driver() {
  if (_initialized && !_updating) {
    if (!_allow_edit_connections) {
      _connection->set_connection_keeping_parameters(_anonymous_connection);
      if (_stored_connection_sel.get_selected_index() != 0)
        _stored_connection_sel.set_selected(0);
    }
    db_mgmt_DriverRef current_driver = _connection->driver();
    db_mgmt_DriverRef new_driver = selected_driver();
    if (new_driver == current_driver)
      return;

    _content.show(false);
    // When switching to/from ssh based connections the value for the host name gets another
    // semantic. In ssh based connections the sshHost is the remote server name (what is otherwise
    // the host name) and the host name is relative to the ssh host (usually localhost).
    db_mgmt_ConnectionRef actual_connection = get_connection(true);
    if (*current_driver->name() == "MysqlNativeSSH") {
      std::string machine = actual_connection->parameterValues().get_string("sshHost");
      if (machine.find(':') != std::string::npos)
        machine = machine.substr(0, machine.find(':'));
      actual_connection->parameterValues().gset("hostName", machine);
    } else if (*new_driver->name() == "MysqlNativeSSH") {
      std::string machine = actual_connection->parameterValues().get_string("hostName");
      actual_connection->parameterValues().gset("sshHost", machine + ":22");
      actual_connection->parameterValues().gset("hostName", "127.0.0.1");
    }

    if (_driver_changed_cb)
      _driver_changed_cb(new_driver);

    _connection->set_driver_and_update(new_driver);

    _content.show();

    // db_mgmt_ConnectionRef conn(_connection->get_connection());
    //    grt::DictRef current_params(conn->parameterValues());
    //    // save current driver params
    //    for (grt::DictRef::const_iterator iter = current_params.begin(); iter != current_params.end(); ++iter)
    //      _parameters_per_driver[conn->driver()->name()]= grt::DictRef::cast_from(grt::copy_value(current_params,
    //      false));
    //    db_mgmt_DriverRef new_driver(selected_driver());
    //    // update params to driver-specific params
    //    if (_parameters_per_driver.find(new_driver->name()) == _parameters_per_driver.end())
    //    {
    //      grt::DictRef params(grt::DictRef::cast_from(grt::copy_value(current_params, false)));
    //
    //      if (!_default_host_name.empty())
    //      {
    //        if (is_ssh_driver(new_driver->name()))
    //          params.gset("sshHost", _default_host_name);
    //        else
    //          params.gset("hostName", _default_host_name);
    //      }
    //
    //      _parameters_per_driver[new_driver->name()]= params;
    //    }
    //    grt::replace_contents(conn->parameterValues(), _parameters_per_driver[new_driver->name()]);

    {
      // we update the validation msg
      _last_validation = _connection->validate_driver_params();
      // notify the frontend that the state has changed but don't show any error
      // even if there is one
      _signal_validation_state_changed("", _last_validation.empty());
    }
  }
}

void DbConnectPanel::refresh_stored_connections() {
  grt::ListRef<db_mgmt_Connection> list(connection_list());
  db_mgmt_RdbmsRef rdbms = selected_rdbms();

  int selected_index = 0, i = 1;

  _stored_connection_sel.clear();
  _stored_connection_sel.add_item("");
  for (grt::ListRef<db_mgmt_Connection>::const_iterator iter = list.begin(); iter != list.end(); ++iter) {
    if (is_connectable_driver_type((*iter)->driver())) {
      if (!rdbms.is_valid() || ((*iter)->driver().is_valid() && (*iter)->driver()->owner() == rdbms)) {
        _stored_connection_sel.add_item((*iter)->name());
        if (*(*iter)->isDefault() && !_dont_set_default_connection)
          selected_index = i;
        i++;
      }
    }
  }

  if (_show_manage_connections) {
    _stored_connection_sel.add_item("-");
    _stored_connection_sel.add_item(_("Manage Stored Connections..."));
  }
  if (_stored_connection_sel.get_selected_index() != selected_index)
    _stored_connection_sel.set_selected(selected_index);
}

/**
 Save the current connection with the given name.
 */
void DbConnectPanel::save_connection_as(const std::string &name) {
  _connection->save_changes();

  db_mgmt_ConnectionRef conn(_connection->get_connection());

  grt::ListRef<db_mgmt_Connection> list(_connection->get_db_mgmt()->storedConns());

  db_mgmt_ConnectionRef dup;
  if ((dup = find_named_object_in_list(list, name, true, "name")).is_valid()) {
    list->remove(dup);
  }

  list = _connection->get_db_mgmt()->otherStoredConns();

  if ((dup = find_named_object_in_list(list, name, true, "name")).is_valid()) {
    list->remove(dup);
  }

  conn->name(name);
  conn->owner(_connection->get_db_mgmt());

  connection_list().insert(conn);

  refresh_stored_connections();
  change_active_stored_conn();
}

bool DbConnectPanel::test_connection() {
  std::string message = "Information related to this connection:\n\n";

  bool failed = false;
  try {
    sql::DriverManager *dbc_drv_man = sql::DriverManager::getDriverManager();
    db_mgmt_ConnectionRef connectionProperties = get_be()->get_connection();
    if (!connectionProperties.is_valid()) {
      db_mgmt_ConnectionRef connection(grt::Initialized);
      connection->owner(get_be()->get_db_mgmt());
      connection->driver(selected_driver());
      set_connection(connection);
      change_active_stored_conn();
      connectionProperties = get_be()->get_connection();
    }
    std::string ssl_cipher;

    message.append("Host: " + connectionProperties->parameterValues().get_string("hostName") + "\n");
    message.append("Port: " + grt::IntegerRef(connectionProperties->parameterValues().get_int("port")).toString() +
                   "\n");
    message.append("User: " + connectionProperties->parameterValues().get_string("userName") + "\n");

    {
      sql::ConnectionWrapper _dbc_conn = dbc_drv_man->getConnection(connectionProperties);

      if (_dbc_conn.get() && !_dbc_conn->isClosed()) {
        // check that we're connecting to a known and supported version of the server
        std::string version;
        {
          std::unique_ptr<sql::Statement> stmt(_dbc_conn->createStatement());
          std::unique_ptr<sql::ResultSet> result(stmt->executeQuery("SELECT version()"));
          if (result->next())
            version = result->getString(1);
        }
        if (!bec::is_supported_mysql_version(version)) {
          logError("Unsupported server version: %s %s\n", _dbc_conn->getMetaData()->getDatabaseProductName().c_str(),
                   version.c_str());
          // TODO: we have the same message in wb_context.cpp. Unite them.
          if (mforms::Utilities::show_warning(
                "Connection Warning",
                base::strfmt("Incompatible/nonstandard server version or connection protocol detected (%s).\n\n"
                             "A connection to this database can be established but some MySQL Workbench features may "
                             "not work properly since the database is not fully compatible with the supported versions "
                             "of MySQL.\n\n"
                             "MySQL Workbench is developed and tested for MySQL Server versions 5.6, 5.7 and 8.0",
                             bec::sanitize_server_version_number(version).c_str()),
                "Continue Anyway", "Cancel") != mforms::ResultOk)
            return false;
        }

        // check ssl
        {
          std::unique_ptr<sql::Statement> stmt(_dbc_conn->createStatement());
          std::unique_ptr<sql::ResultSet> result(stmt->executeQuery("SHOW SESSION STATUS LIKE 'Ssl_cipher'"));
          if (result->next())
            ssl_cipher = result->getString(2);

          if (ssl_cipher.empty())
            message.append("SSL: not enabled\n");
          else
            message.append("SSL: enabled with " + ssl_cipher + "\n");
        }

      } else {
        message = "Connection Failed";
        failed = true;
      }
    }
  } catch (const std::exception &e) {
    message = e.what();
    failed = true;
  }

  bool ret_val = false;
  if (message != "Operation Cancelled") {
    std::string title;
    if (failed) {
      title = base::strfmt("Failed to Connect to %s",
                           bec::get_description_for_connection(get_be()->get_connection()).c_str());
      mforms::Utilities::show_error(title, message, "OK");
    } else {
      message.append("\nA successful MySQL connection was made with\nthe parameters defined for this connection.");
      mforms::Utilities::show_message("Successfully made the MySQL connection", message, "OK");
      ret_val = true;
    }
  }

  return ret_val;
}

void DbConnectPanel::set_active_stored_conn(const std::string &name) {
  if (name.empty())
    _connection->set_connection_keeping_parameters(_anonymous_connection);
  else
    set_active_stored_conn(find_named_object_in_list(connection_list(), name, true, "name"));
}

void DbConnectPanel::set_active_stored_conn(db_mgmt_ConnectionRef connection) {
  _warning.set_text("");
  if (!connection.is_valid())
    connection = _anonymous_connection;

  db_mgmt_DriverRef driver = connection->driver();
  if (!driver.is_valid()) {
    logError("Connection %s has no driver set\n", connection->name().c_str());
    return;
  }

  db_mgmt_RdbmsRef rdbms = db_mgmt_RdbmsRef::cast_from(driver->owner());
  // check if the rdbms of the connection is not the selected one (usually should be)
  if (rdbms.is_valid() && selected_rdbms() != rdbms) {
    size_t rdbms_index = find_object_index_in_list(_allowed_rdbms, rdbms->id());
    _rdbms_sel.set_selected((int)rdbms_index);
    change_active_rdbms();
  }

  // ensure the correct driver is selected in the selector
  ssize_t driver_index = find_object_index_in_list(rdbms->drivers(), driver->id());
  if (driver_index >= 0 && driver_index < _driver_sel.get_item_count())
    _driver_sel.set_selected((int)driver_index);

  // mark this connection as the active one for this rdbms type
  if (!_dont_set_default_connection) {
    grt::ListRef<db_mgmt_Connection> conns(connection_list());
    for (size_t c = conns->count(), i = 0; i < c; i++) {
      db_mgmt_ConnectionRef conn(conns[i]);
      if (conn->driver().is_valid() && conn->driver()->owner() == rdbms)
        conn->isDefault(0);
    }
    connection->isDefault(1);
  }

  // set the connection as the one being edited, updating the UI for it
  _connection->set_connection_and_update(connection);

  if (!_show_connection_combo)
    _name_entry.set_value(connection->name());
}

void DbConnectPanel::change_active_stored_conn() {
  static bool choosing = false;
  if (_initialized && !choosing) {
    _updating = true;

    if (_show_manage_connections &&
        _stored_connection_sel.get_selected_index() == _stored_connection_sel.get_item_count() - 1) {
      choosing = true;
      db_mgmt_ConnectionRef connection = open_editor();
      refresh_stored_connections();
      if (connection.is_valid())
        _stored_connection_sel.set_selected(_stored_connection_sel.index_of_item_with_title(*connection->name()));
      else
        _stored_connection_sel.set_selected(0);
      show(false);
      set_active_stored_conn(connection);
      show();
      choosing = false;
    } else {
      std::string name = _stored_connection_sel.get_string_value();
      show(false);
      set_active_stored_conn(name);
      show();
    }
    _updating = false;

    // Revalidate connection parameters.
    std::string error = _connection->validate_driver_params();
    if (error != _last_validation)
      _signal_validation_state_changed(error, error.empty());
    _last_validation = error;
  }
}

void DbConnectPanel::launch_ssl_wizard() {
  mforms::Form *parent = get_parent_form();
  grt::BaseListRef args(true);
  args.ginsert(mforms_to_grt(parent, "Form"));
  args.ginsert(get_connection(true));
  args.ginsert(grt::StringRef(get_connection(true)->id()));
  grt::GRT::get()->call_module_function("PyWbUtils", "generateCertificates", args);
  _connection->update();
}

void DbConnectPanel::open_ssl_wizard_directory() {
  std::string path = base::joinPath(mforms::App::get()->get_user_data_folder().c_str(), "certificates",
                                    get_connection()->id().c_str(), "");

  if (base::is_directory(path))
    Utilities::open_url(path);
  else
    mforms::Utilities::show_warning(
      _("Cannot Open Directory"),
      _("The directory that should contain the files does not exist yet. Maybe you need to run the SSL Wizard first."),
      _("OK"));
}

db_mgmt_ConnectionRef DbConnectPanel::open_editor() {
  grt::ListRef<db_mgmt_Rdbms> rdbms_list(true);
  rdbms_list.ginsert(selected_rdbms());
  DbConnectionEditor editor(_connection->get_db_mgmt());

  return editor.run(_connection->get_connection());
}

void DbConnectPanel::begin_layout() {
  _last_active_tab = _tab.get_active_tab();
  if (_params_table) {
    _params_panel.remove(_params_table);
    _tab.remove_page(&_params_panel);
  }
  if (_ssl_table) {
    _ssl_panel.remove(_ssl_table);
    _tab.remove_page(&_ssl_panel);
  }
  if (_advanced_table) {
    _advanced_panel.remove(_advanced_table);
    _tab.remove_page(&_advanced_panel);
  }
  if (_options_table) {
    _options_panel.remove(_options_table);
    _tab.remove_page(&_options_panel);
  }

  _params_table = mforms::manage(new mforms::Table());
  _params_table->set_release_on_add();
  _params_table->set_name("Parameters Table");
  _params_table->setInternalName("params_table");
  _params_table->set_column_count(3);
  _params_table->set_row_spacing(MF_TABLE_ROW_SPACING);
  _params_table->set_column_spacing(MF_TABLE_COLUMN_SPACING);
  _params_table->set_padding(MF_PANEL_PADDING);

  _ssl_table = mforms::manage(new mforms::Table());
  _ssl_table->set_name("SSL Table");
  _ssl_table->setInternalName("ssl_table");
  _ssl_table->set_column_count(3);
  _ssl_table->set_row_spacing(MF_TABLE_ROW_SPACING);
  _ssl_table->set_column_spacing(MF_TABLE_COLUMN_SPACING);
  _ssl_table->set_padding(MF_PANEL_PADDING);

  _advanced_table = mforms::manage(new mforms::Table());
  _advanced_table->set_name("Advanced Table");
  _advanced_table->setInternalName("advanced_table");
  _advanced_table->set_column_count(3);
  _advanced_table->set_row_spacing(MF_TABLE_ROW_SPACING);
  _advanced_table->set_column_spacing(MF_TABLE_COLUMN_SPACING);
  _advanced_table->set_padding(MF_PANEL_PADDING);

  _options_table = mforms::manage(new mforms::Table());
  _options_table->set_name("Options Table");
  _options_table->setInternalName("options_table");
  _options_table->set_column_count(3);
  _options_table->set_row_spacing(MF_TABLE_ROW_SPACING);
  _options_table->set_column_spacing(MF_TABLE_COLUMN_SPACING);
  _options_table->set_padding(MF_PANEL_PADDING);

  _views.clear();
  _param_rows.clear();
  _ssl_rows.clear();
  _advanced_rows.clear();
  _options_rows.clear();
}

void DbConnectPanel::end_layout() {
  if (!_param_rows.empty()) {
    _params_panel.add(_params_table);
    _tab.add_page(&_params_panel, _("Parameters"));
  }

  if (!_ssl_rows.empty()) {
    _ssl_panel.add(_ssl_table);
    _tab.add_page(&_ssl_panel, _("SSL"));
  }

  if (!_advanced_rows.empty()) {
    _advanced_panel.add(_advanced_table);
    _tab.add_page(&_advanced_panel, _("Advanced"));
  }

  if (!_options_rows.empty()) {
    _options_panel.add(_options_table);
    _tab.add_page(&_options_panel, _("Options"));
  }

  if (_last_active_tab != -1)
    _tab.set_active_tab(_last_active_tab);
}

void DbConnectPanel::set_keychain_password(DbDriverParam *param, bool clear) {
  std::string storageKey;
  std::string userName;
  grt::DictRef paramValues(get_connection(true)->parameterValues());
  std::vector<std::string> tokens =
    base::split(param->object()->paramTypeDetails().get_string("storageKeyFormat"), "::");
  if (tokens.size() == 2) {
    userName = tokens[0];
    storageKey = tokens[1];
  } else {
    logError("Invalid storage key format for option %s\n", param->object().id().c_str());
    return;
  }

  for (grt::DictRef::const_iterator iter = paramValues.begin(); iter != paramValues.end(); ++iter) {
    storageKey = base::replaceString(storageKey, "%" + iter->first + "%", iter->second.toString());
    userName = base::replaceString(userName, "%" + iter->first + "%", iter->second.toString());
  }

  if (storageKey.substr(0, 3) == "ssh") { // This is ssh, we should check if port was given, if not, we will append the default 22.
    auto pos = storageKey.find_last_of(":");
    if (pos == std::string::npos) {
      storageKey.append(":22");
    }
  }

  if (userName.empty()) {
    mforms::Utilities::show_warning(_("Cannot Set Password"), _("Please fill the username to be used."), _("OK"));
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
      if (mforms::Utilities::ask_for_password("Store Password For Connection", storageKey, userName, password))
        mforms::Utilities::store_password(storageKey, userName, password);
    } catch (std::exception &exc) {
      mforms::Utilities::show_error("Store Password", base::strfmt("Could not store password: %s", exc.what()), "OK");
    }
  }
}

void DbConnectPanel::create_control(::DbDriverParam *driver_param, const ::ControlType ctrl_type,
                                    const ControlBounds &bounds, const std::string &caption) {
  bool is_new_line = false;
  Table *table = NULL;
  Box *box = NULL;
  std::vector<mforms::Box *> *rows = NULL;

  switch (driver_param->object()->layoutAdvanced()) {
    case 0:
      rows = &_param_rows;
      table = _params_table;
      break;
    case 1:
      rows = &_advanced_rows;
      table = _advanced_table;
      break;
    case 2:
      rows = &_ssl_rows;
      table = _ssl_table;
      break;
    case 3:
      rows = &_options_rows;
      table = _options_table;
      break;
    default:
      return;
  }

  if (bounds.top >= (int)rows->size()) {
    is_new_line = true;

    table->set_row_count((int)rows->size() + 1);
    if (ctrl_type == ::ctCheckBox && table != _params_table) {
      rows->push_back(box = mforms::manage(new Box(false)));
      box->set_spacing(0);
    } else {
      rows->push_back(box = mforms::manage(new Box(true)));
      box->set_spacing(4);
    }
    _views.push_back(box);

    mforms::TableItemFlags flags;
    flags = mforms::HExpandFlag | mforms::HFillFlag | mforms::VFillFlag;
    if (driver_param->get_type() == DbDriverParam::ptText)
      flags = flags | mforms::VExpandFlag | mforms::VFillFlag;
    table->add(mforms::manage(box), 1, 2, bounds.top, bounds.top + 1, flags);
  } else
    box = (*rows)[bounds.top];

  switch (ctrl_type) {
    case ::ctLabel: {
      Label *label = new Label();
      label->set_text(caption);
      label->set_name(driver_param->get_accessibility_name());
      label->set_text_align(mforms::TopRight);

      if (is_new_line)
        table->add(mforms::manage(label), 0, 1, bounds.top, bounds.top + 1, mforms::HFillFlag | mforms::VFillFlag);
      else
        box->add(mforms::manage(label), false, true);
      _views.push_back(label);
      break;
    }
    case ::ctDescriptionLabel: {
      Label *label = new Label();
      label->set_text(caption);
      label->set_name(driver_param->get_accessibility_name());
      label->set_text_align(mforms::TopLeft);
      label->set_style(mforms::SmallHelpTextStyle);
      label->set_wrap_text(true);
      label->set_size(250, -1);
      table->add(mforms::manage(label), 2, 3, bounds.top, bounds.top + 1, mforms::HFillFlag | mforms::VFillFlag);
      _views.push_back(label);
      break;
    }
    case ::ctButton: {
      Button *btn = new Button();
      btn->set_text(caption);
      btn->set_size(bounds.width, 30);

      box->add(mforms::manage(btn), false, true);
      _views.push_back(btn);

      if (driver_param->object()->name() == "sslWizard") {
        scoped_connect(btn->signal_clicked(), std::bind(&DbConnectPanel::launch_ssl_wizard, this));
      } else if (driver_param->object()->name() == "openSSLWizardDirectory") {
        scoped_connect(btn->signal_clicked(), std::bind(&DbConnectPanel::open_ssl_wizard_directory, this));
      }

      break;
    }
    case ::ctCheckBox: {
      CheckBox *ctrl = new CheckBox();

      ctrl->set_name(driver_param->get_accessibility_name());
      ctrl->setInternalName(driver_param->get_control_name());

      ctrl->set_text(caption);

      // value
      {
        grt::StringRef value = driver_param->get_value_repr();
        if (value.is_valid())
          ctrl->set_active(*value != "" && *value != "0" && *value != "NULL");
      }

      scoped_connect(ctrl->signal_clicked(), std::bind(&DbConnectPanel::param_value_changed, this, ctrl, false));

      box->add(mforms::manage(ctrl), false, true);
      _views.push_back(ctrl);

      break;
    }
    case ::ctKeychainPassword: {
      Button *btn = new Button();

#ifdef _MSC_VER
      btn->set_text("Store in Vault ...");
      btn->set_tooltip(_("Store the password for this connection in a secured vault"));
#else
      btn->set_text("Store in Keychain ...");
      btn->set_tooltip(_("Store the password for this connection in the system's keychain"));
#endif

      box->add(mforms::manage(btn), false, true);
      _views.push_back(btn);
      scoped_connect(btn->signal_clicked(),
                     std::bind(&DbConnectPanel::set_keychain_password, this, driver_param, false));

      btn = new Button();
      btn->set_text("Clear");
      btn->set_size(100, -1);
#ifdef _MSC_VER
      btn->set_tooltip(_("Remove the previously stored password from the secured vault"));
#else
      btn->set_tooltip(_("Remove the previously stored password from the system's keychain"));
#endif
      box->add(mforms::manage(btn), false, true);
      _views.push_back(btn);
      scoped_connect(btn->signal_clicked(),
                     std::bind(&DbConnectPanel::set_keychain_password, this, driver_param, true));

      break;
    }
    case ::ctTextBox: {
      bool is_password = ::DbDriverParam::ptPassword == driver_param->get_type();
      TextEntry *ctrl = new TextEntry(is_password ? PasswordEntry : NormalEntry);

      ctrl->set_name(driver_param->get_accessibility_name());
      ctrl->setInternalName(driver_param->get_control_name());

      // value
      {
        grt::StringRef value = driver_param->getValue();
        if (value.is_valid())
          ctrl->set_value(*value);
      }

      ctrl->set_size(bounds.width, -1);

      scoped_connect(ctrl->signal_changed(), std::bind(&DbConnectPanel::param_value_changed, this, ctrl, true));

      box->add(mforms::manage(ctrl), true, true);
      _views.push_back(ctrl);

      break;
    }
    case ::ctText: {
      TextBox *ctrl = new TextBox(mforms::VerticalScrollBar);
      ctrl->set_name(driver_param->get_accessibility_name());
      ctrl->setInternalName(driver_param->get_control_name());

      // value
      {
        grt::StringRef value = driver_param->get_value_repr();
        if (value.is_valid())
          ctrl->set_value(*value);
      }

      ctrl->set_size(bounds.width, -1);

      scoped_connect(ctrl->signal_changed(), std::bind(&DbConnectPanel::param_value_changed, this, ctrl, false));

      box->add(mforms::manage(ctrl), true, true);
      _views.push_back(ctrl);

      break;
    }
    case ::ctFileSelector: {
      FsObjectSelector *ctrl = new FsObjectSelector();
      ctrl->set_name(driver_param->get_accessibility_name());
      ctrl->setInternalName(driver_param->get_control_name());
      // value
      grt::StringRef value = driver_param->get_value_repr();
      std::string initial_value = "";
      if (value.is_valid())
        initial_value = *value;

      ctrl->set_size(bounds.width, -1);

      ctrl->initialize(initial_value, mforms::OpenFile, "", true,
                       std::bind(&DbConnectPanel::param_value_changed, this, ctrl, true));
      box->add(mforms::manage(ctrl), true, true);
      _views.push_back(ctrl);
      break;
    }
    case ::ctDirSelector: {
      FsObjectSelector *ctrl = new FsObjectSelector();
      ctrl->set_name(driver_param->get_accessibility_name());
      ctrl->setInternalName(driver_param->get_control_name());
      // value
      grt::StringRef value = driver_param->get_value_repr();
      std::string initial_value = "";
      if (value.is_valid())
        initial_value = *value;

      ctrl->set_size(bounds.width, -1);

      ctrl->initialize(initial_value, mforms::OpenDirectory, "", true,
                       std::bind(&DbConnectPanel::param_value_changed, this, ctrl, true));
      box->add(mforms::manage(ctrl), true, true);
      _views.push_back(ctrl);
      break;
    }
    case ::ctEnumSelector: {
      mforms::Selector *ctrl = new Selector();
      ctrl->set_name(driver_param->get_accessibility_name());
      ctrl->setInternalName(driver_param->get_control_name());
      std::vector<std::pair<std::string, std::string> > options;
      std::vector<std::string> option_ids;
      std::string value = driver_param->get_value_repr();
      int idx = -1;

      try {
        options = driver_param->get_enum_options();
      } catch (std::exception &e) {
        logError("Error calling get_enum_options() for param %s: %s", driver_param->get_control_name().c_str(),
                 e.what());
        mforms::Utilities::show_error(
          "Connection Setup",
          base::strfmt("An error occurred while retrieving values for option '%s' from '%s'.\n\n%s",
                       driver_param->object()->name().c_str(), selected_driver()->name().c_str(), e.what()),
          "OK", "", "");
      }

      for (size_t i = 0; i < options.size(); i++) {
        ctrl->add_item(options[i].second);
        option_ids.push_back(options[i].first);
        if (value == options[i].first)
          idx = (int)i;
      }
      if (idx >= 0)
        ctrl->set_selected(idx);
      enum_param_value_changed(ctrl, option_ids);

      scoped_connect(ctrl->signal_changed(),
                     std::bind(&DbConnectPanel::enum_param_value_changed, this, ctrl, option_ids));
      box->add(mforms::manage(ctrl), true, true);
      _views.push_back(ctrl);
      break;
    }
    case ::ctEnumOption: {
      std::vector<std::pair<std::string, std::string> > options;
      std::string value;
      try {
        options = driver_param->get_enum_options();
        value = _connection->get_connection()->parameterValues().get("kerberosMode").toString();
      } catch (std::exception &e) {
        logError("Error calling get_enum_options() for param %s: %s", driver_param->get_control_name().c_str(),
                 e.what());
        mforms::Utilities::show_error(
          "Connection Setup",
          base::strfmt("An error occurred while retrieving values for option '%s' from '%s'.\n\n%s",
                       driver_param->object()->name().c_str(), selected_driver()->name().c_str(), e.what()),
          "OK", "", "");
      }
      Box *inner_box = new Box(false);
      unsigned int group_id = mforms::RadioButton::new_id();
      
      for (auto &option: options) {
        mforms::RadioButton *rb = new mforms::RadioButton(group_id);
        rb->set_text(option.second);
        rb->set_name(driver_param->get_accessibility_name().toString() + "_" + option.first);
        rb->setInternalName(driver_param->get_control_name().toString() + "_" + option.first);
        
        rb->set_active(option.first == value);
#ifndef _MSC_VER
        if (option.first == "1")
          rb->set_enabled(false);
#endif        

        scoped_connect(rb->signal_clicked(), [=](){
          _connection->get_connection()->parameterValues().gset("kerberosMode", option.first);
        });
         
        inner_box->add(mforms::manage(rb), false, true);
      }

      box->add(mforms::manage(inner_box), true, true);
      break;
    }
    default:
      logWarning("Unknown param type for %s\n", driver_param->get_control_name().c_str());
      break;
  }
}

//--------------------------------------------------------------------------------------------------

bool DbConnectPanel::is_connectable_driver_type(db_mgmt_DriverRef driver) {
  if (driver.is_valid()) {
    std::string d = driver->id();

    if (driver->owner().is_valid()) {
      if (driver->owner()->id() != MYSQL_RDBMS_ID || d == "com.mysql.rdbms.mysql.driver.native" ||
          d == "com.mysql.rdbms.mysql.driver.native_socket" || d == "com.mysql.rdbms.mysql.driver.native_sshtun") {
        return true;
      }
    }
  }
  return false;
}
