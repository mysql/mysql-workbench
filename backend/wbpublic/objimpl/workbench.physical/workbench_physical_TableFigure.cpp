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

#include <grts/structs.workbench.physical.h>

#include <grtpp_util.h>

#include "wbcanvas/workbench_physical_tablefigure_impl.h"

//================================================================================
// workbench_physical_TableFigure

void workbench_physical_TableFigure::init() {
  if (!_data)
    _data = new workbench_physical_TableFigure::ImplData(this);
  model_Figure::set_data(_data);
}

void workbench_physical_TableFigure::set_data(ImplData *data) {
  throw std::logic_error("unexpected");
}

workbench_physical_TableFigure::~workbench_physical_TableFigure() {
  delete _data;
}

void workbench_physical_TableFigure::table(const db_TableRef &value) {
  if (_table == value)
    return;

  if (_is_global && _table.is_valid())
    _table.unmark_global();
  if (_is_global && value.is_valid())
    value.mark_global();

  grt::ValueRef ovalue(_table);
  get_data()->set_table(value);
  member_changed("table", ovalue, value);
}
