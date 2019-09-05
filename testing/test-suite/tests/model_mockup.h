/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include "grts/structs.h"
#include "grts/structs.db.mgmt.h"
#include "grts/structs.db.mysql.h"
#include "grts/structs.workbench.h"
#include "grts/structs.workbench.physical.h"

#include "grt/grt_manager.h"

#include "wb_test_helpers.h"

namespace casmine {

  struct SyntheticMySQLModel {
    db_mgmt_RdbmsRef rdbms;
    workbench_physical_ModelRef model;
    workbench_physical_DiagramRef physicalDiagram;

    db_mysql_CatalogRef catalog;
    db_mysql_SchemaRef schema;
    db_mysql_TableRef table;
    db_mysql_ColumnRef column;
    db_mysql_ColumnRef column2;
    db_mysql_ColumnRef columnText;
    db_mysql_ColumnRef columnDate;
    db_mysql_ColumnRef columnDouble;
    db_mysql_ColumnRef columnEnum;
    db_mysql_IndexRef primaryKey;
    db_mysql_IndexColumnRef indexColumn;
    db_mysql_ForeignKeyRef foreignKey;
    db_mysql_TriggerRef trigger;

    db_mysql_ViewRef view;
    db_mysql_RoutineRef routine;
    db_mysql_RoutineGroupRef routineGroup;
    db_UserRef user;
    db_RoleRef role;

    db_RolePrivilegeRef tablePrivilege;
    db_RolePrivilegeRef viewPrivilege;
    db_RolePrivilegeRef routinePrivilege;

    workbench_physical_TableFigureRef tableFigure;
    workbench_physical_ViewFigureRef viewFigure;
    workbench_physical_RoutineGroupFigureRef routineGroupFigure;

    SyntheticMySQLModel();
    SyntheticMySQLModel(WorkbenchTester *wbt);

    void fillDocumentWithData();
  };

  template <class X, class Model>
  void addToModel(X &, Model &);

  template <>
  inline void addToModel<db_RoleRef, SyntheticMySQLModel>(db_RoleRef &role, SyntheticMySQLModel &model) {
    model.catalog->roles().insert(role);
  }

  template <>
  inline void addToModel(db_UserRef &user, SyntheticMySQLModel &model) {
    model.catalog->users().insert(user);
  }

  template <class X, class Model = SyntheticMySQLModel>
  struct xWrap : public X {
    xWrap(const char *n, Model &model) : X(grt::Initialized) {
      (*this)->owner(model.schema);
      (*this)->name(n);
      addToModel((X &)*this, model);
    };
  };

  typedef xWrap<db_RoleRef> xRole;
  typedef xWrap<db_UserRef> xUser;

  void addPrivilege(SyntheticMySQLModel &model, db_RoleRef &role, db_DatabaseObjectRef obj, const char *priv);
  void addPrivilege(SyntheticMySQLModel &model, db_RoleRef &role, const char *objectType, const char *objectName,
                    const char *priv);
  void assignRole(db_UserRef user, db_RoleRef role);
}
