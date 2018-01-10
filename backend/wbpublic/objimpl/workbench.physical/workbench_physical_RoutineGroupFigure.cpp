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

#include "wbcanvas/workbench_physical_routinegroupfigure_impl.h"

//================================================================================
// workbench_physical_RoutineGroupFigure

void workbench_physical_RoutineGroupFigure::init() {
  if (!_data)
    _data = new workbench_physical_RoutineGroupFigure::ImplData(this);
  model_Figure::set_data(_data);
}

void workbench_physical_RoutineGroupFigure::set_data(ImplData *data) {
}

workbench_physical_RoutineGroupFigure::~workbench_physical_RoutineGroupFigure() {
  delete _data;
}

void workbench_physical_RoutineGroupFigure::routineGroup(const db_RoutineGroupRef &value) {
  if (_routineGroup == value)
    return;
  if (_routineGroup.is_valid() && value.is_valid())
    throw std::runtime_error("Cannot change routineGroup field of figure after its set");

  if (_is_global && _routineGroup.is_valid())
    _routineGroup.unmark_global();
  if (_is_global && value.is_valid())
    value.mark_global();

  grt::ValueRef ovalue(_routineGroup);
  get_data()->set_routine_group(value);
  member_changed("routineGroup", ovalue, value);
}
