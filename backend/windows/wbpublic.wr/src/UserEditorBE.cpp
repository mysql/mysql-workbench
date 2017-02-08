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

#include "grts/structs.workbench.physical.h"

#include "UserEditorBE.h"

using namespace MySQL::Grt;
using namespace MySQL::Grt::Db;

//--------------------------------------------------------------------------------------------------

UserEditorBE::UserEditorBE(::bec::UserEditorBE *inn) : BaseEditorWrapper(inn) {
}

UserEditorBE::~UserEditorBE() {
  if (inner != NULL) {
    delete inner;
    inner = NULL;
  }
}

//--------------------------------------------------------------------------------------------------

UserEditorBE::UserEditorBE(MySQL::Grt::GrtValue ^ arglist)
  : BaseEditorWrapper(new bec::UserEditorBE(
      db_UserRef::cast_from(grt::BaseListRef::cast_from(arglist->get_unmanaged_object()).get(0)))) {
}

//--------------------------------------------------------------------------------------------------

::bec::UserEditorBE *UserEditorBE::get_unmanaged_object() {
  return static_cast<::bec::UserEditorBE *>(inner);
}

//--------------------------------------------------------------------------------------------------

void UserEditorBE::set_name(String ^ name) {
  get_unmanaged_object()->set_name(NativeToCppString(name));
}

//--------------------------------------------------------------------------------------------------

String ^ UserEditorBE::get_name() {
  return CppStringToNative(get_unmanaged_object()->get_name());
}

//--------------------------------------------------------------------------------------------------

void UserEditorBE::set_password(String ^ pass) {
  get_unmanaged_object()->set_password(NativeToCppString(pass));
}

//--------------------------------------------------------------------------------------------------

String ^ UserEditorBE::get_password() {
  return gcnew String(get_unmanaged_object()->get_password().c_str());
}

//--------------------------------------------------------------------------------------------------

void UserEditorBE::set_comment(String ^ comment) {
  get_unmanaged_object()->set_comment(NativeToCppString(comment));
}

//--------------------------------------------------------------------------------------------------

String ^ UserEditorBE::get_comment() {
  return CppStringToNative(get_unmanaged_object()->get_comment());
}

//--------------------------------------------------------------------------------------------------

RoleTreeBE ^ UserEditorBE::get_role_tree() {
  return gcnew RoleTreeBE(get_unmanaged_object()->get_role_tree());
}

//--------------------------------------------------------------------------------------------------

void UserEditorBE::add_role(String ^ pass) {
  get_unmanaged_object()->add_role(NativeToCppString(pass));
}

//--------------------------------------------------------------------------------------------------

void UserEditorBE::remove_role(String ^ pass) {
  get_unmanaged_object()->remove_role(NativeToCppString(pass));
}

//--------------------------------------------------------------------------------------------------

List<String ^> ^ UserEditorBE::get_roles() {
  return CppStringListToNative(get_unmanaged_object()->get_roles());
}

//--------------------------------------------------------------------------------------------------