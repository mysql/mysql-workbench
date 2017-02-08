/*
* Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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
