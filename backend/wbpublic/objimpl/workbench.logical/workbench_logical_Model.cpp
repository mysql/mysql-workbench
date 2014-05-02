#include "stdafx.h"

#include <grts/structs.workbench.logical.h>

#include <grtpp_util.h>


//================================================================================
// workbench_logical_Model


class workbench_logical_Model::ImplData
{
};


void workbench_logical_Model::init()
{
  //if (!_data) _data= new workbench_logical_Model::ImplData();
}


void workbench_logical_Model::set_data(ImplData *data)
{
}

workbench_logical_Model::~workbench_logical_Model()
{
  delete _data;
}


model_DiagramRef workbench_logical_Model::addNewDiagram(long)
{
  // add code here
  return model_DiagramRef();
}





