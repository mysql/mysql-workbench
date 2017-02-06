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

#include "grtdb/db_object_helpers.h"

#include "SchemaEditorWrapper.h"

using namespace MySQL::Grt::Db;

//--------------------------------------------------------------------------------------------------

SchemaEditorWrapper::SchemaEditorWrapper(::bec::SchemaEditorBE *inn) : DBObjectEditorWrapper(inn) {
}

//--------------------------------------------------------------------------------------------------

::bec::SchemaEditorBE *SchemaEditorWrapper::get_unmanaged_object() {
  return static_cast<::bec::SchemaEditorBE *>(inner);
}

//--------------------------------------------------------------------------------------------------

void SchemaEditorWrapper::set_schema_option_by_name(System::String ^ name, System::String ^ value) {
  get_unmanaged_object()->set_schema_option_by_name(NativeToCppString(name), NativeToCppString(value));
}

//--------------------------------------------------------------------------------------------------

String ^ SchemaEditorWrapper::get_schema_option_by_name(String ^ name) {
  return CppStringToNative(get_unmanaged_object()->get_schema_option_by_name(NativeToCppString(name)));
}

//--------------------------------------------------------------------------------------------------
