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

#include "model_figure_impl.h"

#include "workbench_physical_diagram_impl.h"
#include "workbench_physical_tablefigure_impl.h"

#include "grtpp_undo_manager.h"

#include "base/string_utilities.h"

using namespace wbbridge::physical;
using namespace base;

workbench_physical_Diagram::ImplData::ImplData(workbench_physical_Diagram *owner) : super(owner) {
}

workbench_physical_LayerRef workbench_physical_Diagram::ImplData::place_new_layer(double x, double y, double width,
                                                                                  double height,
                                                                                  const std::string &name) {
  workbench_physical_LayerRef layer(grt::Initialized);
  bool skip_undo = !self()->is_global();
  grt::AutoUndo undo(skip_undo);

  layer->owner(self());
  layer->left(x);
  layer->top(y);
  layer->width(width);
  layer->height(height);
  layer->name(name);

  // add to layers list
  self()->_layers.insert(layer);

  model_LayerRef rootLayer(self()->rootLayer());

  // add to root layer
  rootLayer->subLayers().insert(layer);

  // capture items inside the layer area
  Rect bounds(x, y, width, height);
  // check for objects in the parent layer that fall inside this layer and add them
  for (size_t c = rootLayer->figures().count(), i = 0; i < c; i++) {
    size_t idx = c - i - 1;
    model_FigureRef fig(rootLayer->figures().get(idx));
    Rect fbounds(*fig->left(), *fig->top(), *fig->width(), *fig->height());

    if (mdc::bounds_contain_bounds(bounds, fbounds)) {
      fig->layer(layer);
      rootLayer->figures().remove(idx);
      layer->figures().insert(fig);
    }
  }

  undo.end(strfmt(_("Place '%s'"), name.c_str()));

  return layer;
}

workbench_physical_TableFigureRef workbench_physical_Diagram::ImplData::place_table(const db_TableRef &table, double x,
                                                                                    double y) {
  workbench_physical_TableFigureRef figure(grt::Initialized);
  bool skip_undo = !self()->is_global();
  grt::AutoUndo undo(skip_undo);

  figure->owner(self());
  figure->table(table);
  figure->left(x);
  figure->top(y);
  figure->layer(get_layer_under_figure(figure));
  figure->name(table->name());
  figure->color(self()->owner()->get_data()->common_color_for_db_object(table, "table"));

  self()->addFigure(figure);

  create_connections_for_table(table);

  undo.end(strfmt(_("Place '%s'"), figure->name().c_str()));

  return figure;
}

workbench_physical_RoutineGroupFigureRef workbench_physical_Diagram::ImplData::place_routine_group(
  const db_RoutineGroupRef &rgroup, double x, double y) {
  workbench_physical_RoutineGroupFigureRef figure(grt::Initialized);
  bool skip_undo = !self()->is_global();
  grt::AutoUndo undo(skip_undo);

  figure->owner(self());
  figure->routineGroup(rgroup);
  figure->left(x);
  figure->top(y);
  figure->layer(get_layer_under_figure(figure));
  figure->name(rgroup->name());
  figure->color(self()->owner()->get_data()->common_color_for_db_object(rgroup, "routineGroup"));

  self()->addFigure(figure);

  undo.end(strfmt(_("Place '%s'"), figure->name().c_str()));

  return figure;
}

workbench_physical_ViewFigureRef workbench_physical_Diagram::ImplData::place_view(const db_ViewRef &view, double x,
                                                                                  double y) {
  workbench_physical_ViewFigureRef figure(grt::Initialized);
  bool skip_undo = !self()->is_global();
  grt::AutoUndo undo(skip_undo);

  figure->owner(self());
  figure->view(view);
  figure->left(x);
  figure->top(y);
  figure->layer(get_layer_under_figure(figure));
  figure->name(view->name());
  figure->color(self()->owner()->get_data()->common_color_for_db_object(view, "view"));

  self()->addFigure(figure);

  undo.end(strfmt(_("Place '%s'"), figure->name().c_str()));

  return figure;
}

model_FigureRef workbench_physical_Diagram::ImplData::get_figure_for_dbobject(const db_DatabaseObjectRef &obj) {
  if (obj.is_valid()) {
    std::map<std::string, model_FigureRef>::iterator iter;

    iter = _dbobject_to_figure.find(obj.id());

    if (iter != _dbobject_to_figure.end())
      return iter->second;
  }
  return model_FigureRef();
}

void workbench_physical_Diagram::ImplData::add_mapping(const db_DatabaseObjectRef &object,
                                                       const model_FigureRef &figure) {
  _dbobject_to_figure[object.id()] = figure;
}

void workbench_physical_Diagram::ImplData::remove_mapping(const db_DatabaseObjectRef &object) {
  _dbobject_to_figure.erase(object.id());
}

workbench_physical_ConnectionRef workbench_physical_Diagram::ImplData::create_connection_for_foreign_key(
  const db_ForeignKeyRef &fk) {
  // check if both tables referenced in fk are in the view
  if (_fk_to_connection.find(fk.id()) == _fk_to_connection.end() &&
      get_figure_for_dbobject(db_DatabaseObjectRef::cast_from(fk->owner())).is_valid() &&
      get_figure_for_dbobject(fk->referencedTable()).is_valid()) {
    workbench_physical_ConnectionRef conn(grt::Initialized);

    conn->owner(self());
    conn->name("");
    conn->caption(fk->name());
    // start and end figures are set by the internal code from the connection
    conn->foreignKey(fk);

    self()->addConnection(conn);

    return conn;
  }
  return workbench_physical_ConnectionRef();
}

/** Creates or updates connections for a table.

This will create connection objects for each foreign key in the table if
both referencing and referenced tables are in the view. It will also check if
the table is referenced by an existing table's foreign key and create connections
if possible.
 */
int workbench_physical_Diagram::ImplData::create_connections_for_table(const db_TableRef &table) {
  int c = 0;

  if (table.is_valid()) {
    // first create connections for FKs from the table
    for (grt::ListRef<db_ForeignKey>::const_iterator end = table->foreignKeys().end(),
                                                     fk = table->foreignKeys().begin();
         fk != end; ++fk) {
      if (create_connection_for_foreign_key(*fk).is_valid())
        c++;
    }

    // create connections for FKs that reference this one
    db_SchemaRef schema(db_SchemaRef::cast_from(table->owner()));

    if (schema.is_valid()) {
      grt::ListRef<db_ForeignKey> fks(schema->getForeignKeysReferencingTable(table));

      for (grt::ListRef<db_ForeignKey>::const_iterator fk = fks.begin(); fk != fks.end(); ++fk) {
        if (create_connection_for_foreign_key(*fk).is_valid())
          c++;
      }
    }
  }

  return c;
}

void workbench_physical_Diagram::ImplData::delete_connections_for_table(const db_TableRef &table) {
  if (table.is_valid()) {
    // first delete connections for FKs from the table
    for (grt::ListRef<db_ForeignKey>::const_iterator end = table->foreignKeys().end(),
                                                     fk = table->foreignKeys().begin();
         fk != end; ++fk) {
      workbench_physical_ConnectionRef conn(get_connection_for_foreign_key(*fk));
      if (conn.is_valid())
        remove_connection(conn);
    }

    // delete connections for FKs that reference this one
    db_SchemaRef schema(db_SchemaRef::cast_from(table->owner()));

    if (schema.is_valid()) {
      grt::ListRef<db_ForeignKey> fks(schema->getForeignKeysReferencingTable(table));

      for (grt::ListRef<db_ForeignKey>::const_iterator fk = fks.begin(); fk != fks.end(); ++fk) {
        workbench_physical_ConnectionRef conn(get_connection_for_foreign_key(*fk));
        if (conn.is_valid())
          remove_connection(conn);
      }
    }
  }
}

workbench_physical_ConnectionRef workbench_physical_Diagram::ImplData::get_connection_for_foreign_key(
  const db_ForeignKeyRef &fk) {
  std::map<std::string, workbench_physical_ConnectionRef>::iterator iter;

  iter = _fk_to_connection.find(fk.id());

  if (iter != _fk_to_connection.end())
    return iter->second;

  return workbench_physical_ConnectionRef();
}

void workbench_physical_Diagram::ImplData::add_fk_mapping(const db_ForeignKeyRef &fk,
                                                          const workbench_physical_ConnectionRef &connection) {
  _fk_to_connection[fk.id()] = connection;
}

void workbench_physical_Diagram::ImplData::remove_fk_mapping(const db_ForeignKeyRef &fk,
                                                             const workbench_physical_ConnectionRef &connection) {
  // check if the fk really is assigned to the connection before removing it
  if (_fk_to_connection.find(fk.id()) != _fk_to_connection.end() && _fk_to_connection[fk.id()] == connection) {
    _fk_to_connection.erase(fk.id());
  }
}

void workbench_physical_Diagram::ImplData::member_list_changed(grt::internal::OwnedList *alist, bool added,
                                                               const grt::ValueRef &value) {
  grt::BaseListRef list(alist);

  if (list == self()->_connections) {
    workbench_physical_ConnectionRef conn(workbench_physical_ConnectionRef::cast_from(value));

    if (conn->foreignKey().is_valid()) {
      if (added)
        add_fk_mapping(conn->foreignKey(), conn);
      else
        remove_fk_mapping(conn->foreignKey(), conn);
    }
  }

  model_Diagram::ImplData::member_list_changed(alist, added, value);
}

void workbench_physical_Diagram::ImplData::auto_place_db_objects(const grt::ListRef<db_DatabaseObject> &objects) {
  grt::Module *module = grt::GRT::get()->get_module("WbModel");

  grt::BaseListRef args(true);

  args.ginsert(workbench_physical_ModelRef::cast_from(self()->owner())->catalog());
  args.ginsert(objects);

  module->call_function("autoplace", args);
}
