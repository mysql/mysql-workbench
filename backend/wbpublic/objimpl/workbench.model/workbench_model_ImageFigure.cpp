#include "stdafx.h"

#include <grts/structs.workbench.model.h>

#include <grtpp_util.h>

#include "wbcanvas/workbench_model_imagefigure_impl.h"

//================================================================================
// workbench_model_ImageFigure


void workbench_model_ImageFigure::init()
{
  if (!_data) _data= new workbench_model_ImageFigure::ImplData(this);
  model_Figure::set_data(_data);
}

void workbench_model_ImageFigure::set_data(ImplData *data)
{
}

workbench_model_ImageFigure::~workbench_model_ImageFigure()
{
  delete _data;
}


grt::StringRef workbench_model_ImageFigure::setImageFile(const std::string &name)
{
  return get_data()->set_filename(name);
}


void workbench_model_ImageFigure::keepAspectRatio(const grt::IntegerRef &value)
{
  grt::ValueRef ovalue(_keepAspectRatio);
  get_data()->set_keep_aspect_ratio(value != 0);
  member_changed("keepAspectRatio", ovalue, value);
}




