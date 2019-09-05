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

#include "grtdb/editor_table.h"
#include "grtdb/db_object_helpers.h"

#include "sqlide/recordset_be.h"

using namespace grt;
using namespace bec;
using namespace base;

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
  std::unique_ptr<TestTableEditor> editor;

  db_IndexColumnRef findIndexColumnFor(const grt::ListRef<db_IndexColumn> &cols, const std::string &name) {
    for (size_t c = cols.count(), i = 0; i < c; i++) {
      if (cols[i]->referencedColumn()->name() == name)
        return cols[i];
    }
    return db_IndexColumnRef();
  }
};

$describe("Table Editor Backend") {
  $beforeAll([this]() {
    data->tester.reset(new WorkbenchTester());
    data->tester->initializeRuntime();
    data->tester->createNewDocument();

    $expect(data->tester->wb->get_document().is_valid()).toBeTrue("document ok");
    $expect(data->tester->wb->get_document()->physicalModels()[0]->catalog().is_valid()).toBeTrue("catalog");

    db_mysql_SchemaRef schema = db_mysql_SchemaRef(grt::Initialized);
    $expect(schema.is_valid()).toBeTrue("schema ok");
    schema->owner(data->tester->wb->get_document()->physicalModels()[0]->catalog());
    data->tester->wb->get_document()->physicalModels()[0]->catalog()->schemata().insert(schema);

    data->table = db_mysql_TableRef(grt::Initialized);
    $expect(data->table.is_valid()).toBeTrue("table ok");
    data->table->owner(schema);

    data->editor.reset(new TestTableEditor(data->table, data->tester->getRdbms()));
  });

  $it("Table editing", [this]() {
    $expect(*data->table->name()).toEqual("", "initial table name");
    $expect(data->editor->get_name()).toEqual("", "get table name");

    data->editor->set_name("employee");
    $expect(data->editor->get_name()).toEqual("employee", "table name");
    $expect(data->editor->get_comment()).toEqual("", "table comment");

    data->editor->set_comment("this is a test table");
    $expect(data->editor->get_name()).toEqual("employee", "table name");
    $expect(data->editor->get_comment()).toEqual( "this is a test table", "table comment");
  });

  $it("Column editing", [this]() {
    TableColumnsListBE *clist = data->editor->get_columns();
    NodeId node;
    std::string name;
    std::string type;
    ssize_t ispk;
    bool notnull;
    std::string flags;
    std::string defvalue;
    // int defnull;
    bool flag;

    // count is always +1 because of placeholder
    $expect(clist->count()).toEqual(1U, "initial column count");

    // create column
    node = data->editor->add_column("id");
    $expect(node[0]).toEqual(0U, "new column id");
    $expect(clist->count()).toEqual(2U, "column count");

    $expect(data->table->columns().get(0).is_valid()).toBeTrue("new column");

    flag = clist->set_field(node, TableColumnsListBE::IsPK, 1);
    $expect(flag).toBeTrue("change id pk");

    flag = clist->set_field(node, TableColumnsListBE::Type, "int");
    $expect(flag).toBeTrue("change id type");

    // create column
    node = data->editor->add_column("name");
    $expect(node[0]).toEqual(1U, "new column id");
    $expect(clist->count()).toEqual(3U, "column count");

    flag = clist->get_field(node, TableColumnsListBE::Name, name);
    $expect(flag).toBeTrue("name column name get");
    $expect(name).toEqual("name", "name column name");

    flag = clist->get_field(node, TableColumnsListBE::Type, type);
    $expect(flag).toBeTrue("name column type get");
    $expect(type).toEqual("", "name column type");

    flag = clist->get_field(node, TableColumnsListBE::IsPK, ispk);
    $expect(flag).toBeTrue("name column pk get");
    $expect(ispk).toEqual(0U, "name column pk");

    flag = clist->get_field(node, TableColumnsListBE::IsNotNull, notnull);
    $expect(flag).toBeTrue("name column notnull get");
    $expect(notnull).toBeFalse("name column notnull");

    flag = clist->get_field(node, TableColumnsListBE::Flags, flags);
    $expect(flag).toBeTrue("name column flags get");
    $expect(flags).toEqual("", "name column flags");

    flag = clist->get_field(node, TableColumnsListBE::Default, defvalue);
    $expect(flag).toBeTrue("name column default get");
    $expect(defvalue).toEqual("", "name column default");

    flag = clist->set_field(node, TableColumnsListBE::IsNotNull, 1);
    $expect(flag).toBeTrue("set name notnull");

    flag = clist->set_field(node, TableColumnsListBE::Default, "new");
    $expect(flag).toBeTrue("change name column default");

    // create column
    node = data->editor->add_column("salary");
    flag = clist->set_field(node, TableColumnsListBE::IsNotNull, 0);
    $expect(flag).toBeTrue("set salary !notnull");

    // create column
    node = data->editor->add_column("department");

    // create column
    node = data->editor->add_column("email");

    $expect(clist->count()).toEqual(6U, "column count");

    /// full column listing

    bool ispkb;
    bool notnullb;
    bool isunique;
    bool isbin;
    bool isunsigned;
    bool iszerofill;

    std::string charset;
    std::string collation;
    std::string comment;

    flag = clist->get_row(0, name, type, ispkb, notnullb, isunique, isbin, isunsigned, iszerofill, flags, defvalue,
                          charset, collation, comment);
    $expect(flag).toBeTrue("get_row");
    $expect(name).toEqual("id", "id column name");
    $expect(ispkb).toBeTrue("id column pk");
    $expect(notnullb).toBeTrue("id column notnull");
    $expect(flags).toEqual("", "id column flags");
    $expect(defvalue).toEqual("", "id column default");

    flag = clist->get_row(1, name, type, ispkb, notnullb, isunique, isbin, isunsigned, iszerofill, flags, defvalue,
                          charset, collation, comment);
    $expect(flag).toBeTrue("get_row");
    $expect(name).toEqual("name", "name column name");
    $expect(ispkb).toBeFalse("name column pk");
    $expect(notnullb).toBeTrue("name column notnull");
    $expect(flags).toEqual("", "name column flags");
    $expect(defvalue).toEqual("new", "name column default");
    $expect(charset).toEqual("", "name column charset");
    $expect(collation).toEqual("", "name column collation");

    flag = clist->get_row(2, name, type, ispkb, notnullb, isunique, isbin, isunsigned, iszerofill, flags, defvalue,
                          charset, collation, comment);
    $expect(flag).toBeTrue("get_row");
    $expect(name).toEqual("salary", "salary column name");
    $expect(ispkb).toBeFalse("salary column pk");
    $expect(notnullb).toBeFalse("salary column notnull");
    $expect(flags).toEqual("", "salary column flags");
    $expect(defvalue).toEqual("", "salary column default");

    flag = clist->get_row(3, name, type, ispkb, notnullb, isunique, isbin, isunsigned, iszerofill, flags, defvalue,
                          charset, collation, comment);
    $expect(flag).toBeTrue("get_row");
    $expect(name).toEqual("department", "department column name");
    $expect(ispkb).toBeFalse("department column pk");
    $expect(notnullb).toBeFalse("department column notnull");
    $expect(flags).toEqual("", "department column flags");
    $expect(defvalue).toEqual("", "department column default");
    // $expect(defnullb).toBeFalse("department column default null");

    flag = clist->get_row(4, name, type, ispkb, notnullb, isunique, isbin, isunsigned, iszerofill, flags, defvalue,
                          charset, collation, comment);
    $expect(flag).toBeTrue("get_row");
    $expect(name).toEqual("email", "email column name");
    $expect(ispkb).toBeFalse("email column pk");
    $expect(notnullb).toBeFalse("email column notnull");
    $expect(flags).toEqual("", "email column flags");
    $expect(defvalue).toEqual("", "email column default");

    flag = clist->get_row(5, name, type, ispkb, notnullb, isunique, isbin, isunsigned, iszerofill, flags, defvalue,
                          charset, collation, comment);
    $expect(flag).toBeFalse("bad get_row");

    // unset pk
    flag = clist->get_field(0, TableColumnsListBE::IsPK, ispk);
    $expect(flag).toBeTrue("get id pk");
    $expect(ispk).Not.toEqual(0U, "current id pk");

    flag = clist->set_field(0, TableColumnsListBE::IsPK, 0);
    $expect(flag).toBeTrue("unset id pk");

    flag = clist->get_field(0, TableColumnsListBE::IsPK, ispk);
    $expect(flag).toBeTrue("get new pk");
    $expect(ispk).toEqual(0U, "new id pk");

    // unset again
    flag = clist->set_field(0, TableColumnsListBE::IsPK, 0);
    $expect(flag).toBeTrue("unset id pk again");

    flag = clist->get_field(0, TableColumnsListBE::IsPK, ispk);
    $expect(flag).toBeTrue("get new pk");
    $expect(ispk).toEqual(0U, "new id pk");

    // set back
    flag = clist->set_field(0, TableColumnsListBE::IsPK, 1);
    $expect(flag).toBeTrue("unset id pk again");

    flag = clist->get_field(0, TableColumnsListBE::IsPK, ispk);
    $expect(flag).toBeTrue("get new pk");
    $expect(ispk).Not.toEqual(0U, "new id pk");

    // and again
    flag = clist->set_field(0, TableColumnsListBE::IsPK, 1);
    $expect(flag).toBeTrue("unset id pk again");

    flag = clist->get_field(0, TableColumnsListBE::IsPK, ispk);
    $expect(flag).toBeTrue("get new pk");
    $expect(ispk).Not.toEqual(0U, "new id pk");

    name = data->editor->get_column_with_name("name")->name();
    $expect(name).toEqual("name", "get_column_with_name");
  });

  $it("Column deletion", [this]() {
    TableColumnsListBE *clist = data->editor->get_columns();
    bool flag;

    $expect(clist->count()).toEqual(6U, "column count");

    data->editor->remove_column(3);

    $expect(clist->count()).toEqual(5U, "new column count");

    std::string name;

    flag = clist->get_field(2, TableColumnsListBE::Name, name);
    $expect(flag).toBeTrue("get 2");
    $expect(name).toEqual("salary", "2 name");

    flag = clist->get_field(3, TableColumnsListBE::Name, name);
    $expect(flag).toBeTrue("get 3");
    $expect(name).toEqual("email", "3 name");

    // Placeholder row, doesn't return a value, so the previous one stays.
    flag = clist->get_field(4, TableColumnsListBE::Name, name);
    $expect(flag).toBeFalse("get 4");
    $expect(name).toEqual("email", "4 name");
  });

  $it("Setting different types", []() {
    $pending("needs implementation");
  });

  $it("Setting different flags", []() {
    $pending("needs implementation");
  });

  $it("Index editing", [this]() {
    IndexListBE *index = data->editor->get_indexes();
    IndexColumnsListBE *icolumns = index->get_columns();
    std::string name, type, comment;
    bool flag;

    $expect(data->editor->get_columns()->count()).toEqual(5U, "column count");
    $expect(data->editor->get_indexes()->count()).toEqual(2U, "index count");

    NodeId node = data->editor->add_index("idx1");

    index->select_index(node);
    //  index->set_field(0, IndexListBE::Name).toEqual("id");

    $expect(index->count()).toEqual(3U, "index count");

    flag = index->get_field(1, IndexListBE::Name, name);
    $expect(flag).toBeTrue("get index name");
    $expect(name).toEqual("idx1", "index name");
    flag = index->get_field(1, IndexListBE::Type, type);
    $expect(flag).toBeTrue("get index type");
    $expect(type).toEqual("type1", "index type");
    flag = index->get_field(1, IndexListBE::Comment, comment);
    $expect(flag).toBeTrue("get index comment");
    $expect(comment).toEqual("", "index comment");

    // change

    flag = index->set_field(1, IndexListBE::Name, "index1");
    $expect(flag).toBeTrue("set index name");
    flag = index->set_field(1, IndexListBE::Type, "bleqw");
    $expect(flag).toBeFalse("set bad index type");
    flag = index->set_field(1, IndexListBE::Type, "type2");
    $expect(flag).toBeTrue("set index type");
    flag = index->set_field(1, IndexListBE::Comment, "test index");
    $expect(flag).toBeTrue("set index comment");

    index->select_index(1);

    // add column 1 (name) to index
    icolumns->set_column_enabled(1, true);

    // change column stuff
    flag = icolumns->set_field(1, IndexColumnsListBE::Length, 20);
    $expect(flag).toBeTrue("set length");
    flag = icolumns->set_field(1, IndexColumnsListBE::Descending, 1);
    $expect(flag).toBeTrue("set desc");
    std::string x;
    flag = icolumns->get_field(1, IndexColumnsListBE::Descending, x);
    $expect(x).toEqual("1", "set desc");

    $expect(icolumns->count()).toEqual(data->editor->get_columns()->count() - 1, "index column count");

    // list indexes and columns

    // ** the pk index
    flag = index->get_field(0, IndexListBE::Name, name);
    $expect(flag).toBeTrue("get index name");
    $expect(name).toEqual("PRIMARY", "index name");
    flag = index->get_field(0, IndexListBE::Type, type);
    $expect(flag).toBeTrue("get index type");
    $expect(type).toEqual("PRIMARY", "index type");
    flag = index->get_field(0, IndexListBE::Comment, comment);
    $expect(flag).toBeTrue("get index comment");
    $expect(comment).toEqual("", "index comment");

    std::string buf;

    index->select_index(0);
    $expect(icolumns->count()).toEqual(data->editor->get_columns()->count() - 1, "index column count");

    // check if list of index columns matches list of table columns
    db_TableRef table(data->editor->get_table());
    for (size_t i = 0; i < table->columns().count(); i++) {
      flag = icolumns->get_field(i, IndexColumnsListBE::Name, name);
      $expect(flag).toBeTrue(strfmt("index column[%lu] name", i));
      $expect(name).toEqual(*data->table->columns()[i]->name(), strfmt("index column[%lu] name", i));
      flag = icolumns->get_field(i, IndexColumnsListBE::Descending, buf);
      $expect(flag).toBeTrue(strfmt("index column[%lu] desc", i));
      $expect(buf).toEqual("0", strfmt("index column[%lu] desc", i));
      flag = icolumns->get_field(i, IndexColumnsListBE::Length, buf);
      $expect(flag).toBeTrue(strfmt("index column[%lu] length", i));
      $expect(buf).toEqual("0", strfmt("index column[%lu] length", i));
      flag = icolumns->get_column_enabled(i);
      if (name == "id") {
        $expect(flag).toBeTrue(strfmt("index column[%lu] enabled", i));
      } else {
        $expect(flag).toBeFalse(strfmt("index column[%lu] enabled", i));
      }
    }

    // ** the idx we added
    flag = index->get_field(1, IndexListBE::Name, name);
    $expect(flag).toBeTrue("get index name");
    $expect(name).toEqual("index1", "index name");
    flag = index->get_field(1, IndexListBE::Type, type);
    $expect(flag).toBeTrue("get index type");
    $expect(type).toEqual("type2", "index type");
    flag = index->get_field(1, IndexListBE::Comment, comment);
    $expect(flag).toBeTrue("get index comment");
    $expect(comment).toEqual("test index", "index comment");

    index->select_index(1);

    for (size_t i = 0; i < table->columns().count(); i++) {
      flag = icolumns->get_field(i, IndexColumnsListBE::Name, name);
      $expect(flag).toBeTrue(strfmt("index column[%lu] name", i));
      $expect(name).toEqual(*data->table->columns()[i]->name(), strfmt("index column[%lu] name", i));
      if (name == "name") {
        flag = icolumns->get_field(i, IndexColumnsListBE::Descending, buf);
        $expect(flag).toBeTrue(strfmt("index column[%lu] desc", i));
        $expect(buf).toEqual("1", strfmt("index column[%lu] desc", i));
        flag = icolumns->get_field(i, IndexColumnsListBE::Length, buf);
        $expect(flag).toBeTrue(strfmt("index column[%lu] length", i));
        $expect(buf).toEqual("20", strfmt("index column[%lu] length", i));
        flag = icolumns->get_column_enabled(i);
        $expect(flag).toBeTrue(strfmt("index column[%lu] enabled", i));
      } else {
        flag = icolumns->get_field(i, IndexColumnsListBE::Descending, buf);
        $expect(flag).toBeTrue(strfmt("index column[%lu] desc", i));
        $expect(buf).toEqual("0", strfmt("index column[%lu] desc", i));
        flag = icolumns->get_field(i, IndexColumnsListBE::Length, buf);
        $expect(flag).toBeTrue(strfmt("index column[%lu] length", i));
        $expect(buf).toEqual("0", strfmt("index column[%lu] length", i));
        flag = icolumns->get_column_enabled(i);
        $expect(flag).toBeFalse(strfmt("index column[%lu] enabled", i));
      }
    }

    // remove index
    data->editor->remove_index(1, false);

    $expect(index->count()).toEqual(2U, "index count");

    // add new one with convenience func
    std::vector<NodeId> columns;
    columns.push_back(NodeId(1)); // name
    columns.push_back(NodeId(3)); // email
    node = data->editor->add_index_with_columns(columns);

    $expect(node[0]).toEqual(1U, "new index");

    $expect(index->count()).toEqual(3U, "index count");

    index->set_field(1, IndexListBE::Name, "namemail_index");

    flag = index->get_field(1, IndexListBE::Name, name);
    $expect(flag).toBeTrue("get index name");
    $expect(name).toEqual("namemail_index", "index name");
    flag = index->get_field(1, IndexListBE::Type, type);
    $expect(flag).toBeTrue("get index type");
    $expect(type).toEqual("type1", "index type");
    flag = index->get_field(1, IndexListBE::Comment, comment);
    $expect(flag).toBeTrue("get index comment");
    $expect(comment).toEqual("", "index comment");

    index->select_index(1);

    for (size_t i = 0; i < (size_t)icolumns->count(); i++) {
      flag = icolumns->get_field(i, IndexColumnsListBE::Name, name);
      $expect(flag).toBeTrue(strfmt("index column[%lu] name", i));
      $expect(name).toEqual(*data->table->columns()[i]->name(), strfmt("index column[%lu] name", i));
      flag = icolumns->get_field(i, IndexColumnsListBE::Descending, buf);
      $expect(flag).toBeTrue(strfmt("index column[%lu] desc", i));
      $expect(buf).toEqual("0", strfmt("index column[%lu] desc", i));
      flag = icolumns->get_field(i, IndexColumnsListBE::Length, buf);
      $expect(flag).toBeTrue(strfmt("index column[%lu] length", i));
      $expect(buf).toEqual("0", strfmt("index column[%lu] length", i));

      if (name == "name" || name == "email") {
        flag = icolumns->get_column_enabled(i);
        $expect(flag).toBeTrue(strfmt("index column[%lu] enabled", i));
      } else {
        flag = icolumns->get_column_enabled(i);
        $expect(flag).toBeFalse(strfmt("index column[%lu] enabled", i));
      }
    }
  });

  $it("Delete a column and watch index deletion", [this]() {
    IndexListBE *index = data->editor->get_indexes();
    // IndexColumnsListBE *icolumns= index->get_columns();
    std::vector<NodeId> columns;
    columns.push_back(NodeId(3)); // email
    data->editor->add_index_with_columns(columns);

    $expect(index->count()).toEqual(4U, "index count");
    $expect(data->editor->get_columns()->count()).toEqual(5U, "column count");

    std::string name;
    bool flag;

    flag = index->get_field(1, IndexListBE::Name, name);
    $expect(flag).toBeTrue("get index[1] name");
    $expect(name).toEqual("namemail_index", "get index[1] name");
    index->select_index(1);
    $expect(data->table->indices().get(1)->columns().count()).toEqual(2U, "get index[1] column.count()");

    data->editor->remove_column(3); // delete column email

    $expect(data->table->indices().get(1)->columns().count()).toEqual(1U, "column count");
    $expect(index->count()).toEqual(3U, "index count");

    flag = index->get_field(0, IndexListBE::Name, name);
    $expect(flag).toBeTrue("get index[0] name");
    $expect(name).toEqual("PRIMARY", "get index[0] name");
    index->select_index(0);
    $expect(data->table->indices().get(0)->columns().count()).toEqual(1U, "get index[0] column.count()");

    flag = index->get_field(1, IndexListBE::Name, name);
    $expect(flag).toBeTrue("get index[1] name");
    $expect(name).toEqual("namemail_index", "get index[1] name");
    index->select_index(1);
    $expect(data->table->indices().get(0)->columns().count()).toEqual(1U, "get index[1] column.count()");

    flag = index->get_field(2, IndexListBE::Name, name);
    $expect(flag).toBeTrue("get index[2] name");
    $expect(name).toEqual("", "get index[2] name");
  });

  $it("Bug: unsetting an index column is wrong (and may crash)", [this]() {
    db_TableRef table = db_mysql_TableRef(grt::Initialized);
    table->owner(data->tester->getSchema());

    $expect(table.is_valid()).toBeTrue("table ok");

    db_ColumnRef column(grt::Initialized);
    column->owner(table);
    column->name("col1");
    table->columns().insert(column);

    column = db_ColumnRef(grt::Initialized);
    column->owner(table);
    column->name("col2");
    table->columns().insert(column);

    column = db_ColumnRef(grt::Initialized);
    column->owner(table);
    column->name("col3");
    table->columns().insert(column);

    TestTableEditor ed(table, data->tester->getRdbms());

    $expect(ed.get_columns()->count()).toEqual(4U, "column count");

    ed.add_index("hello");

    $expect(ed.get_indexes()->count()).toEqual(2U, "index count");

    ed.get_indexes()->select_index(0);

    $expect(ed.get_indexes()->get_columns()->count()).toEqual(3U, "index column item count");

    IndexColumnsListBE *ic = ed.get_indexes()->get_columns();
    std::string name;

    ic->get_field(0, IndexColumnsListBE::Name, name);
    $expect(name).toEqual("col1", "col1");
    $expect(ic->get_column_enabled(0)).toBeFalse("col1 disabled");

    ic->get_field(1, IndexColumnsListBE::Name, name);
    $expect(name).toEqual("col2", "col2");
    $expect(ic->get_column_enabled(1)).toBeFalse("col2 disabled");

    ic->get_field(2, IndexColumnsListBE::Name, name);
    $expect(name).toEqual("col3", "col3");
    $expect(ic->get_column_enabled(2)).toBeFalse("col3 disabled");

    // enable 2 columns
    ic->set_column_enabled(1, true);
    $expect(data->findIndexColumnFor(table->indices().get(0)->columns(), "col2").is_valid()).toBeTrue("col2 in list");

    ic->set_column_enabled(0, true);
    $expect(data->findIndexColumnFor(table->indices().get(0)->columns(), "col1").is_valid()).toBeTrue("col1 in list");

    // disable one of them and make sure the right column is removed
    ic->set_column_enabled(0, false);
    $expect(data->findIndexColumnFor(table->indices().get(0)->columns(), "col2").is_valid()).toBeTrue("col2 in list");
    $expect(data->findIndexColumnFor(table->indices().get(0)->columns(), "col1").is_valid()).toBeFalse("col1 not in list");

    // disable all
    ic->set_column_enabled(1, false);

    // toggle last (will crash if buggy)
    ic->set_column_enabled(2, true);
    ic->set_column_enabled(2, false);
  });

  $it("Placeholder handling", [this]() {
    db_mysql_TableRef table(grt::Initialized);

    table->owner(data->tester->getSchema());
    table->name("table");

    TestTableEditor editor(table, data->tester->getRdbms());

    editor.get_columns()->set_field(0, 0, "newcol");

    // auto-adds a PK
    editor.get_columns()->set_field(0, 1, "int(11)");
    $expect(table->indices().count()).toEqual(1U, "autoadd PK index");
    $expect(table->indices()[0]->name().c_str()).toEqual("PRIMARY", "autoadd PK index");

    $expect(table->columns().count()).toEqual(1U, "add column");

    $expect(table->indices().count()).toEqual(1U, "add index");
    editor.get_indexes()->set_field(1, 0, "index");
    $expect(table->indices().count()).toEqual(2U, "add index");

    $expect(table->foreignKeys().count()).toEqual(0U, "add fk");
    editor.get_fks()->set_field(0, 0, "newfk");
    $expect(table->foreignKeys().count()).toEqual(1U, "add fk");
  });

  $it("Bug: create fk, select column and then deselect will crash", [this]() {
    db_mysql_TableRef table(grt::Initialized);
    $expect(table.is_valid()).toBeTrue("table ok");

    table->name("table");
    table->owner(data->tester->getSchema());
    data->tester->getSchema()->tables().insert(data->table);

    TestTableEditor editor(table, data->tester->getRdbms());

    $expect(editor.get_catalog().is_valid()).toBeTrue("editor catalog is not ok");

    grt::ListRef<db_UserDatatype> userTypes(editor.get_catalog()->userDatatypes());

    db_mysql_ColumnRef column(grt::Initialized);
    column->owner(table);
    column->name("id");
    column->setParseType("int", data->tester->getRdbms()->simpleDatatypes());
    table->columns().insert(column);

    column = db_mysql_ColumnRef(grt::Initialized);
    column->owner(table);
    column->name("fk");
    column->setParseType("int", data->tester->getRdbms()->simpleDatatypes());
    table->columns().insert(column);

    $expect(editor.get_columns()->count()).toEqual(3U, "columns list ok");

    std::vector<bec::NodeId> columns;
    columns.push_back(bec::NodeId(1));
    editor.add_fk_with_columns(columns);
    $expect(editor.get_fks()->count()).toEqual(2U, "fk added");

    editor.get_fks()->select_fk(bec::NodeId(0));

    editor.get_fks()->set_field(0, bec::FKConstraintListBE::RefTable, "table");

    $expect(editor.get_fks()->get_columns()->count()).toEqual(2U, "columns in fk");
  });
}

}
