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


#include "mysql_table_editor_fe.h"
#include "grtdb/db_object_helpers.h"
#include "mysql_table_editor_trigger_page.h"
#include "mforms/../gtk/lf_view.h"

//------------------------------------------------------------------------------
DbMySQLTableEditorTriggerPage::DbMySQLTableEditorTriggerPage(DbMySQLTableEditor* owner, MySQLTableEditorBE* be,
                                                             Glib::RefPtr<Gtk::Builder> xml)
  : _be(be), _xml(xml) {
  switch_be(be);
  // Gtk::Paned *paned(0);
  //_xml->get("trigger_paned", &paned);
}

//------------------------------------------------------------------------------
DbMySQLTableEditorTriggerPage::~DbMySQLTableEditorTriggerPage() {
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorTriggerPage::switch_be(MySQLTableEditorBE* be) {
  Gtk::Box* trigger_code_win;
  _xml->get_widget("trigger_code_holder", trigger_code_win);

  //  trigger_code_win->remove_all();

  _be = be;
  trigger_code_win->pack_start(*mforms::widget_for_view(be->get_trigger_panel()), true, true);
  trigger_code_win->show_all();
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorTriggerPage::refresh() {
  if (_be)
    _be->load_trigger_sql();
}
