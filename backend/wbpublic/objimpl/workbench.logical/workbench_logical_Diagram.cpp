#include "stdafx.h"

#include <grts/structs.workbench.logical.h>

#include <grtpp_util.h>


//================================================================================
// workbench_logical_Diagram


class workbench_logical_Diagram::ImplData
{
};


void workbench_logical_Diagram::init()
{
  //if (!_data) _data= new workbench_logical_Diagram::ImplData();
}

void workbench_logical_Diagram::set_data(ImplData *data)
{
}


workbench_logical_Diagram::~workbench_logical_Diagram()
{
  delete _data;
}


model_LayerRef workbench_logical_Diagram::placeNewLayer(double x, double y, double width, double height, const std::string &name)
{
  // add code here
  return model_LayerRef();
}





