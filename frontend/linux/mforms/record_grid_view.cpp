/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "record_grid_view.h"
#include "../sqlide/recordset_view.h"
#include "sqlide/recordset_be.h"
#include "mforms/menubar.h"
#include "gtk/lf_native.h"

using namespace mforms;

static mforms::GridView *create_record_grid(std::shared_ptr<Recordset> rset) {
  return new RecordGridView(rset);
}

void lf_record_grid_init() {
  mforms::GridView::register_factory(create_record_grid);
}

static void destroy_nativecontainer(void *ptr) {
  gtk::NativeContainerImpl *container = (gtk::NativeContainerImpl *)ptr;
  if (container)
    delete container;
}

RecordGridView::RecordGridView(Recordset::Ref rset) {
  viewer = RecordsetView::create(rset);
  viewer->grid_view()->view_model()->columns_resized =
    std::bind(&RecordGridView::columns_resized, this, std::placeholders::_1);
  viewer->grid_view()->view_model()->column_right_clicked = std::bind(
    &RecordGridView::column_right_clicked, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
  viewer->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  set_data(new gtk::NativeContainerImpl(this, viewer), destroy_nativecontainer);
  viewer->show_all();
  viewer->grid_view()->refresh(true);
}

RecordGridView::~RecordGridView() {
  delete viewer;
}

int RecordGridView::get_column_count() {
  return viewer->model()->get_column_count();
}

int RecordGridView::get_column_width(int column) {
  Gtk::TreeViewColumn *tc = viewer->grid_view()->get_column(column + 1);
  if (tc)
    return tc->get_width();
  return 0;
}

void RecordGridView::set_column_width(int column, int width) {
  viewer->grid_view()->view_model()->set_column_width(column, width);
}

void RecordGridView::update_columns() {
  viewer->grid_view()->refresh(true);
}

bool RecordGridView::current_cell(size_t &row, int &column) {
  int r, c;
  if (viewer->grid_view()->current_cell(r, c).is_valid())
    return false;
  row = r;
  column = c;
  return true;
}

void RecordGridView::set_current_cell(size_t row, int column) {
  viewer->grid_view()->select_cell(row, column);
}

void RecordGridView::set_column_header_indicator(int column_index, ColumnHeaderIndicator order) {
  Gtk::TreeViewColumn *column = viewer->grid_view()->get_column(column_index + 1);
  switch (order) {
    case NoIndicator:
      column->set_sort_indicator(false);
      break;
    case SortDescIndicator:
      column->set_sort_order(Gtk::SORT_DESCENDING);
      column->set_sort_indicator(true);
      break;
    case SortAscIndicator:
      column->set_sort_order(Gtk::SORT_ASCENDING);
      column->set_sort_indicator(true);
      break;
  }
}

void RecordGridView::set_font(const std::string &font) {
  viewer->grid_view()->override_font(Pango::FontDescription(font));
}

void RecordGridView::column_right_clicked(int c, int x, int y) {
  clicked_header_column(c);
  if (header_menu())
    header_menu()->popup_at(this, base::Point(x, y));
}
