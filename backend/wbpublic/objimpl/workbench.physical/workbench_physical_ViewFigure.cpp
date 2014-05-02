#include "stdafx.h"

#include <grts/structs.workbench.physical.h>

#include <grtpp_util.h>

#include "wbcanvas/workbench_physical_viewfigure_impl.h"


//================================================================================
// workbench_physical_ViewFigure


void workbench_physical_ViewFigure::init()
{
  if (!_data) _data= new workbench_physical_ViewFigure::ImplData(this);
  model_Figure::set_data(_data);
}


void workbench_physical_ViewFigure::set_data(ImplData *data)
{
}

workbench_physical_ViewFigure::~workbench_physical_ViewFigure()
{
  delete _data;
}


void workbench_physical_ViewFigure::view(const db_ViewRef &value)
{
  if (_view == value) return;
  if (_view.is_valid() && value.is_valid())
    throw std::runtime_error("Cannot change view field of figure after its set");

  if (_is_global && _view.is_valid()) _view.unmark_global();
  if (_is_global && value.is_valid()) value.mark_global();

  grt::ValueRef ovalue(_view);
  get_data()->set_view(value);
  member_changed("view", ovalue, value);
}

