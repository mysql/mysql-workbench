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

#ifndef __WB_MYSQL_TABLE_EDITOR_FK_PAGE_H__
#define __WB_MYSQL_TABLE_EDITOR_FK_PAGE_H__

#include "grt/tree_model.h"
#include <gtkmm/builder.h>
#include <gtkmm/checkbutton.h>

class MySQLTableEditorBE;
class ListModelWrapper;
class DbMySQLTableEditor;

//==============================================================================
//
//==============================================================================
class DbMySQLTableEditorFKPage {
public:
  DbMySQLTableEditorFKPage(DbMySQLTableEditor *owner, MySQLTableEditorBE *be, Glib::RefPtr<Gtk::Builder> xml);

  void refresh();

  void switch_be(MySQLTableEditorBE *be);

private:
  static void cell_editing_started(GtkCellRenderer *cr, GtkCellEditable *ce, gchar *path, gpointer udata);
  void fkcol_cell_editing_started(Gtk::CellEditable *cell, const Glib::ustring &path);
  static void cell_editing_done(GtkCellEditable *ce, gpointer udata);
  void fk_cursor_changed();
  void model_only_toggled();

  void update_fk_details();

  void combo_box_changed(
    const int model_column); // column is either FKConstraintListBE::OnUpdate or FKConstraintListBE::OnDelete
  void set_comment(const std::string &comment);

  DbMySQLTableEditor *_owner;
  MySQLTableEditorBE *_be;
  Glib::RefPtr<Gtk::Builder> _xml;

  Gtk::ComboBox *_fk_update_combo;
  Gtk::ComboBox *_fk_delete_combo;

  Gtk::TreeView *_fk_tv;
  Gtk::TreeView *_fk_columns_tv;
  Glib::RefPtr<ListModelWrapper> _fk_model;
  Glib::RefPtr<ListModelWrapper> _fk_columns_model;
  Glib::RefPtr<Gtk::ListStore> _fk_tables_model;
  Gtk::CheckButton *_fk_model_only;

  ::bec::NodeId _fk_node; //!< selected fk node
  gulong _edit_conn;
  GtkCellEditable *_ce;
  // sigc::connection                          _cell_edit_conn;
  sigc::connection _fkcol_cell_edit_conn;

  Gtk::Widget *_fk_page_content;
  Gtk::Label *_fk_page_not_supported_label;
  void check_fk_support();
};

#endif
