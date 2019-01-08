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

#ifndef __WB_COLUMN_PAGE_HANDLING_H__
#define __WB_COLUMN_PAGE_HANDLING_H__

#include "grt/tree_model.h"
#include <gtkmm/builder.h>
#include <gtkmm/radiobutton.h>

class MySQLTableEditorBE;
class ListModelWrapper;
class DbMySQLTableEditor;
class AutoCompletable;

namespace Gtk {
  class ScrolledWindow;
  class ComboBox;
}

//==============================================================================
//
//==============================================================================
class DbMySQLTableEditorColumnPage : public sigc::trackable {
public:
  DbMySQLTableEditorColumnPage(DbMySQLTableEditor* owner, MySQLTableEditorBE* be, Glib::RefPtr<Gtk::Builder> xml);
  ~DbMySQLTableEditorColumnPage();

  void refresh();
  void partial_refresh(const int what);

  void switch_be(MySQLTableEditorBE* be);

private:
  grt::StringListRef get_types_for_table(const db_TableRef table); //!< T

  bool process_event(GdkEvent* event);
  void type_column_event(GdkEvent* e);
  void cursor_changed();

  void update_column_details(const ::bec::NodeId& node);

  void set_comment(const std::string& comment);
  void set_charset();
  void set_collation();
  void update_collation();
  void update_gc_storage_type();
  void set_gc_storage_type();

  void check_resize(Gtk::Allocation& r);
  bool do_on_visible(GdkEventVisibility*);

  bec::NodeId get_selected();

  void start_auto_edit();

private:
  void refill_completions();
  void refill_columns_tv();
  sigc::connection sigQueryTooltip;

  DbMySQLTableEditor* _owner;
  MySQLTableEditorBE* _be;
  Glib::RefPtr<Gtk::Builder> _xml;

  Glib::RefPtr<ListModelWrapper> _model;
  Gtk::TreeView* _tv;
  Gtk::ScrolledWindow* _tv_holder;

  Gtk::ComboBox* _charset_combo;
  Gtk::ComboBox* _collation_combo;

  Gtk::RadioButton* _radioStored;
  Gtk::RadioButton* _radioVirtual;

  gulong _edit_conn;
  GtkCellEditable* _ce;
  int _old_column_count;
  bool _auto_edit_pending;

  // Auto completion of types and related functions
  static std::shared_ptr<AutoCompletable> _types_completion;
  static std::shared_ptr<AutoCompletable> _names_completion;
  static std::shared_ptr<AutoCompletable> types_completion();
  static std::shared_ptr<AutoCompletable> names_completion();
  static void type_cell_editing_started(GtkCellRenderer* cr, GtkCellEditable* ce, gchar* path, gpointer udata);
  static void cell_editing_done(GtkCellEditable* ce, gpointer udata);
  bool _editing;
};

#endif
