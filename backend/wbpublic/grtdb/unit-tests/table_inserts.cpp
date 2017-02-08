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

#include "grt.h"

#include "sqlide/recordset_table_inserts_storage.h"
#include "grtdb/editor_table.h"
#include "grtdb/db_object_helpers.h"
#include "base/string_utilities.h"
#include "sqlide/recordset_be.h"
#include "wb_helpers.h"

using namespace grt;
using namespace bec;
using namespace base;

class TestTableColumnsListBE : public TableColumnsListBE {
public:
  TestTableColumnsListBE(TableEditorBE *ed) : TableColumnsListBE(ed) {
  }

  virtual std::vector<std::string> get_datatype_names() {
    return std::vector<std::string>();
  }
};

// XXX: duplicate implementation (the other is in editor_table_tests.cpp). Merge them!
class TestTableEditor : public TableEditorBE {
  db_TableRef _table;
  TestTableColumnsListBE _columns;
  IndexListBE _indexes;

public:
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4355)
#endif
  TestTableEditor(db_TableRef table, db_mgmt_RdbmsRef rdbms)
    : TableEditorBE(table), _table(table), _columns(this), _indexes(this) {
  }
#ifdef _WIN32
#pragma warning(pop)
#endif

  db_TableRef get_table() {
    return _table;
  }

  virtual TableColumnsListBE *get_columns() {
    return &_columns;
  }

  virtual IndexListBE *get_indexes() {
    return &_indexes;
  }

  virtual void edit_object(const ObjectRef &v) {
  }

  virtual std::vector<std::string> get_index_types() {
    std::vector<std::string> index_types;
    index_types.push_back("type1");
    index_types.push_back("type2");
    index_types.push_back("type3");
    index_types.push_back("PRIMARY");
    return index_types;
  }

  void set_table_option_by_name(const std::string &name, const std::string &value) {
    // TODO: implement
  }

  std::string get_table_option_by_name(const std::string &name) {
    // TODO: implement
    return std::string();
  }

  std::vector<std::string> get_charsets_list() {
    return std::vector<std::string>();
  }

  virtual bool check_column_referenceable_by_fk(const db_ColumnRef &column1, const db_ColumnRef &column2) {
    // TODO: implement
    return false;
  }

  virtual db_TableRef create_stub_table(const std::string &schema, const std::string &table) {
    return db_TableRef(); // TODO: implement and add tests.
  }
};

BEGIN_TEST_DATA_CLASS(table_inserts)
public:
WBTester *wbt;
db_TableRef table;
Auto_release autorel;

TestTableEditor *editor;
TEST_DATA_CONSTRUCTOR(table_inserts) : editor(0) {
  wbt = new WBTester;
  populate_grt(*wbt);
}

TEST_DATA_DESTRUCTOR(table_inserts) {
  delete editor;
}
END_TEST_DATA_CLASS

TEST_MODULE(table_inserts, "Table Editor Inserts backend");

static db_TableRef make_inserts_test_table(const db_mgmt_RdbmsRef &rdbms, const db_CatalogRef &catalog) {
  grt::ListRef<db_UserDatatype> usertypes;

  db_SchemaRef schema(grt::Initialized);
  schema->owner(catalog);

  db_mysql_TableRef table(grt::Initialized);
  table->owner(schema);
  table->name("table");

  db_mysql_ColumnRef col(grt::Initialized);
  col->owner(table);
  col->name("id");
  col->autoIncrement(1);
  col->setParseType("INT", rdbms->simpleDatatypes());
  table->columns().insert(col);
  table->addPrimaryKeyColumn(col);

  col = db_mysql_ColumnRef(grt::Initialized);
  col->owner(table);
  col->name("name");
  col->setParseType("VARCHAR(30)", rdbms->simpleDatatypes());
  table->columns().insert(col);

  col = db_mysql_ColumnRef(grt::Initialized);
  col->owner(table);
  col->name("ts");
  col->setParseType("TIMESTAMP", rdbms->simpleDatatypes());
  table->columns().insert(col);

  col = db_mysql_ColumnRef(grt::Initialized);
  col->owner(table);
  col->name("pic");
  col->setParseType("BLOB", rdbms->simpleDatatypes());
  table->columns().insert(col);

  return table;
}

TEST_FUNCTION(1) {
  wbt->create_new_document();
}

static std::string generate_sql_just_like_fwd_eng(db_TableRef table) {
  // this code copied verbatim from module_db_mysql.cpp
  Recordset_table_inserts_storage::Ref input_storage = Recordset_table_inserts_storage::create();
  input_storage->table(table);

  Recordset::Ref rs = Recordset::create();
  rs->data_storage(input_storage);
  rs->reset();

  Recordset_sql_storage::Ref output_storage = Recordset_sql_storage::create();
  output_storage->table_name(table->name());
  output_storage->rdbms(db_mgmt_RdbmsRef::cast_from(
    table->owner() /*schema*/->owner() /*catalog*/->owner() /*phys.model*/->get_member("rdbms")));
  output_storage->schema_name(table->owner()->name());
  output_storage->omit_schema_qualifier(true);
  output_storage->binding_blobs(false);
  output_storage->serialize(rs);
  return output_storage->sql_script();
}

TEST_FUNCTION(5) {
  // test proper storage of values with trivial values
  db_TableRef table(make_inserts_test_table(wbt->get_rdbms(), wbt->get_catalog()));

  {
    TestTableEditor editor(table, wbt->get_rdbms());

    RecordsetRef rs = editor.get_inserts_model();

    // starts with 1 row, which is the placeholder
    ensure_equals("rows", rs->count(), 1U);
    ensure_equals("columns", rs->get_column_count(), 4U);

    std::string s;
    rs->set_field(0, 0, std::string("1"));
    rs->set_field(0, 1, std::string("test"));
    rs->set_field(0, 2, std::string("2012-01-01"));

    std::string msg;
    bool ok = rs->apply_changes_and_gather_messages(msg);
    ensure_equals("apply changes msg", msg, "Apply complete");
    ensure("apply changes", ok);
  }
  {
    TestTableEditor editor(table, wbt->get_rdbms());

    RecordsetRef rs = editor.get_inserts_model();

    ensure_equals("rows", rs->count(), 2U);
    ensure_equals("columns", rs->get_column_count(), 4U);

    std::string s;
    rs->get_field(0, 0, s);
    ensure_equals("get 0", s, std::string("1"));
    rs->get_field(0, 1, s);
    ensure_equals("get 1", s, std::string("test"));
    rs->get_field(0, 2, s);
    ensure_equals("get 2", s, std::string("2012-01-01"));

    rs->set_field(1, 0, std::string("42"));
    ensure_equals("added temporary", rs->count(), 3U);
    rs->get_field(1, 0, s);
    ensure_equals("get 0 tmp", s, std::string("42"));
    std::string msg;
    rs->rollback_and_gather_messages(msg);
    ensure_equals("rollback", msg, "");
    ensure_equals("reverted", rs->count(), 2U);

    // check generation of SQL
    std::string output = table->inserts();
    ensure_equals("generated sql", output,
                  "INSERT INTO `table` (`id`, `name`, `ts`, `pic`) VALUES (1, 'test', '2012-01-01', NULL);\n");

    output = generate_sql_just_like_fwd_eng(table);
    ensure_equals("fwd eng sql", output,
                  "INSERT INTO `table` (`id`, `name`, `ts`, `pic`) VALUES (1, 'test', '2012-01-01', NULL);\n");
  }
}

static void test_rs_storage(RecordsetRef rs, int row, int column, const std::string &value,
                            const std::string &other_value) {
  ensure_equals("rows before", rs->count(), 1U);

  rs->set_field(row, column, value);
  ensure_equals("rows", rs->count(), 2U);

  std::string msg;
  bool ok = rs->apply_changes_and_gather_messages(msg);
  ensure_equals("apply changes msg", msg, "Apply complete");
  ensure("apply changes", ok);
  ensure_equals("rows after apply", rs->count(), 2U);

  std::string s;
  rs->get_field(row, column, s);
  ensure_equals("get value back", s, value);

  msg.clear();
  // set another value to make sure that UPDATE also works
  rs->set_field(row, column, other_value);
  ok = rs->apply_changes_and_gather_messages(msg);
  ensure_equals("apply changes msg 2", msg, "Apply complete");
  ensure("apply changes 2", ok);
  ensure_equals("rows after apply 2", rs->count(), 2U);

  rs->get_field(row, column, s);
  ensure_equals("get value back 2", s, other_value);

  msg.clear();
  //
  rs->set_field(row, column, value);
  ok = rs->apply_changes_and_gather_messages(msg);
  ensure_equals("apply changes msg 3", msg, "Apply complete");
  ensure("apply changes 3", ok);
  ensure_equals("rows after apply 3", rs->count(), 2U);

  rs->get_field(row, column, s);
  ensure_equals("get value back 3", s, value);
}

TEST_FUNCTION(6) {
  // check storage of NULL value
  db_TableRef table(make_inserts_test_table(wbt->get_rdbms(), wbt->get_catalog()));

  TestTableEditor editor(table, wbt->get_rdbms());

  RecordsetRef rs = editor.get_inserts_model();

  rs->set_field(0, 0, std::string("NULL"));
  rs->set_field(0, 1, std::string("NULL"));
  rs->set_field(0, 2, std::string("NULL"));

  // just setting a field to NULL doesn't make it NULL, just the string NULL
  std::string s;
  rs->get_field(0, 0, s);
  ensure_equals("check null str store", s, "NULL");
  ensure("check null str store", !rs->is_field_null(0, 0));
  rs->get_field(0, 1, s);
  ensure_equals("check null str store", s, "NULL");
  ensure("check null str store", !rs->is_field_null(0, 1));
  rs->get_field(0, 2, s);
  ensure_equals("check null str store", s, "NULL");
  ensure("check null str store", !rs->is_field_null(0, 2));
  std::string msg;
  rs->apply_changes_and_gather_messages(msg);

  // XXX not sure if setting an int field to the NULL string should result in a real NULL, maybe yes
  std::string output = table->inserts();
  ensure_equals("output1", output,
                "INSERT INTO `table` (`id`, `name`, `ts`, `pic`) VALUES (NULL, 'NULL', 'NULL', NULL);\n");

  // now actually set the fields to NULL
  rs->set_field_null(0, 0);
  rs->set_field_null(0, 1);
  rs->set_field_null(0, 2);

  rs->get_field(0, 0, s);
  ensure_equals("check null  store", s, "");
  ensure("check null  store", rs->is_field_null(0, 0));
  rs->get_field(0, 1, s);
  ensure_equals("check null  store", s, "");
  ensure("check null  store", rs->is_field_null(0, 1));
  rs->get_field(0, 2, s);
  ensure_equals("check null  store", s, "");
  ensure("check null  store", rs->is_field_null(0, 2));

  rs->apply_changes_and_gather_messages(msg);

  output = table->inserts();
  ensure_equals("output2", output,
                "INSERT INTO `table` (`id`, `name`, `ts`, `pic`) VALUES (DEFAULT, NULL, NULL, NULL);\n");
}

TEST_FUNCTION(11) {
  // test storage of \\func with int column
  db_TableRef table(make_inserts_test_table(wbt->get_rdbms(), wbt->get_catalog()));

  TestTableEditor editor(table, wbt->get_rdbms());

  RecordsetRef rs = editor.get_inserts_model();

  test_rs_storage(rs, 0, 0, "\\func DEFAULT", "1");

  // check generation of SQL
  std::string output = table->inserts();
  ensure_equals("generated sql", output,
                "INSERT INTO `table` (`id`, `name`, `ts`, `pic`) VALUES (DEFAULT, NULL, NULL, NULL);\n");

  output = generate_sql_just_like_fwd_eng(table);
  ensure_equals("generated sql", output,
                "INSERT INTO `table` (`id`, `name`, `ts`, `pic`) VALUES (DEFAULT, NULL, NULL, NULL);\n");
}

TEST_FUNCTION(12) {
  // test storage of \\func with string column

  db_TableRef table(make_inserts_test_table(wbt->get_rdbms(), wbt->get_catalog()));

  TestTableEditor editor(table, wbt->get_rdbms());

  RecordsetRef rs = editor.get_inserts_model();

  test_rs_storage(rs, 0, 1, "\\func DEFAULT", "hello");

  // check generation of SQL
  std::string output = table->inserts();
  ensure_equals("generated sql", output,
                "INSERT INTO `table` (`id`, `name`, `ts`, `pic`) VALUES (DEFAULT, DEFAULT, NULL, NULL);\n");

  output = generate_sql_just_like_fwd_eng(table);
  ensure_equals("generated sql", output,
                "INSERT INTO `table` (`id`, `name`, `ts`, `pic`) VALUES (DEFAULT, DEFAULT, NULL, NULL);\n");
}

TEST_FUNCTION(13) {
  // test storage of \\func with timestamp column

  db_TableRef table(make_inserts_test_table(wbt->get_rdbms(), wbt->get_catalog()));

  TestTableEditor editor(table, wbt->get_rdbms());

  RecordsetRef rs = editor.get_inserts_model();

  test_rs_storage(rs, 0, 2, "\\func DEFAULT", "2012-01-01");

  // check generation of SQL
  std::string output = table->inserts();
  ensure_equals("generated sql", output,
                "INSERT INTO `table` (`id`, `name`, `ts`, `pic`) VALUES (DEFAULT, NULL, DEFAULT, NULL);\n");

  output = generate_sql_just_like_fwd_eng(table);
  ensure_equals("generated sql", output,
                "INSERT INTO `table` (`id`, `name`, `ts`, `pic`) VALUES (DEFAULT, NULL, DEFAULT, NULL);\n");
}

TEST_FUNCTION(15) {
  // all at once
  db_TableRef table(make_inserts_test_table(wbt->get_rdbms(), wbt->get_catalog()));

  TestTableEditor editor(table, wbt->get_rdbms());

  RecordsetRef rs = editor.get_inserts_model();

  rs->set_field(0, 0, std::string("\\func DEFAULT"));
  rs->set_field(0, 1, std::string("\\func DEFAULT"));
  rs->set_field(0, 2, std::string("\\func NOW()"));
  std::string msg;
  rs->apply_changes_and_gather_messages(msg);
  ensure_equals("apply", msg, "Apply complete");

  // check generation of SQL
  std::string output = table->inserts();
  ensure_equals("generated sql", output,
                "INSERT INTO `table` (`id`, `name`, `ts`, `pic`) VALUES (DEFAULT, DEFAULT, NOW(), NULL);\n");

  output = generate_sql_just_like_fwd_eng(table);
  ensure_equals("generated sql", output,
                "INSERT INTO `table` (`id`, `name`, `ts`, `pic`) VALUES (DEFAULT, DEFAULT, NOW(), NULL);\n");
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  delete wbt;
}

END_TESTS
