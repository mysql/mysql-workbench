/* 
 * Copyright (c) 2010, 2014, Oracle and/or its affiliates. All rights reserved.
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
	WBTester wbt;
    sql::Dbc_connection_handler::Ref dbc_conn; 
END_TEST_DATA_CLASS


TEST_MODULE(recordset, "Recordset");

static void dummy()
{
}

TEST_FUNCTION(1)
{
  sql::DriverManager *dbc_drv_man= sql::DriverManager::getDriverManager();
  sql::Authentication::Ref auth;

  dbc_conn = sql::Dbc_connection_handler::Ref(new sql::Dbc_connection_handler());
  dbc_conn->ref= dbc_drv_man->getConnection(wbt.get_connection_properties(), boost::bind(dummy));
  ensure("connection", dbc_conn->ref.get() != 0);
}


TEST_FUNCTION(2)
{
  Recordset_cdbc_storage::Ref data_storage(Recordset_cdbc_storage::create(wbt.wb->get_grt_manager()));
  data_storage->dbms_conn(dbc_conn);

  Recordset::Ref rs = Recordset::create(wbt.wb->get_grt_manager());
  rs->data_storage(data_storage);

  boost::shared_ptr<sql::Statement> dbc_statement(dbc_conn->ref->createStatement());
  dbc_statement->execute("select convert('', binary), convert(NULL, binary)");

  boost::shared_ptr<sql::ResultSet> rset(dbc_statement->getResultSet());
  data_storage->dbc_resultset(rset);
  data_storage->dbms_conn(dbc_conn); 

  rs->reset(true);

  ensure("empty blob string is not NULL", !rs->is_field_null(0, 0));
  ensure("NULL blob is NULL", rs->is_field_null(0, 1));

}


END_TESTS
