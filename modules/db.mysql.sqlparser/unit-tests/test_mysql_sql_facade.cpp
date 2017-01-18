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

#ifndef _WIN32
#include <sstream>
#endif

#include "grt_test_utility.h"
#include "testgrt.h"
#include "grtsqlparser/sql_facade.h"
#include "wb_helpers.h"

BEGIN_TEST_DATA_CLASS(mysql_sql_facade)
public:
WBTester *wbt;
SqlFacade::Ref sql_facade;
db_mgmt_RdbmsRef rdbms;
DictRef options;
TEST_DATA_CONSTRUCTOR(mysql_sql_facade) : sql_facade(nullptr) {
  wbt = new WBTester;
}
END_TEST_DATA_CLASS

TEST_MODULE(mysql_sql_facade, "SQL Parser FE (MySQL)");

// Creates the needed structure for the testing...
TEST_FUNCTION(1) {
  sql_facade = NULL;
  wbt->create_new_document();

  ensure_equals("loaded physycal model count", wbt->wb->get_document()->physicalModels().count(), 1U);

  options = DictRef(true);
  options.set("gen_fk_names_when_empty", IntegerRef(0));

  rdbms = wbt->wb->get_document()->physicalModels().get(0)->rdbms();

  sql_facade = SqlFacade::instance_for_rdbms(rdbms);
  ensure("failed to get sqlparser module", (NULL != sql_facade));
}

// Pretty simple parsing sample
TEST_FUNCTION(2) {
  ensure("failed to get sqlparser module", (NULL != sql_facade));

  std::string schema_name;
  std::string table_name;
  SqlFacade::String_tuple_list columns;

  std::string query = "select first_name, last_name from sakila.customer;";

  ensure("Unexpexted failure parsing test",
         sql_facade->parseSelectStatementForEdit(query, schema_name, table_name, columns));
  ensure_equals("Unexpected Schema Name", schema_name, "sakila");
  ensure_equals("Unexpected Table Name", table_name, "customer");
  ensure_equals("Unexpected Column Count", columns.size(), 2U);
  ensure("Unexpected Column Name", columns.front().first == "first_name");
  ensure("Unexpected Column Alias", columns.front().second == "first_name");
  columns.pop_front();
  ensure("Unexpected Column Name", columns.front().first == "last_name");
  ensure("Unexpected Column Alias", columns.front().second == "last_name");
  columns.pop_front();
}

// Pretty simple parsing sample using aliases for the columns
TEST_FUNCTION(3) {
  ensure("failed to get sqlparser module", (NULL != sql_facade));

  std::string schema_name;
  std::string table_name;
  SqlFacade::String_tuple_list columns;

  std::string query = "select first_name as 'First Name', last_name as 'Last Name' from sakila.customer;";

  ensure("Unexpexted failure parsing test",
         sql_facade->parseSelectStatementForEdit(query, schema_name, table_name, columns));
  ensure_equals("Unexpected Schema Name", schema_name, "sakila");
  ensure_equals("Unexpected Table Name", table_name, "customer");
  ensure_equals("Unexpected Column Count", columns.size(), 2U);
  ensure("Unexpected Column Name", columns.front().first == "first_name");
  ensure("Unexpected Column Alias", columns.front().second == "First Name");
  columns.pop_front();
  ensure("Unexpected Column Name", columns.front().first == "last_name");
  ensure("Unexpected Column Alias", columns.front().second == "Last Name");
  columns.pop_front();
}

// Using numeric literals as columns will have the function failing
TEST_FUNCTION(5) {
  ensure("failed to get sqlparser module", (NULL != sql_facade));

  std::string schema_name;
  std::string table_name;
  SqlFacade::String_tuple_list columns;

  std::string query = "select customer_id, 10 as 'years' from sakila.customer;";

  ensure("Unexpexted success parsing test",
         !sql_facade->parseSelectStatementForEdit(query, schema_name, table_name, columns));
  ensure_equals("Unexpected Schema Name", schema_name, "");
  ensure_equals("Unexpected Table Name", table_name, "");
  ensure_equals("Unexpected Column Count", columns.size(), 0U);
}

// Using text literals as columns have the function to fail
TEST_FUNCTION(6) {
  ensure("failed to get sqlparser module", (NULL != sql_facade));
  std::string schema_name;
  std::string table_name;
  SqlFacade::String_tuple_list columns;

  std::string query = "select 'Dear' as Greeting, first_name, last_name from sakila.customer;";

  ensure("Unexpexted success parsing test",
         !sql_facade->parseSelectStatementForEdit(query, schema_name, table_name, columns));
  ensure_equals("Unexpected Schema Name", schema_name, "");
  ensure_equals("Unexpected Table Name", table_name, "");
  ensure_equals("Unexpected Column Count", columns.size(), 0U);
}

// Using SELECT *
TEST_FUNCTION(7) {
  ensure("failed to get sqlparser module", (NULL != sql_facade));
  std::string schema_name;
  std::string table_name;
  SqlFacade::String_tuple_list columns;

  std::string query = "select * from `sakila`.`address`";

  ensure("Unexpected failure parsing test",
         sql_facade->parseSelectStatementForEdit(query, schema_name, table_name, columns));
  ensure_equals("Unexpected Schema Name", schema_name, "sakila");
  ensure_equals("Unexpected Table Name", table_name, "address");
  ensure_equals("Unexpected Column Count", columns.size(), 1U);
  ensure("Unexpected Column Name", columns.front().first == "*");
  ensure("Unexpected Column Alias", columns.front().second == "*");
  columns.pop_front();
}

// Using the WHERE clause doesn't impact the parsing as long as the information is being
// retrieved from a single table
TEST_FUNCTION(8) {
  ensure("failed to get sqlparser module", (NULL != sql_facade));
  std::string schema_name;
  std::string table_name;
  SqlFacade::String_tuple_list columns;

  std::string query = "select address, phone as Phone from `sakila`.`address` where district = 'Adana'";

  ensure("Unexpexted failure parsing test",
         sql_facade->parseSelectStatementForEdit(query, schema_name, table_name, columns));
  ensure_equals("Unexpected Schema Name", schema_name, "sakila");
  ensure_equals("Unexpected Table Name", table_name, "address");
  ensure_equals("Unexpected Column Count", columns.size(), 2U);
  ensure("Unexpected Column Name", columns.front().first == "address");
  ensure("Unexpected Column Alias", columns.front().second == "address");
  columns.pop_front();
  ensure("Unexpected Column Name", columns.front().first == "phone");
  ensure("Unexpected Column Alias", columns.front().second == "Phone");
  columns.pop_front();
}

// Using many tables to pull the information causes failure, as expected
TEST_FUNCTION(9) {
  ensure("failed to get sqlparser module", (NULL != sql_facade));
  std::string schema_name;
  std::string table_name;
  SqlFacade::String_tuple_list columns;

  std::string query =
    "SELECT customer.first_name, customer.last_name, address.address FROM sakila.customer, sakila.address where "
    "customer.address_id = address.address_id;";

  ensure("Unexpexted success parsing test",
         !sql_facade->parseSelectStatementForEdit(query, schema_name, table_name, columns));
  ensure_equals("Unexpected Schema Name", schema_name, "");
  ensure_equals("Unexpected Table Name", table_name, "");
  ensure_equals("Unexpected Column Count", columns.size(), 0U);
}

// Should fail if multiple select statements are issued
TEST_FUNCTION(10) {
  ensure("failed to get sqlparser module", (NULL != sql_facade));
  std::string schema_name;
  std::string table_name;
  SqlFacade::String_tuple_list columns;

  std::string query = "SELECT * FROM sakila.customer; SELECT * FROM sakila.address;";

  ensure("Unexpexted success parsing test",
         !sql_facade->parseSelectStatementForEdit(query, schema_name, table_name, columns));
  ensure_equals("Unexpected Schema Name", schema_name, "");
  ensure_equals("Unexpected Table Name", table_name, "");
  ensure_equals("Unexpected Column Count", columns.size(), 0U);
}

// Should fail if multiple functions as columns are used
TEST_FUNCTION(11) {
  ensure("failed to get sqlparser module", (NULL != sql_facade));
  std::string schema_name;
  std::string table_name;
  SqlFacade::String_tuple_list columns;

  std::string query = "SELECT count(*) FROM sakila.customer";

  ensure("Unexpexted success parsing test",
         !sql_facade->parseSelectStatementForEdit(query, schema_name, table_name, columns));
  ensure_equals("Unexpected Schema Name", schema_name, "");
  ensure_equals("Unexpected Table Name", table_name, "");
  ensure_equals("Unexpected Column Count", columns.size(), 0U);
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  delete wbt;
}
END_TESTS
