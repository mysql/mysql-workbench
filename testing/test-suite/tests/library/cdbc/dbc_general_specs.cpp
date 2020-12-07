/*
 * Copyright (c) 2012, 2020, Oracle and/or its affiliates.
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

#include <cppconn/prepared_statement.h>
#include <cppconn/connection.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>
#include <cppconn/metadata.h>

#include "cdbc/src/driver_manager.h"
#include "wb_connection_helpers.h"
#include "wb_test_helpers.h"

#include "helpers.h"
#include "casmine.h"

extern void register_all_metaclasses();

namespace {

$ModuleEnvironment() {};

$describe("DBC: general tests") {
  $beforeAll([&]() {
    // load structs
    register_all_metaclasses();
    grt::GRT::get()->scan_metaclasses_in("../../res/grt/");
    grt::GRT::get()->end_loading_metaclasses();
    $expect(grt::GRT::get()->get_metaclasses().size()).toBe((size_t)INT_METACLASS_COUNT, "load structs");
  });

  $afterAll([&]() {
    db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

    setupConnectionEnvironment(connectionProperties);

    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
    sql::Connection *connection = wrapper.get();

    std::unique_ptr<sql::Statement> stmt(connection->createStatement());
    stmt->execute("DROP SCHEMA IF EXISTS test");

    WorkbenchTester::reinitGRT();
  });

  $it("Checks initial functionality", [&]() {
    $expect(grt::GRT::get()->get_metaclasses().size()).toBe(INT_METACLASS_COUNT, "load structs");
    db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

    setupConnectionEnvironment(connectionProperties);

    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
    sql::Connection *connection = wrapper.get();

    std::unique_ptr<sql::Statement> stmt(connection->createStatement());
    stmt->execute("DROP SCHEMA IF EXISTS test");

    std::unique_ptr<sql::ResultSet> rset1(stmt->executeQuery("SHOW DATABASES like 'test'"));
    $expect(rset1->rowsCount()).toBe(0U, "database test still exists");

    stmt->execute("CREATE SCHEMA test");

    std::unique_ptr<sql::ResultSet> rset2(stmt->executeQuery("SHOW DATABASES like 'test'"));
    $expect(rset2->rowsCount()).toBe(1U, "database test doesn't exists");
  });

  $it("Metadata fetch test", [&]() {
    db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

    setupConnectionEnvironment(connectionProperties);

    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
    sql::Connection *connection = wrapper.get();
    sql::DatabaseMetaData *meta(connection->getMetaData());
    std::unique_ptr<sql::ResultSet> rset(meta->getSchemata());

    while (rset->next()) {
      if (getenv("VERBOSE")) {
        std::cout << rset->getString("Database") << std::endl;
        std::cout << "  Schema Objects:" << std::endl;
      }

      std::unique_ptr<sql::ResultSet> rset2(meta->getSchemaObjects("", rset->getString("Database")));
      while (rset2->next()) {
        if (getenv("VERBOSE"))
          std::cout << rset2->getString("OBJECT_TYPE") << ": " << rset2->getString("NAME") << ","
                    << rset2->getString("DDL") << std::endl;
      }
    }
  });

  $it("Transaction tests", [&]() {
    db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

    setupConnectionEnvironment(connectionProperties);

    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
    sql::Connection *connection = wrapper.get();

    std::unique_ptr<sql::Statement> stmt(connection->createStatement());

    stmt->execute("DROP TABLE IF EXISTS test.product");
    stmt->execute("CREATE TABLE test.product(idproduct INT NOT NULL AUTO_INCREMENT PRIMARY KEY, name VARCHAR(80))");

    connection->setAutoCommit(0);

    if (getenv("VERBOSE"))
      std::cout << "Insert Data." << std::endl;

    std::unique_ptr<sql::PreparedStatement> prepStmt(
      connection->prepareStatement("INSERT INTO test.product(idproduct, name)  VALUES(?, ?)"));
    prepStmt->setInt(1, 1);
    prepStmt->setString(2, "Harry Potter");
    prepStmt->executeUpdate();

    if (getenv("VERBOSE"))
      std::cout << "Display Data." << std::endl;

    std::unique_ptr<sql::ResultSet> rset1(stmt->executeQuery("SELECT * FROM test.product"));

    int i = 0;
    while (rset1->next()) {
      if (getenv("VERBOSE"))
        std::cout << rset1->getString(2) << ", " << rset1->getString("name") << std::endl;
      i++;
    }
    if (getenv("VERBOSE")) {
      printf("%d row(s)", i);

      printf("Rollback");
    }

    connection->rollback();

    if (getenv("VERBOSE"))
      printf("Display Data Again.\n");

    std::unique_ptr<sql::ResultSet> rset2(stmt->executeQuery("SELECT * FROM test.product"));

    i = 0;
    while (rset2->next()) {
      if (getenv("VERBOSE"))
        std::cout << rset2->getString(2) << ", " << rset2->getString("name") << std::endl;
      i++;
    }
    if (getenv("VERBOSE"))
      std::cout << i << " row(s)" << std::endl;
  });
};

}
