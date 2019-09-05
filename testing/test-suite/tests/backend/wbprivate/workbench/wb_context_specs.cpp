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

// High-level testing for Workbench
// This tests WBContext, which will test the integration of all components.

#include "base/util_functions.h"

#include "grtdb/db_helpers.h"
#include "grtdb/db_object_helpers.h"

#include "stub/stub_utilities.h"

#include "casmine.h"
#include "wb_test_helpers.h"
#include "grt_test_helpers.h"

using namespace wb;
using namespace base;
using namespace bec;

namespace {

$ModuleEnvironment() {};

//----------------------------------------------------------------------------------------------------------------------

static mforms::DialogResult messageOtherCallback() {
  return mforms::ResultOther;
}

//----------------------------------------------------------------------------------------------------------------------

static void set_note_content(GrtStoredNoteRef note, const std::string &text) {
  grt::Module *module = grt::GRT::get()->get_module("Workbench");
  if (!module)
    throw std::runtime_error("Workbench module not found");

  note->lastChangeDate(base::fmttime());

  grt::BaseListRef args(true);

  args.ginsert(note->filename());
  args.ginsert(grt::StringRef(text));

  module->call_function("setAttachedFileContents", args);
}

//----------------------------------------------------------------------------------------------------------------------

static std::string get_note_content(const GrtStoredNoteRef &note) {
  grt::Module *module = grt::GRT::get()->get_module("Workbench");
  if (!module)
    throw std::runtime_error("Workbench module not found");

  grt::BaseListRef args(true);

  args.ginsert(note->filename());

  return *grt::StringRef::cast_from(module->call_function("getAttachedFileContents", args));
}

//----------------------------------------------------------------------------------------------------------------------

$TestData {
  std::unique_ptr<WorkbenchTester> tester;
  std::string dataDir = casmine::CasmineContext::get()->tmpDataDir();
  std::string outputDir = casmine::CasmineContext::get()->outputDir();
};

$describe("Workbench model document integration tests") {
  $beforeAll([this]() {
    
    data->tester.reset(new WorkbenchTester());
    data->tester->initializeRuntime();

    // Modeling uses a default server version, which is not related to any server it might have
    // reverse engineered content from, nor where it was sync'ed to. So we have to mimic this here.
    std::string target_version = bec::GRTManager::get()->get_app_option_string("DefaultTargetMySQLVersion");
    if (target_version.empty())
      target_version = "5.5.49";
    data->tester->getRdbms()->version(parse_version(target_version));
  });

  $it("Test creating new document", [this]() {
    // Test creating a new document.
    // General note: many other tests depend on this to work, so there should really be a test
    // order where more complicated tests are based on simpler ones.
    data->tester->wb->new_document();

    $expect(data->tester->wb->get_document().is_valid()).toBeTrue();
    $expect(data->tester->wb->get_document()->physicalModels().count()).toBe(1U);

    $expect(data->tester->closeDocument()).toBeTrue();
    data->tester->wb->close_document_finish();
  });

  $it("Test loading documents", [this]() {
    $expect(data->tester->wb->open_document(data->dataDir + "/workbench/test_model_xml.mwb")).toBeTrue();

    workbench_WorkbenchRef root(data->tester->wb->get_root());

    $expect(root->doc().is_valid()).toBeTrue();

    $expect(root->doc()->physicalModels().count()).toBe(1U);
    $expect(root->doc()->physicalModels()[0]->diagrams().count()).toBe(1U);

    model_DiagramRef view(root->doc()->physicalModels()[0]->diagrams()[0]);
    $expect(view->figures().count() > 0).toBeTrue();

    $expect(data->tester->closeDocument()).toBeTrue();
    data->tester->wb->close_document_finish();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Saving and loading of a document", [this]() {
    data->tester->wb->new_document();
    data->tester->addView();

    $expect(data->tester->wb->get_document()->physicalModels()[0]->diagrams().count()).toBe(1U);

    data->tester->addTableFigure("sometable", 100, 100);
    data->tester->flushUntil(2); // TODO: this is not deterministic and hence should be replaced in tests.

    $expect(data->tester->wb->get_document()->physicalModels().count()).toBe(1U);
    $expect(data->tester->wb->get_document()->physicalModels()[0]->diagrams().count()).toBe(1U);
    $expect(data->tester->wb->get_document()->physicalModels()[0]->diagrams()[0]->figures().count()).toBe(1U);
    $expect(data->tester->wb->get_document()->physicalModels()[0]->diagrams()[0]->rootLayer()->figures().count()).toBe(1U);
    data->tester->syncView();
    $expect(data->tester->wb->save_as(data->outputDir + "/test1_doc.mwb")).toBeTrue();

    $expect(data->tester->closeDocument()).toBeTrue();
    data->tester->wb->close_document_finish();

    $expect(data->tester->wb->open_document(data->outputDir + "/test1_doc.mwb")).toBeTrue();

    $expect(data->tester->wb->get_document()->physicalModels().count()).toBe(1U);
    $expect(data->tester->wb->get_document()->physicalModels()[0]->diagrams().count()).toBe(1U);
    $expect(data->tester->wb->get_document()->physicalModels()[0]->diagrams()[0]->figures().count()).toBe(1U);
    $expect(data->tester->wb->get_document()->physicalModels()[0]->diagrams()[0]->rootLayer()->figures().count()).toBe(1U);

    data->tester->openAllDiagrams();
    data->tester->syncView();

    casmine::deepCompareGrtValues("save/load test", data->tester->wb->get_document(), data->tester->wb->get_document(), true);

    $expect(data->tester->lastView != 0).toBeTrue();

    $expect(data->tester->closeDocument()).toBeTrue();
    data->tester->wb->close_document_finish();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Bug: opening a model with selection will cause a crash", [this]() {
    data->tester->wb->new_document();

    data->tester->addView();

    data->tester->addTableFigure("table", 10, 10);
    model_DiagramRef view(data->tester->getPmodel()->diagrams()[0]);

    $expect(view->selection().count()).toBe(1U);

    data->tester->wb->save_as(data->outputDir + "/test4.mwb");
    $expect(data->tester->closeDocument()).toBeTrue();
    data->tester->wb->close_document_finish();

    $expect(data->tester->wb->open_document(data->outputDir + "/test4.mwb")).toBeTrue();

    view = data->tester->getPmodel()->diagrams()[0];

    $expect(view->figures().count() == 1).toBeTrue();
    $expect(view->selection().count()).toBe(1U);

    $expect(data->tester->closeDocument()).toBeTrue();
    data->tester->wb->close_document_finish();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Bug: dragging related tables to the view won't create the connection", [this]() {
    // test dragging the table with fk 1st
    {
      $expect(data->tester->wb->open_document(data->dataDir + "/workbench/2tables_1fk.mwb")).toBeTrue();
      $expect(data->tester->wb->get_document().is_valid()).toBeTrue();
      $expect(data->tester->getSchema()->tables().count()).toBe(2U);
      $expect(data->tester->getPview()->figures().count()).toBe(0U);
      $expect(data->tester->getPview()->connections().count()).toBe(0U);

      std::list<db_DatabaseObjectRef> objects;
      objects.push_back(grt::find_named_object_in_list(data->tester->getSchema()->tables(), "table2"));

      data->tester->openAllDiagrams();
      data->tester->syncView();

      data->tester->interactivePlaceDbObjects(10, 10, objects);

      $expect(data->tester->getPview()->figures().count()).toBe(1U);
      $expect(data->tester->getPview()->connections().count()).toBe(0U);

      objects.clear();
      objects.push_back(grt::find_named_object_in_list(data->tester->getSchema()->tables(), "table1"));
      data->tester->interactivePlaceDbObjects(10, 150, objects);

      grt::BaseListRef figures(data->tester->getPview()->figures());
      data->tester->flushUntil(1, std::bind(&grt::BaseListRef::count, figures), 2);
      $expect(data->tester->getPview()->figures().count()).toBe(2U);

      grt::BaseListRef connections(data->tester->getPview()->connections());
      data->tester->flushUntil(5, std::bind(&grt::BaseListRef::count, connections), 1);

      $expect(data->tester->getPview()->connections().count()).toBe(1U);

      $expect(data->tester->closeDocument()).toBeTrue();
      data->tester->wb->close_document_finish();
    }

    // test dragging the table with fk last
    {
      $expect(data->tester->wb->open_document(data->dataDir + "/workbench/2tables_1fk.mwb")).toBeTrue();
      $expect(data->tester->wb->get_document().is_valid()).toBeTrue();
      $expect(data->tester->getSchema()->tables().count()).toBe(2U);
      $expect(data->tester->getPview()->figures().count()).toBe(0U);
      $expect(data->tester->getPview()->connections().count()).toBe(0U);

      data->tester->openAllDiagrams();
      data->tester->syncView();

      std::list<db_DatabaseObjectRef> objects;
      objects.push_back(grt::find_named_object_in_list(data->tester->getSchema()->tables(), "table1"));

      data->tester->interactivePlaceDbObjects(10, 10, objects);

      $expect(data->tester->getPview()->figures().count()).toBe(1U);
      $expect(data->tester->getPview()->connections().count()).toBe(0U);

      objects.clear();
      objects.push_back(grt::find_named_object_in_list(data->tester->getSchema()->tables(), "table2"));

      data->tester->interactivePlaceDbObjects(10, 150, objects);

      $expect(data->tester->getPview()->figures().count()).toBe(2U);
      $expect(data->tester->getPview()->connections().count()).toBe(1U);

      $expect(data->tester->closeDocument()).toBeTrue();
      data->tester->wb->close_document_finish();
    }

    // test dragging both tables
    {
      $expect(data->tester->wb->open_document(data->dataDir + "/workbench/2tables_1fk.mwb")).toBeTrue();
      $expect(data->tester->wb->get_document().is_valid()).toBeTrue();
      $expect(data->tester->getSchema()->tables().count()).toBe(2U);
      $expect(data->tester->getPview()->figures().count()).toBe(0U);
      $expect(data->tester->getPview()->connections().count()).toBe(0U);

      data->tester->openAllDiagrams();
      data->tester->syncView();

      std::list<db_DatabaseObjectRef> objects;
      objects.push_back(grt::find_named_object_in_list(data->tester->getSchema()->tables(), "table1"));
      objects.push_back(grt::find_named_object_in_list(data->tester->getSchema()->tables(), "table2"));

      data->tester->interactivePlaceDbObjects(10, 150, objects);

      $expect(data->tester->getPview()->figures().count()).toBe(2U);
      $expect(data->tester->getPview()->connections().count()).toBe(1U);

      $expect(data->tester->closeDocument()).toBeTrue();
      data->tester->wb->close_document_finish();
    }

    // test with a recursive relationship
    {
      $expect(data->tester->wb->open_document(data->dataDir + "/workbench/2tables_1fk.mwb")).toBeTrue();
      $expect(data->tester->wb->get_document().is_valid()).toBeTrue();
      $expect(data->tester->getSchema()->tables().count()).toBe(2U);
      $expect(data->tester->getPview()->figures().count()).toBe(0U);
      $expect(data->tester->getPview()->connections().count()).toBe(0U);

      data->tester->openAllDiagrams();
      data->tester->syncView();

      db_TableRef table(grt::find_named_object_in_list(data->tester->getSchema()->tables(), "table1"));

      $expect(table->foreignKeys().count() == 0).toBeTrue();

      bec::TableHelper::create_foreign_key_to_table(table, table, true, true, true, true, data->tester->getRdbms(),
                                                    grt::DictRef(true), grt::DictRef(true));

      std::list<db_DatabaseObjectRef> objects;
      objects.push_back(table);
      data->tester->interactivePlaceDbObjects(10, 150, objects);

      $expect(data->tester->getPview()->figures().count()).toBe(1U);
      $expect(data->tester->getPview()->connections().count()).toBe(1U);

      $expect(data->tester->closeDocument()).toBeTrue();
      data->tester->wb->close_document_finish();
    }
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Bug: deleting tables with relationship won't delete the connection", [this]() {
    $expect(data->tester->wb->open_document(data->dataDir + "/workbench/2tables_1fk.mwb")).toBeTrue();
    $expect(data->tester->wb->get_document().is_valid()).toBeTrue();
    $expect(data->tester->getSchema()->tables().count()).toBe(2U);
    $expect(data->tester->getPview()->figures().count()).toBe(0U);
    $expect(data->tester->getPview()->connections().count()).toBe(0U);

    data->tester->openAllDiagrams();
    data->tester->syncView();

    std::list<db_DatabaseObjectRef> objects;
    objects.push_back(grt::find_named_object_in_list(data->tester->getSchema()->tables(), "table1"));
    objects.push_back(grt::find_named_object_in_list(data->tester->getSchema()->tables(), "table2"));

    data->tester->interactivePlaceDbObjects(10, 150, objects);

    $expect(data->tester->getPview()->figures().count()).toBe(2U);
    $expect(data->tester->getPview()->connections().count()).toBe(1U);

    // Delete 1 of the tables and see if connection is gone.

    model_FigureRef figure = data->tester->getPview()->figures()[0];

    $expect(data->tester->wb->get_root()->is_global()).toBeTrue();
    $expect(data->tester->getPview()->is_global()).toBeTrue();

    $expect(figure->is_global()).toBeTrue();

    data->tester->wb->get_model_context()->delete_object(figure);

    $expect(data->tester->getPview()->figures().count()).toBe(1U);
    $expect(data->tester->getPview()->connections().count()).toBe(0U);

    // undo
    grt::GRT::get()->get_undo_manager()->undo();

    $expect(data->tester->getPview()->figures().count()).toBe(2U);
    $expect(data->tester->getPview()->connections().count()).toBe(1U);

    $expect(data->tester->closeDocument()).toBeTrue();
    data->tester->wb->close_document_finish();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Deleting table with and without dbobject", [this]() {
    data->tester->wb->new_document();
    data->tester->addView();

    data->tester->addTableFigure("table", 10, 10);

    $expect(data->tester->getSchema()->tables().count()).toBe(1U);
    $expect(data->tester->getPview()->figures().count()).toBe(1U);

    // delete table with dbobject (1)
    data->tester->wb->get_model_context()->delete_object(data->tester->getPview()->figures()[0]);

    $expect(data->tester->getSchema()->tables().count()).toBe(0U);
    $expect(data->tester->getPview()->figures().count()).toBe(0U);

    grt::GRT::get()->get_undo_manager()->undo();

    $expect(data->tester->getSchema()->tables().count()).toBe(1U);
    $expect(data->tester->getPview()->figures().count()).toBe(1U);

    // delete table without dbobject (0)
    data->tester->wb->get_model_context()->remove_figure(data->tester->getPview()->figures()[0]);

    $expect(data->tester->getSchema()->tables().count()).toBe(1U);
    $expect(data->tester->getPview()->figures().count()).toBe(0U);

    grt::GRT::get()->get_undo_manager()->undo();

    $expect(data->tester->getSchema()->tables().count()).toBe(1U);
    $expect(data->tester->getPview()->figures().count()).toBe(1U);

    $expect(data->tester->closeDocument()).toBeTrue();
    data->tester->wb->close_document_finish();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Make sure connections are deleted and recreated with undo/redo", [this]() {
    data->tester->wb->new_document();
    data->tester->addView();

    grt::UndoManager *um = grt::GRT::get()->get_undo_manager();

    db_mysql_TableRef table1 = data->tester->addTableFigure("table1", 10, 10);
    db_mysql_ColumnRef column1(grt::Initialized);
    column1->owner(table1);
    column1->name("pk");
    db_mysql_TableRef table2 = data->tester->addTableFigure("table2", 100, 10);

    db_mysql_ColumnRef column2(grt::Initialized);
    column2->owner(table2);
    column2->name("pk");

    $expect(data->tester->getSchema()->tables().count()).toBe(2U);
    $expect(data->tester->getPview()->figures().count()).toBe(2U);

    grt::GRT::get()->start_tracking_changes();
    table1->addPrimaryKeyColumn(column1);
    table2->addPrimaryKeyColumn(column2);
    grt::GRT::get()->stop_tracking_changes();

    // create 1:n rel and test undo

    grt::AutoUndo undo;
    bec::TableHelper::create_foreign_key_to_table(table2, table1, true, true, true, true, data->tester->getRdbms(),
                                                  grt::DictRef(true), grt::DictRef(true));
    undo.end("create fk");

    $expect(table2->foreignKeys().count()).toBe(1U);

    grt::BaseListRef connections(data->tester->getPview()->connections());

    data->tester->flushUntil(2, std::bind(&grt::BaseListRef::count, connections), 1);

    $expect(connections.count()).toBe(1U);

    um->undo();

    data->tester->flushUntil(2, std::bind(&grt::BaseListRef::count, connections), 0);

    $expect(connections.count()).toBe(0U);

    um->redo();

    data->tester->flushUntil(2, std::bind(&grt::BaseListRef::count, connections), 1);

    $expect(connections.count()).toBe(1U);

    um->undo();
    data->tester->flushUntil(2, std::bind(&grt::BaseListRef::count, connections), 0);

    $expect(data->tester->closeDocument()).toBeTrue();
    data->tester->wb->close_document_finish();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Bug: copy/paste object across schemas are not updating the owner", [this]() {
    data->tester->wb->new_document();

    WBComponentPhysical *ph = data->tester->wb->get_component<WBComponentPhysical>();
    $expect(ph).Not.toBeNull();

    workbench_physical_ModelRef model(data->tester->getPmodel());

    ph->add_new_db_schema(model);

    $expect(model->catalog()->schemata().count()).toBe(2U);

    db_SchemaRef srcschema(model->catalog()->schemata()[0]);
    db_SchemaRef tarschema(model->catalog()->schemata()[1]);

    $expect(srcschema).Not.toEqual(tarschema);

    // add stuff

    ph->add_new_db_table(srcschema);
    ph->add_new_db_view(srcschema);
    ph->add_new_db_routine(srcschema);
    ph->add_new_db_routine_group(srcschema);

    grt::CopyContext context;
    ph->clone_db_object_to_schema(tarschema, srcschema->tables()[0], context);
    ph->clone_db_object_to_schema(tarschema, srcschema->views()[0], context);
    ph->clone_db_object_to_schema(tarschema, srcschema->routines()[0], context);
    ph->clone_db_object_to_schema(tarschema, srcschema->routineGroups()[0], context);

    $expect(tarschema->tables().count()).toBe(1U);
    $expect(tarschema->views().count()).toEqual(1U);
    $expect(tarschema->routines().count()).toEqual(1U);
    $expect(tarschema->routineGroups().count()).toEqual(1U);

    $expect(tarschema->tables()[0]->owner()).toEqual(tarschema);
    $expect(tarschema->views()[0]->owner()).toEqual(tarschema);
    $expect(tarschema->routines()[0]->owner()).toEqual(tarschema);
    $expect(tarschema->routineGroups()[0]->owner()).toEqual(tarschema);

    $expect(data->tester->closeDocument()).toBeTrue();
    data->tester->wb->close_document_finish();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Bug: deleting an identifying relationship doesn't delete indexes", [this]() {
    $expect(data->tester->wb->open_document(data->dataDir + "/workbench/identifying_relationship.mwb")).toBeTrue();
    data->tester->openAllDiagrams();
    data->tester->syncView();

    workbench_DocumentRef doc(data->tester->wb->get_document());

    db_TableRef table1(doc->physicalModels()[0]->catalog()->schemata()[0]->tables()[0]);
    db_TableRef table2(doc->physicalModels()[0]->catalog()->schemata()[0]->tables()[1]);

    $expect(table1->indices().count()).toBe(2U);
    $expect(doc->physicalModels()[0]->diagrams()[0]->connections().count()).toBe(1U);

    workbench_physical_ConnectionRef conn =
      workbench_physical_ConnectionRef::cast_from(doc->physicalModels()[0]->diagrams()[0]->connections().get(0));
    $expect(conn->foreignKey().is_valid()).toBeTrue();

    mforms::stub::UtilitiesWrapper::set_message_callback(messageOtherCallback);
    data->tester->wb->get_model_context()->delete_object(doc->physicalModels()[0]->diagrams()[0]->connections()[0]);

    $expect(doc->physicalModels()[0]->diagrams()[0]->connections().count()).toBe(0U);
    $expect(table1->indices().count()).toBe(1U);

    $expect(data->tester->closeDocument()).toBeTrue();
    data->tester->wb->close_document_finish();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Delete column from table", [this]() {
    $expect(data->tester->wb->open_document(data->dataDir + "/workbench/identifying_relationship.mwb")).toBeTrue();
    data->tester->openAllDiagrams();
    data->tester->syncView();

    workbench_DocumentRef doc(data->tester->wb->get_document());

    db_TableRef table1(doc->physicalModels()[0]->catalog()->schemata()[0]->tables()[0]);
    $expect(table1->indices().count()).toBe(2U);
    $expect(table1->primaryKey().is_valid()).toBeTrue();

    // Delete column.
    table1->removeColumn(table1->columns()[0]);
    $expect(table1->primaryKey().is_valid()).toBeFalse();

    $expect(data->tester->closeDocument()).toBeTrue();
    data->tester->wb->close_document_finish();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Relationship handling when a foreign key is added", [this]() {
    data->tester->wb->new_document();
    data->tester->addView();

    db_mysql_TableRef table1 = data->tester->addTableFigure("table1", 10, 10);
    db_mysql_ColumnRef column1(grt::Initialized);
    column1->owner(table1);
    column1->name("pk");
    db_mysql_TableRef table2 = data->tester->addTableFigure("table2", 100, 10);

    db_mysql_ColumnRef column2(grt::Initialized);
    column2->owner(table2);
    column2->name("fkcol");

    $expect(data->tester->getSchema()->tables().count()).toBe(2U);
    $expect(data->tester->getPview()->figures().count()).toBe(2U);

    grt::GRT::get()->start_tracking_changes();
    table1->addPrimaryKeyColumn(column1);
    grt::GRT::get()->stop_tracking_changes();

    db_mysql_ForeignKeyRef fk(grt::Initialized);
    fk->owner(table2);
    fk->name("fk");

    table2->foreignKeys().insert(fk);
    data->tester->flushUntil(2);

    fk->columns().insert(column2);
    fk->referencedColumns().insert(column1);
    fk->referencedTable(table1);

    grt::BaseListRef connections(data->tester->getPview()->connections());
    data->tester->flushUntil(2, std::bind(&grt::BaseListRef::count, connections), 1);

    $expect(data->tester->getPview()->connections().count()).toBe(1U);

    $expect(data->tester->closeDocument()).toBeTrue();
    data->tester->wb->close_document_finish();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Layer size of a new document", [this]() {
    data->tester->wb->new_document();
    data->tester->addView();

    $expect(*data->tester->getPview()->rootLayer()->width()).toBe(data->tester->lastView->get_total_view_size().width);
    $expect(*data->tester->getPview()->rootLayer()->height()).toBe(data->tester->lastView->get_total_view_size().height);

    $expect(data->tester->closeDocument()).toBeTrue();
    data->tester->wb->close_document_finish();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Creation of a simple model with a relationship", [this]() {
    data->tester->wb->new_document();
    data->tester->addView();

    data->tester->addTableFigure("table1", 100, 100);
    data->tester->addTableFigure("table2", 300, 100);

    data->tester->exportPNG(data->outputDir + "/test20_dump.png");

    $expect(data->tester->closeDocument()).toBeTrue();
    data->tester->wb->close_document_finish();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Bug: loading a model with layers and then hitting new crashes", [this]() {
    $expect(data->tester->wb->open_document(data->dataDir + "/workbench/2tables_conn_layer.mwb")).toBeTrue();
    $expect(data->tester->closeDocument()).toBeTrue();
    data->tester->wb->close_document_finish();
    data->tester->wb->new_document();

    while (dynamic_cast<ModelDiagramForm *>(WBContextUI::get()->get_active_main_form()) != 0)
      data->tester->wb->flush_idle_tasks(false);

    $expect(data->tester->closeDocument()).toBeTrue();
    data->tester->wb->close_document_finish();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Bug: loading model twice causes bad internal state in GUI", [this]() {
    $expect(data->tester->wb->open_document(data->dataDir + "/workbench/sakila.mwb")).toBeTrue();
    $expect(data->tester->closeDocument()).toBeTrue();
    data->tester->wb->close_document_finish();

    $expect(data->tester->wb->open_document(data->dataDir + "/workbench/sakila.mwb")).toBeTrue();
    $expect(data->tester->closeDocument()).toBeTrue();
    data->tester->wb->close_document_finish();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Bug: loading a model with selection won't reselect the items in the canvas", [this]() {
    $expect(data->tester->wb->open_document(data->dataDir + "/workbench/selected_table.mwb")).toBeTrue();
    data->tester->openAllDiagrams();
    data->tester->syncView();

    data->tester->flushWhile(3, std::bind(&mdc::Selection::empty, data->tester->lastView->get_selection()));

    $expect(!data->tester->lastView->get_selection()->empty()).toBeTrue();

    $expect(data->tester->closeDocument()).toBeTrue();
    data->tester->wb->close_document_finish();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Stored note management", [this]() {
    data->tester->wb->new_document();

    // add a note
    WBComponentPhysical *ph = data->tester->wb->get_component<WBComponentPhysical>();

    ph->add_new_stored_note(data->tester->getPmodel());

    data->tester->flushUntil(0.5);

    // check if created
    $expect(data->tester->getPmodel()->notes().count()).toBe(1U);
    $expect(data->tester->getPmodel()->notes().get(0)->filename() != "").toBeTrue();

    // edit the note like an editor would
    set_note_content(data->tester->getPmodel()->notes().get(0), "hello world");
    std::string filename = data->tester->getPmodel()->notes().get(0)->filename();

    data->tester->wb->save_as(data->outputDir + "/notetest.mwb");
    $expect(data->tester->closeDocument()).toBeTrue();
    data->tester->wb->close_document_finish();

    // check if stored on disk
    $expect(data->tester->wb->open_document(data->outputDir + "/notetest.mwb")).toBeTrue();

    $expect(data->tester->getPmodel()->notes().count()).toBe(1U);
    $expect(*data->tester->getPmodel()->notes().get(0)->filename()).toBe(filename);

    // get note contents as an editor
    std::string text = get_note_content(data->tester->getPmodel()->notes().get(0));
    $expect(text).toBe("hello world");

    $expect(data->tester->closeDocument()).toBeTrue();
    data->tester->wb->close_document_finish();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Undo for stored notes", [this]() {
    // create note
    data->tester->wb->new_document();

    // add a note
    WBComponentPhysical *ph = data->tester->wb->get_component<WBComponentPhysical>();

    ph->add_new_stored_note(data->tester->getPmodel());

    data->tester->flushUntil(0.5);

    // check if created
    $expect(data->tester->getPmodel()->notes().count()).toBe(1U);
    $expect(data->tester->getPmodel()->notes().get(0)->filename() != "").toBeTrue();

    std::string fname = data->tester->getPmodel()->notes().get(0)->filename();

    set_note_content(data->tester->getPmodel()->notes().get(0), "some text");

    $expect(get_note_content(data->tester->getPmodel()->notes().get(0))).toBe("some text");

    // delete note and undo
    grt::GRT::get()->get_undo_manager()->add_undo(new grt::UndoListRemoveAction(data->tester->getPmodel()->notes(), 0));
    data->tester->getPmodel()->notes().remove(0);
    grt::GRT::get()->get_undo_manager()->undo();

    $expect(data->tester->getPmodel()->notes().count()).toBe(1U);
    $expect(*data->tester->getPmodel()->notes().get(0)->filename()).toBe(fname);
    $expect(get_note_content(data->tester->getPmodel()->notes().get(0))).toBe("some text");

    for (int i = 0; i < 10; i++) {
      grt::GRT::get()->get_undo_manager()->redo();
      $expect(data->tester->getPmodel()->notes().count()).toBe(0U);
      grt::GRT::get()->get_undo_manager()->undo();
      $expect(data->tester->getPmodel()->notes().count()).toBe(1U);
    }
    $expect(*data->tester->getPmodel()->notes().get(0)->filename()).toBe(fname);
    $expect(get_note_content(data->tester->getPmodel()->notes().get(0))).toBe("some text");

    // undo (should undo create note)

    $expect(data->tester->closeDocument()).toBeTrue();
    data->tester->wb->close_document_finish();
  });
}

}
