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
#include "ModelWrappers.h"
#include "GrtManager.h"

#include "RoleTreeBE.h"
#include "grtdb/db_object_helpers.h"

#include "UserRoleEditorBE.h"

using namespace MySQL::Grt;
using namespace MySQL::Grt::Db;

//--------------------------------------------------------------------------------------------------

RolePrivilegeListWrapper::RolePrivilegeListWrapper(::bec::RolePrivilegeListBE *inn)
  : MySQL::Grt::ListModelWrapper(inn) {
}

//--------------------------------------------------------------------------------------------------

::bec::RolePrivilegeListBE *RolePrivilegeListWrapper::get_unmanaged_object() {
  return static_cast<::bec::RolePrivilegeListBE *>(inner);
}

//--------------------------------------------------------------------------------------------------

RoleObjectListWrapper::RoleObjectListWrapper(::bec::RoleObjectListBE *inn) : MySQL::Grt::ListModelWrapper(inn) {
}

//--------------------------------------------------------------------------------------------------

bec::RoleObjectListBE *RoleObjectListWrapper::get_unmanaged_object() {
  return static_cast<::bec::RoleObjectListBE *>(inner);
}

//--------------------------------------------------------------------------------------------------

void RoleObjectListWrapper::set_selected_node(NodeIdWrapper ^ node) {
  get_unmanaged_object()->set_selected_node(*node->get_unmanaged_object());
}

//--------------------------------------------------------------------------------------------------

RoleEditorBE::RoleEditorBE(MySQL::Grt::GrtValue ^ arglist)
  : BaseEditorWrapper(new bec::RoleEditorBE(
      db_RoleRef::cast_from(grt::BaseListRef::cast_from(arglist->get_unmanaged_object()).get(0)),
      get_rdbms_for_db_object(grt::BaseListRef::cast_from(arglist->get_unmanaged_object()).get(0)))) {
}

//--------------------------------------------------------------------------------------------------

RoleEditorBE::~RoleEditorBE() {
  if (inner != NULL) {
    delete inner;
    inner = NULL;
  }
}

//--------------------------------------------------------------------------------------------------

bec::RoleEditorBE *RoleEditorBE::get_unmanaged_object() {
  return static_cast<::bec::RoleEditorBE *>(inner);
}

//--------------------------------------------------------------------------------------------------

String ^ RoleEditorBE::get_name() {
  return CppStringToNative(get_unmanaged_object()->get_name());
}

//--------------------------------------------------------------------------------------------------

void RoleEditorBE::set_name(String ^ name) {
  get_unmanaged_object()->set_name(NativeToCppString(name));
}

//--------------------------------------------------------------------------------------------------

void RoleEditorBE::set_parent_role(String ^ name) {
  get_unmanaged_object()->set_parent_role(NativeToCppString(name));
}

//--------------------------------------------------------------------------------------------------

String ^ RoleEditorBE::get_parent_role() {
  return CppStringToNative(get_unmanaged_object()->get_parent_role());
}

//--------------------------------------------------------------------------------------------------

RoleTreeBE ^ RoleEditorBE::get_role_tree() {
  return gcnew RoleTreeBE(get_unmanaged_object()->get_role_tree());
}

//--------------------------------------------------------------------------------------------------

List<String ^> ^ RoleEditorBE::get_role_list() {
  return CppStringListToNative(get_unmanaged_object()->get_role_list());
}

//--------------------------------------------------------------------------------------------------

RolePrivilegeListWrapper ^ RoleEditorBE::get_privilege_list() {
  return gcnew RolePrivilegeListWrapper(get_unmanaged_object()->get_privilege_list());
}

//--------------------------------------------------------------------------------------------------

RoleObjectListWrapper ^ RoleEditorBE::get_object_list() {
  return gcnew RoleObjectListWrapper(get_unmanaged_object()->get_object_list());
}

//--------------------------------------------------------------------------------------------------

void RoleEditorBE::add_object(GrtValue ^ object) {
  get_unmanaged_object()->add_object(db_DatabaseObjectRef::cast_from(object->get_unmanaged_object()));
}

//--------------------------------------------------------------------------------------------------

void RoleEditorBE::remove_object(NodeIdWrapper ^ node) {
  get_unmanaged_object()->remove_object(*node->get_unmanaged_object());
}

//--------------------------------------------------------------------------------------------------
