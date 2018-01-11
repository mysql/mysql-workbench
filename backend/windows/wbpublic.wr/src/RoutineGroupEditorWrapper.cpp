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
