/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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
