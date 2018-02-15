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

#include "MySQLTableEditorWrapper.h"
#include "grtdb/db_object_helpers.h"
#include "mforms/view.h"

using namespace System;

using namespace MySQL::Forms;

using namespace MySQL::Grt::Db;

//----------------- MySQLTableColumnsListWrapper ---------------------------------------------------

MySQLTableColumnsListWrapper::MySQLTableColumnsListWrapper(::MySQLTableColumnsListBE *inn)
  : TableColumnsListWrapper(inn) {
}

//----------------- MySQLTableEditorWrapper --------------------------------------------------------

MySQLTableEditorWrapper::MySQLTableEditorWrapper(MySQL::Grt::GrtValue ^ arglist)
  : TableEditorWrapper(new ::MySQLTableEditorBE(
      db_mysql_TableRef::cast_from(::grt::BaseListRef::cast_from(arglist->get_unmanaged_object()).get(0)))) {
}

//--------------------------------------------------------------------------------------------------

MySQLTableEditorWrapper::~MySQLTableEditorWrapper() {
  delete inner; // We created it.
}

//--------------------------------------------------------------------------------------------------

MySQLTableColumnsListWrapper ^ MySQLTableEditorWrapper::get_columns() {
  return gcnew MySQLTableColumnsListWrapper(get_unmanaged_object()->get_columns());
}

//--------------------------------------------------------------------------------------------------

MySQLTablePartitionTreeWrapper ^ MySQLTableEditorWrapper::get_partitions() {
  return gcnew MySQLTablePartitionTreeWrapper(get_unmanaged_object()->get_partitions());
}

//--------------------------------------------------------------------------------------------------

Control ^ MySQLTableEditorWrapper::get_trigger_panel() {
  return dynamic_cast<Control ^>(ObjectMapper::GetManagedComponent(get_unmanaged_object()->get_trigger_panel()));
}

//--------------------------------------------------------------------------------------------------

void MySQLTableEditorWrapper::commit_changes() {
  get_unmanaged_object()->commit_changes();
}

//--------------------------------------------------------------------------------------------------

bool MySQLTableEditorWrapper::is_server_version_at_least(int major, int minor, int release) {
  db_CatalogRef catalog = get_unmanaged_object()->get_catalog();
  return bec::is_supported_mysql_version_at_least(catalog->version(), major, minor, release);
}

//--------------------------------------------------------------------------------------------------

void MySQLTableEditorWrapper::load_trigger_sql() {
  get_unmanaged_object()->load_trigger_sql();
}

//--------------------------------------------------------------------------------------------------
