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

#include "casmine.h"
#include "wb_test_helpers.h"

namespace {

$ModuleEnvironment() {};

$TestData {
  std::unique_ptr<WorkbenchTester> tester;
  SqlFacade::Ref facade;
  db_mgmt_RdbmsRef rdbms;
  grt::DictRef options;
};

$describe("SQL Parser FE (MySQL)") {

  $beforeAll([this]() {
    data->tester.reset(new WorkbenchTester());
    data->facade = nullptr;
    data->tester->createNewDocument();

    $expect(data->tester->wb->get_document()->physicalModels().count()).toBe(1U, "loaded physycal model count");

    data->options = grt::DictRef(true);
    data->options.set("gen_fk_names_when_empty", grt::IntegerRef(0));

    data->rdbms = data->tester->wb->get_document()->physicalModels().get(0)->rdbms();

    data->facade = SqlFacade::instance_for_rdbms(data->rdbms);
    $expect(data->facade).Not.toBeNull("Failed to get sqlparser module");
  });

  $it("Pretty simple parsing sample", [this]() {
    std::string schema_name;
    std::string table_name;
    SqlFacade::String_tuple_list columns;

    std::string query = "select first_name, last_name from sakila.customer;";

    $expect(data->facade->parseSelectStatementForEdit(query, schema_name, table_name, columns)).toBeTrue("Unexpexted failure parsing test");
    $expect(schema_name).toBe("sakila", "Unexpected Schema Name");
    $expect(table_name).toBe("customer", "Unexpected Table Name");
    $expect(columns.size()).toBe(2U, "Unexpected Column Count");
    $expect(columns.front().first).toBe("first_name", "Unexpected Column Name");
    $expect(columns.front().second).toBe("first_name", "Unexpected Column Alias");
    columns.pop_front();
    $expect(columns.front().first).toBe("last_name", "Unexpected Column Name");
    $expect(columns.front().second).toBe("last_name", "Unexpected Column Alias");
    columns.pop_front();
  });

  $it("Simple parsing sample using aliases for the columns", [this]() {
    std::string schema_name;
    std::string table_name;
    SqlFacade::String_tuple_list columns;

    std::string query = "select first_name as 'First Name', last_name as 'Last Name' from sakila.customer;";

    $expect(data->facade->parseSelectStatementForEdit(query, schema_name, table_name, columns)).toBeTrue("Unexpexted failure parsing test");
    $expect(schema_name).toBe("sakila", "Unexpected Schema Name");
    $expect(table_name).toBe("customer", "Unexpected Table Name");
    $expect(columns.size()).toBe(2U, "Unexpected Column Count");
    $expect(columns.front().first).toBe("first_name", "Unexpected Column Name");
    $expect(columns.front().second).toBe("First Name", "Unexpected Column Alias");
    columns.pop_front();
    $expect(columns.front().first).toBe("last_name", "Unexpected Column Name");
    $expect(columns.front().second).toBe("Last Name", "Unexpected Column Alias");
    columns.pop_front();
  });

  $it("Numeric literals as columns", [this]() {
    std::string schema_name;
    std::string table_name;
    SqlFacade::String_tuple_list columns;

    std::string query = "select customer_id, 10 as 'years' from sakila.customer;";

    $expect(data->facade->parseSelectStatementForEdit(query, schema_name, table_name, columns)).toBeFalse("Unexpexted success parsing test");
    $expect(schema_name.empty()).toBeTrue("Unexpected Schema Name");
    $expect(table_name.empty()).toBeTrue("Unexpected Table Name");
    $expect(columns.size()).toEqual(0U, "Unexpected Column Count");
  });

  $it("Using text literals as columns", [this]() {
    std::string schema_name;
    std::string table_name;
    SqlFacade::String_tuple_list columns;

    std::string query = "select 'Dear' as Greeting, first_name, last_name from sakila.customer;";

    $expect(data->facade->parseSelectStatementForEdit(query, schema_name, table_name, columns)).toBeFalse("Unexpexted success parsing test");
    $expect(schema_name.empty()).toBeTrue("Unexpected Schema Name");
    $expect(table_name.empty()).toBeTrue("Unexpected Table Name");
    $expect(columns.empty()).toBeTrue("Unexpected Column Count");
  });

  $it("Using SELECT *", [this]() {
    std::string schema_name;
    std::string table_name;
    SqlFacade::String_tuple_list columns;

    std::string query = "select * from `sakila`.`address`";

    $expect(data->facade->parseSelectStatementForEdit(query, schema_name, table_name, columns)).toBeTrue("Unexpected failure parsing test");
    $expect(schema_name).toBe("sakila", "Unexpected Schema Name");
    $expect(table_name).toBe("address", "Unexpected Table Name");
    $expect(columns.size()).toEqual(1U, "Unexpected Column Count");
    $expect(columns.front().first).toBe("*", "Unexpected Column Name");
    $expect(columns.front().second).toBe("*", "Unexpected Column Alias");
    columns.pop_front();
  });

  $it("Using WHERE", [this]() {
    // Using the WHERE clause doesn't impact the parsing as long as the information is being
    // retrieved from a single table
    std::string schema_name;
    std::string table_name;
    SqlFacade::String_tuple_list columns;

    std::string query = "select address, phone as Phone from `sakila`.`address` where district = 'Adana'";

    $expect(data->facade->parseSelectStatementForEdit(query, schema_name, table_name, columns)).toBeTrue("Unexpexted failure parsing test");
    $expect(schema_name).toBe("sakila", "Unexpected Schema Name");
    $expect(table_name).toBe("address", "Unexpected Table Name");
    $expect(columns.size()).toEqual(2U, "Unexpected Column Count");
    $expect(columns.front().first).toBe("address", "Unexpected Column Name");
    $expect(columns.front().second).toBe("address", "Unexpected Column Alias");
    columns.pop_front();
    $expect(columns.front().first).toBe("phone", "Unexpected Column Name");
    $expect(columns.front().second).toBe("Phone", "Unexpected Column Alias");
    columns.pop_front();
  });

  $it("Using many tables to pull the information ", [this]() {
    std::string schema_name;
    std::string table_name;
    SqlFacade::String_tuple_list columns;

    std::string query =
    "SELECT customer.first_name, customer.last_name, address.address FROM sakila.customer, sakila.address where "
    "customer.address_id = address.address_id;";

    $expect(data->facade->parseSelectStatementForEdit(query, schema_name, table_name, columns)).toBeFalse("Unexpexted success parsing test");
    $expect(schema_name).toBe("");
    $expect(table_name).toBe("");
    $expect(columns.size()).toEqual(0U, "Unexpected Column Count");
  });

  $it("Multiple select statements", [this]() {
    std::string schema_name;
    std::string table_name;
    SqlFacade::String_tuple_list columns;

    std::string query = "SELECT * FROM sakila.customer; SELECT * FROM sakila.address;";

    $expect(data->facade->parseSelectStatementForEdit(query, schema_name, table_name, columns)).toBeFalse("Unexpexted success parsing test");
    $expect(schema_name).toBe("");
    $expect(table_name).toBe("");
    $expect(columns.empty()).toBeTrue("Unexpected Column Count");
  });

  $it("Multiple functions as columns", [this]() {
    std::string schema_name;
    std::string table_name;
    SqlFacade::String_tuple_list columns;

    std::string query = "SELECT count(*) FROM sakila.customer";

    $expect(data->facade->parseSelectStatementForEdit(query, schema_name, table_name, columns)).toBeFalse("Unexpexted success parsing test");
    $expect(schema_name).toBe("");
    $expect(table_name).toBe("");
    $expect(columns.empty()).toBeTrue("Unexpected Column Count");
  });
}

}
