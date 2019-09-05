/*
 * Copyright (c) 2012, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "grt.h"
#include "cdbc/src/driver_manager.h"
#include "cdbc/src/sql_batch_exec.h"
#include "grtsqlparser/sql_facade.h"

#include "wb_connection_helpers.h"
#include "wb_test_helpers.h"
#include "helpers.h"

#include "casmine.h"

#define DATABASE_TO_USE "USE test"

static bool populate_test_table(std::unique_ptr<sql::Statement> &stmt) {
  stmt->execute(DATABASE_TO_USE);
  stmt->execute("DROP TABLE IF EXISTS test_function");
  if (stmt->execute(
      "CREATE TABLE test_function (a integer, b integer, c integer default null)"))
    return false;

  if (stmt->execute("INSERT INTO test_function (a,b,c) VALUES(1, 111, NULL)")) {
    stmt->execute("DROP TABLE test_function");
    return false;
  }
  return true;
}

namespace {

$ModuleEnvironment() {};

$TestData {
  std::unique_ptr<WorkbenchTester> tester;
  SqlFacade::Ref sqlSplitter;

  sql::DriverManager *dm = sql::DriverManager::getDriverManager();
  db_mgmt_ConnectionRef connectionProperties;

  sql::ConnectionWrapper connection() {
    return dm->getConnection(connectionProperties);
  }

  void removeTestTable() {
    auto connection = dm->getConnection(connectionProperties);
    std::unique_ptr<sql::Statement> stmt2(connection->createStatement());
    stmt2->execute(DATABASE_TO_USE);
    stmt2->execute("DROP TABLE test_function");
  }
};

$describe("DBC: statement tests") {
  $beforeAll([this]() {
    data->tester.reset(new WorkbenchTester());
    data->sqlSplitter = SqlFacade::instance_for_rdbms_name("Mysql");
    $expect(data->sqlSplitter).Not.toBeNull("failed to get sqlparser module");

    // Check connection details once.
    data->connectionProperties = db_mgmt_ConnectionRef(grt::Initialized);
    setupConnectionEnvironment(data->connectionProperties);
    $expect(data->dm).Not.toBeNull("Couldn't get a driver manager");
    sql::ConnectionWrapper connection = data->dm->getConnection(data->connectionProperties);
    $expect(connection.get()).Not.toBeNull("Couldn't get a connection from driver");

    std::unique_ptr<sql::Statement> stmt(connection->createStatement());
    stmt->execute("CREATE SCHEMA IF NOT EXISTS test;");
  });

  $afterAll([this]() {
    auto connection = data->connection();
    std::unique_ptr<sql::Statement> stmt(connection->createStatement());
    stmt->execute("DROP SCHEMA IF EXISTS test;");
  });

  $it("Simple update statement against statement object", [this]() {
    auto connection = data->connection();
    std::unique_ptr<sql::Statement> stmt(connection->createStatement());
    $expect(stmt).toBeValid("stmt is NULL");

    $expect(populate_test_table(stmt)).toBeTrue("Data not populated");
    $expect(stmt->execute("UPDATE test_function SET a = 2, b = 222 where b = 111")).toBeFalse("True returned for UPDATE");

    data->removeTestTable();
  });

  $it("Simple query against statement object", [this]() {
    auto connection = data->connection();
    std::unique_ptr<sql::Statement> stmt(connection->createStatement());
    $expect(stmt.get()).Not.toBeNull("stmt is NULL");

    $expect(populate_test_table(stmt)).toBeTrue("Data not populated");
    $expect(stmt->execute("SELECT * FROM test_function")).toBeTrue("False returned for SELECT");

    data->removeTestTable();
  });

  $it("Test executeQuery() - returning a result set.", [this]() {
    auto connection = data->connection();
    std::unique_ptr<sql::Statement> stmt(connection->createStatement());
    $expect(stmt.get()).Not.toBeNull("stmt is NULL");

    $expect(populate_test_table(stmt)).toBeTrue("Data not populated");

    // Get a result set.
    std::unique_ptr<sql::ResultSet> rset(stmt->executeQuery("SELECT * FROM test_function"));
    $expect(rset.get()).Not.toBeNull("NULL returned for result set");

    data->removeTestTable();
  });

  $it("Test executeQuery() - returning empty result set.", [this]() {
    auto connection = data->connection();
    std::unique_ptr<sql::Statement> stmt(connection->createStatement());
    $expect(stmt.get()).Not.toBeNull("stmt is NULL");

    $expect(populate_test_table(stmt)).toBeTrue("Data not populated");

    // Get a result set.
    std::unique_ptr<sql::ResultSet> rset(stmt->executeQuery("SELECT * FROM test_function WHERE 1=2"));
    $expect(rset.get()).Not.toBeNull("NULL returned for result set");
    $expect(rset->next()).toBeFalse("Non-empty result set");

    data->removeTestTable();
  });

  $it("Test executeQuery() - use it for inserting, should generate an exception.", [this]() {
    auto connection = data->connection();
    std::unique_ptr<sql::Statement> stmt(connection->createStatement());
    $expect(stmt.get()).Not.toBeNull("stmt is NULL");

    $expect(populate_test_table(stmt)).toBeTrue("Data not populated");

    $expect([&]() {
      std::unique_ptr<sql::ResultSet> rset(stmt->executeQuery("INSERT INTO test_function VALUES(2,200)"));
    }).toThrow();

    data->removeTestTable();
  });

  $it("Test executeUpdate() - check the returned value.", [this]() {
    auto connection = data->connection();
    std::unique_ptr<sql::Statement> stmt(connection->createStatement());
    $expect(stmt.get()).Not.toBeNull("stmt is NULL");

    $expect(populate_test_table(stmt)).toBeTrue("Data not populated");

    // Get a result set.
    $expect(stmt->executeUpdate("UPDATE test_function SET a = 123")).toEqual(1, "Number of updated rows");

    data->removeTestTable();
  });

  $it("Test executeUpdate() - execute a SELECT, should get an exception", [this]() {
    auto connection = data->connection();
    std::unique_ptr<sql::Statement> stmt(connection->createStatement());
    $expect(stmt.get()).Not.toBeNull("stmt is NULL");

    $expect(populate_test_table(stmt)).toBeTrue("Data not populated");

    // Get a result set.
    try {
      stmt->executeUpdate("SELECT * FROM test_function");
      // TODO: executing a query which returns a result set should throw an exception.
      // fail("No exception thrown");
    } catch (sql::SQLException &) {
    }

    data->removeTestTable();
  });

  $it("Test getFetchSize() - should return int value.", [this]() {
    auto connection = data->connection();
    std::unique_ptr<sql::Statement> stmt(connection->createStatement());
    $expect(stmt.get()).Not.toBeNull("stmt is NULL");

    // TODO: implement and test getFetchSize() and getFechtDirection()
    // $expect(stmt->getFetchSize() > 0).toBeTrue("fetchSize not > 0");
    $pending("needs implementation of getFetchSize and getFetchDirection");
  });

  $it("Test getResultSet() - execute() a query and get the result set.", [this]() {
    // We don't test Statement::getMaxRows(), Statement::getMoreResults() and
    // Statement::getQueryTimeout() as they don't exist yet.
    auto connection = data->connection();
    std::unique_ptr<sql::Statement> stmt(connection->createStatement());
    $expect(stmt.get()).Not.toBeNull("stmt is NULL");

    $expect(populate_test_table(stmt)).toBeTrue("Data not populated");
    $expect(stmt->execute("SELECT * FROM test_function")).toBeTrue("Statement::execute returned false");

    std::unique_ptr<sql::ResultSet> rset(stmt->getResultSet());
    $expect(rset.get()).Not.toBeNull("rset is NULL");

    data->removeTestTable();
  });

  $it("Test getResultSet() - execute() an update query and get the result set - should be empty.", [this]() {
    auto connection = data->connection();
    std::unique_ptr<sql::Statement> stmt(connection->createStatement());
    $expect(stmt.get()).Not.toBeNull("stmt is NULL");

    $expect(populate_test_table(stmt)).toBeTrue("Data not populated");

    $expect(stmt->execute("UPDATE test_function SET a = 222")).toBeFalse("Statement::execute returned true");
    std::unique_ptr<sql::ResultSet> rset(stmt->getResultSet());
    $expect(rset.get()).toBeNull();

    data->removeTestTable();
  });

  // We don't test Statement::getResultSetConcurrency() as such doesn't exist.
  // We don't test Statement::getResultSetType() as such doesn't exist.
  // We don't test Statement::getUpdateCount() as such doesn't exist.
  // We don't test Statement::getWarnings() as such doesn't exist.
  // TODO: Doesn't pass because setFetchSize() is unimplemented.
  $it("Test setFetchSize() - set and get the value.", [&]() {
    db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

    setupConnectionEnvironment(connectionProperties);

    try {
      sql::DriverManager *dm = sql::DriverManager::getDriverManager();
      $expect(dm).Not.toBeNull("dm is NULL");

      sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
      $expect(wrapper.get()).Not.toBeNull("conn is NULL");

      sql::Connection *connection = wrapper.get();

      std::unique_ptr<sql::Statement> stmt(connection->createStatement());
      $expect(stmt.get()).Not.toBeNull("stmt is NULL");
      /*
       int setFetchSize = 50;

       stmt->setFetchSize(setFetchSize);

       ensure_equals("Non-equal", setFetchSize, stmt->getFetchSize());
       */
    } catch (sql::SQLException &e) {
      std::cerr << "ERR: Caught sql::SQLException\n" << e.getErrorCode() << std::endl;
      throw;
    }
  });

  // TODO: Doesn't pass because setFetchSize() is unimplemented.
  $it("Test setFetchSize() - set negative value and expect an exception.", []() {
    db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

    setupConnectionEnvironment(connectionProperties);

    try {
      sql::DriverManager *dm = sql::DriverManager::getDriverManager();
      $expect(dm).Not.toBeNull("dm is NULL");

      sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
      $expect(wrapper.get()).Not.toBeNull("conn is NULL");

      sql::Connection *connection = wrapper.get();

      std::unique_ptr<sql::Statement> stmt(connection->createStatement());
      $expect(stmt.get()).Not.toBeNull("stmt is NULL");
      /*
       try {
       stmt->setFetchSize(-1);
       $expect(false).toBeTrue("No exception");
       } catch (sql::InvalidArgumentException) {
       printf("INFO: Caught sql::InvalidArgumentException\n");
       }
       */
    } catch (sql::SQLException &e) {
      std::cerr << "ERR: Caught sql::SQLException\n" << e.getErrorCode() << std::endl;
      throw;
    }
  });

  // TODO: Doesn't pass because setQueryTimeout() is unimplemented.
  $it("Test setQueryTimeout() - set negative value and expect an exception.", []() {
    db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

    setupConnectionEnvironment(connectionProperties);

    try {
      sql::DriverManager *dm = sql::DriverManager::getDriverManager();
      $expect(dm).Not.toBeNull("dm is NULL");

      sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
      $expect(wrapper.get()).Not.toBeNull("conn is NULL");

      sql::Connection *connection = wrapper.get();

      std::unique_ptr<sql::Statement> stmt(connection->createStatement());
      $expect(stmt.get()).Not.toBeNull("stmt is NULL");
      /*
       try {
       stmt->setQueryTimeout(-1);
       printf("ERR: No exception\n");
       } catch (sql::InvalidArgumentException &e) {
       delete e;
       }
       */
    } catch (sql::SQLException &e) {
      std::cerr << "ERR: Caught sql::SQLException\n" << e.getErrorCode() << std::endl;
      throw;
    }
  });

  $it("Test addBatch()/executeBatch() (includes a test against the 'out of sync' error).", [this]() {
    db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

    setupConnectionEnvironment(connectionProperties);

    try {
      sql::DriverManager *dm = sql::DriverManager::getDriverManager();
      $expect(dm).Not.toBeNull("dm is NULL");

      sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
      $expect(wrapper.get()).Not.toBeNull("conn is NULL");

      sql::Connection *connection = wrapper.get();

      std::unique_ptr<sql::Statement> stmt(connection->createStatement());
      $expect(stmt.get()).Not.toBeNull("stmt is NULL");

      std::string sql_script =
      "DROP DATABASE IF EXISTS dbc_statement_test_15;"
      "CREATE DATABASE dbc_statement_test_15;"
      "CREATE TABLE dbc_statement_test_15.table1 (id int);"
      "SELECT 1;"
      "CREATE TABLE dbc_statement_test_15.table2 (id int);"
      "SELECT 1;"
      "CREATE TABLE dbc_statement_test_15.table3 (id int);"
      "SELECT 1;";
      std::list<std::string> statements;

      $expect(data->sqlSplitter).Not.toBeNull("failed to get sqlparser module");

      data->sqlSplitter->splitSqlScript(sql_script, statements);
      sql::SqlBatchExec()(stmt.get(), statements);

      // Cleanup-
      sql_script = "DROP DATABASE IF EXISTS dbc_statement_test_15;";
      data->sqlSplitter->splitSqlScript(sql_script, statements);
      sql::SqlBatchExec()(stmt.get(), statements);
    } catch (sql::SQLException &e) {
      std::cerr << "ERR: Caught sql::SQLException\n" << e.getErrorCode() << std::endl;
      throw;
    }
  });
}

}
