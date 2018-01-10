/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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
