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

#ifndef _WORKBENCH_PHYSICAL_VIEW_IMPL_H_
#define _WORKBENCH_PHYSICAL_VIEW_IMPL_H_

#include "model_diagram_impl.h"
#include "grts/structs.workbench.physical.h"

class TableFigure;

class WBPUBLICBACKEND_PUBLIC_FUNC workbench_physical_Diagram::ImplData : public model_Diagram::ImplData {
protected:
  typedef model_Diagram::ImplData super;

  std::map<std::string, model_FigureRef> _dbobject_to_figure;
  std::map<std::string, workbench_physical_ConnectionRef> _fk_to_connection;

  virtual void member_list_changed(grt::internal::OwnedList *alist, bool added, const grt::ValueRef &value);

public:
  ImplData(workbench_physical_Diagram *owner);

  workbench_physical_LayerRef place_new_layer(double x, double y, double width, double height, const std::string &name);

  workbench_physical_TableFigureRef place_table(const db_TableRef &table, double x, double y);
  workbench_physical_RoutineGroupFigureRef place_routine_group(const db_RoutineGroupRef &rgroup, double x, double y);
  workbench_physical_ViewFigureRef place_view(const db_ViewRef &view, double x, double y);

  workbench_physical_ConnectionRef create_connection_for_foreign_key(const db_ForeignKeyRef &fk);
  int create_connections_for_table(const db_TableRef &table);
  void delete_connections_for_table(const db_TableRef &table);

  model_FigureRef get_figure_for_dbobject(const db_DatabaseObjectRef &figure);

  void add_mapping(const db_DatabaseObjectRef &object, const model_FigureRef &figure);
  void remove_mapping(const db_DatabaseObjectRef &object);

  workbench_physical_ConnectionRef get_connection_for_foreign_key(const db_ForeignKeyRef &fk);

  void add_fk_mapping(const db_ForeignKeyRef &fk, const workbench_physical_ConnectionRef &connection);
  void remove_fk_mapping(const db_ForeignKeyRef &fk,
                         const workbench_physical_ConnectionRef &connection = workbench_physical_ConnectionRef());

  void auto_place_db_objects(const grt::ListRef<db_DatabaseObject> &objects);

private:
  workbench_physical_Diagram *self() const {
    return (workbench_physical_Diagram *)_self;
  }
};

#endif
