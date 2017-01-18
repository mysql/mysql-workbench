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

static bool populate_test_table(std::auto_ptr<sql::Statement> &stmt) {
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

static bool populate_tx_test_table(std::auto_ptr<sql::Statement> &stmt) {
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

BEGIN_TEST_DATA_CLASS(module_dbc_result_set_test)
public:
sql::Connection *_connection;
TEST_DATA_CONSTRUCTOR(module_dbc_result_set_test) {
  // load structs
  grt::GRT::get()->scan_metaclasses_in("../../res/grt/");
  grt::GRT::get()->end_loading_metaclasses();

  ensure_equals("load structs", grt::GRT::get()->get_metaclasses().size(), (size_t)INT_METACLASS_COUNT);
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);
  sql::DriverManager *dm = sql::DriverManager::getDriverManager();
  ensure("dm is NULL", dm != NULL);

  sql::ConnectionWrapper wrapper1 = dm->getConnection(connectionProperties);
  ensure("wrapper1 is NULL", wrapper1.get() != NULL);

  sql::Connection *connection = wrapper1.get();
  std::auto_ptr<sql::Statement> stmt(connection->createStatement());
  ensure("stmt1 is NULL", stmt.get() != NULL);

  ensure_equals("Connection differs", wrapper1.get(), stmt->getConnection());

  stmt->execute("DROP SCHEMA IF EXISTS test; CREATE SCHEMA test");
}

END_TEST_DATA_CLASS

TEST_MODULE(module_dbc_result_set_test, "DBC: PS tests");

// Test preparation.
TEST_FUNCTION(2) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper1 = dm->getConnection(connectionProperties);
    ensure("wrapper1 is NULL", wrapper1.get() != NULL);

    sql::Connection *connection = wrapper1.get();
    std::auto_ptr<sql::Statement> stmt1(connection->createStatement());
    ensure("stmt1 is NULL", stmt1.get() != NULL);

    ensure_equals("Connection differs", wrapper1.get(), stmt1->getConnection());

    ensure("Data not populated", populate_tx_test_table(stmt1));

    std::auto_ptr<sql::PreparedStatement> ps1(wrapper1->prepareStatement("SELECT a, b, c FROM test_function_tx"));
    ensure("ps1 is NULL", ps1.get() != NULL);

    // TODO: getConnection is not yet implemented.
    // ensure_equals("Connection differs", wrapper1.get(), ps1->getConnection());

    std::auto_ptr<sql::ResultSet> rset(ps1->executeQuery());
    ensure("NULL returned for result set", rset.get() != NULL);
    while (rset->next())
      ;

    stmt1->execute("DROP TABLE test_function_tx");
  } catch (sql::SQLException &e) {
    printf("ERR: Caught sql::SQLException %i : %s\n", e.getErrorCode(), e.what());
    throw;
  }
}

// Test executing executeQuery on the same statement.
TEST_FUNCTION(3) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper1 = dm->getConnection(connectionProperties);
    ensure("wrapper1 is NULL", wrapper1.get() != NULL);

    sql::Connection *connection = wrapper1.get();
    std::auto_ptr<sql::Statement> stmt1(connection->createStatement());
    ensure("stmt1 is NULL", stmt1.get() != NULL);

    std::auto_ptr<sql::ResultSet> rset1(stmt1->executeQuery("SELECT 1 FROM DUAL"));
    ensure("res1 is NULL", rset1.get() != NULL);

    // TODO: Fails currently because the statements are not store-d but use-d.
    // std::auto_ptr<sql::Statement> rset2(stmt1->executeQuery("SELECT 1 FROM DUAL"));
    // ensure("res2 is NULL", rset2.get() != NULL);

    ensure("res1 is empty", rset1->next() != false);
    // TODO: enable once the part above is fixed.
    // ensure("res2 is empty", rset2->next() != false);
  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
    throw;
  }
}

// Test executing two different queries from the same statement.
TEST_FUNCTION(4) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper1 = dm->getConnection(connectionProperties);
    ensure("wrapper1 is NULL", wrapper1.get() != NULL);

    sql::Connection *connection = wrapper1.get();
    std::auto_ptr<sql::Statement> stmt1(connection->createStatement());
    ensure("stmt1 is NULL", stmt1.get() != NULL);

    ensure("Data not populated", true == populate_test_table(stmt1));

    std::auto_ptr<sql::ResultSet> rset1(stmt1->executeQuery("SELECT 1 FROM DUAL"));
    ensure("res1 is NULL", rset1.get() != NULL);
    ensure("res1 is empty", rset1->next() != false);
    ensure("res1 is empty", rset1->next() == false);

    ensure("No rows updated", stmt1->executeUpdate("UPDATE test_function SET a = 2") > 0);

    stmt1->execute("DROP TABLE test_function");
  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
    throw;
  }
}

// Test commit and rollback (autocommit off).
// TODO: Fails because getAutoCommit is buggy.
TEST_FUNCTION(5) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper1 = dm->getConnection(connectionProperties);
    ensure("wrapper1 is NULL", wrapper1.get() != NULL);

    sql::Connection *connection = wrapper1.get();
    std::auto_ptr<sql::Statement> stmt1(connection->createStatement());
    ensure("stmt1 is NULL", stmt1.get() != NULL);

    ensure_equals("Connection differs", wrapper1.get(), stmt1->getConnection());

    bool old_commit_mode = wrapper1->getAutoCommit();
    wrapper1->setAutoCommit(0);

    ensure("Data not populated", true == populate_tx_test_table(stmt1));

    std::auto_ptr<sql::ResultSet> rset1(stmt1->executeQuery("SELECT COUNT(*) FROM test_function_tx"));
    ensure("res1 is NULL", rset1.get() != NULL);
    ensure("res1 is empty", rset1->next() != false);
    int count_full_before = rset1->getInt(1);
    ensure("res1 has more rows ", rset1->next() == false);

    /* Let's delete and then rollback */
    ensure_equals("Deleted less rows", stmt1->executeUpdate("DELETE FROM test_function_tx WHERE 1"), count_full_before);

    std::auto_ptr<sql::ResultSet> rset2(stmt1->executeQuery("SELECT COUNT(*) FROM test_function_tx"));
    ensure("res2 is NULL", rset2.get() != NULL);
    ensure("res2 is empty", rset2->next() != false);
    ensure("Table not empty after delete", rset2->getInt(1) == 0);
    ensure("res2 has more rows ", rset2->next() == false);

    stmt1->getConnection()->rollback();

    std::auto_ptr<sql::ResultSet> rset3(stmt1->executeQuery("SELECT COUNT(*) FROM test_function_tx"));
    ensure("res3 is NULL", rset3.get() != NULL);
    ensure("res3 is empty", rset3->next() != false);
    int count_full_after = rset3->getInt(1);
    ensure("res3 has more rows ", rset3->next() == false);

    ensure("Rollback didn't work", count_full_before == count_full_after);

    /* Now let's delete and then commit */
    ensure_equals("Deleted less rows", stmt1->executeUpdate("DELETE FROM test_function_tx WHERE 1"), count_full_before);
    stmt1->getConnection()->commit();

    std::auto_ptr<sql::ResultSet> rset4(stmt1->executeQuery("SELECT COUNT(*) FROM test_function_tx"));
    ensure("res4 is NULL", rset4.get() != NULL);
    ensure("res4 is empty", rset4->next() != false);
    ensure("Table not empty after delete", rset4->getInt(1) == 0);
    ensure("res4 has more rows ", rset4->next() == false);

    stmt1->execute("DROP TABLE test_function_tx");

    wrapper1->setAutoCommit(old_commit_mode);
  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
    throw;
  }
}

// Test commit and rollback (autocommit on).
TEST_FUNCTION(6) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper1 = dm->getConnection(connectionProperties);
    ensure("wrapper1 is NULL", wrapper1.get() != NULL);

    sql::Connection *connection = wrapper1.get();
    std::auto_ptr<sql::Statement> stmt1(connection->createStatement());
    ensure("stmt1 is NULL", stmt1.get() != NULL);

    ensure_equals("Connection differs", wrapper1.get(), stmt1->getConnection());

    bool old_commit_mode = wrapper1->getAutoCommit();
    wrapper1->setAutoCommit(1);
    ensure("Data not populated", true == populate_tx_test_table(stmt1));

    std::auto_ptr<sql::ResultSet> rset1(stmt1->executeQuery("SELECT COUNT(*) FROM test_function_tx"));
    ensure("res1 is NULL", rset1.get() != NULL);
    ensure("res1 is empty", rset1->next() != false);
    int count_full_before = rset1->getInt(1);
    ensure("res1 has more rows ", rset1->next() == false);

    /* Let's delete and then rollback */
    ensure_equals("Deleted less rows", stmt1->executeUpdate("DELETE FROM test_function_tx WHERE 1"), count_full_before);

    std::auto_ptr<sql::ResultSet> rset2(stmt1->executeQuery("SELECT COUNT(*) FROM test_function_tx"));
    ensure("res2 is NULL", rset2.get() != NULL);
    ensure("res2 is empty", rset2->next() != false);
    ensure("Table not empty after delete", rset2->getInt(1) == 0);
    ensure("res2 has more rows ", rset2->next() == false);

    // In autocommit on, this is a no-op.
    stmt1->getConnection()->rollback();

    std::auto_ptr<sql::ResultSet> rset3(stmt1->executeQuery("SELECT COUNT(*) FROM test_function_tx"));
    ensure("res3 is NULL", rset3.get() != NULL);
    ensure("res3 is empty", rset3->next() != false);
    ensure("Rollback didn't work", rset3->getInt(1) == 0);
    ensure("res3 has more rows ", rset3->next() == false);

    ensure("Data not populated", true == populate_tx_test_table(stmt1));

    /* Now let's delete and then commit */
    ensure_equals("Deleted less rows", stmt1->executeUpdate("DELETE FROM test_function_tx WHERE 1"), count_full_before);
    // In autocommit on, this is a no-op.
    stmt1->getConnection()->commit();

    std::auto_ptr<sql::ResultSet> rset4(stmt1->executeQuery("SELECT COUNT(*) FROM test_function_tx"));
    ensure("res4 is NULL", rset4.get() != NULL);
    ensure("res4 is empty", rset4->next() != false);
    ensure_equals("Table not empty after delete", 0, rset4->getInt(1));
    ensure("res4 has more rows ", rset4->next() == false);

    stmt1->execute("DROP TABLE test_function_tx");

    wrapper1->setAutoCommit(old_commit_mode);
  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
    throw;
  }
}

// Test multistatement off - send two queries in one call.
TEST_FUNCTION(7) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper1 = dm->getConnection(connectionProperties);
    ensure("wrapper1 is NULL", wrapper1.get() != NULL);

    sql::Connection *connection = wrapper1.get();
    std::auto_ptr<sql::Statement> stmt1(connection->createStatement());
    ensure("stmt1 is NULL", stmt1.get() != NULL);

    try {
      std::auto_ptr<sql::ResultSet> rset1(
        stmt1->executeQuery("SELECT COUNT(*) FROM test_function_tx; DELETE FROM test_function_tx"));
      ensure("ERR: Exception not thrown", false);
    } catch (sql::SQLException &) {
    }

  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
    throw;
  }
}

// Test out of bound extraction of data.
TEST_FUNCTION(8) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper1 = dm->getConnection(connectionProperties);
    ensure("wrapper1 is NULL", wrapper1.get() != NULL);

    sql::Connection *connection = wrapper1.get();
    std::auto_ptr<sql::Statement> stmt1(connection->createStatement());
    ensure("stmt1 is NULL", stmt1.get() != NULL);

    ensure_equals("Connection differs", wrapper1.get(), stmt1->getConnection());

    ensure("Data not populated", true == populate_tx_test_table(stmt1));

    std::auto_ptr<sql::ResultSet> rset1(
      stmt1->executeQuery("SELECT COUNT(*) AS 'count of rows' FROM test_function_tx"));
    ensure("res1 is NULL", rset1.get() != NULL);
    ensure("res1 is empty", rset1->next() != false);
    try {
      rset1->getInt(-123);
      ensure("ERR: No InvalidArgumentException thrown", false);
    } catch (sql::InvalidArgumentException &) {
    }
    try {
      rset1->getInt(123);
      ensure("ERR: No InvalidArgumentException thrown", false);
    } catch (sql::InvalidArgumentException &) {
    }
    try {
      rset1->getInt("no_such_column");
      ensure("ERR: No InvalidArgumentException thrown", false);
    } catch (sql::InvalidArgumentException &) {
    }
    try {
      rset1->getString(-123);
      ensure("ERR: No InvalidArgumentException thrown", false);
    } catch (sql::InvalidArgumentException &) {
    }
    try {
      rset1->getString(123);
      ensure("ERR: No InvalidArgumentException thrown", false);
    } catch (sql::InvalidArgumentException &) {
    }
    try {
      rset1->getString("no_such_column");
      ensure("ERR: No InvalidArgumentException thrown", false);
    } catch (sql::InvalidArgumentException &) {
    }
    try {
      rset1->getDouble(-123);
      ensure("ERR: No InvalidArgumentException thrown", false);
    } catch (sql::InvalidArgumentException &) {
    }
    try {
      rset1->getDouble(123);
      ensure("ERR: No InvalidArgumentException thrown", false);
    } catch (sql::InvalidArgumentException &) {
    }
    try {
      rset1->getDouble("no_such_column");
      ensure("ERR: No InvalidArgumentException thrown", false);
    } catch (sql::InvalidArgumentException &) {
    }
    try {
      rset1->getInt(rset1->getInt(1) + 1);
      ensure("ERR: No InvalidArgumentException thrown", false);
    } catch (sql::InvalidArgumentException &) {
    }
    try {
      rset1->isNull(-123);
      ensure("ERR: No InvalidArgumentException thrown", false);
    } catch (sql::InvalidArgumentException &) {
    }
    try {
      rset1->isNull(123);
      ensure("ERR: No InvalidArgumentException thrown", false);
    } catch (sql::InvalidArgumentException &) {
    }
    try {
      rset1->isNull("no_such_column");
      ensure("ERR: No InvalidArgumentException thrown", false);
    } catch (sql::InvalidArgumentException &) {
    }

    ensure("res1 has more rows ", rset1->getInt(1) == 1);
    ensure("res1 has more rows ", rset1->getInt("count of rows") == 1);

    ensure("res1 has more rows ", rset1->getDouble(1) - 1 < 0.1);
    ensure("res1 has more rows ", rset1->getDouble("count of rows") - 1 < 0.1);

    ensure("res1 has more rows ", rset1->getString(1) == std::string("1"));
    ensure("res1 has more rows ", rset1->getString("count of rows") == std::string("1"));

    ensure("c is not null", rset1->isNull(1) == false);

    ensure("res1 has more rows ", rset1->next() == false);
    stmt1->execute("DROP TABLE test_function_tx");
  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
    throw;
  }
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(9) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);
  setup_env(connectionProperties);
  sql::DriverManager *dm = sql::DriverManager::getDriverManager();

  sql::ConnectionWrapper wrapper1 = dm->getConnection(connectionProperties);
  sql::Connection *connection = wrapper1.get();
  std::auto_ptr<sql::Statement> stmt(connection->createStatement());
  stmt->execute("DROP SCHEMA IF EXISTS test;");
}

END_TESTS
