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

#ifndef _CONNECTION_PAGE_H_
#define _CONNECTION_PAGE_H_

#include "grtui/grtdb_connect_panel.h"

class ConnectionPage : public grtui::WizardPage {
public:
  ConnectionPage(grtui::WizardForm *form, const char *name = "connect", const std::string &selection_save_name = "")
    : WizardPage(form, name),
      _dbconn(0),
      _connect(grtui::DbConnectPanelDefaults |
               (selection_save_name.empty() ? 0 : grtui::DbConnectPanelDontSetDefaultConnection)),
      _selection_save_name(selection_save_name) {
    set_title(_("Set Parameters for Connecting to a DBMS"));
    set_short_title(_("Connection Options"));

    add(&_connect, true, true);

    scoped_connect(
      _connect.signal_validation_state_changed(),
      std::bind(&ConnectionPage::connection_validation_changed, this, std::placeholders::_1, std::placeholders::_2));
  }

  void set_db_connection(DbConnection *conn) {
    _dbconn = conn;
    _connect.init(_dbconn);
  }

protected:
  virtual bool pre_load() {
    if (!_dbconn)
      throw std::logic_error("must call set_db_connection() 1st");

    if (!_selection_save_name.empty()) {
      std::string name = bec::GRTManager::get()->get_app_option_string(_selection_save_name);
      if (!name.empty())
        _connect.set_active_stored_conn(name);
    }

    return true;
  }

  virtual bool advance() {
    if (!_selection_save_name.empty()) {
      db_mgmt_ConnectionRef conn(_connect.get_connection());
      if (conn.is_valid() && conn->name() != "")
        bec::GRTManager::get()->set_app_option(_selection_save_name, conn->name());
    }

    return WizardPage::advance();
  }

  void connection_validation_changed(const std::string &error, bool ok) {
    if (!ok)
      _form->set_problem(error);
    else
      _form->clear_problem();
  }

protected:
  DbConnection *_dbconn;
  grtui::DbConnectPanel _connect;
  std::string _selection_save_name;
};

#endif // _CONNECTION_PAGE_H_
