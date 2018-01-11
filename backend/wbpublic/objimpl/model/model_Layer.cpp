/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
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
