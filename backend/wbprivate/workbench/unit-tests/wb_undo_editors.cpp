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

#include "wb_helpers.h"
#include "model/wb_history_tree.h"
#include "grtdb/db_object_helpers.h"
#include "grt_test_utility.h"
#include "mysql_table_editor.h"

using namespace wb;

BEGIN_TEST_DATA_CLASS(wb_undo_editors)
public:
WBTester* tester;
UndoManager* um;
OverviewBE* overview;
db_SchemaRef schema;
db_mgmt_RdbmsRef _rdbms;

size_t last_undo_stack_height;
size_t last_redo_stack_height;

TEST_DATA_CONSTRUCTOR(wb_undo_editors) {
  tester = new WBTester;
  um = grt::GRT::get()->get_undo_manager();
  overview = WBContextUI::get()->get_physical_overview();

  WBContextUI::get()->set_active_form(overview);

  last_undo_stack_height = 0;
  last_redo_stack_height = 0;
}

#include "wb_undo_methods.h"

END_TEST_DATA_CLASS;

// For undo tests we use a single document loaded at the beginning of the group
// Each test must do the test and undo everything so that the state of the document
// never actually changes.
// At the end of the test group, the document is compared to the saved one to check
// for unexpected changes (ie, anything but stuff like timestamps)

TEST_MODULE(wb_undo_editors, "undo tests for editors in Workbench");

// setup
TEST_FUNCTION(1) {
  bool flag = tester->wb->open_document("data/workbench/undo_test_model2.mwb");
  ensure("open_document", flag);

  ensure_equals("schemas", tester->get_catalog()->schemata().count(), 1U);

  schema = tester->get_catalog()->schemata()[0];

  _rdbms = db_mgmt_RdbmsRef::cast_from(grt::GRT::get()->unserialize("data/res/mysql_rdbms_info.xml"));

  // make sure the loaded model contains expected number of things
  ensure_equals("tables", schema->tables().count(), 2U);
  ensure_equals("tables", schema->tables()[0]->columns()->count(), 2U);

  ensure_equals("undo stack is empty", um->get_undo_stack().size(), 0U);
}

// Editors
//----------------------------------------------------------------------------------------
TEST_FUNCTION(2) // General
{
  db_mysql_TableRef table(db_mysql_TableRef::cast_from(schema->tables()[0]));
  const std::string old_name = table->name();
  std::auto_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

  // Rename
  be->set_name("new_name");
  check_only_one_undo_added();
  ensure("Table name set to new value", table->name() == "new_name");
  check_undo();
  ensure("Table name change undo failed", table->name() == old_name);

  // Change Comment
  be->set_comment("comment");
  check_only_one_undo_added();
  ensure("Table comment set to new value", be->get_comment() == "comment");
  check_undo();
  ensure("Table comment undo failed", be->get_comment() == "test_table_comment");
}

//------------------------------------------------------------------------------
TEST_FUNCTION(3) // Table (general)
{
  db_mysql_TableRef table(db_mysql_TableRef::cast_from(schema->tables()[0]));
  std::auto_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

  // Change Collation
  std::string old_value = be->get_table_option_by_name("CHARACTER SET - COLLATE");
  std::vector<std::string> collations(be->get_charset_collation_list());
  if (collations.size() > 2) {
    size_t new_coll_idx = collations.size() / 2;
    if (collations[new_coll_idx] != old_value) {
      be->set_table_option_by_name("CHARACTER SET - COLLATE", collations[new_coll_idx]);
      check_only_one_undo_added();
      ensure("Table collation change failed", old_value != be->get_table_option_by_name("CHARACTER SET - COLLATE"));
      check_undo();
      ensure("Table collation change undo failed",
             old_value == be->get_table_option_by_name("CHARACTER SET - COLLATE"));
    } else
      ensure("can not test collation", false);
  } else
    ensure("can not test collation. list empty", false);

  // Change Engine
  old_value = be->get_table_option_by_name("ENGINE");
  std::vector<std::string> engines(be->get_engines_list());
  if (engines.size() > 2) {
    size_t new_eng_idx = engines.size() / 2;
    if (engines[new_eng_idx] != old_value) {
      be->set_table_option_by_name("ENGINE", engines[new_eng_idx]);
      check_only_one_undo_added();
      ensure("Table engine change failed", old_value != be->get_table_option_by_name("ENGINE"));
      check_undo();
      ensure("Table engine change undo failed", old_value == be->get_table_option_by_name("ENGINE"));
    } else
      ensure("can not test engine change", false);
  } else
    ensure("can not test engine change. list empty", false);
}

//------------------------------------------------------------------------------
TEST_FUNCTION(4) // Table (add column)
{
  db_mysql_TableRef table(db_mysql_TableRef::cast_from(schema->tables()[0]));
  std::auto_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

  // Add
  const size_t existing_cols_nr = table->columns().count();
  ensure("Table has extra columns before test", existing_cols_nr == 2);
  be->add_column("test_column");
  check_only_one_undo_added();
  ensure("Column was not added", table->columns()[existing_cols_nr]->name() == "test_column");
  check_undo();
  ensure("Table has extra columns after undo", table->columns().count() == existing_cols_nr);
  check_redo();
  ensure("Column was not added after redo", table->columns()[existing_cols_nr]->name() == "test_column");
  check_undo();
}

//------------------------------------------------------------------------------
TEST_FUNCTION(5) // Table (rename column)
{
  db_mysql_TableRef table(db_mysql_TableRef::cast_from(schema->tables()[0]));
  std::auto_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

  const std::string old_name = table->columns()[0]->name();
  // Rename
  bec::TableColumnsListBE* cols = be->get_columns();
  cols->set_field(bec::NodeId(0), MySQLTableColumnsListBE::Name, std::string("abyrvalg"));
  check_only_one_undo_added();
  ensure("Column was not renamed", table->columns()[0]->name() == "abyrvalg");
  check_undo();
  ensure("Column name change undo failed", table->columns()[0]->name() == old_name);
}

//------------------------------------------------------------------------------
TEST_FUNCTION(6) // Table (Column type change)
{
  db_mysql_TableRef table(db_mysql_TableRef::cast_from(schema->tables()[0]));
  std::auto_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

  bec::TableColumnsListBE* cols = be->get_columns();

  // Change Type
  // get original type
  const std::string old_value = table->columns()[0]->formattedType();

  // change type via BE, adding undo action
  cols->set_field(bec::NodeId(0), MySQLTableColumnsListBE::Type, std::string("FLOAT"));
  check_only_one_undo_added();
  ensure("Column type was not set to FLOAT", table->columns()[0]->formattedType() == "FLOAT");
  // Store new value to check redo later
  const std::string new_value = table->columns()[0]->formattedType();
  check_undo();

  // verify that type was reverted to original
  ensure("Column type change undo failed", table->columns()[0]->formattedType() == old_value);

  check_redo();
  ensure("Column type was not reset to " + old_value + " after redo",
         table->columns()[0]->formattedType() == new_value);

  // revert all and do a last check
  check_undo();
  ensure("Column type change undo after redo failed", table->columns()[0]->formattedType() == old_value);
}

//------------------------------------------------------------------------------
TEST_FUNCTION(7) // Table (Column flag change)
{
  db_mysql_TableRef table(db_mysql_TableRef::cast_from(schema->tables()[0]));
  std::auto_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

  bec::TableColumnsListBE* cols = be->get_columns();

  // Change Flag
  int flag = cols->get_column_flag(bec::NodeId(0), "UNSIGNED");
  cols->set_column_flag(bec::NodeId(0), "UNSIGNED", !flag);
  check_only_one_undo_added();
  ensure("Column flag was not changed", flag != cols->get_column_flag(bec::NodeId(0), "UNSIGNED"));

  check_undo();
  ensure("Column flag change undo failed", flag == cols->get_column_flag(bec::NodeId(0), "UNSIGNED"));
}

//------------------------------------------------------------------------------
TEST_FUNCTION(8) // Table (column NN change)
{
  db_mysql_TableRef table(db_mysql_TableRef::cast_from(schema->tables()[0]));
  std::auto_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

  bec::TableColumnsListBE* cols = be->get_columns();

  // Change NN, get original flag and revert it
  ssize_t nn = -1;
  cols->get_field(bec::NodeId(0), MySQLTableColumnsListBE::IsNotNull, nn);
  cols->set_field(bec::NodeId(0), MySQLTableColumnsListBE::IsNotNull, !nn);
  check_only_one_undo_added();

  // get value and check that it was changed, they should match
  ssize_t nn2 = -1;
  cols->get_field(bec::NodeId(0), MySQLTableColumnsListBE::IsNotNull, nn2);
  ensure("NN was not set", nn2 == !nn);

  check_undo();
  cols->get_field(bec::NodeId(0), MySQLTableColumnsListBE::IsNotNull, nn2);
  ensure("NN was not set back on undo", nn2 == nn);
}

//------------------------------------------------------------------------------
TEST_FUNCTION(9) // Table (column default change)
{
  db_mysql_TableRef table(db_mysql_TableRef::cast_from(schema->tables()[0]));
  std::auto_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

  bec::TableColumnsListBE* cols = be->get_columns();

  std::string value;
  // Change Default
  cols->get_field(bec::NodeId(0), MySQLTableColumnsListBE::Default, value);
  cols->set_field(bec::NodeId(0), MySQLTableColumnsListBE::Default, "2");
  check_only_one_undo_added();

  check_undo();
  std::string value2;
  cols->get_field(bec::NodeId(0), MySQLTableColumnsListBE::Default, value2);
  ensure("Default value undo failed", value == value2);
}

//------------------------------------------------------------------------------
TEST_FUNCTION(10) // Table (column remove)
{
  db_mysql_TableRef table(db_mysql_TableRef::cast_from(schema->tables()[0]));
  std::auto_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

  const size_t ncols = table->columns().count();
  ensure_equals("tables", ncols, 2U);
  // Remove
  be->remove_column(bec::NodeId(1));
  check_only_one_undo_added();
  // In file we had 2 columns and one index(PRIMARY KEY). We removed column on which PK was.
  check_undo();
  ensure("Table column remove undo failed", table->columns().count() == ncols);
}

//------------------------------------------------------------------------------
TEST_FUNCTION(11) // Table (index add)
{
  db_mysql_TableRef table(db_mysql_TableRef::cast_from(schema->tables()[0]));
  std::auto_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

  const size_t nidx = table->indices().count();
  // Add
  be->add_index("idx");
  check_only_one_undo_added();
  ensure("Index was not added", table->indices()[nidx]->name() == "idx");
  check_undo();
  ensure("Index add undo failed", table->indices().count() == nidx);
  check_redo();
  ensure("Index add redo failed", table->indices()[nidx]->name() == "idx");
  check_undo();
  ensure("Index add undo2 failed", table->indices().count() == nidx);
}

//------------------------------------------------------------------------------
TEST_FUNCTION(12) // Table (index rename)
{
  db_mysql_TableRef table(db_mysql_TableRef::cast_from(schema->tables()[0]));
  std::auto_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

  const size_t nidx = table->indices().count();
  be->add_index("idx");
  check_only_one_undo_added();

  // Rename
  MySQLTableIndexListBE* indices = be->get_indexes();
  indices->set_field(bec::NodeId((int)nidx), bec::IndexListBE::Name, "idx_new");
  check_only_one_undo_added();
  check_undo();
  ensure("Index rename undo failed", table->indices()[nidx]->name() == "idx");

  check_undo(); // undo index addition
}

//------------------------------------------------------------------------------
TEST_FUNCTION(13) // Table (index remove)
{
  db_mysql_TableRef table(db_mysql_TableRef::cast_from(schema->tables()[0]));
  std::auto_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

  const size_t nidx = table->indices().count();
  be->add_index("idx");
  check_only_one_undo_added();

  // Remove
  be->remove_index(bec::NodeId((int)nidx), false);
  check_only_one_undo_added();
  check_undo();
  ensure("Index removal undo failed", table->indices().count() == nidx + 1);

  check_undo();
}

//------------------------------------------------------------------------------
TEST_FUNCTION(14) // Table (index type change)
{
  db_mysql_TableRef table(db_mysql_TableRef::cast_from(schema->tables()[0]));
  std::auto_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

  // set engine to something that supports FULLTEXT and RTREE index options
  table->tableEngine("MyISAM");

  const size_t nidx = table->indices().count();
  be->add_index("idx");
  check_only_one_undo_added();

  MySQLTableIndexListBE* indices = be->get_indexes();

  indices->select_index(bec::NodeId((int)nidx));

  indices->set_field(bec::NodeId((int)nidx), bec::IndexListBE::Type, "FULLTEXT");
  check_only_one_undo_added();
  check_undo();
  ensure("Index type change undo failed", table->indices()[nidx]->indexType() == "INDEX");

  check_undo();
}

//------------------------------------------------------------------------------
TEST_FUNCTION(15) // Table (index storage change)
{
  db_mysql_TableRef table(db_mysql_TableRef::cast_from(schema->tables()[0]));
  std::auto_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

  const size_t nidx = table->indices().count();
  be->add_index("idx");
  check_only_one_undo_added();

  MySQLTableIndexListBE* indices = be->get_indexes();

  indices->select_index(bec::NodeId((int)nidx));

  // Change Storage Type
  indices->set_field(bec::NodeId((int)nidx), MySQLTableIndexListBE::StorageType, "RTREE");
  check_only_one_undo_added();
  check_undo();
  ensure("Index storage type change undo failed", table->indices()[nidx]->indexKind() == "");

  check_undo();
}

//------------------------------------------------------------------------------
TEST_FUNCTION(16) // Table (index column add)
{
  db_mysql_TableRef table(db_mysql_TableRef::cast_from(schema->tables()[0]));
  std::auto_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));

  const size_t nidx = table->indices().count();
  be->add_index("idx");
  check_only_one_undo_added();

  MySQLTableIndexListBE* indices = be->get_indexes();
  indices->select_index(bec::NodeId((int)nidx));

  bec::IndexColumnsListBE* icols = indices->get_columns();
  icols->set_column_enabled(bec::NodeId(icols->count() - 1), true);
  check_only_one_undo_added();
  check_undo();
  ensure("Undo adding column to index failed",
         0 == icols->get_column_enabled(bec::NodeId(bec::NodeId((int)table->columns().count() - 1))));
  check_redo();
  check_undo();

  check_undo();
}

// IDX
// Change Order
// Change Length
// Change Comment

//------------------------------------------------------------------------------
TEST_FUNCTION(17) // Table (fk add)
{
  db_mysql_TableRef table(db_mysql_TableRef::cast_from(schema->tables()[0]));
  std::auto_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));
  const size_t nfks = table->foreignKeys().count();

  be->add_fk("fk1");
  check_only_one_undo_added();
  ensure("FK was not added", table->foreignKeys()[nfks]->name() == "fk1");
  check_undo();
  check_redo();
  ensure("adding FK redo failed", table->foreignKeys()[nfks]->name() == "fk1");
  check_undo();
}

//------------------------------------------------------------------------------
TEST_FUNCTION(18) // Table (fk remove)
{
  db_mysql_TableRef table(db_mysql_TableRef::cast_from(schema->tables()[0]));
  std::auto_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));
  const size_t nfks = table->foreignKeys().count();

  be->add_fk("fk1");
  check_only_one_undo_added();
  ensure_equals("FK was added", nfks + 1, table->foreignKeys().count());
  be->remove_fk(bec::NodeId((int)nfks));
  check_only_one_undo_added();
  ensure_equals("FK was removed", nfks, table->foreignKeys().count());
  check_undo();
  ensure_equals("FK remove undo", nfks + 1, table->foreignKeys().count());

  check_undo();
}

//------------------------------------------------------------------------------
TEST_FUNCTION(19) // Table (fk rename)
{
  db_mysql_TableRef table(db_mysql_TableRef::cast_from(schema->tables()[0]));
  std::auto_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));
  const size_t nfks = table->foreignKeys().count();
  bec::FKConstraintListBE* fks = be->get_fks();

  be->add_fk("fk1");
  check_only_one_undo_added();

  fks->set_field(bec::NodeId((int)nfks), bec::FKConstraintListBE::Name, "fk_new");
  check_only_one_undo_added();
  check_undo();
  ensure("Index rename undo failed", table->foreignKeys()[nfks]->name() == "fk1");
  check_redo();
  check_undo();

  check_undo();
}

//------------------------------------------------------------------------------
TEST_FUNCTION(20) // Table (fk set ref table)
{
  db_mysql_TableRef table(db_mysql_TableRef::cast_from(schema->tables()[0]));
  std::auto_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));
  const size_t nfks = table->foreignKeys().count();
  bec::FKConstraintListBE* fks = be->get_fks();

  be->add_fk("fk1");
  check_only_one_undo_added();
  fks->select_fk(bec::NodeId((int)nfks));

  fks->set_field(bec::NodeId((int)nfks), bec::FKConstraintListBE::RefTable, "table2");
  check_only_one_undo_added();
  ensure("FK ref table was not set", fks->get_selected_fk()->referencedTable()->name() == "table2");
  check_undo();

  check_undo();
}

//------------------------------------------------------------------------------
TEST_FUNCTION(21) // Table (fk set ref column)
{
  db_mysql_TableRef table(db_mysql_TableRef::cast_from(schema->tables()[0]));
  db_mysql_TableRef table2(db_mysql_TableRef::cast_from(schema->tables()[1]));
  std::auto_ptr<MySQLTableEditorBE> be(new MySQLTableEditorBE(table));
  const size_t nfks = table->foreignKeys().count();
  bec::FKConstraintListBE* fks = be->get_fks();

  be->add_fk("fk1");
  check_only_one_undo_added();
  fks->select_fk(bec::NodeId((int)nfks));

  fks->set_field(bec::NodeId((int)nfks), bec::FKConstraintListBE::RefTable, "table2");
  check_only_one_undo_added();
  ensure("FK ref table was not set", fks->get_selected_fk()->referencedTable()->name() == "table2");

  bec::FKConstraintColumnsListBE* fkcols = fks->get_columns();
  fkcols->set_field(bec::NodeId(0), bec::FKConstraintColumnsListBE::Enabled, 1);
  check_only_one_undo_added();
  fkcols->set_field(bec::NodeId(0), bec::FKConstraintColumnsListBE::RefColumn,
                    table2->columns()[table2->columns()->count() - 1]->name());
  check_only_one_undo_added();

  // TODO: Change On Update
  // TODO: Change On Delete

  check_undo(); // set column
  check_undo(); // set ref column

  check_undo(); // set ref table

  check_undo(); // add fk
}

//------------------------------------------------------------------------------

/*

TEST_FUNCTION(36) // Table (inserts)
{
  ensure("not implemented", false);
}


TEST_FUNCTION(37) // Table (partitioning)
{
  ensure("not implemented", false);
}


TEST_FUNCTION(38) // Table (options)
{
  ensure("not implemented", false);
}


TEST_FUNCTION(40) // View
{
  ensure("not implemented", false);
}


TEST_FUNCTION(45) // Routine
{
  ensure("not implemented", false);
}

TEST_FUNCTION(50) // RoutineGroup
{
  ensure("not implemented", false);
}

TEST_FUNCTION(55) // Schema
{
  ensure("not implemented", false);
}

TEST_FUNCTION(60) // Relationship
{
  ensure("not implemented", false);
}

TEST_FUNCTION(61) // Image
{
  ensure("not implemented", false);
}

TEST_FUNCTION(62) // Note
{
  ensure("not implemented", false);
}

TEST_FUNCTION(63) // Stored Script
{
  ensure("not implemented", false);
}

TEST_FUNCTION(64) // Stored Note
{
  ensure("not implemented", false);
}
*/

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  delete tester;
}

END_TESTS
