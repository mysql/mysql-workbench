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

#include "grtdb_connection_editor.h"

#include "cppdbc.h"
#include <grt/common.h>
#include "base/string_utilities.h"
#include "base/log.h"

#include "mforms/uistyle.h"
#include "mforms/utilities.h"

DEFAULT_LOG_DOMAIN(DOMAIN_WB_CONTEXT_UI)

//------------------------------------------------------------------------------

// for MySQL only connection editors
grtui::DbConnectionEditor::DbConnectionEditor(const db_mgmt_ManagementRef &mgmt)
  : mforms::Form(0),
    _mgmt(mgmt),
    _connection_list(mgmt->storedConns()),
    _panel(false),
    _top_vbox(false),
    _top_hbox(true),
    _conn_list_buttons_hbox(true),
    _stored_connection_list(mforms::TreeDefault | mforms::TreeFlatList),
    _bottom_hbox(true) {
  set_name("Connection Editor");
  setInternalName("connection_editor");
  grt::ListRef<db_mgmt_Rdbms> default_allowed_rdbms(true);
  default_allowed_rdbms.ginsert(find_object_in_list(mgmt->rdbms(), "com.mysql.rdbms.mysql"));
  _panel.init(_mgmt, default_allowed_rdbms);

  init();
}

void grtui::DbConnectionEditor::init() {
  set_title(_("Manage DB Connections"));

  _top_vbox.set_padding(MF_WINDOW_PADDING);
  _top_vbox.set_spacing(12);
  _top_hbox.set_spacing(8);
  _top_vbox.add(&_top_hbox, true, true);
  _top_vbox.add(&_bottom_hbox, false, true);

  _bottom_hbox.set_spacing(12);

  scoped_connect(_stored_connection_list.signal_changed(),
                 std::bind(&grtui::DbConnectionEditor::change_active_stored_conn, this));

  _conn_name = _panel.get_name_entry();
  scoped_connect(_conn_name->signal_changed(), std::bind(&grtui::DbConnectionEditor::name_changed, this));

  _dup_conn_button.set_text(_("Duplicate"));
  scoped_connect(_dup_conn_button.signal_clicked(), std::bind(&grtui::DbConnectionEditor::add_stored_conn, this, true));
  _del_conn_button.set_text(_("Delete"));
  scoped_connect(_del_conn_button.signal_clicked(), std::bind(&grtui::DbConnectionEditor::del_stored_conn, this));
  _add_conn_button.set_text(_("New"));
  scoped_connect(_add_conn_button.signal_clicked(),
                 std::bind(&grtui::DbConnectionEditor::add_stored_conn, this, false));
  _move_up_button.set_text(_("Move Up"));
  scoped_connect(_move_up_button.signal_clicked(), std::bind(&grtui::DbConnectionEditor::reorder_conn, this, true));
  _move_down_button.set_text(_("Move Down"));
  scoped_connect(_move_down_button.signal_clicked(), std::bind(&grtui::DbConnectionEditor::reorder_conn, this, false));

  _top_hbox.add(&_stored_connection_list, false, true);
  _top_hbox.add(&_panel, true, true);

  _bottom_hbox.add(&_add_conn_button, false, true);
  _bottom_hbox.add(&_del_conn_button, false, true);
  _bottom_hbox.add(&_dup_conn_button, false, true);
  _bottom_hbox.add(&_move_up_button, false, true);
  _bottom_hbox.add(&_move_down_button, false, true);

  _bottom_hbox.add_end(&_ok_button, false, true);
  //  _bottom_hbox.add_end(&_cancel_button, false, true);
  _bottom_hbox.add_end(&_test_button, false, true);

  _ok_button.set_text(_("Close"));
  scoped_connect(_ok_button.signal_clicked(), std::bind(&DbConnectionEditor::ok_clicked, this));
  _test_button.set_text(_("Test Connection"));
  scoped_connect(_test_button.signal_clicked(), std::bind(&DbConnectPanel::test_connection, std::ref(_panel)));

  _add_conn_button.enable_internal_padding(true);
  _del_conn_button.enable_internal_padding(true);
  _ok_button.enable_internal_padding(true);
  _cancel_button.enable_internal_padding(true);
  _test_button.enable_internal_padding(true);

  _stored_connection_list.set_size(180, -1);

  set_content(&_top_vbox);

  _stored_connection_list.add_column(::mforms::StringColumnType, _("Stored Connections"), 150, false);
  _stored_connection_list.end_columns();

  set_size(900, 500);
}

//------------------------------------------------------------------------------
grtui::DbConnectionEditor::~DbConnectionEditor() {
}

//------------------------------------------------------------------------------
db_mgmt_ConnectionRef grtui::DbConnectionEditor::run(const db_mgmt_ConnectionRef &connection) {
  size_t index;
  // check if the connection is a pre-saved one and if so, just select it in the list
  if ((index = _connection_list.get_index(connection)) != grt::BaseListRef::npos) {
    reset_stored_conn_list();
    _stored_connection_list.select_node(_stored_connection_list.node_at_row((int)index));
    change_active_stored_conn();
  } else {
    reset_stored_conn_list();

    // set the displayed params to the one given to us
    if (connection.is_valid())
      _panel.get_be()->set_connection_and_update(connection);
    // switch the connection object with the anon connection from the panel object
    _panel.get_be()->set_connection_keeping_parameters(_panel.get_default_connection());
    _conn_name->set_value(_("Press New to save these settings"));
  }
  // return the selected connection object
  if (run_modal(&_ok_button, &_cancel_button))
    return _panel.get_be()->get_connection();

  return db_mgmt_ConnectionRef();
}

//--------------------------------------------------------------------------------------------------
void grtui::DbConnectionEditor::run() {
  reset_stored_conn_list();

  if (run_modal(&_ok_button, &_cancel_button))
    grt::GRT::get()->call_module_function("Workbench", "saveConnections", grt::BaseListRef());
}

//--------------------------------------------------------------------------------------------------

void grtui::DbConnectionEditor::reset_stored_conn_list() {
  grt::ListRef<db_mgmt_Connection> list(_connection_list);
  std::string selected_name;

  if (_panel.get_be()->get_connection().is_valid())
    selected_name = _panel.get_be()->get_connection()->name();

  _stored_connection_list.clear();

  mforms::TreeNodeRef selected_node;
  mforms::TreeNodeRef added_row;
  grt::ListRef<db_mgmt_Connection>::const_iterator iter = list.begin();
  grt::ListRef<db_mgmt_Connection>::const_iterator last = list.end();
  for (; iter != last; ++iter) {
    if (_panel.is_connectable_driver_type((*iter)->driver())) {
      added_row = _stored_connection_list.root_node()->add_child();
      if (added_row) {
        added_row->set_string(0, (*iter)->name());
        added_row->set_tag((*iter)->id());
        if ((*iter)->name() == selected_name)
          selected_node = added_row;
      }
    }
  }

  if (selected_node)
    _stored_connection_list.select_node(selected_node);
  change_active_stored_conn();
}

//------------------------------------------------------------------------------
void grtui::DbConnectionEditor::add_stored_conn(bool copy) {
  grt::ListRef<db_mgmt_Connection> list(_connection_list);
  size_t length = std::string("New connection 1").length() - 1;
  int max_conn_nr = 0;
  for (size_t i = 0; i < list.count(); ++i) {
    std::string itname = list[i]->name();
    if (itname.find("New connection") == 0) {
      int conn_nr = base::atoi<int>(itname.substr(length), 0);
      if (conn_nr > max_conn_nr)
        max_conn_nr = conn_nr;
    }
  }
  char buf[128];
  sprintf(buf, "New connection %i", max_conn_nr + 1);

  // use the current values to create a new connection
  db_mgmt_ConnectionRef new_connection(grt::Initialized);

  new_connection->owner(_panel.get_be()->get_db_mgmt());
  new_connection->name(buf);
  new_connection->driver(_panel.selected_driver());
  if (find_named_object_in_list(new_connection->driver()->parameters(), "useSSL").is_valid()) {
    // prefer SSL if possible by default
    new_connection->parameterValues().set("useSSL", grt::IntegerRef(1));
  }
  list.insert(new_connection);

  if (copy)
    _panel.get_be()->set_connection_keeping_parameters(new_connection);
  else
    _panel.set_connection(new_connection);
  reset_stored_conn_list();
  _stored_connection_list.select_node(_stored_connection_list.node_at_row((int)list.count() - 1));
  change_active_stored_conn();
}

//------------------------------------------------------------------------------
void grtui::DbConnectionEditor::del_stored_conn() {
  int idx = _stored_connection_list.get_selected_row();
  if (idx >= 0 && idx < (int)_connection_list.count()) {
    grt::ListRef<db_mgmt_Connection> conns(_connection_list);
    db_mgmt_ConnectionRef conn(conns[idx]);
    bool found = false;
    grt::ListRef<db_mgmt_ServerInstance> instances(_mgmt->storedInstances());
    for (grt::ListRef<db_mgmt_ServerInstance>::const_iterator i = instances.begin(); i != instances.end(); ++i) {
      if ((*i)->connection() == conn) {
        found = true;
        break;
      }
    }
    if (found) {
      mforms::Utilities::show_message(_("Cannot Delete Connection"),
                                      _("One or more Database Server Instances use this connection.\n"
                                        "You must remove them before deleting this connection."),
                                      _("OK"));
      return;
    }

    // Remove password associated with this connection (if stored in keychain/vault). Check first
    // this service/username combination isn't used anymore by other connections.
    bool credentials_still_used = false;
    grt::DictRef parameter_values = conn->parameterValues();
    std::string host = conn->hostIdentifier();
    std::string user = parameter_values.get_string("userName");
    for (grt::ListRef<db_mgmt_Connection>::const_iterator i = conns.begin(); i != conns.end(); ++i) {
      if (*i != conn) {
        grt::DictRef current_parameters = (*i)->parameterValues();
        if (host == *(*i)->hostIdentifier() && user == current_parameters.get_string("userName")) {
          credentials_still_used = true;
          break;
        }
      }
    }
    if (!credentials_still_used) {
      try {
        mforms::Utilities::forget_password(host, user);
      } catch (std::exception &exc) {
        logWarning("Exception caught when clearning the password: %s", exc.what());
        mforms::Utilities::show_error("Clear Password", base::strfmt("Could not clear password: %s", exc.what()), "OK");
      }
    }

    if (idx < (int)conns.count()) {
      conns.remove(idx);

      // Set the current connection to the one before the found index.
      if (idx > 0)
        --idx;
      if (idx < (int)conns->count())
        _panel.get_be()->set_connection_and_update(conns[idx]);
    }

    reset_stored_conn_list();
  }
}

//------------------------------------------------------------------------------
void grtui::DbConnectionEditor::change_active_stored_conn() {
  mforms::TreeNodeRef selected = _stored_connection_list.get_selected_node();
  if (selected) {
    _panel.set_enabled(true);
    _panel.suspend_layout();
    _panel.set_active_stored_conn(selected->get_string(0));
    _panel.resume_layout();

    _test_button.set_enabled(true);
    _del_conn_button.set_enabled(true);
    _dup_conn_button.set_enabled(true);
    _move_up_button.set_enabled(true);
    _move_down_button.set_enabled(true);
  } else {
    _panel.set_enabled(false);
    _test_button.set_enabled(false);
    _del_conn_button.set_enabled(false);
    _dup_conn_button.set_enabled(false);
    _move_up_button.set_enabled(false);
    _move_down_button.set_enabled(false);
  }
}

//------------------------------------------------------------------------------
void grtui::DbConnectionEditor::name_changed() {
  std::string name = _conn_name->get_string_value();
  mforms::TreeNodeRef selnode(_stored_connection_list.get_selected_node());

  if (selnode) {
    std::string oname = selnode->get_string(0);

    if (rename_stored_conn(oname, name))
      selnode->set_string(0, name);
  }
}

//------------------------------------------------------------------------------
void grtui::DbConnectionEditor::ok_clicked() {
  // dont need to do anything, the modal handler will interpret the close butto
}

//------------------------------------------------------------------------------
void grtui::DbConnectionEditor::cancel_clicked() {
}

//------------------------------------------------------------------------------
bool grtui::DbConnectionEditor::rename_stored_conn(const std::string &oname, const std::string &name) {
  if (name == oname)
    return true;

  grt::ListRef<db_mgmt_Connection> conns = _connection_list;
  db_mgmt_ConnectionRef conn = find_named_object_in_list(conns, oname);

  if (conn.is_valid()) {
    // check for duplicate
    if (find_named_object_in_list(conns, name).is_valid())
      return false;

    conn->name(name);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------

void grtui::DbConnectionEditor::reorder_conn(bool up) {
  grt::ListRef<db_mgmt_Connection> conns = _connection_list;
  int row = _stored_connection_list.get_selected_row();

  if (row < 0)
    return;

  if (up) {
    if (row > 0) {
      conns.reorder(row, row - 1);
      _stored_connection_list.select_node(_stored_connection_list.node_at_row(row - 1));
    }
  } else {
    if (row < _stored_connection_list.root_node()->count() - 1) {
      conns.reorder(row, row + 1);
      _stored_connection_list.select_node(_stored_connection_list.node_at_row(row + 1));
    }
  }

  row = 0;
  GRTLIST_FOREACH(db_mgmt_Connection, conns, inst) {
    _stored_connection_list.root_node()->get_child(row++)->set_string(0, (*inst)->name().c_str());
  }
}
