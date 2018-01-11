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

#include "wbcanvas/model_diagram_impl.h"
#include "wbcanvas/model_layer_impl.h"

//================================================================================
// model_Diagram

void model_Diagram::init() {
}

void model_Diagram::set_data(ImplData *data) {
  _data = data;
}

model_Diagram::~model_Diagram() {
}

void model_Diagram::rootLayer(const model_LayerRef &value) {
  grt::ValueRef ovalue(_rootLayer);
  // this member is owned by this object
  if (_rootLayer.is_valid())
    _rootLayer->get_data()->set_in_view(false);
  _rootLayer = value;
  if (_rootLayer.is_valid())
    _rootLayer->get_data()->set_in_view(true);
  owned_member_changed("rootLayer", ovalue, value);
}

void model_Diagram::addConnection(const model_ConnectionRef &connection) {
  _data->add_connection(connection);
}

void model_Diagram::addFigure(const model_FigureRef &figure) {
  _data->add_figure(figure);
}

void model_Diagram::deleteLayer(const model_LayerRef &layer) {
  _data->delete_layer(layer);
}

void model_Diagram::removeConnection(const model_ConnectionRef &connection) {
  _data->remove_connection(connection);
}

void model_Diagram::removeFigure(const model_FigureRef &figure) {
  _data->remove_figure(figure);
}

void model_Diagram::blockUpdates(ssize_t flag) {
  _data->block_updates(flag != 0);
}

void model_Diagram::selectObject(const model_ObjectRef &object) {
  _data->select_object(object);
}

void model_Diagram::setPageCounts(ssize_t xpages, ssize_t ypages) {
  _data->set_page_counts((int)xpages, (int)ypages);
}

void model_Diagram::unselectAll() {
  _data->unselect_all();
}

void model_Diagram::unselectObject(const model_ObjectRef &object) {
  _data->unselect_object(object);
}
