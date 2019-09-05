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

#include "base/string_utilities.h"

#include "grt.h"

#include "sqlide/recordset_table_inserts_storage.h"

#include "grtdb/editor_table.h"
#include "grtdb/db_object_helpers.h"
#include "sqlide/recordset_be.h"

using namespace grt;
using namespace bec;

namespace {

$ModuleEnvironment() {};

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
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4355)
#endif
  TestTableEditor(db_TableRef table, db_mgmt_RdbmsRef rdbms)
    : TableEditorBE(table), _table(table), _columns(this), _indexes(this) {
  }
#ifdef _MSC_VER
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

$TestData {
  std::unique_ptr<WorkbenchTester> tester;
  db_TableRef table;
  //TestTableEditor *editor;

  db_TableRef makeInsertsTestTable(const db_mgmt_RdbmsRef &rdbms, const db_CatalogRef &catalog) {
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

    col = db_mysql_ColumnRef(grt::Initialized);
    col->owner(table);
    col->name("bitcol");
    col->setParseType("BIT(1)", rdbms->simpleDatatypes());
    table->columns().insert(col);

    return table;
  }

  std::string generateSqlLikeForwardEng(db_TableRef table) {
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

  void testRsStorage(RecordsetRef rs, int row, int column, const std::string &value,
                     const std::string &other_value) {
    $expect(rs->count()).toEqual(1U, "rows before");

    rs->set_field(row, column, value);
    $expect(rs->count()).toEqual(2U, "rows");

    std::string msg;
    bool ok = rs->apply_changes_and_gather_messages(msg);
    $expect(msg).toEqual("Apply complete", "apply changes msg");
    $expect(ok).toBeTrue("apply changes");
    $expect(rs->count()).toEqual(2U, "rows after apply");

    std::string s;
    rs->get_field(row, column, s);
    $expect(s).toEqual(value, "get value back");

    msg.clear();
    // set another value to make sure that UPDATE also works
    rs->set_field(row, column, other_value);
    ok = rs->apply_changes_and_gather_messages(msg);
    $expect(msg).toEqual("Apply complete", "apply changes msg 2");
    $expect(ok).toBeTrue("apply changes 2");
    $expect(rs->count()).toEqual(2U, "rows after apply 2");

    rs->get_field(row, column, s);
    $expect(s).toEqual(other_value, "get value back 2");

    msg.clear();
    //
    rs->set_field(row, column, value);
    ok = rs->apply_changes_and_gather_messages(msg);
    $expect(msg).toEqual("Apply complete", "apply changes msg 3");
    $expect(ok).toBeTrue("apply changes 3");
    $expect(rs->count()).toEqual(2U, "rows after apply 3");

    rs->get_field(row, column, s);
    $expect(s).toEqual(value, "get value back 3");
  }

};

$describe("Table Editor Inserts backend") {

  $beforeAll([this]() {
    data->tester.reset(new WorkbenchTester());
    data->tester->initializeRuntime();
    data->tester->createNewDocument();
  });

  $it("Storage of values with trivial values", [this]() {
    db_TableRef table(data->makeInsertsTestTable(data->tester->getRdbms(), data->tester->getCatalog()));
    {
      TestTableEditor editor(table, data->tester->getRdbms());

      RecordsetRef rs = editor.get_inserts_model();

      // starts with 1 row, which is the placeholder
      $expect(rs->count()).toEqual(1U, "rows");
      $expect(rs->get_column_count()).toEqual(5U, "columns");

      std::string s;
      rs->set_field(0, 0, std::string("1"));
      rs->set_field(0, 1, std::string("test"));
      rs->set_field(0, 2, std::string("2012-01-01"));
      rs->set_field(0, 4, std::string("1"));

      std::string msg;
      bool ok = rs->apply_changes_and_gather_messages(msg);
      $expect(msg).toEqual("Apply complete", "apply changes msg");
      $expect(ok).toBeTrue("apply changes");
    }
    {
      TestTableEditor editor(table, data->tester->getRdbms());

      RecordsetRef rs = editor.get_inserts_model();

      $expect(rs->count()).toEqual(2U, "rows");
      $expect(rs->get_column_count()).toEqual(5U, "columns");

      std::string s;
      rs->get_field(0, 0, s);
      $expect(s).toEqual("1", "get 0");
      rs->get_field(0, 1, s);
      $expect(s).toEqual("test", "get 1");
      rs->get_field(0, 2, s);
      $expect(s).toEqual("2012-01-01", "get 2");
      rs->get_field(0, 4, s);
      $expect(s).toEqual("1", "get 4");

      rs->set_field(1, 0, std::string("42"));
      $expect(rs->count()).toEqual(3U, "added temporary");
      rs->get_field(1, 0, s);
      $expect(s).toEqual("42", "get 0 tmp");
      std::string msg;
      rs->rollback_and_gather_messages(msg);
      $expect(msg).toEqual("", "rollback");
      $expect(rs->count()).toEqual(2U, "reverted");

      // check generation of SQL
      std::string output = table->inserts();
      $expect(output).toEqual("INSERT INTO `table` (`id`, `name`, `ts`, `pic`, `bitcol`) VALUES (1, 'test', '2012-01-01', NULL, 1);\n");

      output = data->generateSqlLikeForwardEng(table);
      $expect(output).toEqual("INSERT INTO `table` (`id`, `name`, `ts`, `pic`, `bitcol`) VALUES (1, 'test', '2012-01-01', NULL, 1);\n");
    }
  });

  $it("Storage of NULL value", [this]() {
    db_TableRef table(data->makeInsertsTestTable(data->tester->getRdbms(), data->tester->getCatalog()));

    TestTableEditor editor(table, data->tester->getRdbms());

    RecordsetRef rs = editor.get_inserts_model();

    rs->set_field(0, 0, std::string("NULL"));
    rs->set_field(0, 1, std::string("NULL"));
    rs->set_field(0, 2, std::string("NULL"));
    rs->set_field(0, 4, std::string("NULL"));

    // just setting a field to NULL doesn't make it NULL, just the string NULL
    std::string s;
    rs->get_field(0, 0, s);
    $expect(s).toEqual("NULL", "check null str store");
    $expect(rs->is_field_null(0, 0)).toBeFalse("check null str store");
    rs->get_field(0, 1, s);
    $expect(s).toEqual("NULL", "check null str store");
    $expect(rs->is_field_null(0, 1)).toBeFalse("check null str store");
    rs->get_field(0, 2, s);
    $expect(s).toEqual("NULL", "check null str store");
    $expect(rs->is_field_null(0, 2)).toBeFalse("check null str store");
    rs->get_field(0, 4, s);
    $expect(s).toEqual("NULL", "check null str store");
    $expect(rs->is_field_null(0, 4)).toBeFalse("check null str store");

    std::string msg;
    rs->apply_changes_and_gather_messages(msg);

    // XXX not sure if setting an int field to the NULL string should result in a real NULL, maybe yes
    std::string output = table->inserts();
    $expect(output).toEqual("INSERT INTO `table` (`id`, `name`, `ts`, `pic`, `bitcol`) VALUES (NULL, 'NULL', 'NULL', NULL, NULL);\n");

    // now actually set the fields to NULL
    rs->set_field_null(0, 0);
    rs->set_field_null(0, 1);
    rs->set_field_null(0, 2);

    rs->get_field(0, 0, s);
    $expect(s).toEqual("", "check null  store");
    $expect(rs->is_field_null(0, 0)).toBeTrue("check null  store");
    rs->get_field(0, 1, s);
    $expect(s).toEqual("", "check null  store");
    $expect(rs->is_field_null(0, 1)).toBeTrue("check null  store");
    rs->get_field(0, 2, s);
    $expect(s).toEqual("", "check null  store");
    $expect(rs->is_field_null(0, 2)).toBeTrue("check null  store");

    rs->apply_changes_and_gather_messages(msg);

    output = table->inserts();
    $expect(output).toEqual("INSERT INTO `table` (`id`, `name`, `ts`, `pic`, `bitcol`) VALUES (DEFAULT, NULL, NULL, NULL, NULL);\n");
  });

  $it("Storage of \\func with int column", [this]() {
    db_TableRef table(data->makeInsertsTestTable(data->tester->getRdbms(), data->tester->getCatalog()));

    TestTableEditor editor(table, data->tester->getRdbms());

    RecordsetRef rs = editor.get_inserts_model();

    data->testRsStorage(rs, 0, 0, "\\func DEFAULT", "1");

    // check generation of SQL
    std::string output = table->inserts();
    $expect(output).toEqual("INSERT INTO `table` (`id`, `name`, `ts`, `pic`, `bitcol`) VALUES (DEFAULT, NULL, NULL, NULL, NULL);\n");

    output = data->generateSqlLikeForwardEng(table);
    $expect(output).toEqual("INSERT INTO `table` (`id`, `name`, `ts`, `pic`, `bitcol`) VALUES (DEFAULT, NULL, NULL, NULL, NULL);\n");
  });

  $it("Storage of \\func with string column", [this]() {
    db_TableRef table(data->makeInsertsTestTable(data->tester->getRdbms(), data->tester->getCatalog()));

    TestTableEditor editor(table, data->tester->getRdbms());

    RecordsetRef rs = editor.get_inserts_model();

    data->testRsStorage(rs, 0, 1, "\\func DEFAULT", "hello");

    // check generation of SQL
    std::string output = table->inserts();
    $expect(output).toEqual("INSERT INTO `table` (`id`, `name`, `ts`, `pic`, `bitcol`) VALUES (DEFAULT, DEFAULT, NULL, NULL, NULL);\n");

    output = data->generateSqlLikeForwardEng(table);
    $expect(output).toEqual("INSERT INTO `table` (`id`, `name`, `ts`, `pic`, `bitcol`) VALUES (DEFAULT, DEFAULT, NULL, NULL, NULL);\n");
  });

  $it("Storage of \\func with timestamp column", [this]() {
    db_TableRef table(data->makeInsertsTestTable(data->tester->getRdbms(), data->tester->getCatalog()));

    TestTableEditor editor(table, data->tester->getRdbms());

    RecordsetRef rs = editor.get_inserts_model();

    data->testRsStorage(rs, 0, 2, "\\func DEFAULT", "2012-01-01");

    // check generation of SQL
    std::string output = table->inserts();
    $expect(output).toEqual("INSERT INTO `table` (`id`, `name`, `ts`, `pic`, `bitcol`) VALUES (DEFAULT, NULL, DEFAULT, NULL, NULL);\n");

    output = data->generateSqlLikeForwardEng(table);
    $expect(output).toEqual("INSERT INTO `table` (`id`, `name`, `ts`, `pic`, `bitcol`) VALUES (DEFAULT, NULL, DEFAULT, NULL, NULL);\n");
  });

  $it("All at once", [this]() {
    db_TableRef table(data->makeInsertsTestTable(data->tester->getRdbms(), data->tester->getCatalog()));

    TestTableEditor editor(table, data->tester->getRdbms());

    RecordsetRef rs = editor.get_inserts_model();

    rs->set_field(0, 0, std::string("\\func DEFAULT"));
    rs->set_field(0, 1, std::string("\\func DEFAULT"));
    rs->set_field(0, 2, std::string("\\func NOW()"));
    rs->set_field(0, 4, std::string("\\func DEFAULT"));
    std::string msg;
    rs->apply_changes_and_gather_messages(msg);
    $expect(msg).toEqual("Apply complete");

    // check generation of SQL
    std::string output = table->inserts();
    $expect(output).toEqual("INSERT INTO `table` (`id`, `name`, `ts`, `pic`, `bitcol`) VALUES (DEFAULT, DEFAULT, NOW(), NULL, DEFAULT);\n");

    output = data->generateSqlLikeForwardEng(table);
    $expect(output).toEqual("INSERT INTO `table` (`id`, `name`, `ts`, `pic`, `bitcol`) VALUES (DEFAULT, DEFAULT, NOW(), NULL, DEFAULT);\n");
  });

}

}
