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

#ifndef __RECORDSET_VIEW_H__
#define __RECORDSET_VIEW_H__

#include "sqlide/grid_view.h"
#include "sqlide/recordset_be.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#include <gtkmm.h>
#pragma GCC diagnostic pop
#include "mforms/tabview.h"

class RecordsetView : public Gtk::ScrolledWindow {
public:
  static RecordsetView *create(Recordset::Ref model);
  ~RecordsetView();

protected:
  RecordsetView(Recordset::Ref model);

private:
  virtual void init();

public:
  void model(Recordset::Ref value);
  Recordset::Ref model() {
    return _model;
  }

protected:
  Recordset::Ref _model;
  GridView *_grid;

protected:
  Gtk::Button *_close_btn;
  int _single_row_height;
  boost::signals2::connection _refresh_ui_sig;
  boost::signals2::connection _refresh_ui_stat_sig;

public:
  GridView *grid_view() {
    return _grid;
  }

  virtual void refresh();
  virtual void reset();

  bool has_changes();
  void copy(const std::vector<int> &rows);

protected:
  virtual bool on_event(GdkEvent *event);

  void selected_record_changed();

  void on_commit_btn_clicked();
  void on_rollback_btn_clicked();

  void on_goto_first_row_btn_clicked();
  void on_goto_last_row_btn_clicked();
  void on_record_prev();
  void on_record_next();
  void on_record_edit();
  void on_record_add();
  void on_record_del();
  void on_record_sort_asc();
  void on_record_sort_desc();
  void on_toggle_vertical_sizing();

  void set_fixed_row_height(int);

  bool activate_toolbar_item(const std::string &action);
};

#endif // __RECORDSET_VIEW_H__
