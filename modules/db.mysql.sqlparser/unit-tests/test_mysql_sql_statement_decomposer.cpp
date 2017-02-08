/*
 * Copyright (c) 2012, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _WIN32
#include <sstream>
#endif

#include "grt_test_utility.h"
#include "testgrt.h"
#include "grtsqlparser/sql_facade.h"
#include "wb_helpers.h"
#include "base/string_utilities.h"

BEGIN_TEST_DATA_CLASS(mysql_sql_statement_decomposer)
public:
WBTester *wbt;
SqlFacade::Ref sql_facade;
Sql_statement_decomposer::Ref sql_statement_decomposer;
db_mgmt_RdbmsRef rdbms;
db_ViewRef view;
db_CatalogRef catalog;

db_SchemaRef add_schema(const std::string &name);
db_TableRef add_table(db_SchemaRef schema, const std::string &name);
db_ColumnRef add_table_column(db_TableRef table, const std::string &name);
db_ViewRef add_view(db_SchemaRef schema, const std::string &name);
grt::StringRef add_view_column(db_ViewRef view, const std::string &name);

void test_sql(const std::string &sql, const std::string &master_result);
void test_view(const std::string &sql, const std::string &master_result);
void test_view2(const std::string &sql, const std::string &master_result);
TEST_DATA_CONSTRUCTOR(mysql_sql_statement_decomposer) : sql_facade(nullptr) {
  wbt = new WBTester;
}
END_TEST_DATA_CLASS

TEST_MODULE(mysql_sql_statement_decomposer, "SQL Parser (MySQL): Statement Decomposer");

TEST_FUNCTION(1) {
  wbt->create_new_document();

  ensure_equals("loaded physycal model count", wbt->wb->get_document()->physicalModels().count(), 1U);

  rdbms = wbt->wb->get_document()->physicalModels().get(0)->rdbms();

  sql_facade = SqlFacade::instance_for_rdbms(rdbms);
  ensure("failed to get sqlparser module", (NULL != sql_facade));

  sql_statement_decomposer = sql_facade->sqlStatementDecomposer();
  ensure("failed to instantiate SqlStatementDecomposer class", (NULL != sql_statement_decomposer.get()));

  catalog = db_CatalogRef(grt::Initialized);
  db_SchemaRef schema = add_schema("test");
  view = add_view(schema, "");

  {
    db_TableRef table = add_table(schema, "table1");
    add_table_column(table, "id");
    add_table_column(table, "name");
  }

  {
    db_TableRef table = add_table(schema, "TABLE1");
    add_table_column(table, "ID");
    add_table_column(table, "NAME");
  }

  {
    db_ViewRef view = add_view(schema, "view1");
    add_view_column(view, "id");
    add_view_column(view, "name");
  }
}

db_SchemaRef Test_object_base<mysql_sql_statement_decomposer>::add_schema(const std::string &name) {
  db_SchemaRef schema(grt::Initialized);
  schema->owner(catalog);
  schema->name(name);
  catalog->schemata().insert(schema);
  return schema;
}

db_TableRef Test_object_base<mysql_sql_statement_decomposer>::add_table(db_SchemaRef schema, const std::string &name) {
  db_TableRef table(grt::Initialized);
  table->owner(schema);
  table->name(name);
  schema->tables().insert(table);
  return table;
}

db_ColumnRef Test_object_base<mysql_sql_statement_decomposer>::add_table_column(db_TableRef table,
                                                                                const std::string &name) {
  db_ColumnRef col(grt::Initialized);
  col->owner(table);
  col->name(name);
  table->columns().insert(col);
  return col;
}

db_ViewRef Test_object_base<mysql_sql_statement_decomposer>::add_view(db_SchemaRef schema, const std::string &name) {
  db_ViewRef view(grt::Initialized);
  view->owner(schema);
  view->name(name);
  schema->views().insert(view);
  return view;
}

grt::StringRef Test_object_base<mysql_sql_statement_decomposer>::add_view_column(db_ViewRef view,
                                                                                 const std::string &name) {
  view->columns().insert(name);
  return name;
}

void Test_object_base<mysql_sql_statement_decomposer>::test_sql(const std::string &sql,
                                                                const std::string &master_result) {
  /* parse sql */
  SelectStatement::Ref select_statement(new SelectStatement());
  ensure("Decomposer load error", sql_statement_decomposer.get() != NULL);
  int res = sql_statement_decomposer->decompose_query(sql, select_statement);
  ensure("Failed to parse statement: " + sql, (res == 1));

  /* dump SelectStatement object contents */
  std::ostringstream oss;
  oss << *select_statement;
  ensure_equals(sql, oss.str(), master_result);
  // std::cout << *select_statement << std::endl;
}

void Test_object_base<mysql_sql_statement_decomposer>::test_view2(const std::string &sql,
                                                                  const std::string &master_result) {
  std::string view_sql_def = base::strfmt("create view v1 as %s", sql.c_str());
  test_view(view_sql_def, master_result);
}

void Test_object_base<mysql_sql_statement_decomposer>::test_view(const std::string &sql,
                                                                 const std::string &master_result) {
  view->sqlDefinition(sql);

  /* parse sql */
  SelectStatement::Ref select_statement(new SelectStatement());
  int res = sql_statement_decomposer->decompose_view(view, select_statement);
  ensure(base::strfmt("failed to parse statement: %s", sql.c_str()), (res == 1));

  std::ostringstream oss;
  oss << *select_statement;
  ensure_equals(sql, oss.str(), master_result);
  // std::cout << *select_statement << std::endl;
}

TEST_FUNCTION(2) {
  test_sql("select 1, '2', 3 as a, 4 b, v1.v1_a, v1.* from (select 1 as v1_a from dual) v1",
           "{SELECT\n\
  {}.{}.{}.{1}.{}.{0}\n\
  {}.{}.{}.{'2'}.{}.{0}\n\
  {}.{}.{}.{3}.{a}.{0}\n\
  {}.{}.{}.{4}.{b}.{0}\n\
  {}.{v1}.{v1_a}.{}.{}.{0}\n\
  {}.{v1}.{}.{}.{}.{1}\n\
FROM\n\
  {SELECT\n\
    {}.{}.{}.{1}.{v1_a}.{0}\n\
  FROM\n\
    dual dual\n\
  } v1\n\
}");
}

TEST_FUNCTION(3) {
  test_sql("select 1 as a, 2 b, v1.v1_a, v1.* from (select 1 as v1_a from dual) v1",
           "{SELECT\n\
  {}.{}.{}.{1}.{a}.{0}\n\
  {}.{}.{}.{2}.{b}.{0}\n\
  {}.{v1}.{v1_a}.{}.{}.{0}\n\
  {}.{v1}.{}.{}.{}.{1}\n\
FROM\n\
  {SELECT\n\
    {}.{}.{}.{1}.{v1_a}.{0}\n\
  FROM\n\
    dual dual\n\
  } v1\n\
}");
}

TEST_FUNCTION(4) {
  test_sql("select table1.a, t1.a, table2.b, t2.b from table1 t1, table2 t2",
           "{SELECT\n\
  {}.{table1}.{a}.{}.{}.{0}\n\
  {}.{t1}.{a}.{}.{}.{0}\n\
  {}.{table2}.{b}.{}.{}.{0}\n\
  {}.{t2}.{b}.{}.{}.{0}\n\
FROM\n\
  table1 t1\n\
  table2 t2\n\
}");
}

TEST_FUNCTION(5) {
  test_sql(
    "select t1.a, t2.b, t3.c from table1 t1 inner join table2 t2 on (t1.id=t2.id) inner join table3 t3 on "
    "(t2.id=t3.id)",
    "{SELECT\n\
  {}.{t1}.{a}.{}.{}.{0}\n\
  {}.{t2}.{b}.{}.{}.{0}\n\
  {}.{t3}.{c}.{}.{}.{0}\n\
FROM\n\
  table1 t1\n\
  table2 t2\n\
  table3 t3\n\
}");
}

TEST_FUNCTION(6) {
  test_view2("select 1, '2', 3 as a, 4 b, t1.t1_a, t1.*, 5 c from (select 1 as t1_a from dual) t1",
             "{SELECT\n\
  {}.{}.{}.{1}.{}.{0}\n\
  {}.{}.{}.{'2'}.{}.{0}\n\
  {}.{}.{}.{3}.{a}.{0}\n\
  {}.{}.{}.{4}.{b}.{0}\n\
  {}.{t1}.{t1_a}.{}.{}.{0}\n\
  {}.{t1}.{t1_a}.{}.{}.{0}\n\
  {}.{}.{}.{5}.{c}.{0}\n\
FROM\n\
  {SELECT\n\
    {}.{}.{}.{1}.{t1_a}.{0}\n\
  FROM\n\
    dual dual\n\
  } t1\n\
}");
}

TEST_FUNCTION(7) {
  test_view2("select t1.*, v1.* from table1 t1, view1 v1",
             "{SELECT\n\
  {}.{t1}.{id}.{}.{}.{0}\n\
  {}.{t1}.{name}.{}.{}.{0}\n\
  {}.{v1}.{id}.{}.{}.{0}\n\
  {}.{v1}.{name}.{}.{}.{0}\n\
FROM\n\
  table1 t1\n\
  view1 v1\n\
}");
}

TEST_FUNCTION(8) {
  test_view2("select * from table1 t1, view1 v1",
             "{SELECT\n\
  {}.{t1}.{id}.{}.{}.{0}\n\
  {}.{t1}.{name}.{}.{}.{0}\n\
  {}.{v1}.{id}.{}.{}.{0}\n\
  {}.{v1}.{name}.{}.{}.{0}\n\
FROM\n\
  table1 t1\n\
  view1 v1\n\
}");
}

TEST_FUNCTION(9) {
  test_view("create view v1 (id_1, name_2, id_2, name_2) as select * from table1 t1, view1 v1",
            "{SELECT\n\
  {}.{t1}.{id}.{}.{id_1}.{0}\n\
  {}.{t1}.{name}.{}.{name_2}.{0}\n\
  {}.{v1}.{id}.{}.{id_2}.{0}\n\
  {}.{v1}.{name}.{}.{name_2}.{0}\n\
FROM\n\
  table1 t1\n\
  view1 v1\n\
}");
}

TEST_FUNCTION(10) {
  test_view("create or replace view `view2` as select 'hello world' as c",
            "{SELECT\n\
  {}.{}.{}.{'hello world'}.{c}.{0}\n\
FROM\n\
}");
}

TEST_FUNCTION(11) {
  sql_statement_decomposer->case_sensitive_identifiers(false);
  test_view("create view v as select * from TABLE1",
            "{SELECT\n\
  {}.{TABLE1}.{id}.{}.{}.{0}\n\
  {}.{TABLE1}.{name}.{}.{}.{0}\n\
FROM\n\
  TABLE1 TABLE1\n\
}");
}

TEST_FUNCTION(12) {
  sql_statement_decomposer->case_sensitive_identifiers(true);
  test_view("create view v as select * from TABLE1",
            "{SELECT\n\
  {}.{TABLE1}.{ID}.{}.{}.{0}\n\
  {}.{TABLE1}.{NAME}.{}.{}.{0}\n\
FROM\n\
  TABLE1 TABLE1\n\
}");
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  delete wbt;
}

END_TESTS
