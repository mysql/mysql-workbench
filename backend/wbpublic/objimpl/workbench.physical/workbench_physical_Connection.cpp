/*
* Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; version 2 of the
* License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
* 02110-1301  USA
*/

#include <grts/structs.workbench.physical.h>

#include <grtpp_util.h>

#include "wbcanvas/workbench_physical_connection_impl.h"

//================================================================================
// workbench_physical_Connection

void workbench_physical_Connection::init() {
  if (!_data)
    _data = new workbench_physical_Connection::ImplData(this);
  model_Connection::set_data(_data);
}

void workbench_physical_Connection::set_data(ImplData *data) {
  throw std::logic_error("unexpected");
}

workbench_physical_Connection::~workbench_physical_Connection() {
  delete _data;
}

void workbench_physical_Connection::foreignKey(const db_ForeignKeyRef &value) {
  if (_foreignKey == value)
    return;
  if (_foreignKey.is_valid() && value.is_valid())
    throw std::runtime_error("Cannot change foreignKey field of connection after its set");

  if (_is_global && _foreignKey.is_valid())
    _foreignKey.unmark_global();
  if (_is_global && value.is_valid())
    value.mark_global();

  grt::ValueRef ovalue(_foreignKey);
  get_data()->set_foreign_key(value);
  member_changed("foreignKey", ovalue, value);
}
