/* 
 * Copyright (c) 2011, 2014, Oracle and/or its affiliates. All rights reserved.
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

// High-level testing for Workbench
// This tests WBContext, which will test the integration of all components.

#include "base/util_functions.h"

#include "wb_helpers.h"
#include "grtdb/db_helpers.h"
#include "grtdb/db_object_helpers.h"
#include "grt_test_utility.h"

#include "stub/stub_utilities.h"

using namespace wb;
using namespace base;
using namespace bec;

BEGIN_TEST_DATA_CLASS(wb_context_test)
protected:
  WBTester tester;

TEST_DATA_CONSTRUCTOR(wb_context_test)
{
  // Init datatypes and RDBMS.
  populate_grt(tester.grt, tester);

  // Modeling uses a default server version, which is not related to any server it might have
  // reverse engineered content from, nor where it was sync'ed to. So we have to mimic this here.
  std::string target_version = tester.wb->get_grt_manager()->get_app_option_string("DefaultTargetMySQLVersion");
  if (target_version.empty())
    target_version = "5.5";
  tester.get_rdbms()->version(parse_version(tester.grt, target_version));

}

END_TEST_DATA_CLASS;

TEST_MODULE(wb_context_test, "high-level tests for Workbench");

TEST_FUNCTION(5)
{
  // Test creating a new document.
  // General note: many other tests depend on this to work, so there should really be a test
  // order where more complicated tests are based on simpler ones.
  tester.wb->new_document();

  ensure("doc", tester.wb->get_document().is_valid());
  ensure("physicalModels", tester.wb->get_document()->physicalModels().count() == 1);

  ensure("Could not close document", tester.close_document());
  tester.wb->close_document_finish();
}

TEST_FUNCTION(10)
{
  // Test loading documents (saved as xml). Check if the test file is correct.
  {
    FILE *f = fopen("data/workbench/test_model_xml.mwb", "r");
    ensure("Model file does not exist", f != NULL);
    
    char buffer[100];
    buffer[0] = 0;

    fgets(buffer, sizeof(buffer), f);    
    fclose(f);
    
    ensure("Model file is not pure XML (uncompressed)", strncmp(buffer, "<?xml", 5) == 0);
  }

  ensure("Failed opening document", tester.wb->open_document("data/workbench/test_model_xml.mwb"));
  
  workbench_WorkbenchRef root(tester.wb->get_root());

  ensure("document ok", root->doc().is_valid());

  ensure_equals("physicalModels", root->doc()->physicalModels().count(), 1U);
  ensure_equals("diagrams", root->doc()->physicalModels()[0]->diagrams().count(), 1U);

  model_DiagramRef view(root->doc()->physicalModels()[0]->diagrams()[0]);
  ensure("figure count", view->figures().count() > 0);

  ensure("Could not close document", tester.close_document());
  tester.wb->close_document_finish();
}

/**
 * Test saving + loading of a document and check the canvas state.
 * This test uses by intention local testers to ensure a clean state when loading the test file.
 */
TEST_FUNCTION(15)
{ 
  WBTester *local_tester1 = new WBTester(false, Size(2970, 2100));
  
  local_tester1->wb->new_document();
  local_tester1->add_view();
  
  ensure_equals("view count after adding", local_tester1->wb->get_document()->physicalModels()[0]->diagrams().count(), 1U);
  
  local_tester1->add_table_figure("sometable", 100, 100);
  local_tester1->flush_until(2); // TODO: this is not deterministic and hence should be replaced in tests.

  ensure_equals("original phys model count",
                local_tester1->wb->get_document()->physicalModels().count(),1U);
  ensure_equals("original view count",
                local_tester1->wb->get_document()->physicalModels()[0]->diagrams().count(),1U);
  ensure_equals("original element count",
                local_tester1->wb->get_document()->physicalModels()[0]->diagrams()[0]->figures().count(),1U);
  ensure_equals("original element count in rootLayer",
                local_tester1->wb->get_document()->physicalModels()[0]->diagrams()[0]->rootLayer()->figures().count(),1U);
  local_tester1->sync_view();
  ensure("Saving document failed", local_tester1->wb->save_as("data/test1_doc.mwb"));

  // Second instance.
  WBTester *local_tester2 = new WBTester(false);
  ensure("Failed opening document test1_doc", local_tester2->wb->open_document("data/test1_doc.mwb"));

  ensure_equals("loaded phys model count",
                local_tester2->wb->get_document()->physicalModels().count(),1U);
  ensure_equals("loaded view count",
                local_tester2->wb->get_document()->physicalModels()[0]->diagrams().count(),1U);
  ensure_equals("loaded element count",
                local_tester2->wb->get_document()->physicalModels()[0]->diagrams()[0]->figures().count(),1U);
  ensure_equals("loaded element count in rootLayer",
                local_tester2->wb->get_document()->physicalModels()[0]->diagrams()[0]->rootLayer()->figures().count(),1U);

  local_tester2->open_all_diagrams();
  local_tester2->sync_view();

  grt_ensure_equals("save/load test", local_tester1->wb->get_document(), local_tester2->wb->get_document(), true);

  ensure("view created", local_tester2->last_view != 0);

  ensure("Could not close document", local_tester1->close_document());
  local_tester1->wb->close_document_finish();
  delete local_tester1;

  ensure("Could not close document", local_tester2->close_document());
  local_tester2->wb->close_document_finish();
  delete local_tester2;
}

TEST_FUNCTION(20)
{
  // bug: opening a model with selection will cause a crash
  tester.wb->new_document();

  tester.add_view();

  tester.add_table_figure("table", 10, 10);
  model_DiagramRef view(tester.get_pmodel()->diagrams()[0]);

  ensure_equals("check selection", view->selection().count(), 1U);

  tester.wb->save_as("test4.mwb");
  ensure("Could not close document", tester.close_document());
  tester.wb->close_document_finish();

  ensure("Failed opening document", tester.wb->open_document("test4.mwb"));

  view = tester.get_pmodel()->diagrams()[0];

  ensure("check loaded doc", view->figures().count()==1);
  ensure_equals("check loaded doc", view->selection().count(),1U);

  ensure("Could not close document", tester.close_document());
  tester.wb->close_document_finish();
}

TEST_FUNCTION(25)
{
  // bug: dragging related tables to the view won't create the connection

  // test dragging the table with fk 1st
  {
    WBTester tester(false); // TODO: Is it really needed to create local tester instances?
                            // Creating a tester is quite resource costly.

    ensure("Failed opening document", tester.wb->open_document("data/workbench/2tables_1fk.mwb"));
    ensure("Document is not valid", tester.wb->get_document().is_valid());
    ensure_equals("Table count does not fit", tester.get_schema()->tables().count(), 2U);
    ensure_equals("Unexpected figures", tester.get_pview()->figures().count(), 0U);
    ensure_equals("Unexpected connections", tester.get_pview()->connections().count(), 0U);

    std::list<db_DatabaseObjectRef> objects;
    objects.push_back(grt::find_named_object_in_list(tester.get_schema()->tables(), "table2"));

    tester.open_all_diagrams();
    tester.sync_view();

    tester.interactive_place_db_objects(10, 10, objects);

    ensure_equals("Wrong table count in view", tester.get_pview()->figures().count(), 1U);
    ensure_equals("Wrong connection count in view", tester.get_pview()->connections().count(), 0U);

    objects.clear();
    objects.push_back(grt::find_named_object_in_list(tester.get_schema()->tables(), "table1"));
    tester.interactive_place_db_objects(10, 150, objects);

    grt::BaseListRef figures(tester.get_pview()->figures());
    tester.flush_until(1, boost::bind(&grt::BaseListRef::count, figures), 2);
    ensure_equals("There must be 2 tables in the view now", tester.get_pview()->figures().count(), 2U);

    grt::BaseListRef connections(tester.get_pview()->connections());
    tester.flush_until(5, boost::bind(&grt::BaseListRef::count, connections), 1);

    ensure_equals("connections created", tester.get_pview()->connections().count(), 1U);

    ensure("Could not close document", tester.close_document());
    tester.wb->close_document_finish();
  }

  // test dragging the table with fk last
  {
    WBTester tester(false);

    ensure("Failed opening document", tester.wb->open_document("data/workbench/2tables_1fk.mwb"));
    ensure("load doc", tester.wb->get_document().is_valid());
    ensure_equals("contains tables", tester.get_schema()->tables().count(), 2U);
    ensure_equals("no figures", tester.get_pview()->figures().count(), 0U);
    ensure_equals("no connections", tester.get_pview()->connections().count(), 0U);

    tester.open_all_diagrams();
    tester.sync_view();

    std::list<db_DatabaseObjectRef> objects;
    objects.push_back(grt::find_named_object_in_list(tester.get_schema()->tables(), "table1"));

    tester.interactive_place_db_objects(10, 10, objects);

    ensure_equals("drop 1 table", tester.get_pview()->figures().count(), 1U);
    ensure_equals("no connections", tester.get_pview()->connections().count(), 0U);

    objects.clear();
    objects.push_back(grt::find_named_object_in_list(tester.get_schema()->tables(), "table2"));

    tester.interactive_place_db_objects(10, 150, objects);

    ensure_equals("drop 2nd table", tester.get_pview()->figures().count(), 2U);
    ensure_equals("connections created", tester.get_pview()->connections().count(), 1U);

    ensure("Could not close document", tester.close_document());
    tester.wb->close_document_finish();
  }

  // test dragging both tables
  {
    WBTester tester(false);

    ensure("Failed opening document", tester.wb->open_document("data/workbench/2tables_1fk.mwb"));
    ensure("load doc", tester.wb->get_document().is_valid());
    ensure_equals("contains tables", tester.get_schema()->tables().count(), 2U);
    ensure_equals("no figures", tester.get_pview()->figures().count(), 0U);
    ensure_equals("no connections", tester.get_pview()->connections().count(), 0U);

    tester.open_all_diagrams();
    tester.sync_view();

    std::list<db_DatabaseObjectRef> objects;
    objects.push_back(grt::find_named_object_in_list(tester.get_schema()->tables(), "table1"));
    objects.push_back(grt::find_named_object_in_list(tester.get_schema()->tables(), "table2"));

    tester.interactive_place_db_objects(10, 150, objects);

    ensure_equals("drop 2 tables", tester.get_pview()->figures().count(), 2U);
    ensure_equals("connections created", tester.get_pview()->connections().count(), 1U);

    ensure("Could not close document", tester.close_document());
    tester.wb->close_document_finish();
  }

  // test with a recursive relationship
  {
   WBTester tester(false);
   populate_grt(tester.grt, tester);

    ensure("Failed opening document", tester.wb->open_document("data/workbench/2tables_1fk.mwb"));
    ensure("load doc", tester.wb->get_document().is_valid());
    ensure_equals("contains tables", tester.get_schema()->tables().count(), 2U);
    ensure_equals("no figures", tester.get_pview()->figures().count(), 0U);
    ensure_equals("no connections", tester.get_pview()->connections().count(), 0U);

    tester.open_all_diagrams();
    tester.sync_view();

    db_TableRef table(grt::find_named_object_in_list(tester.get_schema()->tables(), "table1"));

    ensure("no fk", table->foreignKeys().count()==0);

    bec::TableHelper::create_foreign_key_to_table(table, table, true, true, true, true,
      tester.get_rdbms(), grt::DictRef(tester.grt), grt::DictRef(tester.grt));

    std::list<db_DatabaseObjectRef> objects;
    objects.push_back(table);
    tester.interactive_place_db_objects(10, 150, objects);

    ensure_equals("drop 1 table", tester.get_pview()->figures().count(), 1U);
    ensure_equals("connections created", tester.get_pview()->connections().count(), 1U);

    ensure("Could not close document", tester.close_document());
    tester.wb->close_document_finish();
  }
}

TEST_FUNCTION(30)
{
  // bug: deleting tables with relationship won't delete the connection

  ensure("Failed opening document", tester.wb->open_document("data/workbench/2tables_1fk.mwb"));
  ensure("load doc", tester.wb->get_document().is_valid());
  ensure_equals("contains tables", tester.get_schema()->tables().count(), 2U);
  ensure_equals("no figures", tester.get_pview()->figures().count(), 0U);
  ensure_equals("no connections", tester.get_pview()->connections().count(), 0U);

  tester.open_all_diagrams();
  tester.sync_view();

  std::list<db_DatabaseObjectRef> objects;
  objects.push_back(grt::find_named_object_in_list(tester.get_schema()->tables(), "table1"));
  objects.push_back(grt::find_named_object_in_list(tester.get_schema()->tables(), "table2"));

  tester.interactive_place_db_objects(10, 150, objects);

  ensure_equals("place 2 tables", tester.get_pview()->figures().count(), 2U);
  ensure_equals("connections created", tester.get_pview()->connections().count(), 1U);

  // Delete 1 of the tables and see if connection is gone.

  model_FigureRef figure= tester.get_pview()->figures()[0];

  ensure("root global", tester.wb->get_root()->is_global());
  ensure("doc is global", tester.get_pview()->is_global());

  ensure("figure is global", figure->is_global());
    
  tester.wb->get_model_context()->delete_object(figure);

  ensure_equals("delete 1 table", tester.get_pview()->figures().count(), 1U);
  ensure_equals("connection count after expected auto-delete", tester.get_pview()->connections().count(), 0U);

  // undo
  tester.grt->get_undo_manager()->undo();

  ensure_equals("undo delete table", tester.get_pview()->figures().count(), 2U);
  ensure_equals("undo delete table (connection)", tester.get_pview()->connections().count(), 1U);

  ensure("Could not close document", tester.close_document());
  tester.wb->close_document_finish();
}

TEST_FUNCTION(35)
{
  // check if figure/dbobject deletion works
  WBFrontendCallbacks callbacks;

  //TODO: Investigate what was the purpose of this and if it continues needed
  //callbacks.confirm_action= boost::bind(&delegated_confirm_action, &result);

//  WBTester tester(Size(800, 800), callbacks);
//  WBTester& tester = wb;

  tester.wb->new_document();
  tester.add_view();

  tester.add_table_figure("table", 10, 10);

  ensure_equals("table object", tester.get_schema()->tables().count(), 1U);
  ensure_equals("table figure", tester.get_pview()->figures().count(), 1U);

  

  // delete table with dbobject (1)
  tester.wb->get_model_context()->delete_object(tester.get_pview()->figures()[0]);

  ensure_equals("table object", tester.get_schema()->tables().count(), 0U);
  ensure_equals("table figure", tester.get_pview()->figures().count(), 0U);

  tester.grt->get_undo_manager()->undo();

  ensure_equals("table object", tester.get_schema()->tables().count(), 1U);
  ensure_equals("table figure", tester.get_pview()->figures().count(), 1U);

  // delete table without dbobject (0)
  tester.wb->get_model_context()->remove_figure(tester.get_pview()->figures()[0]);

  ensure_equals("table object", tester.get_schema()->tables().count(), 1U);
  ensure_equals("table figure", tester.get_pview()->figures().count(), 0U);

  tester.grt->get_undo_manager()->undo();

  ensure_equals("table object", tester.get_schema()->tables().count(), 1U);
  ensure_equals("table figure", tester.get_pview()->figures().count(), 1U);

  ensure("Could not close document", tester.close_document());
  tester.wb->close_document_finish();
}

TEST_FUNCTION(40)
{
  // make sure connections are deleted and recreated with undo/redo

  tester.wb->new_document();
  tester.add_view();

  grt::UndoManager *um= tester.grt->get_undo_manager();

  db_mysql_TableRef table1= tester.add_table_figure("table1", 10, 10);
  db_mysql_ColumnRef column1(tester.grt);
  column1->owner(table1);
  column1->name("pk");
  db_mysql_TableRef table2= tester.add_table_figure("table2", 100, 10);

  db_mysql_ColumnRef column2(tester.grt);
  column2->owner(table2);
  column2->name("pk");

  ensure_equals("table object", tester.get_schema()->tables().count(), 2U);
  ensure_equals("table figure", tester.get_pview()->figures().count(), 2U);

  tester.grt->start_tracking_changes();
  table1->addPrimaryKeyColumn(column1);
  table2->addPrimaryKeyColumn(column2);
  tester.grt->stop_tracking_changes();

  // create 1:n rel and test undo

  grt::AutoUndo undo(tester.grt);
  bec::TableHelper::create_foreign_key_to_table(table2, table1, true, true, true, true,
                                                tester.get_rdbms(),
                                                grt::DictRef(tester.grt),
                                                grt::DictRef(tester.grt));
  undo.end("create fk");

  ensure_equals("table fks", table2->foreignKeys().count(), 1U);

  grt::BaseListRef connections(tester.get_pview()->connections());

  tester.flush_until(2, boost::bind(&BaseListRef::count, connections), 1);

  ensure_equals("connection created", connections.count(), 1U);

  um->undo();

  tester.flush_until(2, boost::bind(&BaseListRef::count, connections), 0);

  ensure_equals("connection undone", connections.count(), 0U);

  um->redo();

  tester.flush_until(2, boost::bind(&BaseListRef::count, connections), 1);

  ensure_equals("connection redone", connections.count(), 1U);

  um->undo();
  tester.flush_until(2, boost::bind(&BaseListRef::count, connections), 0);

  ensure("Could not close document", tester.close_document());
  tester.wb->close_document_finish();
}

//--------------------------------------------------------------------------------

TEST_FUNCTION(45)
{
  // bug: copy/paste object across schemas are not updating the owner

  tester.wb->new_document();

  WBComponentPhysical *ph= tester.wb->get_component<WBComponentPhysical>();
  ensure("physical component", ph != 0);

  workbench_physical_ModelRef model(tester.get_pmodel());

  ph->add_new_db_schema(model);

  ensure_equals("schemas", model->catalog()->schemata().count(), 2U);

  db_SchemaRef srcschema(model->catalog()->schemata()[0]);
  db_SchemaRef tarschema(model->catalog()->schemata()[1]);

  ensure("schemas !same", srcschema != tarschema);

  // add stuff

  ph->add_new_db_table(srcschema);
  ph->add_new_db_view(srcschema);
  ph->add_new_db_routine(srcschema);
  ph->add_new_db_routine_group(srcschema);

  grt::CopyContext context(tester.grt);
  ph->clone_db_object_to_schema(tarschema, srcschema->tables()[0], context);
  ph->clone_db_object_to_schema(tarschema, srcschema->views()[0], context);
  ph->clone_db_object_to_schema(tarschema, srcschema->routines()[0], context);
  ph->clone_db_object_to_schema(tarschema, srcschema->routineGroups()[0], context);

  ensure_equals("table copied", tarschema->tables().count(), 1U);
  ensure_equals("view copied", tarschema->views().count(), 1U);
  ensure_equals("routine copied", tarschema->routines().count(), 1U);
  ensure_equals("routinegroup copied", tarschema->routineGroups().count(), 1U);

  ensure("table owner", tarschema->tables()[0]->owner() == tarschema);
  ensure("view owner", tarschema->views()[0]->owner() == tarschema);
  ensure("routine owner", tarschema->routines()[0]->owner() == tarschema);
  ensure("routinegroup owner", tarschema->routineGroups()[0]->owner() == tarschema);

  ensure("Could not close document", tester.close_document());
  tester.wb->close_document_finish();
}


static mforms::DialogResult message_other_callback()
{
  return mforms::ResultOther;
}

TEST_FUNCTION(50)
{
  // bug: deleting an identifying relationship doesn't delete indexes.

  ensure("Failed opening document", tester.wb->open_document("data/workbench/identifying_relationship.mwb"));
  tester.open_all_diagrams();
  tester.sync_view();

  workbench_DocumentRef doc(tester.wb->get_document());

  db_TableRef table1(doc->physicalModels()[0]->catalog()->schemata()[0]->tables()[0]);
  db_TableRef table2(doc->physicalModels()[0]->catalog()->schemata()[0]->tables()[1]);
  
  ensure_equals("check index count", table1->indices().count(), 2U);
  ensure_equals("connections", doc->physicalModels()[0]->diagrams()[0]->connections().count(), 1U);

  workbench_physical_ConnectionRef conn= workbench_physical_ConnectionRef::cast_from(doc->physicalModels()[0]->diagrams()[0]->connections().get(0));
  ensure("connection fk", conn->foreignKey().is_valid());
  
  // delete the connection

  mforms::stub::UtilitiesWrapper::set_message_callback(message_other_callback);
  tester.wb->get_model_context()->delete_object(doc->physicalModels()[0]->diagrams()[0]->connections()[0]);

  ensure_equals("connections", doc->physicalModels()[0]->diagrams()[0]->connections().count(), 0U);
  ensure_equals("check index count", table1->indices().count(), 1U);

  ensure("Could not close document", tester.close_document());
  tester.wb->close_document_finish();
}

TEST_FUNCTION(55)
{
  ensure("Failed opening document", tester.wb->open_document("data/workbench/identifying_relationship.mwb"));
  tester.open_all_diagrams();
  tester.sync_view();

  workbench_DocumentRef doc(tester.wb->get_document());

  db_TableRef table1(doc->physicalModels()[0]->catalog()->schemata()[0]->tables()[0]);
  ensure_equals("check index count", table1->indices().count(), 2U);
  ensure("pk", table1->primaryKey().is_valid());
  
  // Delete column.
  table1->removeColumn(table1->columns()[0]);
  //bec::TableHelper::remove_column(table1, table1->columns()[0]);

  ensure("Primary key should no longer exist", !table1->primaryKey().is_valid());

  ensure("Could not close document", tester.close_document());
  tester.wb->close_document_finish();
}

TEST_FUNCTION(60)
{
  // test whether relationship is created when a FK is added and its details
  // are set in steps (non-atomically)

  tester.wb->new_document();
  tester.add_view();

  db_mysql_TableRef table1= tester.add_table_figure("table1", 10, 10);
  db_mysql_ColumnRef column1(tester.grt);
  column1->owner(table1);
  column1->name("pk");
  db_mysql_TableRef table2= tester.add_table_figure("table2", 100, 10);

  db_mysql_ColumnRef column2(tester.grt);
  column2->owner(table2);
  column2->name("fkcol");

  ensure_equals("table object", tester.get_schema()->tables().count(), 2U);
  ensure_equals("table figure", tester.get_pview()->figures().count(), 2U);

  tester.grt->start_tracking_changes();
  table1->addPrimaryKeyColumn(column1);
  tester.grt->stop_tracking_changes();
  
  db_mysql_ForeignKeyRef fk(tester.grt);
  fk->owner(table2);
  fk->name("fk");
  
  table2->foreignKeys().insert(fk);
  tester.flush_until(2);

  fk->columns().insert(column2);
  fk->referencedColumns().insert(column1);
  fk->referencedTable(table1);

  grt::BaseListRef connections(tester.get_pview()->connections());
  tester.flush_until(2, boost::bind(&grt::BaseListRef::count, connections), 1);

  ensure_equals("rel count", tester.get_pview()->connections().count(), 1U);

  ensure("Could not close document", tester.close_document());
  tester.wb->close_document_finish();
}

TEST_FUNCTION(65)
{
  // test if new view has layer size set 

  tester.wb->new_document();
  tester.add_view();

  ensure_equals("rootLayer width", *tester.get_pview()->rootLayer()->width(), tester.last_view->get_total_view_size().width);
  ensure_equals("rootLayer height", *tester.get_pview()->rootLayer()->height(), tester.last_view->get_total_view_size().height);

  ensure("Could not close document", tester.close_document());
  tester.wb->close_document_finish();
}

TEST_FUNCTION(70)
{ // test creation of a simple model with a relationship
  
  tester.wb->new_document();
  tester.add_view();

  tester.add_table_figure("table1", 100, 100);
  tester.add_table_figure("table2", 300, 100);
  
  tester.export_png("test20_dump.png");

  ensure("Could not close document", tester.close_document());
  tester.wb->close_document_finish();
}

TEST_FUNCTION(75)
{
  // bug: loading a model with layers and then hitting new crashes

  ensure("Failed opening document", tester.wb->open_document("data/workbench/2tables_conn_layer.mwb"));
  ensure("Could not close document", tester.close_document());
  tester.wb->close_document_finish();
  tester.wb->new_document();

  while (dynamic_cast<ModelDiagramForm*>(tester.wbui->get_active_main_form())!=0)
    tester.wb->flush_idle_tasks();

  ensure("Could not close document", tester.close_document());
  tester.wb->close_document_finish();
}


TEST_FUNCTION(80)
{
  // bug: loading model twice causes bad internal state in GUI
  
  ensure("Failed opening document", tester.wb->open_document("data/workbench/sakila.mwb"));
  ensure("Could not close document", tester.close_document());
  tester.wb->close_document_finish();

  ensure("Failed opening document", tester.wb->open_document("data/workbench/sakila.mwb"));
  ensure("Could not close document", tester.close_document());
  tester.wb->close_document_finish();
}

TEST_FUNCTION(85)
{
  // bug: loading a model with selection won't reselect the items in the canvas
  ensure("Failed opening document", tester.wb->open_document("data/workbench/selected_table.mwb"));
  tester.open_all_diagrams();
  tester.sync_view();

  tester.flush_while(3, boost::bind(&mdc::Selection::empty, tester.last_view->get_selection()));

  ensure("has selection in canvas", !tester.last_view->get_selection()->empty());

  ensure("Could not close document", tester.close_document());
  tester.wb->close_document_finish();
}

static void set_note_content(grt::GRT *grt, GrtStoredNoteRef note, const std::string &text)
{
  grt::Module *module= grt->get_module("Workbench");
  if (!module)
    throw std::runtime_error("Workbench module not found");

  note->lastChangeDate(base::fmttime());

  grt::BaseListRef args(grt);

  args.ginsert(note->filename());
  args.ginsert(grt::StringRef(text));

  module->call_function("setAttachedFileContents", args);
}

static std::string get_note_content(grt::GRT *grt, const GrtStoredNoteRef &note)
{
  grt::Module *module= grt->get_module("Workbench");
  if (!module)
    throw std::runtime_error("Workbench module not found");

  grt::BaseListRef args(grt);

  args.ginsert(note->filename());

  return *grt::StringRef::cast_from(module->call_function("getAttachedFileContents", args));
}

TEST_FUNCTION(90)
{
  // check stored note management
  tester.wb->new_document();

  // add a note
  WBComponentPhysical *ph= tester.wb->get_component<WBComponentPhysical>();

  ph->add_new_stored_note(tester.get_pmodel());

  tester.flush_until(0.5);

  // check if created
  ensure_equals("note created", tester.get_pmodel()->notes().count(), 1U);
  ensure("note created with file", tester.get_pmodel()->notes().get(0)->filename() != "");

  grt::GRT *grt= tester.wb->get_grt();

  // edit the note like an editor would
  set_note_content(grt, tester.get_pmodel()->notes().get(0), "hello world");
  std::string filename= tester.get_pmodel()->notes().get(0)->filename();

  tester.wb->save_as("notetest.mwb");
  ensure("Could not close document", tester.close_document());
  tester.wb->close_document_finish();

  // check if stored on disk
  ensure("Failed opening document", tester.wb->open_document("notetest.mwb"));

  ensure_equals("note still in model", tester.get_pmodel()->notes().count(), 1U);
  ensure_equals("note filename", *tester.get_pmodel()->notes().get(0)->filename(), filename);
  
  // get note contents as an editor
  std::string text= get_note_content(grt, tester.get_pmodel()->notes().get(0));
  ensure_equals("note contents", text, "hello world");

  ensure("Could not close document", tester.close_document());
  tester.wb->close_document_finish();
}



TEST_FUNCTION(95)
{
  // check undo for stored notes

  // create note
  tester.wb->new_document();

  // add a note
  WBComponentPhysical *ph= tester.wb->get_component<WBComponentPhysical>();

  ph->add_new_stored_note(tester.get_pmodel());

  tester.flush_until(0.5);

  // check if created
  ensure_equals("note created", tester.get_pmodel()->notes().count(), 1U);
  ensure("note created with file", tester.get_pmodel()->notes().get(0)->filename()!="");

  std::string fname= tester.get_pmodel()->notes().get(0)->filename();

  set_note_content(tester.grt, tester.get_pmodel()->notes().get(0), "some text");
  
  ensure_equals("note content set", get_note_content(tester.grt, tester.get_pmodel()->notes().get(0)), "some text");

  // delete note and undo
  tester.grt->get_undo_manager()->add_undo(new grt::UndoListRemoveAction(tester.get_pmodel()->notes(), 0));
  tester.get_pmodel()->notes().remove(0);
  tester.grt->get_undo_manager()->undo();

  ensure_equals("note undeleted", tester.get_pmodel()->notes().count(), 1U);
  ensure_equals("note file", *tester.get_pmodel()->notes().get(0)->filename(), fname);
  ensure_equals("note content", get_note_content(tester.grt, tester.get_pmodel()->notes().get(0)), "some text");


  for (int i= 0; i < 10; i++)
  {
    tester.grt->get_undo_manager()->redo();
    ensure_equals("note re-deleted", tester.get_pmodel()->notes().count(), 0U);
    tester.grt->get_undo_manager()->undo();
    ensure_equals("note un-deleted", tester.get_pmodel()->notes().count(), 1U);
  }
  ensure_equals("note file", *tester.get_pmodel()->notes().get(0)->filename(), fname);
  ensure_equals("note content", get_note_content(tester.grt, tester.get_pmodel()->notes().get(0)), "some text");

  // undo (should undo create note)

  ensure("Could not close document", tester.close_document());
  tester.wb->close_document_finish();
}

END_TESTS

  
  
  
