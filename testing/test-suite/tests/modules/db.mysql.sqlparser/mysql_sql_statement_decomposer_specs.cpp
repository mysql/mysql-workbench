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

#include "grtsqlparser/sql_facade.h"
#include "base/string_utilities.h"

#include "casmine.h"
#include "wb_test_helpers.h"
#include "grt_test_helpers.h"

using namespace casmine;

namespace {

$ModuleEnvironment() {};

$TestData {
  std::unique_ptr<WorkbenchTester> tester;
  SqlFacade::Ref sqlFacade;
  Sql_statement_decomposer::Ref sqlStatementDecomposer;
  db_mgmt_RdbmsRef rdbms;
  db_ViewRef view;
  db_CatalogRef catalog;

  //--------------------------------------------------------------------------------------------------------------------

  db_SchemaRef add_schema(const std::string &name) {
    db_SchemaRef schema(grt::Initialized);
    schema->owner(catalog);
    schema->name(name);
    catalog->schemata().insert(schema);
    return schema;
  }

  //--------------------------------------------------------------------------------------------------------------------

  db_TableRef add_table(db_SchemaRef schema, const std::string &name) {
    db_TableRef table(grt::Initialized);
    table->owner(schema);
    table->name(name);
    schema->tables().insert(table);
    return table;
  }

  //--------------------------------------------------------------------------------------------------------------------

  db_ColumnRef add_table_column(db_TableRef table, const std::string &name) {
    db_ColumnRef col(grt::Initialized);
    col->owner(table);
    col->name(name);
    table->columns().insert(col);
    return col;
  }

  //--------------------------------------------------------------------------------------------------------------------

  db_ViewRef add_view(db_SchemaRef schema, const std::string &name) {
    db_ViewRef view(grt::Initialized);
    view->owner(schema);
    view->name(name);
    schema->views().insert(view);
    return view;
  }

  //--------------------------------------------------------------------------------------------------------------------

  grt::StringRef add_view_column(db_ViewRef view, const std::string &name) {
    view->columns().insert(name);
    return name;
  }

  //--------------------------------------------------------------------------------------------------------------------

  void test_sql(const std::string &sql, const std::string &master_result) {
    SelectStatement::Ref select_statement(new SelectStatement());
    int res = sqlStatementDecomposer->decompose_query(sql, select_statement);
    $expect(res).toEqual(1, "Failed to parse statement: " + sql);

    std::ostringstream oss;
    oss << *select_statement;
    $expect(oss.str()).toBe(master_result);
  }

  //--------------------------------------------------------------------------------------------------------------------

  void test_view2(const std::string &sql, const std::string &master_result) {
    std::string view_sql_def = base::strfmt("create view v1 as %s", sql.c_str());
    test_view(view_sql_def, master_result);
  }

  //--------------------------------------------------------------------------------------------------------------------

  void test_view(const std::string &sql, const std::string &master_result) {
    view->sqlDefinition(sql);

    SelectStatement::Ref select_statement(new SelectStatement());
    int res = sqlStatementDecomposer->decompose_view(view, select_statement);
    $expect(res).toEqual(1, base::strfmt("failed to parse statement: %s", sql.c_str()));

    std::ostringstream oss;
    oss << *select_statement;
    $expect(oss.str()).toBe(master_result, sql);
  }

};

$describe("SQL Parser (MySQL): Statement Decomposer") {

  $beforeAll([this]() {
    data->tester = std::make_unique<WorkbenchTester>();
    data->tester->createNewDocument();

    $expect(data->tester->wb->get_document()->physicalModels().count()).toBe(1U, "loaded physycal model count");

    data->rdbms = data->tester->wb->get_document()->physicalModels().get(0)->rdbms();
    data->sqlFacade = SqlFacade::instance_for_rdbms(data->rdbms);
    $expect(data->sqlFacade).Not.toBeNull("failed to get sqlparser module");

    data->sqlStatementDecomposer = data->sqlFacade->sqlStatementDecomposer();
    $expect(data->sqlStatementDecomposer).toBeValid("failed to instantiate Sql_statement_decomposer class");

    data->catalog = db_CatalogRef(grt::Initialized);
    db_SchemaRef schema = data->add_schema("test");
    data->view = data->add_view(schema, "");

    {
      db_TableRef table = data->add_table(schema, "table1");
      data->add_table_column(table, "id");
      data->add_table_column(table, "name");
    }

    {
      db_TableRef table = data->add_table(schema, "TABLE1");
      data->add_table_column(table, "ID");
      data->add_table_column(table, "NAME");
    }

    {
      db_ViewRef view = data->add_view(schema, "view1");
      data->add_view_column(view, "id");
      data->add_view_column(view, "name");
    }
  });

  $it("Query 1", [this]() {
    data->test_sql("select 1, '2', 3 as a, 4 b, v1.v1_a, v1.* from (select 1 as v1_a from dual) v1",
      "{SELECT\n"
      "  {}.{}.{}.{1}.{}.{0}\n"
      "  {}.{}.{}.{'2'}.{}.{0}\n"
      "  {}.{}.{}.{3}.{a}.{0}\n"
      "  {}.{}.{}.{4}.{b}.{0}\n"
      "  {}.{v1}.{v1_a}.{}.{}.{0}\n"
      "  {}.{v1}.{}.{}.{}.{1}\n"
      "FROM\n"
      "  {SELECT\n"
      "    {}.{}.{}.{1}.{v1_a}.{0}\n"
      "  FROM\n"
      "    dual dual\n"
      "  } v1\n"
      "}");
  });

  $it("Query 2", [this]() {
    data->test_sql("select 1 as a, 2 b, v1.v1_a, v1.* from (select 1 as v1_a from dual) v1",
      "{SELECT\n"
      "  {}.{}.{}.{1}.{a}.{0}\n"
      "  {}.{}.{}.{2}.{b}.{0}\n"
      "  {}.{v1}.{v1_a}.{}.{}.{0}\n"
      "  {}.{v1}.{}.{}.{}.{1}\n"
      "FROM\n"
      "  {SELECT\n"
      "    {}.{}.{}.{1}.{v1_a}.{0}\n"
      "  FROM\n"
      "    dual dual\n"
      "  } v1\n"
      "}");
  });

  $it("Query 3", [this]() {
    data->test_sql("select table1.a, t1.a, table2.b, t2.b from table1 t1, table2 t2",
      "{SELECT\n"
      "  {}.{table1}.{a}.{}.{}.{0}\n"
      "  {}.{t1}.{a}.{}.{}.{0}\n"
      "  {}.{table2}.{b}.{}.{}.{0}\n"
      "  {}.{t2}.{b}.{}.{}.{0}\n"
      "FROM\n"
      "  table1 t1\n"
      "  table2 t2\n"
      "}");
  });

  $it("Query 4", [this]() {
    data->test_sql(
      "select t1.a, t2.b, t3.c from table1 t1 inner join table2 t2 on (t1.id=t2.id) inner join table3 t3 on (t2.id=t3.id)",
      "{SELECT\n"
      "  {}.{t1}.{a}.{}.{}.{0}\n"
      "  {}.{t2}.{b}.{}.{}.{0}\n"
      "  {}.{t3}.{c}.{}.{}.{0}\n"
      "FROM\n"
      "  table1 t1\n"
      "  table2 t2\n"
      "  table3 t3\n"
      "}");
  });

  $it("Query 5", [this]() {
    data->test_view2("select 1, '2', 3 as a, 4 b, t1.t1_a, t1.*, 5 c from (select 1 as t1_a from dual) t1",
      "{SELECT\n"
      "  {}.{}.{}.{1}.{}.{0}\n"
      "  {}.{}.{}.{'2'}.{}.{0}\n"
      "  {}.{}.{}.{3}.{a}.{0}\n"
      "  {}.{}.{}.{4}.{b}.{0}\n"
      "  {}.{t1}.{t1_a}.{}.{}.{0}\n"
      "  {}.{t1}.{t1_a}.{}.{}.{0}\n"
      "  {}.{}.{}.{5}.{c}.{0}\n"
      "FROM\n"
      "  {SELECT\n"
      "    {}.{}.{}.{1}.{t1_a}.{0}\n"
      "  FROM\n"
      "    dual dual\n"
      "  } t1\n"
      "}");
  });

  $it("Query 6", [this]() {
    data->test_view2("select t1.*, v1.* from table1 t1, view1 v1",
      "{SELECT\n"
      "  {}.{t1}.{id}.{}.{}.{0}\n"
      "  {}.{t1}.{name}.{}.{}.{0}\n"
      "  {}.{v1}.{id}.{}.{}.{0}\n"
      "  {}.{v1}.{name}.{}.{}.{0}\n"
      "FROM\n"
      "  table1 t1\n"
      "  view1 v1\n"
      "}");
  });

  $it("Query 7", [this]() {
    data->test_view2("select * from table1 t1, view1 v1",
      "{SELECT\n"
      "  {}.{t1}.{id}.{}.{}.{0}\n"
      "  {}.{t1}.{name}.{}.{}.{0}\n"
      "  {}.{v1}.{id}.{}.{}.{0}\n"
      "  {}.{v1}.{name}.{}.{}.{0}\n"
      "FROM\n"
      "  table1 t1\n"
      "  view1 v1\n"
      "}");
  });

  $it("Query 8", [this]() {
    data->test_view("create view v1 (id_1, name_2, id_2, name_2) as select * from table1 t1, view1 v1",
      "{SELECT\n"
      "  {}.{t1}.{id}.{}.{id_1}.{0}\n"
      "  {}.{t1}.{name}.{}.{name_2}.{0}\n"
      "  {}.{v1}.{id}.{}.{id_2}.{0}\n"
      "  {}.{v1}.{name}.{}.{name_2}.{0}\n"
      "FROM\n"
      "  table1 t1\n"
      "  view1 v1\n"
      "}");
  });

  $it("Query 9", [this]() {
    data->test_view("create or replace view `view2` as select 'hello world' as c",
      "{SELECT\n"
      "  {}.{}.{}.{'hello world'}.{c}.{0}\n"
      "FROM\n"
      "}");
  });

  $it("Query 10", [this]() {
    data->sqlStatementDecomposer->case_sensitive_identifiers(false);
    data->test_view("create view v as select * from TABLE1",
      "{SELECT\n"
      "  {}.{TABLE1}.{id}.{}.{}.{0}\n"
      "  {}.{TABLE1}.{name}.{}.{}.{0}\n"
      "FROM\n"
      "  TABLE1 TABLE1\n"
      "}");
  });

  $it("Query 11", [this]() {
    data->sqlStatementDecomposer->case_sensitive_identifiers(true);
    data->test_view("create view v as select * from TABLE1",
      "{SELECT\n"
      "  {}.{TABLE1}.{ID}.{}.{}.{0}\n"
      "  {}.{TABLE1}.{NAME}.{}.{}.{0}\n"
      "FROM\n"
      "  TABLE1 TABLE1\n"
      "}");
  });
}

}
