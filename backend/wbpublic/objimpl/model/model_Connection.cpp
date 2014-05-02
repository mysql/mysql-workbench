#include "stdafx.h"

#include <grts/structs.model.h>

#include <grtpp_util.h>

#include "wbcanvas/model_connection_impl.h"

//================================================================================
// model_Connection


void model_Connection::init()
{
}

void model_Connection::set_data(ImplData *data)
{
  _data= data;
  model_Object::set_data(data);
}


model_Connection::~model_Connection()
{
}


