/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __DB_SQL_EDITOR_OUTPUT_VIEW_H__
#define __DB_SQL_EDITOR_OUTPUT_VIEW_H__

#include "mforms/toolbar.h"
#include "sqlide/wb_sql_editor_form.h"
#include "grid_view.h"
#include <glib.h>

#include <gtkmm/box.h>
#include <gtkmm/notebook.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/paned.h>
#include <gtkmm/textview.h>

class DbSqlEditorView;

//==============================================================================
//
//==============================================================================
class QueryOutputView {
public:
  QueryOutputView(const SqlEditorForm::Ref& be, DbSqlEditorView* db_sql_editor_view);
  ~QueryOutputView();

  Gtk::Widget& get_outer() {
    return _top_box;
  }
  void refresh();
  void output_text(const std::string& text, const bool bring_to_front);

private:
  void mode_change_requested();
  int on_history_entries_refresh();
  int on_history_details_refresh();
  void on_history_entries_selection_changed();
  bool on_query_tooltip(int x, int y, bool keyboard_tooltip, const Glib::RefPtr<Gtk::Tooltip>& tooltip);

  void output_menu_will_show();
  void handle_history_context_menu(const std::string& action);
  void history_context_menu_responder();

  SqlEditorForm::Ref _be;
  Gtk::Box _top_box;
  Gtk::Notebook _note;
  Gtk::ComboBoxText _mode;
  Gtk::Menu _context_menu;

  // Text output part
  Gtk::ScrolledWindow _text_swnd;
  Gtk::TextView _text_output;

  // Action output
  GridView _action_output;
  Gtk::ScrolledWindow _action_swnd;

  // History output
  Gtk::Paned _history_box;
  Gtk::ScrolledWindow _entries_swnd;
  GridView _entries_grid;
  Gtk::ScrolledWindow _details_swnd;
  GridView _details_grid;

  sigc::connection _on_history_entries_selection_changed_conn;
  DbSqlEditorView* _db_sql_editor_view;

  boost::signals2::connection _refresh_ui_sig_entries;
  boost::signals2::connection _refresh_ui_sig_details;
  boost::signals2::connection _refresh_ui_sig_log;
};

#endif // __DB_SQL_EDITOR_OUTPUT_VIEW_H__
