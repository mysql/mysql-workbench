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

#include "wbcanvas/model_figure_impl.h"

//================================================================================
// model_Figure

void model_Figure::init() {
}

void model_Figure::set_data(ImplData *data) {
  _data = data;
  model_Object::set_data(data);
}

model_Figure::~model_Figure() {
}

void model_Figure::color(const grt::StringRef &value) {
  grt::ValueRef ovalue(_color);
  _color = value;
  owned_member_changed("color", ovalue, value);
}

void model_Figure::layer(const model_LayerRef &value) {
  grt::ValueRef ovalue(_layer);
  get_data()->set_layer(value);
  owned_member_changed("layer", ovalue, value);
}
