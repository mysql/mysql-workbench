/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _WIN32
#include <sstream>
#endif

#include "sqlide/recordset_cdbc_storage.h"
#include "sqlide/recordset_be.h"
#include "connection_helpers.h"
#include "cppdbc.h"
#include "wb_helpers.h"

BEGIN_TEST_DATA_CLASS(recordset)
public:
WBTester *wbt;
sql::Dbc_connection_handler::Ref dbc_conn;
TEST_DATA_CONSTRUCTOR(recordset) {
  wbt = new WBTester;
}
END_TEST_DATA_CLASS

TEST_MODULE(recordset, "Recordset");

static void dummy() {
}

TEST_FUNCTION(1) {
  populate_grt(*wbt);

  sql::DriverManager *dbc_drv_man = sql::DriverManager::getDriverManager();
  sql::Authentication::Ref auth;

  dbc_conn = sql::Dbc_connection_handler::Ref(new sql::Dbc_connection_handler());
  dbc_conn->ref = dbc_drv_man->getConnection(wbt->get_connection_properties(), std::bind(dummy));

  ensure("connection", dbc_conn->ref.get() != 0);
}

TEST_FUNCTION(2) {
  Recordset_cdbc_storage::Ref data_storage(Recordset_cdbc_storage::create());

  //  data_storage->dbms_conn(dbc_conn);
  base::RecMutex _connLock;
  data_storage->setUserConnectionGetter(
    [&](sql::Dbc_connection_handler::Ref &conn, bool LockOnly = false) -> base::RecMutexLock {
      base::RecMutexLock lock(_connLock, false);
      conn = dbc_conn;
      return lock;
    });

  Recordset::Ref rs = Recordset::create();
  rs->data_storage(data_storage);

  std::shared_ptr<sql::Statement> dbc_statement(dbc_conn->ref->createStatement());
  dbc_statement->execute("select convert('', binary), convert(NULL, binary)");

  std::shared_ptr<sql::ResultSet> rset(dbc_statement->getResultSet());
  data_storage->dbc_resultset(rset);

  rs->reset(true);

  ensure("empty blob string is not NULL", !rs->is_field_null(0, 0));
  ensure("NULL blob is NULL", rs->is_field_null(0, 1));
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  delete wbt;
}

END_TESTS
