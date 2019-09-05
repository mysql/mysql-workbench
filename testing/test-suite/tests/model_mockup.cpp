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

#include "model_mockup.h"

#include "grtdb/db_helpers.h"
#include "grtdb/db_object_helpers.h"

#include "grtsqlparser/mysql_parser_services.h"

using namespace bec;
using namespace casmine;

SyntheticMySQLModel::SyntheticMySQLModel(WorkbenchTester *tester)
  : physicalDiagram(grt::Initialized),
  schema(grt::Initialized),
  table(grt::Initialized),
  column(grt::Initialized),
  column2(grt::Initialized),
  columnText(grt::Initialized),
  columnDate(grt::Initialized),
  columnDouble(grt::Initialized),
  columnEnum(grt::Initialized),
  primaryKey(grt::Initialized),
  indexColumn(grt::Initialized),
  foreignKey(grt::Initialized),
  trigger(grt::Initialized),
  view(grt::Initialized),
  routine(grt::Initialized),
  routineGroup(grt::Initialized),
  user(grt::Initialized),
  role(grt::Initialized),
  tablePrivilege(grt::Initialized),
  viewPrivilege(grt::Initialized),
  routinePrivilege(grt::Initialized),
  tableFigure(grt::Initialized),
  viewFigure(grt::Initialized),
  routineGroupFigure(grt::Initialized) {
  model = grt::copy_object(tester->wb->get_document()->physicalModels().get(0));
  rdbms = model->rdbms();
  catalog = db_mysql_CatalogRef::cast_from(model->catalog());

  schema = catalog->schemata().get(0);

  fillDocumentWithData();
}

SyntheticMySQLModel::SyntheticMySQLModel()
  : model(grt::Initialized),
  physicalDiagram(grt::Initialized),
  catalog(grt::Initialized),
  schema(grt::Initialized),
  table(grt::Initialized),
  column(grt::Initialized),
  column2(grt::Initialized),
  columnText(grt::Initialized),
  columnDate(grt::Initialized),
  columnDouble(grt::Initialized),
  columnEnum(grt::Initialized),
  primaryKey(grt::Initialized),
  indexColumn(grt::Initialized),
  foreignKey(grt::Initialized),
  trigger(grt::Initialized),
  view(grt::Initialized),
  routine(grt::Initialized),
  routineGroup(grt::Initialized),
  user(grt::Initialized),
  role(grt::Initialized),
  tablePrivilege(grt::Initialized),
  viewPrivilege(grt::Initialized),
  routinePrivilege(grt::Initialized),
  tableFigure(grt::Initialized),
  viewFigure(grt::Initialized),
  routineGroupFigure(grt::Initialized) {

  // Set up a basic root structure.
  workbench_WorkbenchRef wb(grt::Initialized);
  workbench_DocumentRef doc(grt::Initialized);
  doc->owner(wb);
  wb->doc(doc);
  grt::GRT::get()->set("/wb", wb);

  app_OptionsRef options(grt::Initialized);
  options->owner(wb);
  wb->options(options);

  // Fill model with data.
  model->catalog(catalog);
  catalog->owner(model);
  auto version = parse_version("8.0.5");
  version->owner(catalog);
  catalog->version(version);

  grt::ListRef<db_DatatypeGroup> grouplist = grt::ListRef<db_DatatypeGroup>::cast_from(
    grt::GRT::get()->unserialize("../../res/grtdata/db_datatype_groups.xml")
  );

  // unfortunately we have to do these lines to make sure that references (links) will be resolved during import
  grt::ValueRef root = grt::GRT::get()->root();
  grt::DictRef::cast_from(root).set("grouplist", grouplist);

  rdbms = db_mgmt_RdbmsRef::cast_from(grt::GRT::get()->unserialize("data/res/mysql_rdbms_info.xml"));
  model->rdbms(rdbms);

  catalog->name("test_catalog");
  grt::replace_contents(catalog->simpleDatatypes(), rdbms->simpleDatatypes());
  grt::replace_contents(catalog->characterSets(), rdbms->characterSets());
  catalog->schemata().insert(schema);
  schema->owner(catalog);

  fillDocumentWithData();
}

void SyntheticMySQLModel::fillDocumentWithData() {
  // MODEL DATA
  schema->name("test_schema");
  schema->tables().insert(table);
  table->owner(schema);
  schema->views().insert(view);
  view->owner(schema);
  schema->routines().insert(routine);
  routine->owner(schema);
  schema->routineGroups().insert(routineGroup);
  routineGroup->owner(schema);

  table->name("t1");
  table->columns().insert(column);
  column->owner(table);
  table->columns().insert(column2);
  column2->owner(table);
  table->columns().insert(columnText);
  columnText->owner(table);
  table->columns().insert(columnDate);
  columnDate->owner(table);
  table->columns().insert(columnDouble);
  columnDouble->owner(table);
  table->columns().insert(columnEnum);
  columnEnum->owner(table);
  table->indices().insert(primaryKey);
  primaryKey->owner(table);
  table->foreignKeys().insert(foreignKey);
  foreignKey->owner(table);
  table->primaryKey(primaryKey);
  table->triggers().insert(trigger);
  trigger->owner(table);

  table->tableEngine("InnoDB");

  view->name("v1");
  view->sqlDefinition("create view v1 as select id from t1");

  routine->name("r1");
  routine->routineType("function");

#ifndef NL
#define NL "\n"
#endif

  // Note: delimiters are *not* part of SQL. They are control commands for the command line client
  //       and for us to find individual statements. Hence they have nothing to do with object SQL
  //       which is by nature a single statement only.
  routine->sqlDefinition(
    "CREATE FUNCTION r1(less_than INT, greather_than INT) RETURNS INT" NL
    "    DETERMINISTIC" NL
    "    READS SQL DATA" NL
    "BEGIN" NL
    "       #OK, here some comment" NL
    "  DECLARE res INTEGER; #FEES PAID TO RENT THE VIDEOS INITIALLY" NL
    "  SELECT count(*) INTO res" NL
    "    FROM t1" NL
    "    WHERE id > less_than AND id < greather_than;" NL
    "  RETURN res;" NL
    "END");

  routineGroup->name("group1");
  routineGroup->routines().insert(routine);

  grt::ListRef<db_SimpleDatatype> dataTypes = model->rdbms()->simpleDatatypes();

  column->name("id");
  column->simpleType(parsers::MySQLParserServices::findDataType(dataTypes, catalog->version(), "tinyint"));
  column->defaultValueIsNull(0);
  column->isNotNull(1);

  column2->name("parent_id");
  column2->simpleType(parsers::MySQLParserServices::findDataType(dataTypes, catalog->version(), "tinyint"));
  column2->defaultValueIsNull(0);

  columnText->name("desc");
  columnText->simpleType(parsers::MySQLParserServices::findDataType(dataTypes, catalog->version(), "decimal"));
  columnText->defaultValueIsNull(0);

  columnDate->name("datetimefield");
  columnDate->simpleType(parsers::MySQLParserServices::findDataType(dataTypes, catalog->version(), "datetime"));
  columnDate->defaultValueIsNull(0);

  columnDouble->name("doublefield");
  columnDouble->simpleType(parsers::MySQLParserServices::findDataType(dataTypes, catalog->version(), "double"));
  columnDouble->defaultValueIsNull(0);

  columnEnum->name("enumfield");
  columnEnum->simpleType(parsers::MySQLParserServices::findDataType(dataTypes, catalog->version(), "enum"));
  columnEnum->datatypeExplicitParams("('e1','e2')");
  columnEnum->defaultValueIsNull(0);

  primaryKey->name("PK");
  primaryKey->columns().insert(indexColumn);
  primaryKey->isPrimary(1);

  indexColumn->owner(primaryKey);
  indexColumn->referencedColumn(column);

  foreignKey->name("FK");
  foreignKey->columns().insert(column2);
  foreignKey->referencedColumns().insert(column);
  foreignKey->referencedTable(table);

  trigger->name("trig");
  trigger->sqlDefinition("CREATE TRIGGER trig BEFORE INSERT ON t1 FOR EACH ROW SET NEW.id = id;");

  // PRIVILEGES
  catalog->users().insert(user);      user->owner(catalog);
  catalog->roles().insert(role);      role->owner(catalog);

  user->name("User1");
  user->roles().insert(role);

  role->name("Role1");
  role->privileges().insert(tablePrivilege);  tablePrivilege->owner(role);
  role->privileges().insert(viewPrivilege);   viewPrivilege->owner(role);
  role->privileges().insert(routinePrivilege);routinePrivilege->owner(role);

  tablePrivilege->databaseObject(table);
  tablePrivilege->privileges().insert("ALL");
  viewPrivilege->databaseObject(view);
  viewPrivilege->privileges().insert("ALL");
  routinePrivilege->databaseObject(routine);
  routinePrivilege->privileges().insert("ALL");

  // ER DIAGRAM
  model->diagrams().insert(physicalDiagram);
  physicalDiagram->owner(model);

  tableFigure->table(table);
  tableFigure->owner(physicalDiagram);
  physicalDiagram->figures().insert(tableFigure);

  viewFigure->view(view);
  viewFigure->owner(physicalDiagram);
  physicalDiagram->figures().insert(viewFigure);

  routineGroupFigure->routineGroup(routineGroup);
  routineGroupFigure->owner(physicalDiagram);
  physicalDiagram->figures().insert(routineGroupFigure);
}

void casmine::addPrivilege(SyntheticMySQLModel &model, db_RoleRef &role, db_DatabaseObjectRef obj, const char *priv) {
  db_RolePrivilegeRef privilege(grt::Initialized);
  role->privileges().insert(privilege);
  privilege->owner(role);

  privilege->databaseObject(obj);
  privilege->privileges().insert(priv);
}

void casmine::addPrivilege(SyntheticMySQLModel &model, db_RoleRef &role, const char* objectType, const char* objectName,
  const char *priv) {
  db_RolePrivilegeRef privilege(grt::Initialized);
  role->privileges().insert(privilege);
  privilege->owner(role);

  privilege->databaseObjectName(objectName);
  privilege->databaseObjectType(objectType);
  privilege->privileges().insert(priv);
}

void casmine::assignRole(db_UserRef user, db_RoleRef role) {
  user->roles().insert(role);
}
