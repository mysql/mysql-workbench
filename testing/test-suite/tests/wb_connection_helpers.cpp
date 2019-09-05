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

#include "wb_connection_helpers.h"

#include "grt.h"
#include "grtpp_util.h"
#include "cppdbc.h"

#include "casmine.h"

//----------------------------------------------------------------------------------------------------------------------

void setupConnectionEnvironment(const db_mgmt_ConnectionRef &connectionProperties, db_mgmt_DriverRef driver) {
  auto context = casmine::CasmineContext::get();
  grt::DictRef conn_params(true);
  conn_params.set("hostName", grt::StringRef(context->getConfigurationStringValue("db/host")));
  conn_params.set("port", grt::IntegerRef(context->getConfigurationIntValue("db/port")));
  conn_params.set("userName", grt::StringRef(context->getConfigurationStringValue("db/user")));
  conn_params.set("password", grt::StringRef(context->getConfigurationStringValue("db/password")));
  grt::replace_contents(connectionProperties->parameterValues(), conn_params);

  if (driver.is_valid()) {
    connectionProperties->driver(driver);
  } else {
    db_mgmt_DriverRef driverProperties(grt::Initialized);
    driverProperties->driverLibraryName(grt::StringRef("mysqlcppconn"));
    connectionProperties->driver(driverProperties);
  }
  connectionProperties->name("Test_conn");
}

//----------------------------------------------------------------------------------------------------------------------

sql::ConnectionWrapper createConnectionForImport() {
  db_mgmt_ConnectionRef properties(grt::Initialized);
  setupConnectionEnvironment(properties);

  sql::DriverManager *dm = sql::DriverManager::getDriverManager();
  return dm->getConnection(properties);
}

//----------------------------------------------------------------------------------------------------------------------
