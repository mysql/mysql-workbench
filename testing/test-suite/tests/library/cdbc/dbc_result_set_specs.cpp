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

#include "wb_connection_helpers.h"
#include "wb_test_helpers.h"
#include "helpers.h"
#include "casmine.h"

extern void register_all_metaclasses();

static bool populate_test_table(std::unique_ptr<sql::Statement> &stmt) {
  stmt->execute("USE test");
  stmt->execute("DROP TABLE IF EXISTS test_function");
  if (true == stmt->execute("CREATE TABLE test_function (a integer, b integer, c integer default null)"))
    return false;

  if (true == stmt->execute("INSERT INTO test_function (a,b,c) VALUES(1, 111, NULL)")) {
    stmt->execute("DROP TABLE test_function");
    return false;
  }
  return true;
}

static bool populate_tx_test_table(std::unique_ptr<sql::Statement> &stmt) {
  stmt->execute("USE test");
  stmt->execute("DROP TABLE IF EXISTS test_function_tx");
  if (true ==
      stmt->execute("CREATE TABLE test_function_tx (a integer, b integer, c integer default null) engine = innodb"))
    return false;

  if (true == stmt->execute("INSERT INTO test_function_tx (a,b,c) VALUES(1, 111, NULL)")) {
    stmt->execute("DROP TABLE test_function_tx");
    return false;
  }
  stmt->getConnection()->commit();
  return true;
}

namespace {

$ModuleEnvironment() {};

$describe("DBC: PS tests") {
  $beforeAll([&]() {
    register_all_metaclasses();
    grt::GRT::get()->scan_metaclasses_in("../../res/grt/");
    grt::GRT::get()->end_loading_metaclasses();
    $expect(grt::GRT::get()->get_metaclasses().size()).toBe((size_t)INT_METACLASS_COUNT, "load structs");
  });

  $afterAll([&]() {
    db_mgmt_ConnectionRef connectionProperties(grt::Initialized);
    setupConnectionEnvironment(connectionProperties);
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();

    sql::ConnectionWrapper wrapper1 = dm->getConnection(connectionProperties);
    sql::Connection *connection = wrapper1.get();
    std::unique_ptr<sql::Statement> stmt(connection->createStatement());
    stmt->execute("DROP SCHEMA IF EXISTS test;");

    WorkbenchTester::reinitGRT();
  });

  $it("test connection", [&]() {
    db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

    setupConnectionEnvironment(connectionProperties);
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    $expect(dm).Not.toBeNull("dm is NULL");

    sql::ConnectionWrapper wrapper1 = dm->getConnection(connectionProperties);
    $expect(wrapper1.get()).Not.toBeNull();

    sql::Connection *connection = wrapper1.get();
    std::unique_ptr<sql::Statement> stmt(connection->createStatement());
    $expect(stmt.get()).Not.toBeNull();

    $expect(connection).toBe(stmt->getConnection());

    stmt->execute("DROP SCHEMA IF EXISTS test; CREATE SCHEMA test");
  });

  $it("Test preparation.", [&]() {
    db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

    setupConnectionEnvironment(connectionProperties);

    try {
      sql::DriverManager *dm = sql::DriverManager::getDriverManager();
      $expect(dm).Not.toBe(nullptr, "dm is NULL");

      sql::ConnectionWrapper wrapper1 = dm->getConnection(connectionProperties);
      $expect(wrapper1.get()).Not.toBe(nullptr, "conn is NULL");

      sql::Connection *connection = wrapper1.get();
      std::unique_ptr<sql::Statement> stmt1(connection->createStatement());
      $expect(stmt1.get()).Not.toBe(nullptr, "stmt1 is NULL");

      $expect(wrapper1.get()).toBe(stmt1->getConnection(), "Connection differs");

      $expect(populate_tx_test_table(stmt1)).toBeTrue("Data not populated");

      std::unique_ptr<sql::PreparedStatement> ps1(wrapper1->prepareStatement("SELECT a, b, c FROM test_function_tx"));

      $expect(ps1.get()).Not.toBe(nullptr, "ps1 is NULL");

      // TODO: getConnection is not yet implemented.
      // ensure_equals("Connection differs", wrapper1.get(), ps1->getConnection());

      std::unique_ptr<sql::ResultSet> rset(ps1->executeQuery());
      $expect(rset.get()).Not.toBe(nullptr, "NULL returned for result set");
      while (rset->next())
       ;

      stmt1->execute("DROP TABLE test_function_tx");
    } catch (sql::SQLException &e) {
      std::cerr << "ERR: Caught sql::SQLException\n" << e.getErrorCode() << std::endl;
      throw;
    }
  });

  $it("Test executing executeQuery on the same statement.", [&]() {
    db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

    setupConnectionEnvironment(connectionProperties);

    try {
      sql::DriverManager *dm = sql::DriverManager::getDriverManager();
      $expect(dm).Not.toBe(nullptr, "dm is NULL");

      sql::ConnectionWrapper wrapper1 = dm->getConnection(connectionProperties);
      $expect(wrapper1.get()).Not.toBe(nullptr, "conn is NULL");

      sql::Connection *connection = wrapper1.get();
      std::unique_ptr<sql::Statement> stmt1(connection->createStatement());
      $expect(stmt1.get()).Not.toBe(nullptr, "stmt1 is NULL");

      std::unique_ptr<sql::ResultSet> rset1(stmt1->executeQuery("SELECT 1 FROM DUAL"));
      $expect(rset1.get()).Not.toBe(nullptr, "rset1 is NULL");

      // TODO: Fails currently because the statements are not store-d but use-d.
      // std::auto_ptr<sql::Statement> rset2(stmt1->executeQuery("SELECT 1 FROM DUAL"));
      // ensure("res2 is NULL", rset2.get() != NULL);
      $expect(rset1->next()).toBe(true, "res1 is empty");
      // TODO: enable once the part above is fixed.
      // ensure("res2 is empty", rset2->next() != false);
    } catch (sql::SQLException &e) {
      std::cerr << "ERR: Caught sql::SQLException\n" << e.getErrorCode() << std::endl;
      throw;
    }
  });

  $it("Test executing two different queries from the same statement.", [&]() {
    db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

    setupConnectionEnvironment(connectionProperties);

    try {
      sql::DriverManager *dm = sql::DriverManager::getDriverManager();
      $expect(dm).Not.toBe(nullptr, "dm is NULL");

      sql::ConnectionWrapper wrapper1 = dm->getConnection(connectionProperties);
      $expect(wrapper1.get()).Not.toBe(nullptr, "conn is NULL");

      sql::Connection *connection = wrapper1.get();
      std::unique_ptr<sql::Statement> stmt1(connection->createStatement());
      $expect(stmt1.get()).Not.toBe(nullptr, "stmt1 is NULL");

      $expect(populate_test_table(stmt1)).toBeTrue("Data not populated");

      std::unique_ptr<sql::ResultSet> rset1(stmt1->executeQuery("SELECT 1 FROM DUAL"));

      $expect(rset1.get()).Not.toBeNull("res1 is NULL");
      $expect(rset1->next()).toBeTrue("rset1 is empty");
      $expect(rset1->next()).toBeFalse("rset1 is empty");

      $expect(stmt1->executeUpdate("UPDATE test_function SET a = 2")).toBeGreaterThan(0, "No rows updated");

      stmt1->execute("DROP TABLE test_function");
    } catch (sql::SQLException &e) {
      std::cerr << "ERR: Caught sql::SQLException\n" << e.getErrorCode() << std::endl;
      throw;
    }
  });

  $it("Test commit and rollback (autocommit off).", [&]() {
    db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

    setupConnectionEnvironment(connectionProperties);

    try {
      sql::DriverManager *dm = sql::DriverManager::getDriverManager();
      $expect(dm).Not.toBe(nullptr, "dm is NULL");

      sql::ConnectionWrapper wrapper1 = dm->getConnection(connectionProperties);
      $expect(wrapper1.get()).Not.toBe(nullptr, "conn is NULL");

      sql::Connection *connection = wrapper1.get();
      std::unique_ptr<sql::Statement> stmt1(connection->createStatement());
      $expect(stmt1.get()).Not.toBe(nullptr, "stmt1 is NULL");

      $expect(wrapper1.get()).toBe(stmt1->getConnection(), "Connection differs");

      bool old_commit_mode = wrapper1->getAutoCommit();
      wrapper1->setAutoCommit(0);
      $expect(populate_tx_test_table(stmt1)).toBeTrue("Data not populated");

      std::unique_ptr<sql::ResultSet> rset1(stmt1->executeQuery("SELECT COUNT(*) FROM test_function_tx"));
      $expect(rset1.get()).Not.toBe(nullptr, "res1 is NULL");
      $expect(rset1->next()).toBeTrue("res1 is empty");
      int count_full_before = rset1->getInt(1);
      $expect(rset1->next()).toBeFalse("res1 has more rows");

      /* Let's delete and then rollback */
      $expect(stmt1->executeUpdate("DELETE FROM test_function_tx WHERE 1")).toBe(count_full_before, "Deleted less rows");

      std::unique_ptr<sql::ResultSet> rset2(stmt1->executeQuery("SELECT COUNT(*) FROM test_function_tx"));
      $expect(rset2.get()).Not.toBe(nullptr, "res2 is NULL");

      $expect(rset2->next()).toBeTrue("res2 is empty");

      $expect(rset2->getInt(1)).toEqual(0, "Table not empty after delete");
      $expect(rset2->next()).toBeFalse("res2 has more rows");

      stmt1->getConnection()->rollback();

      std::unique_ptr<sql::ResultSet> rset3(stmt1->executeQuery("SELECT COUNT(*) FROM test_function_tx"));
      $expect(rset3.get()).Not.toBe(nullptr, "res3 is NULL");
      $expect(rset3->next()).toBeTrue("res3 is empty");

      int count_full_after = rset3->getInt(1);
      $expect(rset3->next()).toBeFalse("res3 has more rows");

      $expect(count_full_before).toBe(count_full_after, "Rollback didn't work");

      /* Now let's delete and then commit */
      $expect(stmt1->executeUpdate("DELETE FROM test_function_tx WHERE 1")).toBe(count_full_before, "Deleted less rows");
      stmt1->getConnection()->commit();

      std::unique_ptr<sql::ResultSet> rset4(stmt1->executeQuery("SELECT COUNT(*) FROM test_function_tx"));
      $expect(rset4.get()).Not.toBe(nullptr, "res4 is NULL");
      $expect(rset4->next()).toBeTrue("res4 is empty");
      $expect(rset4->getInt(1)).toEqual(0, "Table not empty after delete");
      $expect(rset4->next()).toBeFalse("res4 has more rows");

      stmt1->execute("DROP TABLE test_function_tx");

      wrapper1->setAutoCommit(old_commit_mode);
    } catch (sql::SQLException &e) {
      std::cerr << "ERR: Caught sql::SQLException\n" << e.getErrorCode() << std::endl;
      throw;
    }
  });

  $it("Test commit and rollback (autocommit on).", [&]() {
    db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

    setupConnectionEnvironment(connectionProperties);

    try {
      sql::DriverManager *dm = sql::DriverManager::getDriverManager();
      $expect(dm).Not.toBe(nullptr, "dm is NULL");

      sql::ConnectionWrapper wrapper1 = dm->getConnection(connectionProperties);
      $expect(wrapper1.get()).Not.toBe(nullptr, "conn is NULL");

      sql::Connection *connection = wrapper1.get();
      std::unique_ptr<sql::Statement> stmt1(connection->createStatement());
      $expect(stmt1.get()).Not.toBe(nullptr, "stmt1 is NULL");

      $expect(wrapper1.get()).toBe(stmt1->getConnection(), "Connection differs");

      bool old_commit_mode = wrapper1->getAutoCommit();
      wrapper1->setAutoCommit(1);
      $expect(populate_tx_test_table(stmt1)).toBeTrue("Data not populated");

      std::unique_ptr<sql::ResultSet> rset1(stmt1->executeQuery("SELECT COUNT(*) FROM test_function_tx"));
      $expect(rset1.get()).Not.toBe(nullptr, "res1 is NULL");
      $expect(rset1->next()).toBeTrue("res1 is empty");

      int count_full_before = rset1->getInt(1);
      $expect(rset1->next()).toBeFalse("res1 has more rows");

      /* Let's delete and then rollback */
      $expect(stmt1->executeUpdate("DELETE FROM test_function_tx WHERE 1")).toBe(count_full_before, "Deleted less rows");

      std::unique_ptr<sql::ResultSet> rset2(stmt1->executeQuery("SELECT COUNT(*) FROM test_function_tx"));
      $expect(rset2.get()).Not.toBe(nullptr, "res2 is NULL");
      $expect(rset2->next()).toBeTrue("res2 is empty");
      $expect(rset2->getInt(1)).toEqual(0);
      $expect(rset2->next()).toBeFalse("res2 has more rows");

      // In autocommit on, this is a no-op.
      stmt1->getConnection()->rollback();

      std::unique_ptr<sql::ResultSet> rset3(stmt1->executeQuery("SELECT COUNT(*) FROM test_function_tx"));
      $expect(rset3.get()).Not.toBe(nullptr, "res3 is NULL");
      $expect(rset3->next()).toBeTrue("res3 is empty");
      $expect(rset3->getInt(1)).toEqual(0, "Rollback didn't work");
      $expect(rset3->next()).toBeFalse("res3 has more rows");

      $expect(populate_tx_test_table(stmt1)).toBeTrue("Data not populated");

      /* Now let's delete and then commit */
      $expect(stmt1->executeUpdate("DELETE FROM test_function_tx WHERE 1")).toBe(count_full_before, "Deleted less rows");
      // In autocommit on, this is a no-op.
      stmt1->getConnection()->commit();

      std::unique_ptr<sql::ResultSet> rset4(stmt1->executeQuery("SELECT COUNT(*) FROM test_function_tx"));
      $expect(rset4.get()).Not.toBe(nullptr, "res4 is NULL");
      $expect(rset4->next()).toBeTrue("res4 is empty");
      $expect(rset4->getInt(1)).toEqual(0, "Table not empty after delete");
      $expect(rset4->next()).toBeFalse("res4 has more rows");

      stmt1->execute("DROP TABLE test_function_tx");

      wrapper1->setAutoCommit(old_commit_mode);
    } catch (sql::SQLException &e) {
      std::cerr << "ERR: Caught sql::SQLException\n" << e.getErrorCode() << std::endl;
      throw;
    }
  });

  $it("Test multistatement off - send two queries in one call.", [&]() {
    db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

    setupConnectionEnvironment(connectionProperties);

    try {
      sql::DriverManager *dm = sql::DriverManager::getDriverManager();
      $expect(dm).Not.toBe(nullptr, "dm is NULL");

      sql::ConnectionWrapper wrapper1 = dm->getConnection(connectionProperties);
      $expect(wrapper1.get()).Not.toBe(nullptr, "conn is NULL");

      sql::Connection *connection = wrapper1.get();
      std::unique_ptr<sql::Statement> stmt1(connection->createStatement());
      $expect(stmt1.get()).Not.toBe(nullptr, "stmt1 is NULL");

      try {
        std::unique_ptr<sql::ResultSet> rset1(
          stmt1->executeQuery("SELECT COUNT(*) FROM test_function_tx; DELETE FROM test_function_tx"));
        $fail("ERR: Exception not thrown");
      } catch (sql::SQLException &) {
      }

    } catch (sql::SQLException &e) {
      std::cerr << "ERR: Caught sql::SQLException\n" << e.getErrorCode() << std::endl;
      throw;
    }
  });

  $it("Test out of bound extraction of data.", [&]() {
    db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

    setupConnectionEnvironment(connectionProperties);

    try {
      sql::DriverManager *dm = sql::DriverManager::getDriverManager();
      $expect(dm).Not.toBe(nullptr, "dm is NULL");

      sql::ConnectionWrapper wrapper1 = dm->getConnection(connectionProperties);
      $expect(wrapper1.get()).Not.toBe(nullptr, "conn is NULL");

      sql::Connection *connection = wrapper1.get();
      std::unique_ptr<sql::Statement> stmt1(connection->createStatement());
      $expect(stmt1.get()).Not.toBe(nullptr, "stmt1 is NULL");

      $expect(wrapper1.get()).toBe(stmt1->getConnection(), "Connection differs");

      $expect(populate_tx_test_table(stmt1)).toBeTrue("Data not populated");

      std::unique_ptr<sql::ResultSet> rset1(
        stmt1->executeQuery("SELECT COUNT(*) AS 'count of rows' FROM test_function_tx"));
      $expect(rset1.get()).Not.toBe(nullptr, "res1 is NULL");
      $expect(rset1->next()).toBeTrue("res1 is empty");

      try {
        rset1->getInt(-123);
        $fail("ERR: No InvalidArgumentException thrown");
      } catch (sql::InvalidArgumentException &) {
      }
      try {
        rset1->getInt(123);
        $fail("ERR: No InvalidArgumentException thrown");
      } catch (sql::InvalidArgumentException &) {
      }
      try {
        rset1->getInt("no_such_column");
        $fail("ERR: No InvalidArgumentException thrown");
      } catch (sql::InvalidArgumentException &) {
      }
      try {
        rset1->getString(-123);
        $fail("ERR: No InvalidArgumentException thrown");
      } catch (sql::InvalidArgumentException &) {
      }
      try {
        rset1->getString(123);
        $fail("ERR: No InvalidArgumentException thrown");
      } catch (sql::InvalidArgumentException &) {
      }
      try {
        rset1->getString("no_such_column");
        $fail("ERR: No InvalidArgumentException thrown");
      } catch (sql::InvalidArgumentException &) {
      }
      try {
        rset1->getDouble(-123);
        $fail("ERR: No InvalidArgumentException thrown");
      } catch (sql::InvalidArgumentException &) {
      }
      try {
        rset1->getDouble(123);
        $fail("ERR: No InvalidArgumentException thrown");
      } catch (sql::InvalidArgumentException &) {
      }
      try {
        rset1->getDouble("no_such_column");
        $fail("ERR: No InvalidArgumentException thrown");
      } catch (sql::InvalidArgumentException &) {
      }
      try {
        rset1->getInt(rset1->getInt(1) + 1);
        $fail("ERR: No InvalidArgumentException thrown");
      } catch (sql::InvalidArgumentException &) {
      }
      try {
        rset1->isNull(-123);
        $fail("ERR: No InvalidArgumentException thrown");
      } catch (sql::InvalidArgumentException &) {
      }
      try {
        rset1->isNull(123);
        $fail("ERR: No InvalidArgumentException thrown");
      } catch (sql::InvalidArgumentException &) {
      }
      try {
        rset1->isNull("no_such_column");
        $fail("ERR: No InvalidArgumentException thrown");
      } catch (sql::InvalidArgumentException &) {
      }

      $expect(rset1->getInt(1)).toEqual(1, "getInt invalid for column index");
      $expect(rset1->getInt("count of rows")).toEqual(1, "getInt invalid for column name");

      $expect(rset1->getDouble(1) - 1 < 0.1).toBeTrue("getDouble invalid for column index");
      $expect(rset1->getDouble("count of rows") - 1 < 0.1).toBeTrue("getDouble invalid for column name");

      $expect(rset1->getString(1)).toEqual(std::string("1"), "getString invalid for column index");
      $expect(rset1->getString("count of rows")).toEqual(std::string("1"), "getString invalid for column name");

      $expect(rset1->isNull(1)).toBeFalse("c is not null");
      $expect(rset1->next()).toBeFalse("rest1 has more rows");

      stmt1->execute("DROP TABLE test_function_tx");
    } catch (sql::SQLException &e) {
      std::cerr << "ERR: Caught sql::SQLException\n" << e.getErrorCode() << std::endl;
      throw;
    }
  });
};

}
