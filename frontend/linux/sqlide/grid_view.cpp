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

#include "sqlide/grid_view.h"
#include <gtkmm/scrolledwindow.h>
#include <boost/foreach.hpp>
#include "base/wb_iterators.h"
#include "mforms/menu.h"
#include "mforms/utilities.h"
#include "base/string_utilities.h"

//------------------------------------------------------------------------------
GridView *GridView::create(bec::GridModel::Ref model, bool fixed_height_mode, bool allow_cell_selection) {
  GridView *view = Gtk::manage(new GridView(model, fixed_height_mode, allow_cell_selection));
  // This function is used only by recordset so if we're forcing fixed height mode, then we need speed optimization.
  view->set_text_cell_fixed_height(fixed_height_mode);

  view->init();
  return view;
}

GridView::GridView(bec::GridModel::Ref model, bool fixed_height_mode, bool allow_cell_selection)
  : Glib::ObjectBase(typeid(GridView)),
    _row_count(0),
    _context_menu(0),
    _allow_cell_selection(allow_cell_selection),
    _selected_cell(false),
    _text_cell_fixed_height(false) {
  if (fixed_height_mode)
    set_fixed_height_mode(true);

  this->model(model);
  signal_cursor_changed().connect_notify(sigc::mem_fun(this, &GridView::on_signal_cursor_changed));

  // In GTK we can't monitor single column resize easily, instead we monitor release button and check if it was header,
  // we store all column size if so.
  signal_button_release_event().connect_notify(sigc::mem_fun(this, &GridView::on_signal_button_release_event));
}

GridView::~GridView() {
}

void GridView::set_text_cell_fixed_height(bool val) {
  _text_cell_fixed_height = val;
}

void GridView::on_signal_cursor_changed() {
  int row = -1, col = -1;
  current_cell(row, col);
  if (col == -2) // It can be -2 if we have _row_numbers_visible.
    col = -1;
  _model->set_edited_field(row, col);
}

void GridView::on_signal_button_release_event(GdkEventButton *ev) {
  Gtk::TreeModel::Path path;
  Gtk::TreeViewColumn *col;
  int x, y;

  if (get_path_at_pos(ev->x, ev->y, path, col, x, y) && get_headers_visible() && _view_model) {
    if (path[0] == 0) // User resized column
      _view_model->onColumnsResized(get_columns());
  }
}

void GridView::set_context_menu(mforms::Menu *menu) {
  _context_menu = menu;
}

void GridView::set_context_menu_responder(const sigc::slot<void> &slot) {
  _context_menu_responder = slot;
}

void GridView::init() {
  //! set_fixed_height_mode(true);
  //  set_grid_lines(Gtk::TREE_VIEW_GRID_LINES_BOTH);

  set_reorderable(false);
  get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);

  // signal_cell_edited().connect(sigc::mem_fun(*this, &GridView::on_cell_edited));

  show();
}

void GridView::model(bec::GridModel::Ref value) {
  _model = value;
  _view_model = GridViewModel::create(_model, this, "grid_view");
  _view_model->set_text_cell_fixed_height(_text_cell_fixed_height);
}

int GridView::refresh(bool reset_columns) {
  freeze_notify();

  Gtk::ScrolledWindow *swin = dynamic_cast<Gtk::ScrolledWindow *>(get_parent());
  float value = -1;
  Gtk::TreePath path;
  Gtk::TreeViewColumn *col = 0;
  if (swin) {
    swin->set_policy(Gtk::POLICY_ALWAYS, Gtk::POLICY_ALWAYS);
    value = swin->get_vadjustment()->get_value();
    get_cursor(path, col);
  }

  if (get_model())
    unset_model();

  _view_model->refresh(reset_columns);
  _row_count = _model->count();
  set_model(_view_model);

  std::vector<Gtk::TreeViewColumn *> cols = get_columns();
  for (size_t i = 0; i < cols.size(); ++i) {
    cols[i]->set_sizing(Gtk::TREE_VIEW_COLUMN_GROW_ONLY);
  }
  reset_sorted_columns();

  if (swin) {
    swin->get_vadjustment()->set_value(value);
    swin->get_vadjustment()->value_changed();
    if (!path.empty()) {
      if (col && !reset_columns)
        set_cursor(path, *col, false);
      else
        set_cursor(path);
    }
  }

  thaw_notify();

  return 0;
}

void GridView::copy() {
  if (_copy_func_ptr)
    _copy_func_ptr(this->get_selected_rows());
}

void GridView::scroll_to(const int whence) // whence == 0 seeks to start, whence == 1 seeks to end
{
  Gtk::ScrolledWindow *swin = dynamic_cast<Gtk::ScrolledWindow *>(get_parent());
  if (swin) {
    if (whence == 0)
      swin->get_vadjustment()->set_value(swin->get_vadjustment()->get_lower());
    else if (whence == 1)
      swin->get_vadjustment()->set_value(swin->get_vadjustment()->get_upper());
  }
}

bec::NodeId GridView::current_cell(int &row, int &col) {
  bec::NodeId node;
  Gtk::TreeModel::Path path;
  Gtk::TreeViewColumn *column;
  get_cursor(path, column);
  if (path) {
    node = _view_model->get_node_for_path(path);
    row = node[0];
    col = (column) ? _view_model->column_index(column) : -1;
  } else {
    row = -1;
    col = -1;
  }
  return node;
}

void GridView::select_cell(int row, Gtk::TreeViewColumn &col) {
  Gtk::TreePath path;
  path.push_back(row);
  set_cursor(path, col, false);
  _selected_cell = true;
  queue_draw();
}

void GridView::select_cell(int row, int col) {
  Gtk::TreePath path;
  path.push_back(row);
  set_cursor(path, *get_column(col + 1), false);
  if (col < 0)
    _selected_cell = false;
  else
    _selected_cell = true;
  queue_draw();
}

void GridView::delete_selected_rows() {
  std::vector<int> rows = get_selected_rows();
  std::sort(rows.begin(), rows.end());
  for (ssize_t i = rows.size() - 1; i >= 0; --i)
    _model->delete_node(bec::NodeId(rows[i]));
  sync_row_count();
}

int GridView::current_row() {
  int row, col;
  current_cell(row, col);
  return row;
}

bool GridView::on_key_press_event(GdkEventKey *event) {
  bool processed = false;

  if (GDK_KEY_PRESS == event->type) {
    switch (event->keyval) {
      case GDK_KEY_Menu: {
        if (_context_menu)
          _context_menu->popup();
        else if (!_context_menu_responder.empty())
          _context_menu_responder();
        processed = true;
        break;
      }
      case GDK_KEY_Up:
      case GDK_KEY_Down:
      case GDK_KEY_Left:
      case GDK_KEY_Right:
        if (_selected_cell) {
          Gtk::TreePath path;
          Gtk::TreeViewColumn *column = NULL;
          int i;
          get_cursor(path, column);
          if (column) {
            switch (event->keyval) {
              case GDK_KEY_Up:
                path.prev();
                break;
              case GDK_KEY_Down:
                path.next();
                break;
              case GDK_KEY_Left:
                i = 0;
                for (Gtk::TreeViewColumn *c = get_column(i); c != NULL; c = get_column(++i)) {
                  if (c == column) {
                    if (i > 0)
                      column = get_column(i - 1);
                    break;
                  }
                }
                break;
              case GDK_KEY_Right:
                i = 0;
                for (Gtk::TreeViewColumn *c = get_column(i); c != NULL; c = get_column(++i)) {
                  if (c == column) {
                    column = get_column(i + 1);
                    if (!column)
                      column = c;
                    break;
                  }
                }
                break;
            }
            scroll_to_cell(path, *column);
            set_cursor(path, *column, false);
            if (_allow_cell_selection)
              get_selection()->unselect_all();
            queue_draw();
            processed = true;
          }
        }
        break;
      case GDK_KEY_Delete:
      case GDK_KEY_KP_Delete: {
        if (0 == event->state) {
          if (!_model->is_readonly()) {
            delete_selected_rows();
            processed = true;
          }
        }
        break;
      }
      case GDK_KEY_Tab:
      case GDK_KEY_ISO_Left_Tab: {
        Gtk::TreeViewColumn *col = _column_edited;
        if (col) {
          Gtk::TreePath path = _path_edited;

          const std::vector<Gtk::TreeViewColumn *> cols = get_columns();
          const int size = cols.size();

          for (int i = 0; i < size; ++i) {
            if (cols[i] == col) {
              if ((event->state & GDK_SHIFT_MASK) && event->keyval == GDK_KEY_ISO_Left_Tab) {
                if (--i == 0) {
                  path.prev();
                  i = size - 1;
                }
              } else {
                if (++i == size) {
                  path.next();
                  i = 1;
                }
              }

              if (i < size && i >= 0) {
                col = cols[i];
                _cell_editable->editing_done();

                set_cursor(path, *col, true);
                break;
              }
            }
          }
        }
        processed = true;
        break;
      }
    }
  }

  if (!processed)
    processed = Gtk::TreeView::on_key_press_event(event);

  return processed;
}

bool GridView::on_button_press_event(GdkEventButton *event) {
  if (event->button == 1) {
    Gtk::TreePath path, opath;
    Gtk::TreeViewColumn *column, *ocolumn;
    int cellx, celly;
    if (_allow_cell_selection && get_path_at_pos(event->x, event->y, path, column, cellx, celly) && event->state == 0 &&
        column != get_column(0)) {
      get_cursor(opath, ocolumn);
      grab_focus();
      set_cursor(path, *column, opath && path && opath == path && column == ocolumn);
      get_selection()->unselect_all();
      _selected_cell = true;
      queue_draw();
      return true;
    } else {
      _selected_cell = false;
      queue_draw();
    }
  } else if (event->button == 3) {
    if (_context_menu)
      _context_menu->popup();
    else if (!_context_menu_responder.empty())
      _context_menu_responder();
    return true;
  }
  return Gtk::TreeView::on_button_press_event(event);
}

static void add_node_for_path(const Gtk::TreeModel::Path &path, std::vector<int> *rows) {
  rows->push_back((int)path[0]);
}

std::vector<int> GridView::get_selected_rows() {
  std::vector<int> rows;
  get_selection()->selected_foreach_path(sigc::bind(sigc::ptr_fun(add_node_for_path), &rows));
  return rows;
}

void GridView::on_cell_edited(const Glib::ustring &path_string, const Glib::ustring &new_text) {
  _signal_cell_edited.emit(path_string, new_text);
  sync_row_count();
}

bool GridView::on_focus_out(GdkEventFocus *event, Gtk::CellRenderer *cell, Gtk::Entry *e) {
  // Emulate pressing Enter on the text entry so that a focus out will save ongoing changes
  // instead of discarding them
  if (!event->in) {
    if (_cell_editable != 0)
      _cell_editable->editing_done();

    this->on_cell_edited("", e->get_text());
  }
  return false;
}

void GridView::on_cell_editing_started(Gtk::CellEditable *e, const Glib::ustring &path, Gtk::TreeViewColumn *column) {
  _path_edited = Gtk::TreePath(path);
  _column_edited = column;
  _cell_editable = e;
  Gtk::Widget *w = dynamic_cast<Gtk::Widget *>(e);
  if (w) {

    Gtk::Entry *entry = dynamic_cast<Gtk::Entry *>(w);
    if (entry) {
      Glib::RefPtr<Gtk::EntryBuffer> ebuff = entry->get_buffer();
      ebuff->signal_inserted_text().connect(sigc::mem_fun(this, &GridView::on_text_insert));
    }

    w->signal_hide().connect(sigc::mem_fun(this, &GridView::on_cell_editing_done));
    w->signal_focus_out_event().connect(sigc::bind(sigc::mem_fun(this, &GridView::on_focus_out),
                                                   *column->get_cells().begin(), dynamic_cast<Gtk::Entry *>(e)),
                                        false);
  }
}

void GridView::on_text_insert(unsigned int position, const char *incoming_text, unsigned int character_num) {
  if ((unsigned int)g_utf8_strlen(incoming_text, -1) != character_num)
    mforms::Utilities::show_warning(_("Text Truncation"),
                                    _("Inserted data has been truncated as the control's limit was reached. Please use "
                                      "the value editor instead for editing such large text data."),
                                    "Ok", "", "");
}

void GridView::on_cell_editing_done() {
  _column_edited = 0;
  _cell_editable = 0;
}

void GridView::sync_row_count() {
  if (_model->count() != _row_count) {
    refresh(false);
    _signal_row_count_changed.emit();
  }
}

void GridView::on_column_header_clicked(Gtk::TreeViewColumn *column, int column_index) {
  if (column_index >= 0) {
    int sort_direction = 1;

    if (column->get_sort_indicator())
      sort_direction = (column->get_sort_order() == Gtk::SORT_ASCENDING) ? -1 : 0;

    if (!(sort_direction))
      column->set_sort_indicator(false);

    _model->sort_by(column_index, sort_direction, true);
  } else {
    _model->sort_by(0, 0, false);
    reset_sorted_columns();
  }
}

void GridView::sort_by_column(int column_index, int sort_direction, bool retaining) {
  _model->sort_by(column_index, sort_direction, retaining);
  reset_sorted_columns();
}

void GridView::reset_sorted_columns() {
  bec::GridModel::SortColumns sort_columns = _model->sort_columns();
  BOOST_FOREACH (bec::GridModel::SortColumns::value_type &sort_column, sort_columns) {
    Gtk::TreeViewColumn *column = get_column(
      sort_column.first + 1); // as opposed to documentation TtreeView::get_column accepts 1-based column index
    Gtk::SortType sort_type = (sort_column.second != 1) ? Gtk::SORT_DESCENDING : Gtk::SORT_ASCENDING;
    column->set_sort_order(sort_type);
    column->set_sort_indicator(true);
  }
}

int GridView::row_count() const {
  return _row_count;
}
