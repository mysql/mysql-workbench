#include "stdafx.h"

#include <grts/structs.workbench.physical.h>

#include <grtpp_util.h>

#include "wbcanvas/workbench_physical_tablefigure_impl.h"

//================================================================================
// workbench_physical_TableFigure


void workbench_physical_TableFigure::init()
{
  if (!_data)
    _data= new workbench_physical_TableFigure::ImplData(this);
  model_Figure::set_data(_data);
}

void workbench_physical_TableFigure::set_data(ImplData *data)
{
  throw std::logic_error("unexpected");
}

workbench_physical_TableFigure::~workbench_physical_TableFigure()
{
  delete _data;
}


void workbench_physical_TableFigure::table(const db_TableRef &value)
{
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

