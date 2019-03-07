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

#ifndef __GRID_VIEW_H__
#define __GRID_VIEW_H__

#include "grid_view_model.h"
namespace mforms {
  class Menu;
};

class GridView : public Gtk::TreeView {
public:
  static GridView *create(bec::GridModel::Ref model, bool fixed_row_height = true, bool allow_cell_selection = true);
  GridView(bec::GridModel::Ref model, bool fixed_row_height = true, bool allow_cell_selection = true);
  ~GridView();

  void set_text_cell_fixed_height(bool val);

  bool allow_cell_selection() {
    return _allow_cell_selection;
  }

  void set_context_menu(mforms::Menu *menu);
  void set_context_menu_responder(const sigc::slot<void> &slot);

  std::vector<int> get_selected_rows();

  void model(bec::GridModel::Ref value);

  int refresh(bool reset_columns);

  void scroll_to(const int whence); // whence == 0 seeks to start, whence == 1 seeks to end

  bool selection_is_cell() {
    return _selected_cell;
  }
  bec::NodeId current_cell(int &row, int &col);
  int current_row();
  void select_cell(int row, int col);
  void select_cell(int row, Gtk::TreeViewColumn &col);

  void on_column_header_clicked(Gtk::TreeViewColumn *column, int column_index);
  void sort_by_column(int column_index, int sort_direction, bool retaining);

  int row_count() const;
  void row_numbers_visible(bool value) {
    _view_model->row_numbers_visible(value);
  }

  sigc::signal<void, const Glib::ustring &, const Glib::ustring &> signal_cell_edited() {
    return _signal_cell_edited;
  }
  // sigc::slot<void, const Glib::ustring&, const Glib::ustring&> slot_cell_edited() { return
  // _signal_cell_edited.make_slot(); }
  sigc::signal<void> signal_row_count_changed() {
    return _signal_row_count_changed;
  }
  sigc::signal<void, int, int, bool> signal_sort_by_column;

  void on_cell_edited(const Glib::ustring &path_string, const Glib::ustring &new_text);
  void on_cell_editing_started(Gtk::CellEditable *e, const Glib::ustring &path, Gtk::TreeViewColumn *column);
  void on_text_insert(unsigned int position, const char *incoming_text, unsigned int character_num);
  void on_cell_editing_done();

  void set_ellipsize(const int column, const bool on) {
    _view_model->set_ellipsize(column, on);
  }
  GridViewModel::Ref view_model() {
    return _view_model;
  }

  void sync_row_count();

  std::function<void(std::vector<int>)> _copy_func_ptr;
  void copy();

protected:
  virtual bool on_key_press_event(GdkEventKey *event);
  virtual bool on_button_press_event(GdkEventButton *event);
  bool on_focus_out(GdkEventFocus *event, Gtk::CellRenderer *cell, Gtk::Entry *e);
  void on_signal_cursor_changed();
  void on_signal_button_release_event(GdkEventButton *ev);
  void reset_sorted_columns();

private:
  virtual void init();

  sigc::signal<void, const Glib::ustring &, const Glib::ustring &> _signal_cell_edited;
  sigc::signal<void> _signal_row_count_changed;

  void activate_popup_menu_item(const std::string &action, const std::vector<int> &rows, int clicked_column);
  void delete_selected_rows();

  bec::GridModel::Ref _model;
  GridViewModel::Ref _view_model;
  size_t _row_count;

  Gtk::TreePath _path_edited;
  Gtk::TreeViewColumn *_column_edited;
  Gtk::CellEditable *_cell_editable;

  mforms::Menu *_context_menu;
  sigc::slot<void> _context_menu_responder;

  bool _allow_cell_selection;
  bool _selected_cell;
  bool _text_cell_fixed_height;
};

#endif // __GRID_VIEW_H__
