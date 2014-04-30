#include "stdafx.h"

#include <grts/structs.workbench.model.h>

#include <grtpp_util.h>


#include "wbcanvas/workbench_model_notefigure_impl.h"

//================================================================================
// workbench_model_NoteFigure


void workbench_model_NoteFigure::init()
{
  if (!_data) _data= new workbench_model_NoteFigure::ImplData(this);
  model_Figure::set_data(_data);
}

void workbench_model_NoteFigure::set_data(ImplData *data)
{
}

workbench_model_NoteFigure::~workbench_model_NoteFigure()
{
  delete _data;
}


void workbench_model_NoteFigure::text(const grt::StringRef &value)
{
  grt::ValueRef ovalue(_text);
  _text= value;
  _data->set_text(_text);
  member_changed("text", ovalue, value);
}




