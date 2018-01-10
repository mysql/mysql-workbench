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