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

BEGIN_TEST_DATA_CLASS(module_dbc_metadata_test)
public:
END_TEST_DATA_CLASS

TEST_MODULE(module_dbc_metadata_test, "DBC: metadata tests");

TEST_FUNCTION(1) {
  // load structs
  grt::GRT::get()->scan_metaclasses_in("../../res/grt/");
  grt::GRT::get()->end_loading_metaclasses();

  ensure_equals("load structs", grt::GRT::get()->get_metaclasses().size(), (size_t)INT_METACLASS_COUNT);
}

// Test DatabaseMetaData::getCatalogs().
TEST_FUNCTION(2) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
    ensure("conn is NULL", wrapper.get() != NULL);

    sql::Connection *connection = wrapper.get();
    sql::DatabaseMetaData *meta(connection->getMetaData());
    ensure("meta is NULL", meta != NULL);

    // TODO: getCatalogs() is not implemented.
    // std::auto_ptr<sql::Statement> rset(meta->getCatalogs());
    // ensure("rset is not NULL", rset.get() != NULL);
    // while (rset->next());
  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
    throw;
  } catch (...) {
    printf("ERR: Caught unknown exception\n");
    throw;
  }
}

// Test DatabaseMetaData::getDatabaseProductName.
TEST_FUNCTION(4) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
    ensure("conn is NULL", wrapper.get() != NULL);

    sql::Connection *connection = wrapper.get();
    sql::DatabaseMetaData *meta(connection->getMetaData());
    ensure("meta is NULL", meta != NULL);

    meta->getDatabaseProductName();
  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
  }
}

// Test DatabaseMetaData::getDatabaseProductVersion.
TEST_FUNCTION(5) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
    ensure("conn is NULL", wrapper.get() != NULL);

    sql::Connection *connection = wrapper.get();
    sql::DatabaseMetaData *meta(connection->getMetaData());
    ensure("meta is NULL", meta != NULL);

    meta->getDatabaseProductVersion();
  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
    throw;
  }
}

// Test DatabaseMetaData::getDriverMajorVersion.
TEST_FUNCTION(7) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
    ensure("conn is NULL", wrapper.get() != NULL);

    sql::Connection *connection = wrapper.get();
    sql::DatabaseMetaData *meta(connection->getMetaData());
    ensure("meta is NULL", meta != NULL);

    meta->getDriverMajorVersion();
  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
    throw;
  }
}

// Test DatabaseMetaData::getDriverMinorVersion.
TEST_FUNCTION(8) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
    ensure("conn is NULL", wrapper.get() != NULL);

    sql::Connection *connection = wrapper.get();
    sql::DatabaseMetaData *meta(connection->getMetaData());
    ensure("meta is NULL", meta != NULL);

    meta->getDriverMinorVersion();
  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
    throw;
  }
}

// Test DatabaseMetaData::getDriverName.
TEST_FUNCTION(9) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
    ensure("conn is NULL", wrapper.get() != NULL);

    sql::Connection *connection = wrapper.get();
    sql::DatabaseMetaData *meta(connection->getMetaData());
    ensure("meta is NULL", meta != NULL);

    meta->getDriverName();
  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
    throw;
  }
}

END_TESTS
