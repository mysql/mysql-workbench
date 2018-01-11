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

#include "grtdb/dbobject_roles.h"
#include "grtdb/db_object_helpers.h"

#include "ObjectRolesWrapper.h"

using namespace MySQL::Grt;
using namespace MySQL::Grt::Db;

//--------------------------------------------------------------------------------------------------

ObjectRoleListWrapper::ObjectRoleListWrapper(DBObjectEditorWrapper ^ editor)
  : MySQL::Grt::ListModelWrapper(new ::bec::ObjectRoleListBE(
      editor->get_unmanaged_object(), get_rdbms_for_db_object(editor->get_unmanaged_object()->get_dbobject()))) {
}

//--------------------------------------------------------------------------------------------------

ObjectPrivilegeListBE ^ ObjectRoleListWrapper::get_privilege_list() {
  return gcnew ObjectPrivilegeListBE(get_unmanaged_object()->get_privilege_list());
}

//--------------------------------------------------------------------------------------------------

ObjectRoleListWrapper::~ObjectRoleListWrapper() {
  delete inner;
}

//--------------------------------------------------------------------------------------------------

::bec::ObjectRoleListBE *ObjectRoleListWrapper::get_unmanaged_object() {
  return static_cast<::bec::ObjectRoleListBE *>(inner);
}

//--------------------------------------------------------------------------------------------------

void ObjectRoleListWrapper::add_role_for_privileges(GrtValue ^ role) {
  get_unmanaged_object()->add_role_for_privileges(db_RoleRef::cast_from(role->get_unmanaged_object()));
}

//--------------------------------------------------------------------------------------------------

void ObjectRoleListWrapper::remove_role_from_privileges(GrtValue ^ role) {
  get_unmanaged_object()->remove_role_from_privileges(db_RoleRef::cast_from(role->get_unmanaged_object()));
}

//--------------------------------------------------------------------------------------------------

void ObjectRoleListWrapper::set_selected(NodeIdWrapper ^ node) {
  get_unmanaged_object()->select_role(*node->get_unmanaged_object());
}

//--------------------------------------------------------------------------------------------------

ObjectPrivilegeListBE::ObjectPrivilegeListBE(::bec::ObjectPrivilegeListBE *inn) : MySQL::Grt::ListModelWrapper(inn) {
}

//--------------------------------------------------------------------------------------------------

::bec::ObjectPrivilegeListBE *ObjectPrivilegeListBE::get_unmanaged_object() {
  return static_cast<::bec::ObjectPrivilegeListBE *>(inner);
}

//--------------------------------------------------------------------------------------------------
