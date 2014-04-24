/* 
 * Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "grts/structs.workbench.physical.h"

#include "MySQLRoutineGroupEditorBE.h"

using namespace MySQL::Grt::Db;

//--------------------------------------------------------------------------------------------------

MySQL::Grt::Db::MySQLRoutineGroupEditorBE::MySQLRoutineGroupEditorBE(::MySQLRoutineGroupEditorBE *inn)
  : RoutineGroupEditorBE(inn)
{
}

//--------------------------------------------------------------------------------------------------

MySQL::Grt::Db::MySQLRoutineGroupEditorBE::MySQLRoutineGroupEditorBE(MySQL::Grt::GrtManager^ grtm, MySQL::Grt::GrtValue^ arglist)
  : RoutineGroupEditorBE
    (
      new ::MySQLRoutineGroupEditorBE(grtm->get_unmanaged_object(), 
        db_mysql_RoutineGroupRef::cast_from(grt::BaseListRef::cast_from(arglist->get_unmanaged_object()).get(0)), 
        get_rdbms_for_db_object(grt::BaseListRef::cast_from(arglist->get_unmanaged_object()).get(0))
      )
    )
{
}

//--------------------------------------------------------------------------------------------------

::MySQLRoutineGroupEditorBE *MySQL::Grt::Db::MySQLRoutineGroupEditorBE::get_unmanaged_object()
{
  return static_cast<::MySQLRoutineGroupEditorBE *>(inner);
}

//--------------------------------------------------------------------------------------------------

String^ MySQL::Grt::Db::MySQLRoutineGroupEditorBE::get_procedure_body()
{
  return CppStringToNative(get_unmanaged_object()->get_procedure_body());
}

//--------------------------------------------------------------------------------------------------

String^ MySQL::Grt::Db::MySQLRoutineGroupEditorBE::get_function_body()
{
  return CppStringToNative(get_unmanaged_object()->get_function_body());
}

//--------------------------------------------------------------------------------------------------

void MySQL::Grt::Db::MySQLRoutineGroupEditorBE::load_routines_sql()
{
  get_unmanaged_object()->load_routines_sql();
}

//--------------------------------------------------------------------------------------------------

void MySQL::Grt::Db::MySQLRoutineGroupEditorBE::commit_changes()
{
  get_unmanaged_object()->commit_changes();
}

//--------------------------------------------------------------------------------------------------

