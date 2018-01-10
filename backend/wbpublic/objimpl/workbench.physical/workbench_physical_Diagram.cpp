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

#include <grts/structs.workbench.physical.h>

#include <grtpp_util.h>

#include "wbcanvas/workbench_physical_diagram_impl.h"

#include "wbcanvas/model_layer_impl.h"

//================================================================================
// workbench_physical_Diagram

void workbench_physical_Diagram::init() {
  if (!_data)
    _data = new workbench_physical_Diagram::ImplData(this);
  model_Diagram::set_data(_data);

  if (_rootLayer.is_valid())
    throw std::logic_error("rootLayer value is already initialized");

  rootLayer(workbench_physical_LayerRef(grt::Initialized));
  _rootLayer->owner(this);
  _rootLayer->width(width());
  _rootLayer->height(height());
}

void workbench_physical_Diagram::set_data(ImplData *data) {
  throw std::logic_error("unexpected");
}

workbench_physical_Diagram::~workbench_physical_Diagram() {
  delete _data;
}

void workbench_physical_Diagram::autoPlaceDBObjects(const grt::ListRef<db_DatabaseObject> &objects) {
  get_data()->auto_place_db_objects(objects);
}

model_FigureRef workbench_physical_Diagram::getFigureForDBObject(const db_DatabaseObjectRef &object) {
  return get_data()->get_figure_for_dbobject(object);
}

model_LayerRef workbench_physical_Diagram::placeNewLayer(double x, double y, double width, double height,
                                                         const std::string &name) {
  return get_data()->place_new_layer(x, y, width, height, name);
}

workbench_physical_RoutineGroupFigureRef workbench_physical_Diagram::placeRoutineGroup(
  const db_RoutineGroupRef &routineGroup, double x, double y) {
  return get_data()->place_routine_group(routineGroup, x, y);
}

workbench_physical_TableFigureRef workbench_physical_Diagram::placeTable(const db_TableRef &table, double x, double y) {
  return get_data()->place_table(table, x, y);
}

workbench_physical_ViewFigureRef workbench_physical_Diagram::placeView(const db_ViewRef &view, double x, double y) {
  return get_data()->place_view(view, x, y);
}

workbench_physical_ConnectionRef workbench_physical_Diagram::createConnectionForForeignKey(const db_ForeignKeyRef &fk) {
  return get_data()->create_connection_for_foreign_key(fk);
}

grt::IntegerRef workbench_physical_Diagram::createConnectionsForTable(const db_TableRef &table) {
  return get_data()->create_connections_for_table(table);
}

void workbench_physical_Diagram::deleteConnectionsForTable(const db_TableRef &table) {
  get_data()->delete_connections_for_table(table);
}

workbench_physical_ConnectionRef workbench_physical_Diagram::getConnectionForForeignKey(const db_ForeignKeyRef &fk) {
  return get_data()->get_connection_for_foreign_key(fk);
}
