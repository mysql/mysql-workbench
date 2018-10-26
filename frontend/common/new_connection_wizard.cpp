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

#include "new_connection_wizard.h"

#include "cppdbc.h"
#include "grtpp_util.h"
#include "grt/common.h"
#include "mforms/utilities.h"
#include "base/string_utilities.h"

#include "new_server_instance_wizard.h"

using namespace mforms;
using namespace base;

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

//----------------- NewConnectionWizard ------------------------------------------------------------

NewConnectionWizard::NewConnectionWizard(wb::WBContext *context, const db_mgmt_ManagementRef &mgmt)
  : mforms::Form(0), _context(context), _mgmt(mgmt), _panel(false), _top_vbox(false), _bottom_hbox(true) {
  set_title(_("Manage DB Connections"));
  set_name("New Connection Wizard");
  setInternalName("new_connection_wizard");

  _top_vbox.set_padding(MF_WINDOW_PADDING);
  _top_vbox.set_spacing(12);
  _top_vbox.add(&_panel, true, true);
  _top_vbox.add(&_bottom_hbox, false, true);

  _bottom_hbox.set_spacing(12);
  _bottom_hbox.set_name("Button Bar");
  _panel.init(_mgmt);

  _conn_name = _panel.get_name_entry();

  scoped_connect(_config_button.signal_clicked(), std::bind(&NewConnectionWizard::open_remote_mgm_config, this));
  _config_button.set_text(_("Configure Server Management..."));
  _config_button.enable_internal_padding(true);

  _bottom_hbox.add(&_config_button, false, true);
  _bottom_hbox.add_end(&_ok_button, false, true);
  _bottom_hbox.add_end(&_cancel_button, false, true);
  _bottom_hbox.add_end(&_test_button, false, true);

  _ok_button.set_text(_("OK"));
  _cancel_button.set_text(_("Cancel"));
  _test_button.set_text(_("Test Connection"));
  scoped_connect(_test_button.signal_clicked(), std::bind(&grtui::DbConnectPanel::test_connection, &_panel));

  _ok_button.enable_internal_padding(true);
  _cancel_button.enable_internal_padding(true);
  _test_button.enable_internal_padding(true);

  set_content(&_top_vbox);

  set_size(800, 500);
  center();
}

//--------------------------------------------------------------------------------------------------

NewConnectionWizard::~NewConnectionWizard() {
}

//--------------------------------------------------------------------------------------------------

void NewConnectionWizard::open_remote_mgm_config() {
  NewServerInstanceWizard wizard(_context, _panel.get_connection());
  wizard.run_modal();
}

//--------------------------------------------------------------------------------------------------

db_mgmt_ConnectionRef NewConnectionWizard::run() {
  _connection = db_mgmt_ConnectionRef(grt::Initialized);
  _connection->driver(_mgmt->rdbms()[0]->defaultDriver());
  if (find_named_object_in_list(_connection->driver()->parameters(), "useSSL").is_valid()) {
    // prefer SSL if possible by default
    _connection->parameterValues().set("useSSL", grt::IntegerRef(1));
  }
  _connection->hostIdentifier("Mysql@127.0.0.1:3306");
  _panel.get_be()->set_connection_and_update(_connection);
  _panel.get_be()->save_changes();
  // Return the newly created connection object.
  while (run_modal(&_ok_button, &_cancel_button)) {
    // Check for duplicate names.
    bool name_ok = true;
    std::string name = _conn_name->get_string_value();
    if (base::trim(name).empty()) {
      std::string text = _("Please enter a proper name for your new connection.");
      Utilities::show_error(_("Improper name"), text, _("OK"));
      name_ok = false;
    }

    if (name_ok) {
      GRTLIST_FOREACH(db_mgmt_Connection, _mgmt->storedConns(), conn) {
        if ((*conn)->name() == name) {
          std::string text =
            strfmt(_("A connection with the name %s exists already. Please choose another one."), name.c_str());
          Utilities::show_error(_("Duplicate name"), text, _("OK"));
          name_ok = false;
          break;
        }
      }
    }

    if (name_ok) {
      _connection->name(_conn_name->get_string_value().c_str());
      _mgmt->storedConns().insert(_connection);

      // Auto create an unconfigured server instance for this connection.
      // The module creates the instance, adds it to the stored instances and stores them on disk.
      {
        grt::BaseListRef args(true);
        args.ginsert(_connection);
        if (is_local_connection(_connection))
          db_mgmt_ServerInstanceRef::cast_from(
            grt::GRT::get()->call_module_function("WbAdmin", "autoDetectLocalInstance", args));
        else
          db_mgmt_ServerInstanceRef::cast_from(
            grt::GRT::get()->call_module_function("WbAdmin", "autoDetectRemoteInstance", args));
      }

      return _connection;
    }
  }
  return db_mgmt_ConnectionRef();
}

//--------------------------------------------------------------------------------------------------
