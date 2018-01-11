/*
 * Copyright (c) 2012, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <grts/structs.ui.h>
#include <grts/structs.db.mgmt.h>

#include <grtpp_util.h>
#include "grtui/grtdb_connect_panel.h"
#include "../wrapper/mforms_ObjectReference_impl.h"

//================================================================================
// ui_db_ConnectPanel

class ui_db_ConnectPanel::ImplData {
  grtui::DbConnectPanel *_panel;

public:
  ImplData() : _panel(0) {
  }

  void init(const db_mgmt_ManagementRef &mgmt) {
    if (!_panel) {
      _panel = new grtui::DbConnectPanel();
      _panel->init(mgmt);
    }
  }

  void init(const db_mgmt_ManagementRef &mgmt, const grt::ListRef<db_mgmt_Rdbms> &rdbms_list) {
    if (!_panel) {
      _panel =
        new grtui::DbConnectPanel(grtui::DbConnectPanelShowConnectionCombo | grtui::DbConnectPanelShowRDBMSCombo);
      _panel->init(mgmt, rdbms_list);
    }
  }

  grtui::DbConnectPanel *panel() {
    return _panel;
  }

  ~ImplData() {
    delete _panel;
  }
};

void ui_db_ConnectPanel::init() {
  _data = new ImplData();
}

ui_db_ConnectPanel::~ui_db_ConnectPanel() {
  delete _data;
}

void ui_db_ConnectPanel::set_data(ImplData *data) {
  throw std::logic_error("wrong call to set_data()");
}

void ui_db_ConnectPanel::initialize(const grt::Ref<db_mgmt_Management> &mgmt) {
  _data->init(mgmt);
}

void ui_db_ConnectPanel::initializeWithRDBMSSelector(const grt::Ref<db_mgmt_Management> &mgmt,
                                                     const grt::ListRef<db_mgmt_Rdbms> &rdbms_list) {
  _data->init(mgmt, rdbms_list);
}

grt::Ref<db_mgmt_Connection> ui_db_ConnectPanel::connection() const {
  if (_data && _data->panel()) {
    _data->panel()->get_be()->save_changes();
    return _data->panel()->get_connection();
  }
  return db_mgmt_ConnectionRef();
}

void ui_db_ConnectPanel::connection(const grt::Ref<db_mgmt_Connection> &value) {
  if (_data && _data->panel())
    _data->panel()->set_connection(value);
  throw std::logic_error("Cannot set connection value to non-initialized ui.db.ConnectionPanel instance");
}

grt::Ref<mforms_ObjectReference> ui_db_ConnectPanel::view() const {
  if (_data && _data->panel())
    return mforms_to_grt(_data->panel(), "Box");
  return grt::Ref<mforms_ObjectReference>();
}

void ui_db_ConnectPanel::saveConnectionAs(const std::string &name) {
  if (_data && _data->panel())
    _data->panel()->save_connection_as(name);
}
