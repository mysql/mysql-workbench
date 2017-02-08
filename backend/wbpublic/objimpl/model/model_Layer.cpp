/*
* Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; version 2 of the
* License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
* 02110-1301  USA
*/

#include <grts/structs.model.h>

#include <grtpp_util.h>

#include "wbcanvas/model_layer_impl.h"

//================================================================================
// model_Layer

void model_Layer::init() {
  _data = new ImplData(this);
  model_Object::set_data(_data);
}

void model_Layer::set_data(ImplData *data) {
  throw std::logic_error("unexpected");
}

model_Layer::~model_Layer() {
  delete _data;
}

void model_Layer::lowerFigure(const model_FigureRef &figure) {
  get_data()->lower_figure(figure);
}

void model_Layer::raiseFigure(const model_FigureRef &figure) {
  get_data()->raise_figure(figure);
}
