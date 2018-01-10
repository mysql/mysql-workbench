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
