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

// High-level testing for Workbench.
// This tests WBContext, which will test the integration of all components.

#include "wb_test_helpers.h"
#include "workbench/wb_overview.h"
#include "base/util_functions.h"
#include "wbcanvas/workbench_physical_tablefigure_impl.h"

#include "grtdb/db_object_helpers.h"

#include "grt_test_helpers.h"
#include "wbcanvas/table_figure.h"

using namespace wb;

#ifndef _MSC_VER
#include <signal.h>

void signal_handler(int sig) {
  printf("Exiting after signal[%d] was trapped\n", sig);
  exit(0);
}
#endif

#include "casmine.h"

namespace {

$ModuleEnvironment() {};

$TestData {
  std::unique_ptr<WorkbenchTester> tester;
};

$describe("Low-level tests for Workbench context") {
  $beforeAll([&]() {
    data->tester.reset(new WorkbenchTester());
    data->tester->initializeRuntime();
  #ifndef _MSC_VER
    if (signal(SIGSEGV, signal_handler) == SIG_ERR) {
      printf("Failed to setup the signal handler\n");
    }
  #endif
  });

  $it("Stored connections test", [this]() {
    $pending("need investigate why connection is not avaiable");
    $expect(data->tester->wb->get_root()->rdbmsMgmt()->storedConns().is_valid()).toBeTrue();

    // We cannot check the exact number because on Windows, if there are no server instances yet,
    // instances and connections are created automatically from all installed servers.
    // So we can't know in advance how many connections we will have (but at least 1, that in the test
    // connection file).
    $expect(data->tester->wb->get_root()->rdbmsMgmt()->storedConns().count() > 0).toBeTrue();

    $expect(data->tester->wb->get_root()->rdbmsMgmt()->rdbms().get(0)->drivers().count() > 0).toBeTrue();

    $expect(data->tester->wb->get_root()->rdbmsMgmt()->storedConns().get(0)->driver().is_valid()).toBeTrue();
  });

  $it("Check if creating a fk between 2 tables will create the connection", [this]() {
    data->tester->wb->new_document();
    data->tester->addView();

    db_mysql_TableRef table1(data->tester->addTableFigure("table1", 10, 10));
    db_mysql_TableRef table2(data->tester->addTableFigure("table2", 10, 100));

    $expect(data->tester->getPview()->figures().count()).toBe(2U);
    $expect(data->tester->getPview()->connections().count()).toBe(0U);

    db_mysql_ColumnRef column(grt::Initialized);
    column->owner(table1);
    column->name("id1");
    column->setParseType("int", data->tester->getRdbms()->simpleDatatypes());
    // bec::ColumnHelper::parse_column_type(data->tester->getRdbms(), data->tester->getCatalog()->userDatatypes(), "int", column);
    table1->columns().insert(column);

    column = db_mysql_ColumnRef(grt::Initialized);
    column->owner(table1);
    column->name("col1");
    column->setParseType("varchar(100)", data->tester->getRdbms()->simpleDatatypes());
    //  bec::ColumnHelper::parse_column_type(data->tester->getRdbms(), data->tester->getCatalog()->userDatatypes(), "varchar(100)",
    //  column);
    table1->columns().insert(column);
    table1->addPrimaryKeyColumn(column);
    // bec::TableHelper::make_primary_key(table1, column, true);

    column = db_mysql_ColumnRef(grt::Initialized);
    column->owner(table2);
    column->name("id2");
    column->setParseType("int", data->tester->getRdbms()->simpleDatatypes());
    // bec::ColumnHelper::parse_column_type(data->tester->getRdbms(), data->tester->getCatalog()->userDatatypes(), "int", column);
    table2->columns().insert(column);

    column = db_mysql_ColumnRef(grt::Initialized);
    column->owner(table2);
    column->name("col2");
    column->setParseType("varchar(100)", data->tester->getRdbms()->simpleDatatypes());
    // bec::ColumnHelper::parse_column_type(data->tester->getRdbms(), data->tester->getCatalog()->userDatatypes(), "varchar(100)",
    // column);
    table2->columns().insert(column);
    table2->addPrimaryKeyColumn(column);
    // bec::TableHelper::make_primary_key(table2, column, true);

    bec::TableHelper::create_foreign_key_to_table(table1, table2, true, true, true, true, data->tester->getRdbms(),
                                                  grt::DictRef(true), grt::DictRef(true));

    $expect(table1->foreignKeys().count() > 0).toBeTrue();
    $expect(table2->foreignKeys().count() == 0).toBeTrue();

    grt::ListRef<model_Connection> tmp(data->tester->getPview()->connections());

    data->tester->flushUntil(3, std::bind(&grt::ListRef<model_Connection>::count, tmp), 1);

    $expect(data->tester->getPview()->connections().count()).toBe(1U);
    data->tester->wb->close_document();
    data->tester->wb->close_document_finish();
  });

  $it("Bug: check if creating a recursive fk will create the connection", [this]() {
    data->tester->wb->new_document();
    data->tester->addView();

    db_mysql_TableRef table = data->tester->addTableFigure("table", 10, 10);

    $expect(data->tester->getPview()->figures().count()).toBe(1U);
    $expect(data->tester->getPview()->connections().count()).toBe(0U);

    db_mysql_ColumnRef column(grt::Initialized);
    column->owner(table);
    column->name("id");
    table->columns().insert(column);

    column = db_mysql_ColumnRef(grt::Initialized);
    column->owner(table);
    column->name("col2");
    table->columns().insert(column);

    table->addPrimaryKeyColumn(column);
    // bec::TableHelper::make_primary_key(table, column, true);

    bec::TableHelper::create_foreign_key_to_table(table, table, true, true, true, true, data->tester->getRdbms(),
                                                  grt::DictRef(true), grt::DictRef(true));

    $expect(table->foreignKeys().count() > 0).toBeTrue();

    grt::ListRef<model_Connection> tmp(data->tester->getPview()->connections());
    data->tester->flushUntil(3, std::bind(&grt::ListRef<model_Connection>::count, tmp), 1);

    $expect(data->tester->getPview()->connections().count()).toBe(1U);

    data->tester->wb->close_document();
    data->tester->wb->close_document_finish();
  });

  $it("Bug: check if deleting an object with privileges will delete the privs too", [this]() {
    data->tester->createNewDocument();

    WBComponentPhysical *phys = data->tester->wb->get_component<WBComponentPhysical>();
    $expect(data->tester->getPmodel()->catalog()->roles().count()).toBe(5U);
    phys->add_new_role(data->tester->getPmodel());
    phys->add_new_role(data->tester->getPmodel());

    db_SchemaRef schema(data->tester->getPmodel()->catalog()->schemata()[0]);

    phys->add_new_db_table(schema);
    phys->add_new_db_table(schema);

    $expect(schema->tables().count()).toBe(2U);
    $expect(data->tester->getPmodel()->catalog()->roles().count()).toBe(2U + 5);

    // add some privs to the table
    db_RoleRef role(data->tester->getPmodel()->catalog()->roles().get(5));
    db_TableRef table(schema->tables().get(0));

    db_TableRef table2(schema->tables().get(1));

    db_RolePrivilegeRef priv(grt::Initialized);

    priv->databaseObject(table);
    priv->databaseObjectType(table.class_name());
    priv->databaseObjectName(table->name());
    priv->privileges().insert("CREATE");
    priv->privileges().insert("DELETE");

    role->privileges().insert(priv);

    db_RolePrivilegeRef priv2(grt::Initialized);

    priv2->databaseObject(table2);
    priv2->databaseObjectType(table.class_name());
    priv2->databaseObjectName(table->name());
    priv2->privileges().insert("CREATE");
    priv2->privileges().insert("INSERT");

    role->privileges().insert(priv2);

    $expect(role->privileges().count()).toBe(2U);

    // delete the 1st table
    phys->delete_db_object(table);

    $expect(data->tester->getPmodel()->catalog()->schemata()[0]->tables().count()).toBe(1U);
    $expect(data->tester->getPmodel()->catalog()->roles().count()).toBe(2U + 5);

    $expect(role->privileges().count()).toBe(1U);

    $expect(role->privileges().get(0)->databaseObject() == table2).toBeTrue();

    data->tester->wb->close_document();
    data->tester->wb->close_document_finish();
  });

  $it("Bug: undo drop table will not reset TableFigure::table_figure_for_dbtable()", [this]() {
    db_TableRef table;

    data->tester->wb->open_document("data/workbench/2tables_1fk.mwb");
    workbench_DocumentRef doc = data->tester->wb->get_document();
    $expect(doc.is_valid()).toBeTrue();

    data->tester->openAllDiagrams();

    $expect(data->tester->getCatalog()->schemata().count()).toBe(1U);

    std::list<db_DatabaseObjectRef> objects;
    $expect(data->tester->getSchema()->tables().count()).toBe(2U);
    objects.push_back(grt::find_named_object_in_list(data->tester->getSchema()->tables(), "table1"));
    $expect(objects.front().is_valid()).toBeTrue();
    $expect(*objects.front()->name()).toBe("table1");
    $expect(doc->physicalModels()[0]->diagrams().count()).toBe(1U);
    data->tester->interactivePlaceDbObjects(10, 150, objects);

    data->tester->flushUntil(2);
    $expect(data->tester->getPview()->figures().count()).toBe(1U);

    grt::GRT::get()->get_undo_manager()->undo();

    $expect(data->tester->getPview()->figures().count()).toBe(0U);
    data->tester->wb->close_document();
    data->tester->wb->close_document_finish();
  });
}
}
