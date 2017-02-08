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

#include "GrtWrapper.h"

#include "GrtTemplates.h"
#include "DelegateWrapper.h"
#include "GrtManager.h"

#include "RoutineGroupEditorWrapper.h"

#include "grtdb/editor_routinegroup.h"

using namespace MySQL::Grt::Db;

//--------------------------------------------------------------------------------------------------

RoutineGroupEditorWrapper::RoutineGroupEditorWrapper(::bec::RoutineGroupEditorBE *inn) : DBObjectEditorWrapper(inn) {
}

//--------------------------------------------------------------------------------------------------

::bec::RoutineGroupEditorBE *RoutineGroupEditorWrapper::get_unmanaged_object() {
  return static_cast<::bec::RoutineGroupEditorBE *>(inner);
}

//--------------------------------------------------------------------------------------------------

String ^ RoutineGroupEditorWrapper::get_sql() {
  return CppStringToNative(get_unmanaged_object()->get_sql());
}

//--------------------------------------------------------------------------------------------------

String ^ RoutineGroupEditorWrapper::get_routine_sql(MySQL::Grt::GrtValue ^ routine) {
  return CppStringToNative(
    get_unmanaged_object()->get_routine_sql(db_RoutineRef::cast_from(routine->get_unmanaged_object())));
}

//--------------------------------------------------------------------------------------------------

void RoutineGroupEditorWrapper::set_sql(String ^ query) {
  get_unmanaged_object()->set_sql(NativeToCppString(query));
}

//--------------------------------------------------------------------------------------------------

List<String ^> ^ RoutineGroupEditorWrapper::get_routines_names() {
  return CppStringListToNative(get_unmanaged_object()->get_routines_names());
}

//--------------------------------------------------------------------------------------------------

void RoutineGroupEditorWrapper::delete_routine_with_name(String ^ name) {
  get_unmanaged_object()->delete_routine_with_name(NativeToCppString(name));
}

//--------------------------------------------------------------------------------------------------

void RoutineGroupEditorWrapper::append_routine_with_id(String ^ id) {
  get_unmanaged_object()->append_routine_with_id(NativeToCppString(id));
}

//--------------------------------------------------------------------------------------------------

String ^ RoutineGroupEditorWrapper::get_name() {
  return CppStringToNative(get_unmanaged_object()->get_name());
}

//--------------------------------------------------------------------------------------------------

void RoutineGroupEditorWrapper::set_name(String ^ query) {
  get_unmanaged_object()->set_name(NativeToCppString(query));
}

//--------------------------------------------------------------------------------------------------

String ^ RoutineGroupEditorWrapper::get_comment() {
  return CppStringToNative(get_unmanaged_object()->get_comment());
}

//--------------------------------------------------------------------------------------------------

void RoutineGroupEditorWrapper::set_comment(String ^ query) {
  get_unmanaged_object()->set_comment(NativeToCppString(query));
}

//--------------------------------------------------------------------------------------------------

void RoutineGroupEditorWrapper::open_editor_for_routine_at_index(size_t index) {
  get_unmanaged_object()->open_editor_for_routine_at_index(index);
}

//--------------------------------------------------------------------------------------------------
