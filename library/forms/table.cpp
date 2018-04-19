/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mforms/mforms.h"

using namespace mforms;

Table::Table() {
  _table_impl = &ControlFactory::get_instance()->_table_impl;

  _table_impl->create(this);
}

void Table::set_row_count(int c) {
  _table_impl->set_row_count(this, c);
}

void Table::set_column_count(int c) {
  _table_impl->set_column_count(this, c);
}

void Table::set_row_spacing(int c) {
  _table_impl->set_row_spacing(this, c);
}

void Table::set_column_spacing(int c) {
  _table_impl->set_column_spacing(this, c);
}

void Table::add(View *subview, int row_left, int row_right, int col_top, int col_bottom, int flags) {
  if (row_left > row_right)
    throw std::invalid_argument("table cell left must be <= right");
  if (col_top > col_bottom)
    throw std::invalid_argument("table cell top must be <= bottom");

  cache_view(subview);
  _table_impl->add(this, subview, row_left, row_right, col_top, col_bottom, flags);
  subview->show();
}

void Table::remove(View *sv) {
  _table_impl->remove(this, sv);
  View::remove_from_cache(sv);
#ifdef _MSC_VER // XXX this shouldn't be needed here, the plat specific code is supposed to do this
  relayout();
#endif
}

void Table::set_homogeneous(bool value) {
  _table_impl->set_homogeneous(this, value);
}
