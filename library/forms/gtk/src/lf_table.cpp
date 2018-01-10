/*
 * Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "../lf_table.h"
#include "base/string_utilities.h"

Gtk::Widget *mforms::gtk::TableImpl::get_outer() const {
  return _outerBox;
}
//------------------------------------------------------------------------------
Gtk::Widget *mforms::gtk::TableImpl::get_inner() const {
  return _grid;
}
//------------------------------------------------------------------------------
mforms::gtk::TableImpl::TableImpl(::mforms::Table *self) : ViewImpl(self) {
  _outerBox = new Gtk::Box();
  _grid = new Gtk::Grid();
  _outerBox->pack_start(*_grid, true, true);
  _outerBox->show_all();
  _grid->set_halign(Gtk::ALIGN_FILL);
  _grid->set_valign(Gtk::ALIGN_FILL);

  _rowCount = 0;
  _colCount = 0;
  setup();
}

mforms::gtk::TableImpl::~TableImpl() {
  delete _grid;
  delete _outerBox;
}
//------------------------------------------------------------------------------
bool mforms::gtk::TableImpl::create(::mforms::Table *self) {
  return new TableImpl(self);
}
//------------------------------------------------------------------------------
void mforms::gtk::TableImpl::set_row_count(Table *self, int count) {
  // Gtk::set_row_count is deprecated.
  TableImpl *table = self->get_data<TableImpl>();
  if (table)
    table->_rowCount = count;
}
//------------------------------------------------------------------------------
void mforms::gtk::TableImpl::set_col_count(Table *self, int count) {
  // Gtk::set_col_count is deprecated.
  TableImpl *table = self->get_data<TableImpl>();
  if (table)
    table->_colCount = count;
}
//------------------------------------------------------------------------------
void mforms::gtk::TableImpl::add(Table *self, View *child, int left, int right, int top, int bottom, int flags) {
  TableImpl *table = self->get_data<TableImpl>();
  if (table) {
    if ((int)table->_colCount < right || (int)table->_rowCount < bottom)
      throw std::logic_error(base::strfmt("Tried to use col %d, row %d, but the table has got only %d cols, %d rows.",
                                          right, bottom, (int)table->_colCount, (int)table->_rowCount));
    Gtk::Widget *widget = child->get_data<ViewImpl>()->get_outer();
    if (widget) {
      int height = bottom - top;
      int width = right - left;
      table->_grid->attach(*widget, left, top, width, height);

      if (flags & mforms::VExpandFlag)
        widget->set_vexpand();
      if (flags & mforms::VFillFlag)
        widget->set_valign(Gtk::ALIGN_FILL);
      if (flags & mforms::HExpandFlag)
        widget->set_hexpand();
      if (flags & mforms::HFillFlag)
        widget->set_halign(Gtk::ALIGN_FILL);

      widget->show();
    }
  }
}
//------------------------------------------------------------------------------
void mforms::gtk::TableImpl::remove(Table *self, View *child) {
  TableImpl *table = self->get_data<TableImpl>();

  table->_grid->remove(*child->get_data<ViewImpl>()->get_outer());
}
//------------------------------------------------------------------------------
void mforms::gtk::TableImpl::set_row_spacing(Table *self, int space) {
  TableImpl *table = self->get_data<TableImpl>();

  table->_grid->set_row_spacing(space);
}
//------------------------------------------------------------------------------
void mforms::gtk::TableImpl::set_col_spacing(Table *self, int space) {
  TableImpl *table = self->get_data<TableImpl>();

  table->_grid->set_column_spacing(space);
}
//------------------------------------------------------------------------------
void mforms::gtk::TableImpl::set_homogeneous(Table *self, bool flag) {
  TableImpl *table = self->get_data<TableImpl>();

  table->_grid->set_column_homogeneous(flag);
  table->_grid->set_row_homogeneous(flag);
}
//------------------------------------------------------------------------------
void mforms::gtk::TableImpl::set_padding_impl(int left, int top, int right, int bottom) {
  if (left < 0 && top < 0 && right < 0 && bottom < 0) {
    _grid->set_valign(Gtk::ALIGN_CENTER);
    _grid->set_halign(Gtk::ALIGN_CENTER);
    _grid->set_border_width(0);
  } else {
    _grid->set_valign(Gtk::ALIGN_FILL);
    _grid->set_halign(Gtk::ALIGN_FILL);
    _grid->set_border_width(left);
  }
}
//------------------------------------------------------------------------------
void mforms::gtk::TableImpl::init() {
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_table_impl.create = &TableImpl::create;
  f->_table_impl.set_row_count = &TableImpl::set_row_count;
  f->_table_impl.set_column_count = &TableImpl::set_col_count;
  f->_table_impl.add = &TableImpl::add;
  f->_table_impl.remove = &TableImpl::remove;
  f->_table_impl.set_row_spacing = &TableImpl::set_row_spacing;
  f->_table_impl.set_column_spacing = &TableImpl::set_col_spacing;
  f->_table_impl.set_homogeneous = &TableImpl::set_homogeneous;
}
//------------------------------------------------------------------------------
