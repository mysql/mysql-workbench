/* 
 * Copyright (c) 2009, 2013, Oracle and/or its affiliates. All rights reserved.
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

#include "stdafx.h"

#include "db.mysql.sqlide.h"
#include "mysql_sql_editor_be.h"

using namespace bec;
using namespace grt;

GRT_MODULE_ENTRY_POINT(MysqlSqlImpl);

Sql_editor::Ref MysqlSqlImpl::getSqlEditor(db_mgmt_RdbmsRef rdbms, GrtVersionRef version)
{
  return Sql_editor::Ref(new Mysql_sql_editor(rdbms, version));
}
