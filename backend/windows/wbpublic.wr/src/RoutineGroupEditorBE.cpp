/* 
 * Copyright (c) 2008, 2013, Oracle and/or its affiliates. All rights reserved.
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

#include "Grt.h"
#include "GrtTemplates.h"
#include "DelegateWrapper.h"
#include "GrtManager.h"

#include "DBObjectEditorWrapper.h"
#include "grts/structs.workbench.physical.h"

#include "RoutineGroupEditorBE.h"

using namespace MySQL::Grt::Db;

//--------------------------------------------------------------------------------------------------

RoutineGroupEditorBE::RoutineGroupEditorBE(::bec::RoutineGroupEditorBE *inn)
  : DBObjectEditorWrapper(inn)
{
}

//--------------------------------------------------------------------------------------------------

::bec::RoutineGroupEditorBE *RoutineGroupEditorBE::get_unmanaged_object()
{
  return static_cast<::bec::RoutineGroupEditorBE *>(inner);
}

//--------------------------------------------------------------------------------------------------

String^ RoutineGroupEditorBE::get_routines_sql()
{
  return CppStringToNative(get_unmanaged_object()->get_routines_sql());
}

//--------------------------------------------------------------------------------------------------

String^ RoutineGroupEditorBE::get_routine_sql(MySQL::Grt::GrtValue^ routine)
{
  return CppStringToNative(get_unmanaged_object()->get_routine_sql(db_RoutineRef::cast_from(routine->get_unmanaged_object())));
}

//--------------------------------------------------------------------------------------------------

void RoutineGroupEditorBE::set_routines_sql(String ^query, bool sync)
{
  get_unmanaged_object()->set_routines_sql(NativeToCppString(query), sync);
}

//--------------------------------------------------------------------------------------------------

List<String^>^ RoutineGroupEditorBE::get_routines_names()
{
  return CppStringListToNative(get_unmanaged_object()->get_routines_names());
}

//--------------------------------------------------------------------------------------------------

void RoutineGroupEditorBE::delete_routine_with_name(String^ name)
{
  get_unmanaged_object()->delete_routine_with_name(NativeToCppString(name));
}

//--------------------------------------------------------------------------------------------------

void RoutineGroupEditorBE::append_routine_with_id(String^ id)
{
  get_unmanaged_object()->append_routine_with_id(NativeToCppString(id));
}

//--------------------------------------------------------------------------------------------------

String^ RoutineGroupEditorBE::get_routine_name(String^ id)
{
  return CppStringToNative(get_unmanaged_object()->get_routine_name(NativeToCppString(id)));
}

//--------------------------------------------------------------------------------------------------

String^ RoutineGroupEditorBE::get_name()
{
  return CppStringToNative(get_unmanaged_object()->get_name());
}

//--------------------------------------------------------------------------------------------------

void RoutineGroupEditorBE::set_name(String ^query)
{
  get_unmanaged_object()->set_name(NativeToCppString(query));
}

//--------------------------------------------------------------------------------------------------

String^ RoutineGroupEditorBE::get_comment()
{
  return CppStringToNative(get_unmanaged_object()->get_comment());
}

//--------------------------------------------------------------------------------------------------

void RoutineGroupEditorBE::set_comment(String ^query)
{
  get_unmanaged_object()->set_comment(NativeToCppString(query));
}

//--------------------------------------------------------------------------------------------------

bool RoutineGroupEditorBE::has_syntax_error()
{
  return get_unmanaged_object()->has_syntax_error();
}

//--------------------------------------------------------------------------------------------------

void RoutineGroupEditorBE::open_editor_for_routine_at_index(size_t index)
{
  get_unmanaged_object()->open_editor_for_routine_at_index(index);
}
