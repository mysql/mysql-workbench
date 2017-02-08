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
#include "GrtManager.h"

#include "DelegateWrapper.h"
#include "DBObjectEditorWrapper.h"

#include "RoutineEditorWrapper.h"

#include "grtdb/editor_routine.h"

using namespace MySQL::Grt::Db;

//--------------------------------------------------------------------------------------------------

RoutineEditorWrapper::RoutineEditorWrapper(::bec::RoutineEditorBE *inn) : DBObjectEditorWrapper(inn) {
}

//--------------------------------------------------------------------------------------------------

::bec::RoutineEditorBE *RoutineEditorWrapper::get_unmanaged_object() {
  return static_cast<::bec::RoutineEditorBE *>(inner);
}

//--------------------------------------------------------------------------------------------------

String ^ RoutineEditorWrapper::get_sql() {
  return CppStringToNative(get_unmanaged_object()->get_sql());
}

//--------------------------------------------------------------------------------------------------

void RoutineEditorWrapper::set_sql(String ^ query) {
  get_unmanaged_object()->set_sql(NativeToCppString(query));
}

//--------------------------------------------------------------------------------------------------

String ^ RoutineEditorWrapper::get_name() {
  return CppStringToNative(get_unmanaged_object()->get_name());
}

//--------------------------------------------------------------------------------------------------

void RoutineEditorWrapper::set_name(String ^ name) {
  get_unmanaged_object()->set_name(NativeToCppString(name));
}

//--------------------------------------------------------------------------------------------------

String ^ RoutineEditorWrapper::get_comment() {
  return CppStringToNative(get_unmanaged_object()->get_comment());
}

//--------------------------------------------------------------------------------------------------

void RoutineEditorWrapper::set_comment(String ^ comment) {
  get_unmanaged_object()->set_comment(NativeToCppString(comment));
}

//--------------------------------------------------------------------------------------------------
