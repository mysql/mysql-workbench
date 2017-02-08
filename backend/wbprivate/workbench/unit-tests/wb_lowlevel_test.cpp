/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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

// High-level testing for Workbench.
// This tests WBContext, which will test the integration of all components.

#include "wb_helpers.h"
#include "workbench/wb_overview.h"
#include "base/util_functions.h"
#include "wbcanvas/workbench_physical_tablefigure_impl.h"

#include "grtdb/db_object_helpers.h"

#include "grt_test_utility.h"
#include "wbcanvas/table_figure.h"

#include "wb.model.advfind/wb_find_dialog.h"

using namespace wb;

#ifndef _WIN32
#include <signal.h>

void signal_handler(int sig) {
  printf("Exiting after signal[%d] was trapped\n", sig);
  exit(0);
}
#endif

BEGIN_TEST_DATA_CLASS(wb_lowlevel_test)
public:
WBTester *tester;
TEST_DATA_CONSTRUCTOR(wb_lowlevel_test) {
  // Check that loaded connections file correctly sets the driver link.
  // Create a connections config file from the template first.
  // Have to go via a copy as the connection file itself is modified every time
  // the test runs, so it cannot be under version control directly.
  if (!copy_file("data/connections_template.xml", "data/connections.xml"))
    fail("Could not copy connection file.");
  tester = new WBTester();
#ifndef _WIN32
  if (signal(SIGSEGV, signal_handler) == SIG_ERR) {
    printf("Failed to setup the signal handler\n");
  }
#endif
}
TEST_DATA_DESTRUCTOR(wb_lowlevel_test) {
}

END_TEST_DATA_CLASS;

TEST_MODULE(wb_lowlevel_test, "Low-level tests for Workbench context");

TEST_FUNCTION(5) {
  ensure("stored connections loaded", tester->wb->get_root()->rdbmsMgmt()->storedConns().is_valid());

  // We cannot check the exact number because on Windows, if there are no server instances yet,
  // instances and connections are created automatically from all installed servers.
  // So we can't know in advance how many connections we will have (but at least 1, that in the test
  // connection file).
  ensure("stored connections loaded", tester->wb->get_root()->rdbmsMgmt()->storedConns().count() > 0);

  ensure("check drivers list", tester->wb->get_root()->rdbmsMgmt()->rdbms().get(0)->drivers().count() > 0);

  ensure("stored connections loaded sets driver field",
         tester->wb->get_root()->rdbmsMgmt()->storedConns().get(0)->driver().is_valid());
}

TEST_FUNCTION(10) {
  // check if creating a fk between 2 tables will create the connection
  populate_grt(*tester);

  tester->wb->new_document();
  tester->add_view();

  db_mysql_TableRef table1(tester->add_table_figure("table1", 10, 10));
  db_mysql_TableRef table2(tester->add_table_figure("table2", 10, 100));

  ensure_equals("figure created", tester->get_pview()->figures().count(), 2U);
  ensure_equals("connections", tester->get_pview()->connections().count(), 0U);

  db_mysql_ColumnRef column(grt::Initialized);
  column->owner(table1);
  column->name("id1");
  column->setParseType("int", tester->get_rdbms()->simpleDatatypes());
  // bec::ColumnHelper::parse_column_type(tester->get_rdbms(), tester->get_catalog()->userDatatypes(), "int", column);
  table1->columns().insert(column);

  column = db_mysql_ColumnRef(grt::Initialized);
  column->owner(table1);
  column->name("col1");
  column->setParseType("varchar(100)", tester->get_rdbms()->simpleDatatypes());
  //  bec::ColumnHelper::parse_column_type(tester->get_rdbms(), tester->get_catalog()->userDatatypes(), "varchar(100)",
  //  column);
  table1->columns().insert(column);
  table1->addPrimaryKeyColumn(column);
  // bec::TableHelper::make_primary_key(table1, column, true);

  column = db_mysql_ColumnRef(grt::Initialized);
  column->owner(table2);
  column->name("id2");
  column->setParseType("int", tester->get_rdbms()->simpleDatatypes());
  // bec::ColumnHelper::parse_column_type(tester->get_rdbms(), tester->get_catalog()->userDatatypes(), "int", column);
  table2->columns().insert(column);

  column = db_mysql_ColumnRef(grt::Initialized);
  column->owner(table2);
  column->name("col2");
  column->setParseType("varchar(100)", tester->get_rdbms()->simpleDatatypes());
  // bec::ColumnHelper::parse_column_type(tester->get_rdbms(), tester->get_catalog()->userDatatypes(), "varchar(100)",
  // column);
  table2->columns().insert(column);
  table2->addPrimaryKeyColumn(column);
  // bec::TableHelper::make_primary_key(table2, column, true);

  bec::TableHelper::create_foreign_key_to_table(table1, table2, true, true, true, true, tester->get_rdbms(),
                                                grt::DictRef(true), grt::DictRef(true));

  ensure("fk created in right table", table1->foreignKeys().count() > 0);
  ensure("fk created in right table", table2->foreignKeys().count() == 0);

  grt::ListRef<model_Connection> tmp(tester->get_pview()->connections());

  tester->flush_until(3, std::bind(&grt::ListRef<model_Connection>::count, tmp), 1);

  ensure_equals("connection created", tester->get_pview()->connections().count(), 1U);
  tester->wb->close_document();
  tester->wb->close_document_finish();
}

TEST_FUNCTION(15) {
  // bug: check if creating a recursive fk will create the connection
  populate_grt(*tester);

  tester->wb->new_document();
  tester->add_view();

  db_mysql_TableRef table = tester->add_table_figure("table", 10, 10);

  ensure_equals("figure created", tester->get_pview()->figures().count(), 1U);
  ensure_equals("connections", tester->get_pview()->connections().count(), 0U);

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

  bec::TableHelper::create_foreign_key_to_table(table, table, true, true, true, true, tester->get_rdbms(),
                                                grt::DictRef(true), grt::DictRef(true));

  ensure("fk created", table->foreignKeys().count() > 0);

  grt::ListRef<model_Connection> tmp(tester->get_pview()->connections());
  tester->flush_until(3, std::bind(&grt::ListRef<model_Connection>::count, tmp), 1);

  ensure_equals("connection created", tester->get_pview()->connections().count(), 1U);

  tester->wb->close_document();
  tester->wb->close_document_finish();
}

// TODO : Check if this function still valid as the function has been removed from WBContext
/*
TEST_FUNCTION(20)
{
  // test check_plugin_input_available from WBContext
  WBTester tester;
  WBContext *wb= tester->wb;
  app_PluginObjectInputRef pdef(grt::Initialized);

  wb->new_document();

  // with no views
  ensure("nothing active", tester->wbui->get_active_main_form()==0);
  ensure("nothing active", tester->wbui->get_active_form()==0);


  // test for model.View
  pdef->objectStructName(model_Diagram::static_class_name());
  ensure("model.View", !wb->check_plugin_input_available(pdef));

  // test for workbench.physical.View
  pdef->objectStructName(workbench_physical_Diagram::static_class_name());
  ensure("workbench.physical.Diagram", !wb->check_plugin_input_available(pdef));

  // test for db.Catalog
  pdef->objectStructName(db_Catalog::static_class_name());
  ensure("db.Catalog", !wb->check_plugin_input_available(pdef));

  // test for db.mysql.Catalog
  pdef->objectStructName(db_mysql_Catalog::static_class_name());
  ensure("db.mysql.Catalog", !wb->check_plugin_input_available(pdef));

  // test for model.Model
  pdef->objectStructName(model_Model::static_class_name());
  ensure("model.Model", !wb->check_plugin_input_available(pdef));

  // test for workbench.physical.Model
  pdef->objectStructName(workbench_physical_Model::static_class_name());
  ensure("workbench.physical.Model", !wb->check_plugin_input_available(pdef));

  //---------------------
  // with a view (physical model)
  tester->add_view();
  tester->wbui->set_active_form(tester->wb->get_model_context()->get_diagram_form(tester->last_view));

  ensure("view active", dynamic_cast<ModelDiagramForm*>(tester->wbui->get_active_main_form())!=0);
  ensure("view active", tester->wbui->get_active_form()!=0);

  // test for model.View
  pdef->objectStructName(model_Diagram::static_class_name());
  ensure("model.Diagram", wb->check_plugin_input_available(pdef));

  // test for workbench.physical.View
  pdef->objectStructName(workbench_physical_Diagram::static_class_name());
  ensure("workbench.physical.View", wb->check_plugin_input_available(pdef));

  // test for db.Catalog
  pdef->objectStructName(db_Catalog::static_class_name());
  ensure("db.Catalog", wb->check_plugin_input_available(pdef));

  // test for db.mysql.Catalog
  pdef->objectStructName(db_mysql_Catalog::static_class_name());
  ensure("db.mysql.Catalog", wb->check_plugin_input_available(pdef));

  // test for model.Model
  pdef->objectStructName(model_Model::static_class_name());
  ensure("model.Model", wb->check_plugin_input_available(pdef));

  // test for workbench.physical.Model
  pdef->objectStructName(workbench_physical_Model::static_class_name());
  ensure("workbench.physical.Model", wb->check_plugin_input_available(pdef));


  //---------------------
  // with overview
  tester->wbui->set_active_form(tester->wbui->get_physical_overview());

  ensure("overview active", dynamic_cast<OverviewBE*>(tester->wbui->get_active_main_form())!=0);
  ensure("overview active", tester->wbui->get_active_form()!=0);
  ensure("overview has physical model",
dynamic_cast<OverviewBE*>(tester->wbui->get_active_main_form())->get_model().is_valid());

  // test for model.View
  pdef->objectStructName(model_Diagram::static_class_name());
  ensure("model.View", !wb->check_plugin_input_available(pdef));

  // test for workbench.physical.View
  pdef->objectStructName(workbench_physical_Diagram::static_class_name());
  ensure("workbench.physical.View", !wb->check_plugin_input_available(pdef));

  // test for db.Catalog
  pdef->objectStructName(db_Catalog::static_class_name());
  ensure("db.Catalog", wb->check_plugin_input_available(pdef));

  // test for db.mysql.Catalog
  pdef->objectStructName(db_mysql_Catalog::static_class_name());
  ensure("db.mysql.Catalog", wb->check_plugin_input_available(pdef));

  // test for model.Model
  pdef->objectStructName(model_Model::static_class_name());
  ensure("model.Model", wb->check_plugin_input_available(pdef));

  // test for workbench.physical.Model
  pdef->objectStructName(workbench_physical_Model::static_class_name());
  ensure("workbench.physical.Model", wb->check_plugin_input_available(pdef));
}*/

TEST_FUNCTION(25) {
  // bug: check if deleting an object with privileges will delete the privs too
  populate_grt(*tester);

  tester->create_new_document();

  WBComponentPhysical *phys = tester->wb->get_component<WBComponentPhysical>();
  ensure_equals("initial role count", tester->get_pmodel()->catalog()->roles().count(), 5U);
  phys->add_new_role(tester->get_pmodel());
  phys->add_new_role(tester->get_pmodel());

  db_SchemaRef schema(tester->get_pmodel()->catalog()->schemata()[0]);

  phys->add_new_db_table(schema);
  phys->add_new_db_table(schema);

  ensure_equals("tables created", schema->tables().count(), 2U);
  ensure_equals("roles created", tester->get_pmodel()->catalog()->roles().count(), 2U + 5);

  // add some privs to the table
  db_RoleRef role(tester->get_pmodel()->catalog()->roles().get(5));
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

  ensure_equals("role privs", role->privileges().count(), 2U);

  // delete the 1st table
  phys->delete_db_object(table);

  ensure_equals("tables left", tester->get_pmodel()->catalog()->schemata()[0]->tables().count(), 1U);
  ensure_equals("roles left", tester->get_pmodel()->catalog()->roles().count(), 2U + 5);

  ensure_equals("privs left", role->privileges().count(), 1U);

  ensure("priv owner", role->privileges().get(0)->databaseObject() == table2);

  tester->wb->close_document();
  tester->wb->close_document_finish();
}

TEST_FUNCTION(30) {
  populate_grt(*tester);

  // bug: undo drop table will not reset TableFigure::table_figure_for_dbtable()

  db_TableRef table;

  tester->wb->open_document("data/workbench/2tables_1fk.mwb");
  workbench_DocumentRef doc = tester->wb->get_document();
  ensure("load doc", doc.is_valid());

  tester->open_all_diagrams();

  ensure_equals("loaded 1 schema", tester->get_catalog()->schemata().count(), 1U);

  std::list<db_DatabaseObjectRef> objects;
  ensure_equals("loaded model correctly", tester->get_schema()->tables().count(), 2U);
  objects.push_back(grt::find_named_object_in_list(tester->get_schema()->tables(), "table1"));
  ensure("found table", objects.front().is_valid());
  ensure_equals("found correct table", *objects.front()->name(), "table1");
  ensure_equals("diagram count", doc->physicalModels()[0]->diagrams().count(), 1U);
  tester->interactive_place_db_objects(10, 150, objects);

  tester->flush_until(2);
  ensure_equals("check table", tester->get_pview()->figures().count(), 1U);

  grt::GRT::get()->get_undo_manager()->undo();

  ensure_equals("check table gone", tester->get_pview()->figures().count(), 0U);
  tester->wb->close_document();
  tester->wb->close_document_finish();
}

TEST_FUNCTION(31) {
  delete tester;
}

END_TESTS
