#include "stdafx.h"

#include <grts/structs.workbench.physical.h>

#include <grtpp_util.h>

#include "wbcanvas/workbench_physical_routinegroupfigure_impl.h"

//================================================================================
// workbench_physical_RoutineGroupFigure


void workbench_physical_RoutineGroupFigure::init()
{
  if (!_data) _data= new workbench_physical_RoutineGroupFigure::ImplData(this);
  model_Figure::set_data(_data);
}

void workbench_physical_RoutineGroupFigure::set_data(ImplData *data)
{
}

workbench_physical_RoutineGroupFigure::~workbench_physical_RoutineGroupFigure()
{
  delete _data;
}


void workbench_physical_RoutineGroupFigure::routineGroup(const db_RoutineGroupRef &value)
{
  if (_routineGroup == value) return;
  if (_routineGroup.is_valid() && value.is_valid())
    throw std::runtime_error("Cannot change routineGroup field of figure after its set");

  if (_is_global && _routineGroup.is_valid()) _routineGroup.unmark_global();
  if (_is_global && value.is_valid()) value.mark_global();

  grt::ValueRef ovalue(_routineGroup);
  get_data()->set_routine_group(value);
  member_changed("routineGroup", ovalue, value);
}




