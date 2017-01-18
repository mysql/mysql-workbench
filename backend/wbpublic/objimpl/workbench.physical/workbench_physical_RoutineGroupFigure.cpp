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
