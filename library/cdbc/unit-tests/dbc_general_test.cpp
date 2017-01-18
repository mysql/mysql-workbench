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

#include "connection_helpers.h"

// XXX these tests are useless without calls to ensure

using namespace std;

BEGIN_TEST_DATA_CLASS(module_dbc_general_test)
public:
TEST_DATA_CONSTRUCTOR(module_dbc_general_test) {
  // load structs
  grt::GRT::get()->scan_metaclasses_in("../../res/grt/");
  grt::GRT::get()->end_loading_metaclasses();

  ensure_equals("load structs", grt::GRT::get()->get_metaclasses().size(), (size_t)INT_METACLASS_COUNT);

  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  sql::DriverManager *dm = sql::DriverManager::getDriverManager();
  sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
  sql::Connection *connection = wrapper.get();

  std::auto_ptr<sql::Statement> stmt(connection->createStatement());
  stmt->execute("DROP SCHEMA IF EXISTS test");
  stmt->execute("CREATE SCHEMA test");
}

END_TEST_DATA_CLASS

TEST_MODULE(module_dbc_general_test, "DBC: general tests");

TEST_FUNCTION(2) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  sql::DriverManager *dm = sql::DriverManager::getDriverManager();
  sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
  sql::Connection *connection = wrapper.get();
  sql::DatabaseMetaData *meta(connection->getMetaData());
  std::auto_ptr<sql::ResultSet> rset(meta->getSchemata());

  while (rset->next()) {
    if (getenv("VERBOSE")) {
      std::cout << rset->getString("Database") << std::endl;
      std::cout << "  Schema Objects:" << std::endl;
    }

    std::auto_ptr<sql::ResultSet> rset2(meta->getSchemaObjects("", rset->getString("database")));
    while (rset2->next()) {
      if (getenv("VERBOSE"))
        std::cout << rset2->getString("object_type") << ": " << rset2->getString("name") << ","
                  << rset2->getString("ddl") << std::endl;
    }
  }
}

TEST_FUNCTION(3) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  sql::DriverManager *dm = sql::DriverManager::getDriverManager();
  sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
  sql::Connection *connection = wrapper.get();

  std::auto_ptr<sql::Statement> stmt(connection->createStatement());

  stmt->execute("DROP TABLE IF EXISTS test.product");
  stmt->execute("CREATE TABLE test.product(idproduct INT NOT NULL AUTO_INCREMENT PRIMARY KEY, name VARCHAR(80))");

  connection->setAutoCommit(0);

  if (getenv("VERBOSE"))
    std::cout << "Insert Data." << std::endl;

  std::auto_ptr<sql::PreparedStatement> prepStmt(
    connection->prepareStatement("INSERT INTO test.product(idproduct, name)  VALUES(?, ?)"));
  prepStmt->setInt(1, 1);
  prepStmt->setString(2, "Harry Potter");
  prepStmt->executeUpdate();

  if (getenv("VERBOSE"))
    std::cout << "Display Data." << std::endl;

  std::auto_ptr<sql::ResultSet> rset1(stmt->executeQuery("SELECT * FROM test.product"));

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

  std::auto_ptr<sql::ResultSet> rset2(stmt->executeQuery("SELECT * FROM test.product"));

  i = 0;
  while (rset2->next()) {
    if (getenv("VERBOSE"))
      std::cout << rset2->getString(2) << ", " << rset2->getString("name") << std::endl;
    i++;
  }
  if (getenv("VERBOSE"))
    std::cout << i << " row(s)" << std::endl;
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(4) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  sql::DriverManager *dm = sql::DriverManager::getDriverManager();
  sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
  sql::Connection *connection = wrapper.get();

  std::auto_ptr<sql::Statement> stmt(connection->createStatement());
  stmt->execute("DROP SCHEMA IF EXISTS test");
}

END_TESTS
