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

#include "MySQLSchemaEditorWrapper.h"

#include "ConvUtils.h"

using namespace System;

using namespace MySQL::Grt;
using namespace MySQL::Grt::Db;

//--------------------------------------------------------------------------------------------------

MySQLSchemaEditorWrapper::MySQLSchemaEditorWrapper(GrtValue ^ arglist)
  : SchemaEditorWrapper(new MySQLSchemaEditorBE(
      db_mysql_SchemaRef::cast_from(grt::BaseListRef::cast_from(arglist->get_unmanaged_object()).get(0)))) {
}

//--------------------------------------------------------------------------------------------------

MySQLSchemaEditorWrapper::~MySQLSchemaEditorWrapper() {
  delete inner; // We created it.
}

//--------------------------------------------------------------------------------------------------

MySQLSchemaEditorBE *MySQLSchemaEditorWrapper::get_unmanaged_object() {
  return static_cast<::MySQLSchemaEditorBE *>(inner);
}

//--------------------------------------------------------------------------------------------------

bool MySQLSchemaEditorWrapper::is_new_object() {
  return get_unmanaged_object()->get_schema()->oldName() == "";
}

//--------------------------------------------------------------------------------------------------

void MySQLSchemaEditorWrapper::refactor_catalog_upon_schema_rename(String ^ old_name, String ^ new_name) {
  get_unmanaged_object()->refactor_catalog_upon_schema_rename(NativeToCppString(old_name), NativeToCppString(new_name));
}

//--------------------------------------------------------------------------------------------------

bool MySQLSchemaEditorWrapper::refactor_possible() {
  return get_unmanaged_object()->refactor_possible();
}

//--------------------------------------------------------------------------------------------------

void MySQLSchemaEditorWrapper::refactor_catalog() {
  get_unmanaged_object()->refactor_catalog();
}

//--------------------------------------------------------------------------------------------------
