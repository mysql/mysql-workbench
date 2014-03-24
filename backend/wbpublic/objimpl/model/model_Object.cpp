#include "stdafx.h"

#include <grts/structs.model.h>

#include <grtpp_util.h>

#include "wbcanvas/model_object_impl.h"

//================================================================================
// model_Object


void model_Object::init()
{
}

void model_Object::set_data(ImplData *data)
{
  _data= data;
}

model_Object::~model_Object()
{
}

