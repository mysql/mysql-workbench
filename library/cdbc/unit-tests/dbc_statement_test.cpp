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
#include "grtsqlparser/sql_facade.h"

#define DATABASE_TO_USE "USE test"

static bool populate_test_table(std::auto_ptr<sql::Statement> &stmt) {
  stmt->execute(DATABASE_TO_USE);
  stmt->execute("DROP TABLE IF EXISTS test_function");
  if (stmt->execute("CREATE TABLE test_function (a integer, b integer, c integer default null)"))
    return false;

  if (stmt->execute("INSERT INTO test_function (a,b,c) VALUES(1, 111, NULL)")) {
    stmt->execute("DROP TABLE test_function");
    return false;
  }
  return true;
}

BEGIN_TEST_DATA_CLASS(module_dbc_statement_test)
public:
WBTester *_tester;
SqlFacade::Ref sql_splitter;

TEST_DATA_CONSTRUCTOR(module_dbc_statement_test) {
  _tester = new WBTester();
  sql_splitter = SqlFacade::instance_for_rdbms_name("Mysql");
  ensure("failed to get sqlparser module", (NULL != sql_splitter));

  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  sql::DriverManager *dm = sql::DriverManager::getDriverManager();
  ensure("dm is NULL", dm != NULL);

  sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
  ensure("conn is NULL", wrapper.get() != NULL);

  sql::Connection *connection = wrapper.get();
  std::auto_ptr<sql::Statement> stmt(connection->createStatement());
  stmt->execute("DROP SCHEMA IF EXISTS test; CREATE SCHEMA test");
}

END_TEST_DATA_CLASS

TEST_MODULE(module_dbc_statement_test, "DBC: statement tests");

// Test construction of a statement object.
TEST_FUNCTION(2) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
    ensure("conn is NULL", wrapper.get() != NULL);

    sql::Connection *connection = wrapper.get();
    {
      /* Going out scope will free the object. We test this as there is no close() method */
      std::auto_ptr<sql::Statement> stmt(connection->createStatement());
    }
  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
    throw;
  }
}

// Test simple update statement against statement object.
TEST_FUNCTION(3) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
    ensure("conn is NULL", wrapper.get() != NULL);

    sql::Connection *connection = wrapper.get();

    std::auto_ptr<sql::Statement> stmt(connection->createStatement());
    ensure("stmt is NULL", stmt.get() != NULL);

    ensure("Data not populated", populate_test_table(stmt));
    if (stmt->execute("UPDATE test_function SET a = 2, b = 222 where b = 111"))
      ensure("True returned for UPDATE", false);

    stmt->execute("DROP TABLE test_function");
  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
    throw;
  } catch (...) {
    printf("ERR: Caught unknown exception\n");
    throw;
  }
}

// Test simple query against statement object.
TEST_FUNCTION(4) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
    ensure("conn is NULL", wrapper.get() != NULL);

    sql::Connection *connection = wrapper.get();

    std::auto_ptr<sql::Statement> stmt(connection->createStatement());
    ensure("stmt is NULL", stmt.get() != NULL);

    ensure("Data not populated", populate_test_table(stmt));
    if (false == stmt->execute("SELECT * FROM test_function"))
      ensure("False returned for SELECT", false);

    /* Clean */
    sql::ConnectionWrapper wrapper2 = dm->getConnection(connectionProperties);
    ensure("conn2 is NULL", wrapper2.get() != NULL);
    sql::Connection *connection2 = wrapper2.get();
    std::auto_ptr<sql::Statement> stmt2(connection2->createStatement());
    ensure("stmt is NULL", stmt2.get() != NULL);
    stmt2->execute(DATABASE_TO_USE);
    stmt2->execute("DROP TABLE test_function");
  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
    throw;
  } catch (...) {
    printf("ERR: Caught unknown exception\n");
    throw;
  }
}

// Test executeQuery() - returning a result set.
TEST_FUNCTION(5) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
    ensure("conn is NULL", wrapper.get() != NULL);

    sql::Connection *connection = wrapper.get();

    std::auto_ptr<sql::Statement> stmt(connection->createStatement());
    ensure("stmt is NULL", stmt.get() != NULL);

    ensure("Data not populated", populate_test_table(stmt));
    /* Get a result set */
    try {
      std::auto_ptr<sql::ResultSet> rset(stmt->executeQuery("SELECT * FROM test_function"));
      ensure("NULL returned for result set", rset.get() != NULL);
    } catch (sql::SQLException &) {
      printf("ERR: sql::SQLException caught\n");
      throw;
    }

    /* Clean */
    sql::ConnectionWrapper wrapper2 = dm->getConnection(connectionProperties);
    ensure("wrapper2 is NULL", wrapper2.get() != NULL);
    sql::Connection *connection2 = wrapper2.get();
    std::auto_ptr<sql::Statement> stmt2(connection2->createStatement());
    ensure("stmt is NULL", stmt2.get() != NULL);
    stmt2->execute(DATABASE_TO_USE);
    stmt2->execute("DROP TABLE test_function");
  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
    throw;
  } catch (...) {
    printf("ERR: Caught unknown exception\n");
    throw;
  }
}

// Test executeQuery() - returning empty result set.
TEST_FUNCTION(6) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
    ensure("conn is NULL", wrapper.get() != NULL);

    sql::Connection *connection = wrapper.get();

    std::auto_ptr<sql::Statement> stmt(connection->createStatement());
    ensure("stmt is NULL", stmt.get() != NULL);

    ensure("Data not populated", populate_test_table(stmt));
    /* Get a result set */
    try {
      std::auto_ptr<sql::ResultSet> rset(stmt->executeQuery("SELECT * FROM test_function WHERE 1=2"));
      ensure("NULL returned for result set", rset.get() != NULL);
      ensure("Non-empty result set", false == rset->next());

    } catch (sql::SQLException &) {
      printf("ERR: Caught sql::SQLException\n");
      throw;
    }

    /* Clean */
    sql::ConnectionWrapper wrapper2 = dm->getConnection(connectionProperties);
    ensure("wrapper2 is NULL", wrapper2.get() != NULL);
    sql::Connection *connection2 = wrapper2.get();
    std::auto_ptr<sql::Statement> stmt2(connection2->createStatement());
    ensure("stmt is NULL", stmt2.get() != NULL);
    stmt2->execute(DATABASE_TO_USE);
    stmt2->execute("DROP TABLE test_function");
  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
    throw;
  } catch (...) {
    printf("ERR: Caught unknown exception\n");
    throw;
  }
}

// Test executeQuery() - use it for inserting, should generate an exception.
TEST_FUNCTION(7) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
    ensure("conn is NULL", wrapper.get() != NULL);

    sql::Connection *connection = wrapper.get();

    std::auto_ptr<sql::Statement> stmt(connection->createStatement());
    ensure("stmt is NULL", stmt.get() != NULL);

    ensure("Data not populated", populate_test_table(stmt));
    /* Get a result set */
    try {
      std::auto_ptr<sql::ResultSet> rset(stmt->executeQuery("INSERT INTO test_function VALUES(2,200)"));
      ensure("NULL returned for result set", rset.get() == NULL);
      ensure("Non-empty result set", false == rset->next());
    } catch (sql::SQLException &) {
    } catch (...) {
      printf("ERR: Incorrectly sql::SQLException ist not thrown\n");
      throw;
    }
    /* Clean */
    sql::ConnectionWrapper wrapper2 = dm->getConnection(connectionProperties);
    ensure("wrapper2 is NULL", wrapper2.get() != NULL);
    sql::Connection *connection2 = wrapper2.get();
    std::auto_ptr<sql::Statement> stmt2(connection2->createStatement());
    ensure("stmt is NULL", stmt2.get() != NULL);
    stmt2->execute(DATABASE_TO_USE);
    stmt2->execute("DROP TABLE test_function");
  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
    throw;
  } catch (...) {
    printf("ERR: Caught unknown exception\n");
    throw;
  }
}

// Test executeUpdate() - check the returned value.
TEST_FUNCTION(8) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
    ensure("conn is NULL", wrapper.get() != NULL);

    sql::Connection *connection = wrapper.get();

    std::auto_ptr<sql::Statement> stmt(connection->createStatement());
    ensure("stmt is NULL", stmt.get() != NULL);

    ensure("Data not populated", populate_test_table(stmt));
    /* Get a result set */
    try {
      ensure_equals("Number of updated rows", stmt->executeUpdate("UPDATE test_function SET a = 123"), 1);
    } catch (sql::SQLException &) {
      printf("ERR: Caught sql::SQLException\n");
      throw;
    }

    /* Clean */
    sql::ConnectionWrapper wrapper2 = dm->getConnection(connectionProperties);
    ensure("wrapper2 is NULL", wrapper2.get() != NULL);
    sql::Connection *connection2 = wrapper2.get();
    std::auto_ptr<sql::Statement> stmt2(connection2->createStatement());
    ensure("stmt is NULL", stmt2.get() != NULL);
    stmt2->execute(DATABASE_TO_USE);
    stmt2->execute("DROP TABLE test_function");
  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
    throw;
  } catch (...) {
    printf("ERR: Caught unknown exception\n");
    throw;
  }
}

// Test executeUpdate() - execute a SELECT, should get an exception
TEST_FUNCTION(9) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
    ensure("conn is NULL", wrapper.get() != NULL);

    sql::Connection *connection = wrapper.get();

    std::auto_ptr<sql::Statement> stmt(connection->createStatement());
    ensure("stmt is NULL", stmt.get() != NULL);

    ensure("Data not populated", populate_test_table(stmt));
    /* Get a result set */
    try {
      stmt->executeUpdate("SELECT * FROM test_function");
      // TODO: executing a query which returns a result set should throw an exception.
      // fail("No exception thrown");
    } catch (sql::SQLException &) {
    }

    /* Clean */
    sql::ConnectionWrapper wrapper2 = dm->getConnection(connectionProperties);
    ensure("wrapper2 is NULL", wrapper2.get() != NULL);
    sql::Connection *connection2 = wrapper2.get();
    std::auto_ptr<sql::Statement> stmt2(connection2->createStatement());
    ensure("stmt is NULL", stmt2.get() != NULL);
    stmt2->execute(DATABASE_TO_USE);
    stmt2->execute("DROP TABLE test_function");
  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
    throw;
  }
}

// Test getFetchSize() - should return int value.
TEST_FUNCTION(10) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
    ensure("conn is NULL", wrapper.get() != NULL);

    sql::Connection *connection = wrapper.get();

    std::auto_ptr<sql::Statement> stmt(connection->createStatement());
    ensure("stmt is NULL", stmt.get() != NULL);

    // TODO: implement and test getFetchSize() and getFechtDirection()
    // ensure("fetchSize not > 0", stmt->getFetchSize() > 0);

  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
    throw;
  }
}

// We don't test Statement::getMaxRows() as such doesn't exist.
// We don't test Statement::getMoreResults() as such doesn't exist.
// We don't test Statement::getQueryTimeout() as such doesn't exist.

// Test getResultSet() - execute() a query and get the result set.
TEST_FUNCTION(11) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
    ensure("conn is NULL", wrapper.get() != NULL);

    sql::Connection *connection = wrapper.get();

    std::auto_ptr<sql::Statement> stmt(connection->createStatement());
    ensure("stmt is NULL", stmt.get() != NULL);

    ensure("Data not populated", populate_test_table(stmt));
    ensure("Statement::execute returned false", stmt->execute("SELECT * FROM test_function"));

    std::auto_ptr<sql::ResultSet> rset(stmt->getResultSet());
    ensure("rset is NULL", rset.get() != NULL);

    /* Clean */
    sql::ConnectionWrapper wrapper2 = dm->getConnection(connectionProperties);
    ensure("wrapper2 is NULL", wrapper2.get() != NULL);
    sql::Connection *connection2 = wrapper2.get();
    std::auto_ptr<sql::Statement> stmt2(connection2->createStatement());
    ensure("stmt is NULL", stmt2.get() != NULL);
    stmt2->execute(DATABASE_TO_USE);
    stmt2->execute("DROP TABLE test_function");
  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
    throw;
  }
}

// Test getResultSet() - execute() an update query and get the result set - should be empty.
// TODO: Doesn't test much as stmt::getResultSet() is not implemented.
TEST_FUNCTION(12) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
    ensure("conn is NULL", wrapper.get() != NULL);

    sql::Connection *connection = wrapper.get();

    std::auto_ptr<sql::Statement> stmt(connection->createStatement());
    ensure("stmt is NULL", stmt.get() != NULL);

    ensure("Data not populated", populate_test_table(stmt));

    ensure("Statement::execute returned true", false == stmt->execute("UPDATE test_function SET a = 222"));

    try {
      std::auto_ptr<sql::ResultSet> rset(stmt->getResultSet());
      if (NULL == rset.get())
        throw sql::SQLException();
      fail("Got result set for an update operation.");
    } catch (sql::SQLException &) {
    }

    /* Clean */
    sql::ConnectionWrapper wrapper2 = dm->getConnection(connectionProperties);
    ensure("wrapper2 is NULL", wrapper2.get() != NULL);
    sql::Connection *connection2 = wrapper2.get();
    std::auto_ptr<sql::Statement> stmt2(connection2->createStatement());
    ensure("stmt is NULL", stmt2.get() != NULL);
    stmt2->execute(DATABASE_TO_USE);
    stmt2->execute("DROP TABLE test_function");
  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
    throw;
  }
}

// We don't test Statement::getResultSetConcurrency() as such doesn't exist.
// We don't test Statement::getResultSetType() as such doesn't exist.
// We don't test Statement::getUpdateCount() as such doesn't exist.
// We don't test Statement::getWarnings() as such doesn't exist.

// Test setFetchSize() - set and get the value.
// TODO: Doesn't pass because setFetchSize() is unimplemented.
TEST_FUNCTION(13) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
    ensure("conn is NULL", wrapper.get() != NULL);

    sql::Connection *connection = wrapper.get();

    std::auto_ptr<sql::Statement> stmt(connection->createStatement());
    ensure("stmt is NULL", stmt.get() != NULL);
    /*
        int setFetchSize = 50;

        stmt->setFetchSize(setFetchSize);

        ensure_equals("Non-equal", setFetchSize, stmt->getFetchSize());
    */
  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
    throw;
  }
}

// Test setFetchSize() - set negative value and expect an exception.
// TODO: Doesn't pass because setFetchSize() is unimplemented.
TEST_FUNCTION(14) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
    ensure("conn is NULL", wrapper.get() != NULL);

    sql::Connection *connection = wrapper.get();

    std::auto_ptr<sql::Statement> stmt(connection->createStatement());
    ensure("stmt is NULL", stmt.get() != NULL);
    /*
        try {
          stmt->setFetchSize(-1);
          ensure("No exception", false);
        } catch (sql::InvalidArgumentException) {
          printf("INFO: Caught sql::InvalidArgumentException\n");
        }
    */
  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
    throw;
  }
}

// Test setQueryTimeout() - set negative value and expect an exception.
// TODO: Doesn't pass because setQueryTimeout() is unimplemented.
TEST_FUNCTION(15) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
    ensure("conn is NULL", wrapper.get() != NULL);

    sql::Connection *connection = wrapper.get();

    std::auto_ptr<sql::Statement> stmt(connection->createStatement());
    ensure("stmt is NULL", stmt.get() != NULL);
    /*
        try {
          stmt->setQueryTimeout(-1);
          printf("ERR: No exception\n");
        } catch (sql::InvalidArgumentException &e) {
          delete e;
        }
    */
  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
    throw;
  }
}

// Test addBatch()/executeBatch() (includes a test against the 'out of sync' error).
TEST_FUNCTION(16) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  try {
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    ensure("dm is NULL", dm != NULL);

    sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
    ensure("conn is NULL", wrapper.get() != NULL);

    sql::Connection *connection = wrapper.get();

    std::auto_ptr<sql::Statement> stmt(connection->createStatement());
    ensure("stmt is NULL", stmt.get() != NULL);

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

    ensure("failed to get sqlparser module", (NULL != sql_splitter));

    sql_splitter->splitSqlScript(sql_script, statements);
    sql::SqlBatchExec()(stmt.get(), statements);

    //  Cleanup
    sql_script = "DROP DATABASE IF EXISTS dbc_statement_test_15;";
    sql_splitter->splitSqlScript(sql_script, statements);
    sql::SqlBatchExec()(stmt.get(), statements);
  } catch (sql::SQLException &) {
    printf("ERR: Caught sql::SQLException\n");
    throw;
  }
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);
  sql::DriverManager *dm = sql::DriverManager::getDriverManager();
  ensure("dm is NULL", dm != NULL);

  sql::ConnectionWrapper wrapper = dm->getConnection(connectionProperties);
  ensure("conn is NULL", wrapper.get() != NULL);

  sql::Connection *connection = wrapper.get();
  std::auto_ptr<sql::Statement> stmt(connection->createStatement());
  stmt->execute("DROP SCHEMA IF EXISTS test;");

  delete _tester;
}

END_TESTS
