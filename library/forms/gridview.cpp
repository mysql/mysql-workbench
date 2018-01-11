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

#include "mforms/mforms.h"

using namespace mforms;

static GridView* (*record_grid_factory)(std::shared_ptr<Recordset> rset) = NULL;

GridView* GridView::create(std::shared_ptr<Recordset> rset) {
  return record_grid_factory(rset);
}

GridView::GridView() : _header_menu(NULL), _clicked_header_column(0) {
}

void GridView::register_factory(GridView* (*create)(std::shared_ptr<Recordset> rset)) {
  record_grid_factory = create;
}

void GridView::set_header_menu(ContextMenu* menu) {
  _header_menu = menu;
}

void GridView::clicked_header_column(int column) {
  _clicked_header_column = column;
}
