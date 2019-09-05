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

#include "sqlide/recordset_cdbc_storage.h"
#include "sqlide/recordset_be.h"
#include "cppdbc.h"

#include "casmine.h"
#include "wb_test_helpers.h"
#include "wb_connection_helpers.h"

namespace {

$ModuleEnvironment() {};

$TestData {
  std::unique_ptr<WorkbenchTester> tester;
  sql::Dbc_connection_handler::Ref connection;
};

static void dummy() {
}

$describe("Recordset") {

  $beforeAll([this]() {
    data->tester.reset(new WorkbenchTester());
    data->tester->initializeRuntime();

    sql::DriverManager *manager = sql::DriverManager::getDriverManager();
    sql::Authentication::Ref auth;

    data->connection = sql::Dbc_connection_handler::Ref(new sql::Dbc_connection_handler());

    db_mgmt_ConnectionRef connectionProperties(grt::Initialized);
    setupConnectionEnvironment(connectionProperties);
    data->connection->ref = manager->getConnection(connectionProperties, std::bind(dummy));

    $expect(data->connection->ref.get()).Not.toBeNull("connection");
  });

  $it("Recordset storage", [this]() {
    Recordset_cdbc_storage::Ref data_storage(Recordset_cdbc_storage::create());

    base::RecMutex _connLock;
    data_storage->setUserConnectionGetter(
      [&](sql::Dbc_connection_handler::Ref &conn, bool LockOnly = false) -> base::RecMutexLock {
        base::RecMutexLock lock(_connLock, false);
        conn = data->connection;
        return lock;
      }
    );

    Recordset::Ref rs = Recordset::create();
    rs->data_storage(data_storage);

    std::shared_ptr<sql::Statement> dbc_statement(data->connection->ref->createStatement());
    dbc_statement->execute("select convert('', binary), convert(NULL, binary)");

    std::shared_ptr<sql::ResultSet> rset(dbc_statement->getResultSet());
    data_storage->dbc_resultset(rset);

    rs->reset(true);

    $expect(rs->is_field_null(0, 0)).toBeFalse("empty blob string is not NULL");
    $expect(rs->is_field_null(0, 1)).toBeTrue("NULL blob is NULL");
  });

}

}
