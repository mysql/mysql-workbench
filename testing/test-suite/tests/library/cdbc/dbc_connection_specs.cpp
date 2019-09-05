/*
 * Copyright (c) 2011, 2019, Oracle and/or its affiliates. All rights reserved.
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

$TestData {
  db_mgmt_ConnectionRef connectionProperties;
};

$describe("DBC: connection tests") {
  $beforeAll([&]() {
    register_all_metaclasses();
    grt::GRT::get()->scan_metaclasses_in("../../res/grt/");
    grt::GRT::get()->end_loading_metaclasses();
    $expect(grt::GRT::get()->get_metaclasses().size()).toBe((size_t)INT_METACLASS_COUNT, "load structs");

    data->connectionProperties = db_mgmt_ConnectionRef(grt::Initialized);
    setupConnectionEnvironment(data->connectionProperties);
  });

  $afterAll([&]() {
    WorkbenchTester::reinitGRT();
  });

  $it("Test initialization of a connection and it's destruction", [&]() {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    $expect(dm).Not.toBe(nullptr, "dm is NULL");

  });

  $it("Test initialization of a statement and it's destruction.", [&]() {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    $expect(dm).Not.toBe(nullptr, "dm is NULL");

    sql::ConnectionWrapper wrapper = dm->getConnection(data->connectionProperties);
    $expect(wrapper.get()).Not.toBe(nullptr, "conn is NULL");
    sql::Connection *connection = wrapper.get();
    {
      std::unique_ptr<sql::Statement> stmt(connection->createStatement());
      $expect(stmt.get()).Not.toBe(nullptr, "stmt is NULL");
    }
  });

  $it("Test construction of a metadata object.", [&]() {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    $expect(dm).Not.toBe(nullptr, "dm is NULL");

    sql::ConnectionWrapper wrapper = dm->getConnection(data->connectionProperties);
    $expect(wrapper.get()).Not.toBe(nullptr, "conn is NULL");
    sql::Connection *connection = wrapper.get();
    {
      sql::DatabaseMetaData *meta(connection->getMetaData());
      $expect(meta).Not.toBe(nullptr, "meta is NULL");
    }
  });

  $it("Test autocommit.", [&]() {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    $expect(dm).Not.toBe(nullptr, "dm is NULL");


    sql::ConnectionWrapper wrapper = dm->getConnection(data->connectionProperties);
    $expect(wrapper.get()).Not.toBe(nullptr, "conn is NULL");

    sql::Connection *connection = wrapper.get();
    try {
      connection->commit();
      connection->rollback();

      bool hadAutoCommit = connection->getAutoCommit();
      connection->setAutoCommit(true);
      $expect(connection->getAutoCommit()).toBeTrue("autocommit differs");

      connection->commit();
      connection->setAutoCommit(false);
      $expect(connection->getAutoCommit()).toBeFalse("autocommit differs");


      connection->commit();
      /* Try to set an invalid mode */
      // try {
      //  conn->setAutoCommit(-1);
      //  ensure("sql::InvalidArgumentException expected but not thrown", false);
      //} catch (sql::InvalidArgumentException &e) {
      //  /* Correctly thrown exception */
      //}
      /* Last valid was 0, we should leave it 0 */
      $expect(connection->getAutoCommit()).toBeFalse("autocommit differs");

      /* Leave the connection in the same state */
      connection->setAutoCommit(hadAutoCommit);
      $expect(connection->getAutoCommit()).toBe(hadAutoCommit, "autocommit differs");

    } catch (sql::SQLException &e) {
      printf("ERR: Caught sql::SQLException: %s\n", e.what());
      throw;
    }
  });

  $it("Test clearWarnings.", [&]() {
    // db_mgmt_ConnectionRef connectionProperties;
    // setupConnectionEnvironment(connectionProperties);

    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    $expect(dm).Not.toBe(nullptr, "dm is NULL");

    sql::ConnectionWrapper wrapper = dm->getConnection(data->connectionProperties);
    $expect(wrapper.get()).Not.toBe(nullptr, "wrapper is NULL");
    sql::Connection *connection = wrapper.get();

    /* Clear tripple times */ // WHY? ml
    connection->clearWarnings();
    connection->clearWarnings();
    connection->clearWarnings();
  });



  $it("Test 2 connections.", [&]() {
    try {
      sql::DriverManager *dm = sql::DriverManager::getDriverManager();
      $expect(dm).Not.toBe(nullptr, "dm is NULL");

      sql::ConnectionWrapper wrapper1 = dm->getConnection(data->connectionProperties);
      $expect(wrapper1.get()).Not.toBe(nullptr, "conn is NULL");
      sql::Connection *connection1 = wrapper1.get();

      sql::ConnectionWrapper wrapper2 = dm->getConnection(data->connectionProperties);
      $expect(wrapper2.get()).Not.toBe(nullptr, "conn2 is NULL");
      sql::Connection *connection2 = wrapper2.get();

      std::unique_ptr<sql::Statement> stmt1(connection1->createStatement());
      $expect(stmt1.get()).Not.toBe(nullptr, "stmt1 is NULL");

      std::unique_ptr<sql::Statement> stmt2(connection2->createStatement());
      $expect(stmt2.get()).Not.toBe(nullptr, "stmt2 is NULL");


      std::unique_ptr<sql::ResultSet> rset1(stmt1->executeQuery("SELECT CONNECTION_ID()"));
      $expect(rset1.get()).Not.toBe(nullptr, "rset1 is NULL");

      std::unique_ptr<sql::ResultSet> rset2(stmt2->executeQuery("SELECT CONNECTION_ID()"));
      $expect(rset2.get()).Not.toBe(nullptr, "rset2 is NULL");


      $expect(rset1->next()).toBeTrue("rset1 is empty");
      $expect(rset2->next()).toBeTrue("rset2 is empty");

      $expect(rset1->getInt(1)).Not.toBe(rset2->getInt(1), "same connection");
    } catch (sql::SQLException &e) {
      printf("ERR: Caught sql::SQLException: %s\n", e.what());
      throw;
    }
  });


  $it("Test kill ourselves 1.", [&]() {
    try {
      sql::DriverManager *dm = sql::DriverManager::getDriverManager();
      $expect(dm).Not.toBe(nullptr, "dm is NULL");

      sql::ConnectionWrapper wrapper1 = dm->getConnection(data->connectionProperties);
      $expect(wrapper1.get()).Not.toBe(nullptr, "conn is NULL");
      sql::Connection *connection = wrapper1.get();

      std::unique_ptr<sql::Statement> stmt1(connection->createStatement());
      $expect(stmt1.get()).Not.toBe(nullptr, "stmt1 is NULL");

      std::unique_ptr<sql::ResultSet> rset1(stmt1->executeQuery("SELECT CONNECTION_ID()"));
      $expect(rset1.get()).Not.toBe(nullptr, "rset1 is NULL");

      $expect(rset1->next()).toBeTrue("res1 is empty");
      // DBC is not supposed to check that, instead DBC user has to check validity of connection when needed
      // snprintf(buff, sizeof(buff), "KILL %d", rset1->getInt(1));
      // try
      //{
      //  stmt1->execute(buff);
      //  fail("An exception should have shown up.");
      //}
      // catch (sql::SQLException &e) {
      //  // Expected.
      //  ensure_equals("Unexpected exception", e.what(), "Commands out of sync; you can't run this command now");
      //}
    } catch (sql::SQLException &e) {
      printf("ERR: Caught sql::SQLException: %s\n", e.what());
      throw;
    }
  });

  $it("Test kill ourselves 2 - kill and query thereafter.", [&]() {
    try {
      sql::DriverManager *dm = sql::DriverManager::getDriverManager();
      $expect(dm).Not.toBe(nullptr, "dm is NULL");

      sql::ConnectionWrapper wrapper1 = dm->getConnection(data->connectionProperties);
      $expect(wrapper1.get()).Not.toBe(nullptr, "conn is NULL");
      sql::Connection *connection = wrapper1.get();

      std::unique_ptr<sql::Statement> stmt1(connection->createStatement());
      $expect(stmt1.get()).Not.toBe(nullptr, "stmt1 is NULL");

      std::unique_ptr<sql::ResultSet> rset1(stmt1->executeQuery("SELECT CONNECTION_ID()"));
      $expect(rset1.get()).Not.toBe(nullptr, "rset1 is NULL");

      $expect(rset1->next()).toBeTrue("rset1 is empty");

      // DBC is not supposed to check that, instead DBC user has to check validity of connection when needed
      // snprintf(buff, sizeof(buff), "KILL %d", rset1->getInt(1));
      // try
      //{
      //  // Kill the connection. This will give us an exception.
      //  stmt1->execute(buff);
      //  fail("An exception should have shown up.");
      //}
      // catch (sql::SQLException &e) {
      //  // Expected.
      //  ensure_equals("Unexpected exception", e.what(), "Commands out of sync; you can't run this command now");
      //}

      // Try another statement. This should give us another exception
      try {
        std::unique_ptr<sql::ResultSet> rset2(stmt1->executeQuery("SELECT CONNECTION_ID()"));
      } catch (sql::SQLException &e) {
        // Expected.
        $expect(e.what()).toBe("Commands out of sync; you can't run this command now", "Unexpected exception");
      }
    } catch (sql::SQLException &e) {
      printf("ERR: Caught sql::SQLException: %s\n", e.what());
      throw;
    }
  });
};

}
