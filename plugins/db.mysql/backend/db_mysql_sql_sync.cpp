/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "grtdb/db_object_helpers.h"

#include "grts/structs.h"
#include "grts/structs.db.mgmt.h"
#include "grts/structs.workbench.h"
#include "grt.h"

using namespace grt;

#include "db_mysql_sql_sync.h"

#include "diff/diffchange.h"

DbMySQLSync::DbMySQLSync() : Db_plugin(), DbMySQLValidationPage() {
  DbMySQLSync::grtm(false);
  _catalog = db_mysql_CatalogRef::cast_from(grt::GRT::get()->get("/wb/doc/physicalModels/0/catalog"));
}

void DbMySQLSync::set_option(const std::string& name, const std::string& value) {
  if (name.compare("InputFileName") == 0)
    _input_filename = value;
  else if (name.compare("OutputFileName") == 0)
    _output_filename = value;
  else if (name.compare("ScriptToApply") == 0)
    _script_to_apply = value;
}

void DbMySQLSync::start_apply_script_to_db() {
  sql_script(_script_to_apply);
  Db_plugin::exec_task();
}
