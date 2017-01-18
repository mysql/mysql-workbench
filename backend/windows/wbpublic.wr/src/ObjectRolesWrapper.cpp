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
