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

BEGIN_TEST_DATA_CLASS(editor_table_tests)
public:
WBTester *wbt;
db_TableRef table;
Auto_release autorel;

TestTableEditor *editor;
TEST_DATA_CONSTRUCTOR(editor_table_tests) : wbt(new WBTester()), editor(0) {
  populate_grt(*wbt);
}

TEST_DATA_DESTRUCTOR(editor_table_tests) {
  delete editor;
}
END_TEST_DATA_CLASS

TEST_MODULE(editor_table_tests, "Table Editor backend");

TEST_FUNCTION(1) {
  wbt->create_new_document();

  ensure("document ok", wbt->wb->get_document().is_valid());
  ensure("catalog", wbt->wb->get_document()->physicalModels()[0]->catalog().is_valid());

  db_mysql_SchemaRef schema = db_mysql_SchemaRef(grt::Initialized);
  ensure("schema ok", schema.is_valid());
  schema->owner(wbt->wb->get_document()->physicalModels()[0]->catalog());
  wbt->wb->get_document()->physicalModels()[0]->catalog()->schemata().insert(schema);

  table = db_mysql_TableRef(grt::Initialized);
  ensure("table ok", table.is_valid());
  table->owner(schema);

  editor = new TestTableEditor(table, wbt->get_rdbms());

  // Don't use the auto releaser here. It conflicts with the static test class leading to
  // a crash when the tests are finished.
}

TEST_FUNCTION(2) { // test table editing
  ensure("table editor !NULL", editor != NULL);

  ensure_equals("initial table name", *table->name(), "");

  ensure_equals("get table name", editor->get_name(), "");
  editor->set_name("employee");
  ensure_equals("table name", editor->get_name(), "employee");
  ensure_equals("table comment", editor->get_comment(), "");

  editor->set_comment("this is a test table");
  ensure_equals("table name", editor->get_name(), "employee");
  ensure_equals("table comment", editor->get_comment(), "this is a test table");
}

TEST_FUNCTION(3) { // test column editing
  ensure("table editor !NULL", editor != NULL);

  TableColumnsListBE *clist = editor->get_columns();
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
  ensure_equals("initial column count", clist->count(), 1U);

  // create column
  node = editor->add_column("id");
  ensure_equals("new column id", node[0], 0U);
  ensure_equals("column count", clist->count(), 2U);

  ensure("new column", table->columns().get(0).is_valid());

  flag = clist->set_field(node, TableColumnsListBE::IsPK, 1);
  ensure("change id pk", flag);

  //
  //  flag= clist->set_field(node, TableColumnsListBE::Type, "int");
  //  ensure("change id type", flag);

  // create column
  node = editor->add_column("name");
  ensure_equals("new column id", node[0], 1U);
  ensure_equals("column count", clist->count(), 3U);

  flag = clist->get_field(node, TableColumnsListBE::Name, name);
  ensure("name column name get", flag);
  ensure_equals("name column name", name, "name");

  flag = clist->get_field(node, TableColumnsListBE::Type, type);
  ensure("name column type get", flag);
  ensure_equals("name column type", type, "");

  flag = clist->get_field(node, TableColumnsListBE::IsPK, ispk);
  ensure("name column pk get", flag);
  ensure_equals("name column pk", ispk != 0, false);

  flag = clist->get_field(node, TableColumnsListBE::IsNotNull, notnull);
  ensure("name column notnull get", flag);
  ensure_equals("name column notnull", notnull, false);

  flag = clist->get_field(node, TableColumnsListBE::Flags, flags);
  ensure("name column flags get", flag);
  ensure_equals("name column flags", flags, "");

  flag = clist->get_field(node, TableColumnsListBE::Default, defvalue);
  ensure("name column default get", flag);
  ensure_equals("name column default", defvalue, "");

  //  flag= clist->get_field(node, TableColumnsListBE::DefaultNull, defnull);
  //  ensure("name column defaultnull get", flag);
  //  ensure_equals("name column default null", defnull != 0, false);

  //  flag= clist->set_field(node, TableColumnsListBE::Type, "varchar(40)");
  //  ensure("set name type", flag);

  flag = clist->set_field(node, TableColumnsListBE::IsNotNull, 1);
  ensure("set name notnull", flag);

  flag = clist->set_field(node, TableColumnsListBE::Default, "new");
  ensure("change name column default", flag);

  // create column
  node = editor->add_column("salary");
  flag = clist->set_field(node, TableColumnsListBE::IsNotNull, 0);
  ensure("set salary !notnull", flag);

  // create column
  node = editor->add_column("department");

  // create column
  node = editor->add_column("email");

  ensure_equals("column count", clist->count(), 6U);

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
  ensure("get_row", flag);
  ensure_equals("id column name", name, "id");
  //  ensure_equals("id column type", type, "");
  ensure_equals("id column pk", ispkb != 0, true);
  ensure_equals("id column notnull", notnullb, true);
  ensure_equals("id column flags", flags, "");
  ensure_equals("id column default", defvalue, "");
  // ensure_equals("id column default null", defnullb != 0, false);

  flag = clist->get_row(1, name, type, ispkb, notnullb, isunique, isbin, isunsigned, iszerofill, flags, defvalue,
                        charset, collation, comment);
  ensure("get_row", flag);
  ensure_equals("name column name", name, "name");
  //  ensure_equals("name column type", type, "varchar(32)");
  ensure_equals("name column pk", ispkb, false);
  ensure_equals("name column notnull", notnullb, true);
  ensure_equals("name column flags", flags, "");
  ensure_equals("name column default", defvalue, "new");
  //  ensure_equals("name column default null", defnullb, false);
  ensure_equals("name column charset", charset, "");
  ensure_equals("name column collation", collation, "");

  flag = clist->get_row(2, name, type, ispkb, notnullb, isunique, isbin, isunsigned, iszerofill, flags, defvalue,
                        charset, collation, comment);
  ensure("get_row", flag);
  ensure_equals("salary column name", name, "salary");
  //  ensure_equals("salary column type", type, "decimal(10,2)");
  ensure_equals("salary column pk", ispkb, false);
  ensure_equals("salary column notnull", notnullb, false);
  ensure_equals("salary column flags", flags, "");
  ensure_equals("salary column default", defvalue, "");
  // ensure_equals("salary column default null", defnullb, false);

  flag = clist->get_row(3, name, type, ispkb, notnullb, isunique, isbin, isunsigned, iszerofill, flags, defvalue,
                        charset, collation, comment);
  ensure("get_row", flag);
  ensure_equals("department column name", name, "department");
  //  ensure_equals("department column type", type, "decimal(10,2)");
  ensure_equals("department column pk", ispkb, false);
  ensure_equals("department column notnull", notnullb, false);
  ensure_equals("department column flags", flags, "");
  ensure_equals("department column default", defvalue, "");
  // ensure_equals("department column default null", defnullb, false);

  flag = clist->get_row(4, name, type, ispkb, notnullb, isunique, isbin, isunsigned, iszerofill, flags, defvalue,
                        charset, collation, comment);
  ensure("get_row", flag);
  ensure_equals("email column name", name, "email");
  //  ensure_equals("email column type", type, "decimal(10,2)");
  ensure_equals("email column pk", ispkb, false);
  ensure_equals("email column notnull", notnullb, false);
  ensure_equals("email column flags", flags, "");
  ensure_equals("email column default", defvalue, "");
  // ensure_equals("email column default null", defnullb, false);

  flag = clist->get_row(5, name, type, ispkb, notnullb, isunique, isbin, isunsigned, iszerofill, flags, defvalue,
                        charset, collation, comment);
  ensure("bad get_row", !flag);

  // unset pk
  flag = clist->get_field(0, TableColumnsListBE::IsPK, ispk);
  ensure("get id pk", flag);
  ensure_equals("current id pk", ispk != 0, true);

  flag = clist->set_field(0, TableColumnsListBE::IsPK, 0);
  ensure("unset id pk", flag);

  flag = clist->get_field(0, TableColumnsListBE::IsPK, ispk);
  ensure("get new pk", flag);
  ensure_equals("new id pk", ispk != 0, false);

  // unset again
  flag = clist->set_field(0, TableColumnsListBE::IsPK, 0);
  ensure("unset id pk again", flag);

  flag = clist->get_field(0, TableColumnsListBE::IsPK, ispk);
  ensure("get new pk", flag);
  ensure_equals("new id pk", ispk != 0, false);

  // set back
  flag = clist->set_field(0, TableColumnsListBE::IsPK, 1);
  ensure("unset id pk again", flag);

  flag = clist->get_field(0, TableColumnsListBE::IsPK, ispk);
  ensure("get new pk", flag);
  ensure_equals("new id pk", ispk != 0, true);

  // and again
  flag = clist->set_field(0, TableColumnsListBE::IsPK, 1);
  ensure("unset id pk again", flag);

  flag = clist->get_field(0, TableColumnsListBE::IsPK, ispk);
  ensure("get new pk", flag);
  ensure_equals("new id pk", ispk != 0, true);

  //

  name = editor->get_column_with_name("name")->name();
  ensure_equals("get_column_with_name", name, "name");
}

TEST_FUNCTION(4) { // test delete column

  ensure("table editor !NULL", editor != NULL);

  TableColumnsListBE *clist = editor->get_columns();
  bool flag;

  ensure_equals("column count", clist->count(), 6U);

  editor->remove_column(3);

  ensure_equals("new column count", clist->count(), 5U);

  std::string name;

  flag = clist->get_field(2, TableColumnsListBE::Name, name);
  ensure("get 2", flag);
  ensure_equals("2 name", name, "salary");

  flag = clist->get_field(3, TableColumnsListBE::Name, name);
  ensure("get 3", flag);
  ensure_equals("3 name", name, "email");

  // Placeholder row, doesn't return a value, so the previous one stays.
  flag = clist->get_field(4, TableColumnsListBE::Name, name);
  ensure("get 4", !flag);
  ensure_equals("4 name", name, "email");
}

/*
TEST_FUNCTION(5)
{  // test setting different types

  ensure("column types", false);
}


TEST_FUNCTION(6)
{  // test setting different flags

  ensure("flags", false);
}
*/

TEST_FUNCTION(10) { // index editing
  ensure("table editor !NULL", editor != NULL);

  IndexListBE *index = editor->get_indexes();
  IndexColumnsListBE *icolumns = index->get_columns();
  std::string name, type, comment;
  bool flag;

  ensure_equals("column count", editor->get_columns()->count(), 5U);

  ensure_equals("index count", editor->get_indexes()->count(), 2U);

  NodeId node = editor->add_index("idx1");

  index->select_index(node);
  //  index->set_field(0, IndexListBE::Name, "id");

  ensure_equals("index count", index->count(), 3U);

  flag = index->get_field(1, IndexListBE::Name, name);
  ensure("get index name", flag);
  ensure_equals("index name", name, "idx1");
  flag = index->get_field(1, IndexListBE::Type, type);
  ensure("get index type", flag);
  ensure_equals("index type", type, "type1");
  flag = index->get_field(1, IndexListBE::Comment, comment);
  ensure("get index comment", flag);
  ensure_equals("index comment", comment, "");

  // change

  flag = index->set_field(1, IndexListBE::Name, "index1");
  ensure("set index name", flag);
  flag = index->set_field(1, IndexListBE::Type, "bleqw");
  ensure("set bad index type", !flag);
  flag = index->set_field(1, IndexListBE::Type, "type2");
  ensure("set index type", flag);
  flag = index->set_field(1, IndexListBE::Comment, "test index");
  ensure("set index comment", flag);

  index->select_index(1);

  // add column 1 (name) to index
  icolumns->set_column_enabled(1, true);

  // change column stuff
  flag = icolumns->set_field(1, IndexColumnsListBE::Length, 20);
  ensure("set length", flag);
  flag = icolumns->set_field(1, IndexColumnsListBE::Descending, 1);
  ensure("set desc", flag);
  std::string x;
  flag = icolumns->get_field(1, IndexColumnsListBE::Descending, x);
  ensure_equals("set desc", x, "1");

  ensure_equals("index column count", icolumns->count(), editor->get_columns()->count() - 1);

  // list indexes and columns

  // ** the pk index
  flag = index->get_field(0, IndexListBE::Name, name);
  ensure("get index name", flag);
  ensure_equals("index name", name, "PRIMARY");
  flag = index->get_field(0, IndexListBE::Type, type);
  ensure("get index type", flag);
  ensure_equals("index type", type, "PRIMARY");
  flag = index->get_field(0, IndexListBE::Comment, comment);
  ensure("get index comment", flag);
  ensure_equals("index comment", comment, "");

  std::string buf;

  index->select_index(0);
  ensure_equals("index column count", icolumns->count(), editor->get_columns()->count() - 1);

  // check if list of index columns matches list of table columns
  db_TableRef table(editor->get_table());
  for (size_t i = 0; i < table->columns().count(); i++) {
    flag = icolumns->get_field(i, IndexColumnsListBE::Name, name);
    ensure(strfmt("index column[%lu] name", i), flag);
    ensure_equals(strfmt("index column[%lu] name", i), name, *table->columns()[i]->name());
    flag = icolumns->get_field(i, IndexColumnsListBE::Descending, buf);
    ensure(strfmt("index column[%lu] desc", i), flag);
    ensure_equals(strfmt("index column[%lu] desc", i), buf, "0");
    flag = icolumns->get_field(i, IndexColumnsListBE::Length, buf);
    ensure(strfmt("index column[%lu] length", i), flag);
    ensure_equals(strfmt("index column[%lu] length", i), buf, "0");
    flag = icolumns->get_column_enabled(i);
    if (name == "id")
      ensure_equals(strfmt("index column[%lu] enabled", i), flag, true);
    else
      ensure_equals(strfmt("index column[%lu] enabled", i), flag, false);
  }

  // ** the idx we added
  flag = index->get_field(1, IndexListBE::Name, name);
  ensure("get index name", flag);
  ensure_equals("index name", name, "index1");
  flag = index->get_field(1, IndexListBE::Type, type);
  ensure("get index type", flag);
  ensure_equals("index type", type, "type2");
  flag = index->get_field(1, IndexListBE::Comment, comment);
  ensure("get index comment", flag);
  ensure_equals("index comment", comment, "test index");

  index->select_index(1);

  for (size_t i = 0; i < table->columns().count(); i++) {
    flag = icolumns->get_field(i, IndexColumnsListBE::Name, name);
    ensure(strfmt("index column[%lu] name", i), flag);
    ensure_equals(strfmt("index column[%lu] name", i), name, *table->columns()[i]->name());
    if (name == "name") {
      flag = icolumns->get_field(i, IndexColumnsListBE::Descending, buf);
      ensure(strfmt("index column[%lu] desc", i), flag);
      ensure_equals(strfmt("index column[%lu] desc", i), buf, "1");
      flag = icolumns->get_field(i, IndexColumnsListBE::Length, buf);
      ensure(strfmt("index column[%lu] length", i), flag);
      ensure_equals(strfmt("index column[%lu] length", i), buf, "20");
      flag = icolumns->get_column_enabled(i);
      ensure_equals(strfmt("index column[%lu] enabled", i), flag, true);
    } else {
      flag = icolumns->get_field(i, IndexColumnsListBE::Descending, buf);
      ensure(strfmt("index column[%lu] desc", i), flag);
      ensure_equals(strfmt("index column[%lu] desc", i), buf, "0");
      flag = icolumns->get_field(i, IndexColumnsListBE::Length, buf);
      ensure(strfmt("index column[%lu] length", i), flag);
      ensure_equals(strfmt("index column[%lu] length", i), buf, "0");
      flag = icolumns->get_column_enabled(i);
      ensure_equals(strfmt("index column[%lu] enabled", i), flag, false);
    }
  }

  // remove index
  editor->remove_index(1, false);

  ensure_equals("index count", index->count(), 2U);

  // add new one with convenience func
  std::vector<NodeId> columns;
  columns.push_back(NodeId(1)); // name
  columns.push_back(NodeId(3)); // email
  node = editor->add_index_with_columns(columns);

  ensure_equals("new index", node[0], 1U);

  ensure_equals("index count", index->count(), 3U);

  index->set_field(1, IndexListBE::Name, "namemail_index");

  flag = index->get_field(1, IndexListBE::Name, name);
  ensure("get index name", flag);
  ensure_equals("index name", name, "namemail_index");
  flag = index->get_field(1, IndexListBE::Type, type);
  ensure("get index type", flag);
  ensure_equals("index type", type, "type1");
  flag = index->get_field(1, IndexListBE::Comment, comment);
  ensure("get index comment", flag);
  ensure_equals("index comment", comment, "");

  index->select_index(1);

  for (size_t i = 0; i < (size_t)icolumns->count(); i++) {
    flag = icolumns->get_field(i, IndexColumnsListBE::Name, name);
    ensure(strfmt("index column[%lu] name", i), flag);
    ensure_equals(strfmt("index column[%lu] name", i), name, *table->columns()[i]->name());
    flag = icolumns->get_field(i, IndexColumnsListBE::Descending, buf);
    ensure(strfmt("index column[%lu] desc", i), flag);
    ensure_equals(strfmt("index column[%lu] desc", i), buf, "0");
    flag = icolumns->get_field(i, IndexColumnsListBE::Length, buf);
    ensure(strfmt("index column[%lu] length", i), flag);
    ensure_equals(strfmt("index column[%lu] length", i), buf, "0");

    if (name == "name" || name == "email") {
      flag = icolumns->get_column_enabled(i);
      ensure_equals(strfmt("index column[%lu] enabled", i), flag, true);
    } else {
      flag = icolumns->get_column_enabled(i);
      ensure_equals(strfmt("index column[%lu] enabled", i), flag, false);
    }
  }
}

TEST_FUNCTION(11) { // delete a column and see index deletion
  ensure("table editor !NULL", editor != NULL);

  IndexListBE *index = editor->get_indexes();
  // IndexColumnsListBE *icolumns= index->get_columns();
  std::vector<NodeId> columns;
  columns.push_back(NodeId(3)); // email
  editor->add_index_with_columns(columns);

  ensure_equals("index count", index->count(), 4U);

  ensure_equals("column count", editor->get_columns()->count(), 5U);

  std::string name;
  bool flag;

  flag = index->get_field(1, IndexListBE::Name, name);
  ensure("get index[1] name", flag);
  ensure_equals("get index[1] name", name, "namemail_index");
  index->select_index(1);
  ensure_equals("get index[1] column.count()", table->indices().get(1)->columns().count(), 2U);

  editor->remove_column(3); // delete column email

  ensure_equals("column count", table->indices().get(1)->columns().count(), 1U);

  ensure_equals("index count", index->count(), 3U);

  flag = index->get_field(0, IndexListBE::Name, name);
  ensure("get index[0] name", flag);
  ensure_equals("get index[0] name", name, "PRIMARY");
  index->select_index(0);
  ensure_equals("get index[0] column.count()", table->indices().get(0)->columns().count(), 1U);

  flag = index->get_field(1, IndexListBE::Name, name);
  ensure("get index[1] name", flag);
  ensure_equals("get index[1] name", name, "namemail_index");
  index->select_index(1);
  ensure_equals("get index[1] column.count()", table->indices().get(0)->columns().count(), 1U);

  flag = index->get_field(2, IndexListBE::Name, name);
  ensure("get index[2] name", flag);
  ensure_equals("get index[2] name", name, "");
}

static db_IndexColumnRef find_index_column_for(const grt::ListRef<db_IndexColumn> &cols, const std::string &name) {
  for (size_t c = cols.count(), i = 0; i < c; i++) {
    if (cols[i]->referencedColumn()->name() == name)
      return cols[i];
  }
  return db_IndexColumnRef();
}

TEST_FUNCTION(12) {
  ensure("table editor !NULL", editor != NULL);

  // bug: unsetting an index column is wrong (and may crash)
  db_TableRef table = db_mysql_TableRef(grt::Initialized);
  table->owner(wbt->get_schema());

  ensure("table ok", table.is_valid());

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

  TestTableEditor ed(table, wbt->get_rdbms());

  ensure_equals("column count", ed.get_columns()->count(), 4U);

  ed.add_index("hello");

  ensure_equals("index count", ed.get_indexes()->count(), 2U);

  ed.get_indexes()->select_index(0);

  ensure_equals("index column item count", ed.get_indexes()->get_columns()->count(), 3U);

  IndexColumnsListBE *ic = ed.get_indexes()->get_columns();
  std::string name;

  ic->get_field(0, IndexColumnsListBE::Name, name);
  ensure_equals("col1", name, "col1");
  ensure_equals("col1 disabled", ic->get_column_enabled(0), false);

  ic->get_field(1, IndexColumnsListBE::Name, name);
  ensure_equals("col2", name, "col2");
  ensure_equals("col2 disabled", ic->get_column_enabled(1), false);

  ic->get_field(2, IndexColumnsListBE::Name, name);
  ensure_equals("col3", name, "col3");
  ensure_equals("col3 disabled", ic->get_column_enabled(2), false);

  // enable 2 columns
  ic->set_column_enabled(1, true);
  ensure("col2 in list", find_index_column_for(table->indices().get(0)->columns(), "col2").is_valid());

  ic->set_column_enabled(0, true);
  ensure("col1 in list", find_index_column_for(table->indices().get(0)->columns(), "col1").is_valid());

  // disable one of them and make sure the right column is removed
  ic->set_column_enabled(0, false);
  ensure("col2 in list", find_index_column_for(table->indices().get(0)->columns(), "col2").is_valid());
  ensure("col1 not in list", !find_index_column_for(table->indices().get(0)->columns(), "col1").is_valid());

  // disable all
  ic->set_column_enabled(1, false);

  // toggle last (will crash if buggy)
  ic->set_column_enabled(2, true);
  ic->set_column_enabled(2, false);
}

TEST_FUNCTION(13) {
  // check if adding columns/indices/foreign keys by setting name of placeholder item works

  db_mysql_TableRef table(grt::Initialized);

  table->owner(wbt->get_schema());
  table->name("table");

  TestTableEditor editor(table, wbt->get_rdbms());

  editor.get_columns()->set_field(0, 0, "newcol");

  // auto-adds a PK
  editor.get_columns()->set_field(0, 1, "int(11)");
  ensure_equals("autoadd PK index", table->indices().count(), 1U);
  ensure_equals("autoadd PK index", table->indices()[0]->name().c_str(), "PRIMARY");

  ensure_equals("add column", table->columns().count(), 1U);

  ensure_equals("add index", table->indices().count(), 1U);
  editor.get_indexes()->set_field(1, 0, "index");
  ensure_equals("add index", table->indices().count(), 2U);

  ensure_equals("add fk", table->foreignKeys().count(), 0U);
  editor.get_fks()->set_field(0, 0, "newfk");
  ensure_equals("add fk", table->foreignKeys().count(), 1U);
}

TEST_FUNCTION(20) {
  // bug: create fk, select column and then deselect will crash

  db_mysql_TableRef table(grt::Initialized);
  ensure("table ok", table.is_valid());

  table->name("table");
  table->owner(wbt->get_schema());

  wbt->get_schema()->tables().insert(table);

  TestTableEditor editor(table, wbt->get_rdbms());

  ensure("editor catalog is not ok", editor.get_catalog().is_valid());

  grt::ListRef<db_UserDatatype> userTypes(editor.get_catalog()->userDatatypes());

  db_mysql_ColumnRef column(grt::Initialized);
  column->owner(table);
  column->name("id");
  column->setParseType("int", wbt->get_rdbms()->simpleDatatypes());
  // bec::ColumnHelper::parse_column_type(editor.get_rdbms(), userTypes, "int", column);
  table->columns().insert(column);

  column = db_mysql_ColumnRef(grt::Initialized);
  column->owner(table);
  column->name("fk");
  column->setParseType("int", wbt->get_rdbms()->simpleDatatypes());
  // bec::ColumnHelper::parse_column_type(editor.get_rdbms(), userTypes, "int", column);
  table->columns().insert(column);

  ensure_equals("columns list ok", editor.get_columns()->count(), 3U);

  std::vector<bec::NodeId> columns;
  columns.push_back(bec::NodeId(1));
  editor.add_fk_with_columns(columns);
  ensure_equals("fk added", editor.get_fks()->count(), 2U);

  editor.get_fks()->select_fk(bec::NodeId(0));

  editor.get_fks()->set_field(0, bec::FKConstraintListBE::RefTable, "table");

  ensure_equals("columns in fk", editor.get_fks()->get_columns()->count(), 2U);
  /* not working
  bool enabled= false;
  bool flag;
  flag= editor.get_fks()->get_columns()->get_field(1, bec::FKConstraintColumnsListBE::Enabled, enabled);
  ensure("get fk column enabled", flag);
  ensure("fk column enabled", enabled);

  flag= editor.get_fks()->get_columns()->set_field(1, bec::FKConstraintColumnsListBE::Enabled, false);
  ensure("set fk column disabled", flag);

  flag= editor.get_fks()->get_columns()->get_field(1, bec::FKConstraintColumnsListBE::Enabled, enabled);
  ensure("get fk column enabled", flag);
  ensure("fk column disabled", !enabled);
  */
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  delete wbt;
}

END_TESTS
