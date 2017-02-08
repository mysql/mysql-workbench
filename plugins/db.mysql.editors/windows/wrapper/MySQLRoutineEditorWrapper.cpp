/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "MySQLRoutineEditorWrapper.h"

using namespace MySQL::Grt::Db;

//--------------------------------------------------------------------------------------------------

MySQLRoutineEditorWrapper::MySQLRoutineEditorWrapper(MySQL::Grt::GrtValue ^ arglist)
  : RoutineEditorWrapper(new ::MySQLRoutineEditorBE(
      db_mysql_RoutineRef::cast_from(grt::BaseListRef::cast_from(arglist->get_unmanaged_object()).get(0)))) {
}

//--------------------------------------------------------------------------------------------------

MySQLRoutineEditorWrapper::~MySQLRoutineEditorWrapper() {
  delete inner; // We created it.
}

//--------------------------------------------------------------------------------------------------

MySQLRoutineEditorBE *MySQLRoutineEditorWrapper::get_unmanaged_object() {
  return static_cast<::MySQLRoutineEditorBE *>(inner);
}

//--------------------------------------------------------------------------------------------------

void MySQL::Grt::Db::MySQLRoutineEditorWrapper::load_routine_sql() {
  get_unmanaged_object()->load_routine_sql();
}

//--------------------------------------------------------------------------------------------------

void MySQL::Grt::Db::MySQLRoutineEditorWrapper::commit_changes() {
  get_unmanaged_object()->commit_changes();
}

//--------------------------------------------------------------------------------------------------
