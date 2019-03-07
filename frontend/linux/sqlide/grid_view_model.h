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

#ifndef __GRID_VIEW_MODEL_H__
#define __GRID_VIEW_MODEL_H__

#include "listmodel_wrapper.h"
#include "grt/tree_model.h"

class GridView;

class GridViewModel : public ListModelWrapper {
public:
  typedef Glib::RefPtr<GridViewModel> Ref;
  static Ref create(bec::GridModel::Ref model, GridView *view, const std::string &name);
  ~GridViewModel();

  virtual bool handle_popup_event(GdkEvent *event);
  int refresh(bool reset_columns);
  int column_index(Gtk::TreeViewColumn *col);
  void row_numbers_visible(bool value) {
    _row_numbers_visible = value;
  }
  bool row_numbers_visible() {
    return _row_numbers_visible;
  }
  void set_ellipsize(const int column, const bool on);

  void set_column_width(int column, int width);
  void set_text_cell_fixed_height(bool val);

  void ignore_column_resizes(bool flag) {
    if (flag)
      _ignore_column_resizes++;
    else
      _ignore_column_resizes--;
  }

  sigc::slot<void, const int, Glib::ValueBase *> before_render;

  sigc::slot<void, int> column_resized;
  sigc::slot<void, const std::vector<int> > columns_resized;
  sigc::slot<void, int, int, int> column_right_clicked;
  //  void on_column_resized(Gtk::TreeViewColumn *c);
  void onColumnsResized(const std::vector<Gtk::TreeViewColumn *> &cols);

protected:
  GridViewModel(bec::GridModel::Ref model, GridView *view, const std::string &name);
  virtual void get_value_vfunc(const iterator &iter, int column, Glib::ValueBase &value) const;

private:
  bec::GridModel::Ref _model;
  GridView *_view;
  std::map<Gtk::TreeViewColumn *, int> _col_index_map;
  std::map<int, int> _current_column_size;
  int _ignore_column_resizes;
  bool _row_numbers_visible;
  bool _text_cell_fixed_height;

  template <typename ValueTypeTraits>
  Gtk::TreeViewColumn *add_column(int index, const std::string &name, Editable editable,
                                  Gtk::TreeModelColumnBase *color_column);

  void get_cell_value(const iterator &iter, int column, GType type, Glib::ValueBase &value);
  void set_cell_value(const iterator &itier, int column, GType type, const Glib::ValueBase &value);

  void on_column_header_button_press(GdkEventButton *ev, Gtk::TreeViewColumn *col);
};

#endif // __GRID_VIEW_MODEL_H__
