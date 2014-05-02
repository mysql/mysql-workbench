#include "stdafx.h"

#include <grts/structs.model.h>

#include <grtpp_util.h>

#include "wbcanvas/model_layer_impl.h"

//================================================================================
// model_Layer


void model_Layer::init()
{
  _data= new ImplData(this);
  model_Object::set_data(_data);
}

void model_Layer::set_data(ImplData *data)
{
  throw std::logic_error("unexpected");

  _data= data;
}

model_Layer::~model_Layer()
{
  delete _data;
}


void model_Layer::lowerFigure(const model_FigureRef &figure)
{
  get_data()->lower_figure(figure);
}


void model_Layer::raiseFigure(const model_FigureRef &figure)
{
  get_data()->raise_figure(figure);
}





