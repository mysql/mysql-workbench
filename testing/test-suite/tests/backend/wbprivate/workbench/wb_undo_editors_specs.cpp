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

#include "casmine.h"
#include "wb_test_helpers.h"
#include "grt_test_helpers.h"

#include "model/wb_history_tree.h"
#include "grtdb/db_object_helpers.h"
#include "mysql_table_editor.h"

using namespace wb;

namespace {

$ModuleEnvironment() {};

$TestData {
  std::unique_ptr<WorkbenchTester> tester;
  grt::UndoManager* um = nullptr;
  OverviewBE* overview = nullptr;
  db_SchemaRef schema;
  db_mgmt_RdbmsRef rdbms;

  size_t lastUndoStackSize = 0;
  size_t lastRedoStackSize = 0;

  #include "wb_undo_helpers.h"

};

$describe("Undo Tests for Editors") {

  $beforeAll([this]() {
    data->tester.reset(new WorkbenchTester());
    data->um = grt::GRT::get()->get_undo_manager();
    data->overview = WBContextUI::get()->get_physical_overview();

    WBContextUI::get()->set_active_form(data->overview);

    std::string dataDir = casmine::CasmineContext::get()->tmpDataDir();
    bool flag = data->tester->wb->open_document(dataDir + "/workbench/undo_test_model2.mwb");
    $expect(flag).toBeTrue("open_document");

    $expect(data->tester->getCatalog()->schemata().count()).toEqual(1U, "schemas");

    data->schema = data->tester->getCatalog()->schemata()[0];
    data->rdbms = db_mgmt_RdbmsRef::cast_from(grt::GRT::get()->unserialize(dataDir + "/res/mysql_rdbms_info.xml"));

    // make sure the loaded model contains expected number of things
    $expect(data->schema->tables().count()).toEqual(2U, "tables");
    $expect(data->schema->tables()[0]->columns()->count()).toEqual(2U, "tables");

    $expect(data->um->get_undo_stack().size()).toEqual(0U, "undo stack is empty");
  });

  // For undo tests we use a single document loaded at the beginning of the group
  // Each test must do the test and undo everything so that the state of the document
  // never actually changes.
  // At the end of the test group, the document is compared to the saved one to check
  // for unexpected changes (ie, anything but stuff like timestamps)

  //--------------------------------------------------------------------------------------------------------------------

  $it("Editors general", [this]() {
    db_mysql_TableRef table(db_mysql_TableRef::cast_from(data->schema->tables()[0]));
    const std::string old_name = table->name();
    std::unique_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

    // Rename
    be->set_name("new_name");
    data->checkOnlyOneUndoAdded();
    $expect(table->name()).toEqual("new_name", "Table name set to new value");
    data->checkUndo();
    $expect(table->name()).toEqual(old_name, "Table name change undo failed");

    // Change Comment
    be->set_comment("comment");
    data->checkOnlyOneUndoAdded();
    $expect(be->get_comment()).toEqual("comment", "Table comment set to new value");
    data->checkUndo();
    $expect(be->get_comment()).toEqual("test_table_comment", "Table comment undo failed");
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Table general", [this]() {
    db_mysql_TableRef table(db_mysql_TableRef::cast_from(data->schema->tables()[0]));
    std::unique_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

    // Change Collation
    std::string old_value = be->get_table_option_by_name("CHARACTER SET - COLLATE");
    std::vector<std::string> collations(be->get_charset_collation_list());
    if (collations.size() > 2) {
      size_t new_coll_idx = collations.size() / 2;
      if (collations[new_coll_idx] != old_value) {
        be->set_table_option_by_name("CHARACTER SET - COLLATE", collations[new_coll_idx]);
        data->checkOnlyOneUndoAdded();
        $expect(old_value).Not.toEqual(be->get_table_option_by_name("CHARACTER SET - COLLATE"), "Table collation change failed");
        data->checkUndo();
        $expect(old_value).toEqual(be->get_table_option_by_name("CHARACTER SET - COLLATE"), "Table collation change undo failed");
      } else
        $fail("Cannot test collation");
    } else
      $fail("Cannot test collation. list empty");

    // Change Engine
    old_value = be->get_table_option_by_name("ENGINE");
    std::vector<std::string> engines(be->get_engines_list());
    if (engines.size() > 2) {
      size_t new_eng_idx = engines.size() / 2;
      if (engines[new_eng_idx] != old_value) {
        be->set_table_option_by_name("ENGINE", engines[new_eng_idx]);
        data->checkOnlyOneUndoAdded();
        $expect(old_value).Not.toEqual( be->get_table_option_by_name("ENGINE"), "Table engine change failed");
        data->checkUndo();
        $expect(old_value).toEqual(be->get_table_option_by_name("ENGINE"), "Table engine change undo failed");
      } else
        $fail("Cannot test engine change");
    } else
      $fail("Cannot test engine change. list empty");
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Table add column", [this]() {
    db_mysql_TableRef table(db_mysql_TableRef::cast_from(data->schema->tables()[0]));
    std::unique_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

    // Add
    const size_t existing_cols_nr = table->columns().count();
    $expect(existing_cols_nr).toEqual(2U, "Table has extra columns before test");
    be->add_column("test_column");
    data->checkOnlyOneUndoAdded();
    $expect(table->columns()[existing_cols_nr]->name()).toEqual("test_column", "Column was not added");
    data->checkUndo();
    $expect(table->columns().count()).toEqual(existing_cols_nr, "Table has extra columns after undo");
    data->checkRedo();
    $expect(table->columns()[existing_cols_nr]->name()).toEqual("test_column", "Column was not added after redo");
    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Table rename column", [this]() {
    db_mysql_TableRef table(db_mysql_TableRef::cast_from(data->schema->tables()[0]));
    std::unique_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

    const std::string old_name = table->columns()[0]->name();
    // Rename
    bec::TableColumnsListBE* cols = be->get_columns();
    cols->set_field(bec::NodeId(0), MySQLTableColumnsListBE::Name, std::string("abyrvalg"));
    data->checkOnlyOneUndoAdded();
    $expect(table->columns()[0]->name()).toEqual("abyrvalg", "Column was not renamed");
    data->checkUndo();
    $expect(table->columns()[0]->name()).toEqual(old_name, "Column name change undo failed");
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Table column type change", [this]() {
    db_mysql_TableRef table(db_mysql_TableRef::cast_from(data->schema->tables()[0]));
    std::unique_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

    bec::TableColumnsListBE* cols = be->get_columns();

    // Change Type
    // get original type
    const std::string old_value = table->columns()[0]->formattedType();

    // change type via BE, adding undo action
    cols->set_field(bec::NodeId(0), MySQLTableColumnsListBE::Type, std::string("FLOAT"));
    data->checkOnlyOneUndoAdded();
    $expect(table->columns()[0]->formattedType()).toEqual("FLOAT", "Column type was not set to FLOAT");
    // Store new value to check redo later
    const std::string new_value = table->columns()[0]->formattedType();
    data->checkUndo();

    // verify that type was reverted to original
    $expect(table->columns()[0]->formattedType()).toEqual(old_value, "Column type change undo failed");

    data->checkRedo();
    $expect(table->columns()[0]->formattedType()).toEqual(new_value, "Column type was not reset to " + old_value + " after redo");

    // revert all and do a last check
    data->checkUndo();
    $expect(table->columns()[0]->formattedType()).toEqual(old_value, "Column type change undo after redo failed");
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Table column flag change", [this]() {
    db_mysql_TableRef table(db_mysql_TableRef::cast_from(data->schema->tables()[0]));
    std::unique_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

    bec::TableColumnsListBE* cols = be->get_columns();

    // Change Flag
    int flag = cols->get_column_flag(bec::NodeId(0), "UNSIGNED");
    cols->set_column_flag(bec::NodeId(0), "UNSIGNED", !flag);
    data->checkOnlyOneUndoAdded();
    $expect(flag).Not.toEqual(cols->get_column_flag(bec::NodeId(0), "UNSIGNED"), "Column flag was not changed");

    data->checkUndo();
    $expect(flag).toEqual(cols->get_column_flag(bec::NodeId(0), "UNSIGNED"), "Column flag change undo failed");
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Table column NN change", [this]() {
    db_mysql_TableRef table(db_mysql_TableRef::cast_from(data->schema->tables()[0]));
    std::unique_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

    bec::TableColumnsListBE* cols = be->get_columns();

    // Change NN, get original flag and revert it
    ssize_t nn = -1;
    cols->get_field(bec::NodeId(0), MySQLTableColumnsListBE::IsNotNull, nn);
    cols->set_field(bec::NodeId(0), MySQLTableColumnsListBE::IsNotNull, !nn);
    data->checkOnlyOneUndoAdded();

    // get value and check that it was changed, they should match
    ssize_t nn2 = -1;
    cols->get_field(bec::NodeId(0), MySQLTableColumnsListBE::IsNotNull, nn2);
    $expect(static_cast<bool>(nn2)).toEqual(!nn, "NN was not set");

    data->checkUndo();
    cols->get_field(bec::NodeId(0), MySQLTableColumnsListBE::IsNotNull, nn2);
    $expect(nn2).toEqual(nn, "NN was not set back on undo");
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Table column default change", [this]() {
    db_mysql_TableRef table(db_mysql_TableRef::cast_from(data->schema->tables()[0]));
    std::unique_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

    bec::TableColumnsListBE* cols = be->get_columns();

    std::string value;
    // Change Default
    cols->get_field(bec::NodeId(0), MySQLTableColumnsListBE::Default, value);
    cols->set_field(bec::NodeId(0), MySQLTableColumnsListBE::Default, "2");
    data->checkOnlyOneUndoAdded();

    data->checkUndo();
    std::string value2;
    cols->get_field(bec::NodeId(0), MySQLTableColumnsListBE::Default, value2);
    $expect(value).toEqual(value2, "Default value undo failed");
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Table column remove", [this]() {
    db_mysql_TableRef table(db_mysql_TableRef::cast_from(data->schema->tables()[0]));
    std::unique_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

    const size_t ncols = table->columns().count();
    $expect(ncols).toEqual(2U, "tables");
    // Remove
    be->remove_column(bec::NodeId(1));
    data->checkOnlyOneUndoAdded();
    // In file we had 2 columns and one index(PRIMARY KEY). We removed column on which PK was.
    data->checkUndo();
    $expect(table->columns().count()).toEqual(ncols, "Table column remove undo failed");
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Table index add", [this]() {
    db_mysql_TableRef table(db_mysql_TableRef::cast_from(data->schema->tables()[0]));
    std::unique_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

    const size_t nidx = table->indices().count();
    // Add
    be->add_index("idx");
    data->checkOnlyOneUndoAdded();
    $expect(table->indices()[nidx]->name()).toEqual("idx", "Index was not added");
    data->checkUndo();
    $expect(table->indices().count()).toEqual(nidx, "Index add undo failed");
    data->checkRedo();
    $expect(table->indices()[nidx]->name()).toEqual("idx", "Index add redo failed");
    data->checkUndo();
    $expect(table->indices().count()).toEqual(nidx, "Index add undo2 failed");
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Table index rename", [this]() {
    db_mysql_TableRef table(db_mysql_TableRef::cast_from(data->schema->tables()[0]));
    std::unique_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

    const size_t nidx = table->indices().count();
    be->add_index("idx");
    data->checkOnlyOneUndoAdded();

    // Rename
    MySQLTableIndexListBE* indices = be->get_indexes();
    indices->set_field(bec::NodeId((int)nidx), bec::IndexListBE::Name, "idx_new");
    data->checkOnlyOneUndoAdded();
    data->checkUndo();
    $expect(table->indices()[nidx]->name()).toEqual("idx", "Index rename undo failed");

    data->checkUndo(); // undo index addition
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Table index remove", [this]() {
    db_mysql_TableRef table(db_mysql_TableRef::cast_from(data->schema->tables()[0]));
    std::unique_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

    const size_t nidx = table->indices().count();
    be->add_index("idx");
    data->checkOnlyOneUndoAdded();

    // Remove
    be->remove_index(bec::NodeId((int)nidx), false);
    data->checkOnlyOneUndoAdded();
    data->checkUndo();
    $expect(table->indices().count()).toEqual(nidx + 1, "Index removal undo failed");

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Table index type change", [this]() {
    db_mysql_TableRef table(db_mysql_TableRef::cast_from(data->schema->tables()[0]));
    std::unique_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

    // set engine to something that supports FULLTEXT and RTREE index options
    table->tableEngine("MyISAM");

    const size_t nidx = table->indices().count();
    be->add_index("idx");
    data->checkOnlyOneUndoAdded();

    MySQLTableIndexListBE* indices = be->get_indexes();

    indices->select_index(bec::NodeId((int)nidx));

    indices->set_field(bec::NodeId((int)nidx), bec::IndexListBE::Type, "FULLTEXT");
    data->checkOnlyOneUndoAdded();
    data->checkUndo();
    $expect(table->indices()[nidx]->indexType()).toEqual("INDEX", "Index type change undo failed");

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Table index storage change", [this]() {
    db_mysql_TableRef table(db_mysql_TableRef::cast_from(data->schema->tables()[0]));
    std::unique_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

    const size_t nidx = table->indices().count();
    be->add_index("idx");
    data->checkOnlyOneUndoAdded();

    MySQLTableIndexListBE* indices = be->get_indexes();

    indices->select_index(bec::NodeId((int)nidx));

    // Change Storage Type
    indices->set_field(bec::NodeId((int)nidx), MySQLTableIndexListBE::StorageType, "RTREE");
    data->checkOnlyOneUndoAdded();
    data->checkUndo();
    $expect(*table->indices()[nidx]->indexKind()).toEqual("", "Index storage type change undo failed");

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Table index column add", [this]() {
    db_mysql_TableRef table(db_mysql_TableRef::cast_from(data->schema->tables()[0]));
    std::unique_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

    const size_t nidx = table->indices().count();
    be->add_index("idx");
    data->checkOnlyOneUndoAdded();

    MySQLTableIndexListBE* indices = be->get_indexes();
    indices->select_index(bec::NodeId((int)nidx));

    bec::IndexColumnsListBE* icols = indices->get_columns();
    icols->set_column_enabled(bec::NodeId(icols->count() - 1), true);
    data->checkOnlyOneUndoAdded();
    data->checkUndo();
    $expect(icols->get_column_enabled(bec::NodeId(bec::NodeId((int)table->columns().count() - 1)))).toBeFalse("Undo adding column to index failed");
    data->checkRedo();
    data->checkUndo();

    data->checkUndo();
  });

  // IDX
  // Change Order
  // Change Length
  // Change Comment

  //--------------------------------------------------------------------------------------------------------------------

  $it("Table FK add", [this]() {
    db_mysql_TableRef table(db_mysql_TableRef::cast_from(data->schema->tables()[0]));
    std::unique_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));
    const size_t nfks = table->foreignKeys().count();

    be->add_fk("fk1");
    data->checkOnlyOneUndoAdded();
    $expect(table->foreignKeys()[nfks]->name()).toEqual("fk1", "FK was not added");
    data->checkUndo();
    data->checkRedo();
    $expect(table->foreignKeys()[nfks]->name()).toEqual("fk1", "adding FK redo failed");
    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Table FK remove", [this]() {
    db_mysql_TableRef table(db_mysql_TableRef::cast_from(data->schema->tables()[0]));
    std::unique_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));
    const size_t nfks = table->foreignKeys().count();

    be->add_fk("fk1");
    data->checkOnlyOneUndoAdded();
    $expect(nfks + 1).toEqual(table->foreignKeys().count(), "FK was added");
    be->remove_fk(bec::NodeId((int)nfks));
    data->checkOnlyOneUndoAdded();
    $expect(nfks).toEqual(table->foreignKeys().count(), "FK was removed");
    data->checkUndo();
    $expect(nfks + 1).toEqual(table->foreignKeys().count(), "FK remove undo");

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Table FK rename", [this]() {
    db_mysql_TableRef table(db_mysql_TableRef::cast_from(data->schema->tables()[0]));
    std::unique_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));
    const size_t nfks = table->foreignKeys().count();
    bec::FKConstraintListBE* fks = be->get_fks();

    be->add_fk("fk1");
    data->checkOnlyOneUndoAdded();

    fks->set_field(bec::NodeId((int)nfks), bec::FKConstraintListBE::Name, "fk_new");
    data->checkOnlyOneUndoAdded();
    data->checkUndo();
    $expect(table->foreignKeys()[nfks]->name()).toEqual("fk1", "Index rename undo failed");
    data->checkRedo();
    data->checkUndo();

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Table FK set ref table", [this]() {
    db_mysql_TableRef table(db_mysql_TableRef::cast_from(data->schema->tables()[0]));
    std::unique_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));
    const size_t nfks = table->foreignKeys().count();
    bec::FKConstraintListBE* fks = be->get_fks();

    be->add_fk("fk1");
    data->checkOnlyOneUndoAdded();
    fks->select_fk(bec::NodeId((int)nfks));

    fks->set_field(bec::NodeId((int)nfks), bec::FKConstraintListBE::RefTable, "table2");
    data->checkOnlyOneUndoAdded();
    $expect(fks->get_selected_fk()->referencedTable()->name()).toEqual("table2", "FK ref table was not set");
    data->checkUndo();

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Table FK set ref column", [this]() {
    db_mysql_TableRef table(db_mysql_TableRef::cast_from(data->schema->tables()[0]));
    db_mysql_TableRef table2(db_mysql_TableRef::cast_from(data->schema->tables()[1]));
    std::unique_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));
    const size_t nfks = table->foreignKeys().count();
    bec::FKConstraintListBE* fks = be->get_fks();

    be->add_fk("fk1");
    data->checkOnlyOneUndoAdded();
    fks->select_fk(bec::NodeId((int)nfks));

    fks->set_field(bec::NodeId((int)nfks), bec::FKConstraintListBE::RefTable, "table2");
    data->checkOnlyOneUndoAdded();
    $expect(fks->get_selected_fk()->referencedTable()->name()).toEqual("table2", "FK ref table was not set");

    bec::FKConstraintColumnsListBE* fkcols = fks->get_columns();
    fkcols->set_field(bec::NodeId(0), bec::FKConstraintColumnsListBE::Enabled, 1);
    data->checkOnlyOneUndoAdded();
    fkcols->set_field(bec::NodeId(0), bec::FKConstraintColumnsListBE::RefColumn,
                      table2->columns()[table2->columns()->count() - 1]->name());
    data->checkOnlyOneUndoAdded();

    // TODO: Change On Update
    // TODO: Change On Delete

    data->checkUndo(); // set column
    data->checkUndo(); // set ref column

    data->checkUndo(); // set ref table

    data->checkUndo(); // add fk
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Table inserts", []() {
    $pending("not implemented");
  });

  $it("Table partitioning", []() {
    $pending("not implemented");
  });

  $it("Table options", []() {
    $pending("not implemented");
  });

  $it("View", []() {
    $pending("not implemented");
  });

  $it("Routine", []() {
    $pending("not implemented");
  });

  $it("Routine group", []()  {
    $pending("not implemented");
  });

  $it("Schema", []() {
    $pending("not implemented");
  });

  $it("Relationship", []() {
    $pending("not implemented");
  });

  $it("Image", []() {
    $pending("not implemented");
  });

  $it("Note", []() {
    $pending("not implemented");
  });

  $it("Stored script", []() {
    $pending("not implemented");
  });

  $it("Stored note", []() {
    $pending("not implemented");
  });

}

}
