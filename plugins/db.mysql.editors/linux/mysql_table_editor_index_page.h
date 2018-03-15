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

#ifndef __WB_MYSQL_TABLE_EDITOR_INDEX_PAGE_H__
#define __WB_MYSQL_TABLE_EDITOR_INDEX_PAGE_H__

#include "grt/tree_model.h"
#include <gtkmm/builder.h>
#include <gtkmm/checkbutton.h>

class MySQLTableEditorBE;
class ListModelWrapper;
class DbMySQLTableEditor;

//==============================================================================
//
//==============================================================================
class DbMySQLTableEditorIndexPage {
public:
  DbMySQLTableEditorIndexPage(DbMySQLTableEditor* owner, MySQLTableEditorBE* be, Glib::RefPtr<Gtk::Builder> xml);

  void refresh();
  bool real_refresh();

  void switch_be(MySQLTableEditorBE* be);

  ~DbMySQLTableEditorIndexPage();

private:
  void cell_editing_started(Gtk::CellEditable* cell, const Glib::ustring& path);
  void cell_editing_done(GtkCellEditable* ce);
  static void cell_editing_done_proxy(GtkCellEditable* ce, gpointer data);

  //! index_cursor_changed is called when current item in the index treeview is changed
  void index_cursor_changed();
  //! update_index_details fetches index details for selected index. selected index is stored in _index_node
  void update_index_details();
  //! get_value is a source of data for treeview columns: set/unset column for index, order of index
  //! get_value is connected to a model wrapper in update_index_details
  void get_value(const Gtk::TreeModel::iterator& iter, int column, GType type, Glib::ValueBase& value);
  //! set_value is a model wrapper setter of data for treeview columns: set/unset column for index, order of index
  //! set_value is connected to a model wrapper in update_index_details
  void set_value(const Gtk::TreeModel::iterator& iter, int column, GType type, const Glib::ValueBase& value);

  void update_index_storage_type_in_be();

  void set_index_key_block_size(const std::string& value);
  void set_index_parser(const std::string& value);
  void set_index_comment(const std::string& value);

  void update_gui_for_server();

  Glib::RefPtr<Gtk::ListStore> recreate_order_model();

  DbMySQLTableEditor* _owner;
  MySQLTableEditorBE* _be;
  Glib::RefPtr<Gtk::Builder> _xml;

  Gtk::TreeView* _indexes_tv;
  Glib::RefPtr<ListModelWrapper> _indexes_model;
  Glib::RefPtr<ListModelWrapper> _indexes_columns_model;
  Glib::RefPtr<Gtk::ListStore> _sort_order_model;
  Glib::RefPtr<Gtk::ListStore> _order_model; //!< for index columns
  Gtk::ComboBox* _index_storage_combo;
  sigc::connection _index_storage_combo_conn;

  ::bec::NodeId _index_node;
  sigc::connection _editing_sig;
  sigc::connection _refresh_sig;
  gulong _editing_done_id;
  GtkCellEditable* _editable_cell;
  std::string _user_index_name;
  Gtk::CheckButton *_indexVisibility;
  sigc::connection _visibilitySignal;
};

#endif
