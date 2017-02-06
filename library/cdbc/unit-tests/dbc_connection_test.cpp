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

#include "connection_helpers.h"
#include "wb_helpers.h"

using namespace grt;
using namespace bec;
using namespace tut;

BEGIN_TEST_DATA_CLASS(module_dbc_connection_test)
public:
WBTester *wbt;
db_mgmt_ConnectionRef connectionProperties;
TEST_DATA_CONSTRUCTOR(module_dbc_connection_test) {
  wbt = new WBTester();
}
END_TEST_DATA_CLASS

TEST_MODULE(module_dbc_connection_test, "DBC: connection tests");

// Test initialization of a connection and it's destruction.
TEST_FUNCTION(1) {
  connectionProperties = db_mgmt_ConnectionRef(grt::Initialized);
  setup_env(connectionProperties);

  sql::DriverManager *dm = sql::DriverManager::getDriverManager();
  ensure("dm is NULL", dm != NULL);

  /* QQQ
  getDrivers() is not implemented

  std::list<sql::Driver *> drivers = dm->getDrivers();
  for (std::list<sql::Driver *>::iterator it = drivers.begin(); it != drivers.end(); it++)
  {
    sql::Driver *driver = *it;
    driver->getMajorVersion();
    driver->getMinorVersion();
    driver->getName();
  }
  */
}

// Test initialization of a statement and it's destruction.
TEST_FUNCTION(3) {
  // db_mgmt_ConnectionRef connectionProperties;
  // setup_env(connectionProperties);

  sql::DriverManager *dm = sql::DriverManager::getDriverManager();
  ensure("dm is NULL", dm != NULL);

  sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
  ensure("conn is NULL", wrapper.get() != NULL);
  sql::Connection *connection = wrapper.get();
  {
    std::auto_ptr<sql::Statement> stmt(connection->createStatement());
    ensure("stmt is NULL", stmt.get() != NULL);
  }
}

// Test construction of a metadata object.
TEST_FUNCTION(4) {
  // db_mgmt_ConnectionRef connectionProperties;
  // setup_env(connectionProperties);

  sql::DriverManager *dm = sql::DriverManager::getDriverManager();
  ensure("dm is NULL", dm != NULL);

  sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
  ensure("conn is NULL", wrapper.get() != NULL);
  sql::Connection *connection = wrapper.get();
  {
    sql::DatabaseMetaData *meta(connection->getMetaData());
    ensure("meta is NULL", meta != NULL);
  }
}

// Test autocommit.
TEST_FUNCTION(5) {
  // db_mgmt_ConnectionRef connectionProperties;
  // setup_env(connectionProperties);

  sql::DriverManager *dm = sql::DriverManager::getDriverManager();
  ensure("dm is NULL", dm != NULL);

  sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
  ensure("conn is NULL", wrapper.get() != NULL);
  sql::Connection *connection = wrapper.get();
  try {
    connection->commit();
    connection->rollback();

    bool hadAutoCommit = connection->getAutoCommit();
    connection->setAutoCommit(true);
    ensure_equals("autocommit differs", connection->getAutoCommit(), true);

    connection->commit();
    connection->setAutoCommit(false);
    ensure_equals("autocommit differs", connection->getAutoCommit(), false);

    connection->commit();
    /* Try to set an invalid mode */
    // try {
    //  conn->setAutoCommit(-1);
    //  ensure("sql::InvalidArgumentException expected but not thrown", false);
    //} catch (sql::InvalidArgumentException &e) {
    //  /* Correctly thrown exception */
    //}
    /* Last valid was 0, we should leave it 0 */
    ensure_equals("autocommit differs", connection->getAutoCommit(), false);

    /* Leave the connection in the same state */
    connection->setAutoCommit(hadAutoCommit);
    ensure_equals("autocommit differs", connection->getAutoCommit(), hadAutoCommit);

  } catch (sql::SQLException &e) {
    printf("ERR: Caught sql::SQLException: %s\n", e.what());
    throw;
  }
}

// Test clearWarnings.
TEST_FUNCTION(6) {
  // db_mgmt_ConnectionRef connectionProperties;
  // setup_env(connectionProperties);

  sql::DriverManager *dm = sql::DriverManager::getDriverManager();
  ensure("dm is NULL", dm != NULL);

  sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
  ensure("conn is NULL", wrapper.get() != NULL);
  sql::Connection *connection = wrapper.get();

  /* Clear tripple times */ // WHY? ml
  connection->clearWarnings();
  connection->clearWarnings();
  connection->clearWarnings();
}

// Test 2 connections.
TEST_FUNCTION(7) {
  // db_mgmt_ConnectionRef connectionProperties;
  // setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper1 = dm->getConnection(connectionProperties);
    ensure("wrapper1 is NULL", wrapper1.get() != NULL);
    sql::Connection *connection1 = wrapper1.get();

    sql::ConnectionWrapper wrapper2 = dm->getConnection(connectionProperties);
    ensure("wrapper2 is NULL", wrapper2.get() != NULL);
    sql::Connection *connection2 = wrapper2.get();

    std::auto_ptr<sql::Statement> stmt1(connection1->createStatement());
    ensure("stmt1 is NULL", stmt1.get() != NULL);

    std::auto_ptr<sql::Statement> stmt2(connection2->createStatement());
    ensure("stmt2 is NULL", stmt2.get() != NULL);

    std::auto_ptr<sql::ResultSet> rset1(stmt1->executeQuery("SELECT CONNECTION_ID()"));
    ensure("res1 is NULL", rset1.get() != NULL);

    std::auto_ptr<sql::ResultSet> rset2(stmt2->executeQuery("SELECT CONNECTION_ID()"));
    ensure("res2 is NULL", rset2.get() != NULL);

    ensure("res1 is empty", rset1->next() != false);
    ensure("res2 is empty", rset2->next() != false);

    ensure("same connection", rset1->getInt(1) != rset2->getInt(1));
  } catch (sql::SQLException &e) {
    printf("ERR: Caught sql::SQLException: %s\n", e.what());
    throw;
  }
}

// Test kill ourselves 1.
TEST_FUNCTION(8) {
  // db_mgmt_ConnectionRef connectionProperties;
  // setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper1 = dm->getConnection(connectionProperties);
    ensure("wrapper1 is NULL", wrapper1.get() != NULL);
    sql::Connection *connection = wrapper1.get();

    std::auto_ptr<sql::Statement> stmt1(connection->createStatement());
    ensure("stmt1 is NULL", stmt1.get() != NULL);

    std::auto_ptr<sql::ResultSet> rset1(stmt1->executeQuery("SELECT CONNECTION_ID()"));
    ensure("res1 is NULL", rset1.get() != NULL);

    ensure("res1 is empty", rset1->next() != false);

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
}

// Test kill ourselves 2 - kill and query thereafter.
TEST_FUNCTION(9) {
  // db_mgmt_ConnectionRef connectionProperties;
  // setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper1 = dm->getConnection(connectionProperties);
    ensure("wrapper1 is NULL", wrapper1.get() != NULL);
    sql::Connection *connection = wrapper1.get();

    std::auto_ptr<sql::Statement> stmt1(connection->createStatement());
    ensure("stmt1 is NULL", stmt1.get() != NULL);

    std::auto_ptr<sql::ResultSet> rset1(stmt1->executeQuery("SELECT CONNECTION_ID()"));
    ensure("res1 is NULL", rset1.get() != NULL);

    ensure("res1 is empty", rset1->next() != false);

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
      std::auto_ptr<sql::ResultSet> rset2(stmt1->executeQuery("SELECT CONNECTION_ID()"));
    } catch (sql::SQLException &e) {
      // Expected.
      ensure_equals("Unexpected exception", e.what(), "Commands out of sync; you can't run this command now");
    }
  } catch (sql::SQLException &e) {
    printf("ERR: Caught sql::SQLException: %s\n", e.what());
    throw;
  }
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  delete wbt;
}
END_TESTS
