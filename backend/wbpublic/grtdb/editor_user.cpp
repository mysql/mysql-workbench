/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "editor_user.h"
#include "db_object_helpers.h"
#include "base/string_utilities.h"

using namespace bec;
using namespace grt;
using namespace base;

UserEditorBE::UserEditorBE(const db_UserRef &user)
  : DBObjectEditorBE(user), _user(user), _role_tree(db_CatalogRef::cast_from(user->owner())) {
}

//--------------------------------------------------------------------------------------------------

void UserEditorBE::set_password(const std::string &pass) {
  if (get_password() != pass) {
    AutoUndoEdit undo(this, get_user(), "password");

    get_user()->password(pass);

    update_change_date();
    undo.end(strfmt(_("Set Password for User '%s'"), get_user()->name().c_str()));
  }
}

//--------------------------------------------------------------------------------------------------

std::string UserEditorBE::get_password() {
  return get_user()->password();
}

//--------------------------------------------------------------------------------------------------

RoleTreeBE *UserEditorBE::get_role_tree() {
  return &_role_tree;
}

//--------------------------------------------------------------------------------------------------

void UserEditorBE::add_role(const std::string &role_name) {
  db_RoleRef role(grt::find_named_object_in_list(db_CatalogRef::cast_from(get_user()->owner())->roles(), role_name));

  if (role.is_valid() && BaseListRef::npos == get_user()->roles().get_index(role)) {
    AutoUndoEdit undo(this);
    get_user()->roles().insert(role);
    update_change_date();
    undo.end(strfmt(_("Assign Role '%s' to User '%s'"), role_name.c_str(), get_name().c_str()));
  }
}

//--------------------------------------------------------------------------------------------------

void UserEditorBE::remove_role(const std::string &role_name) {
  db_RoleRef role(grt::find_named_object_in_list(db_CatalogRef::cast_from(get_user()->owner())->roles(), role_name));

  if (role.is_valid()) {
    const size_t index = get_user()->roles().get_index(role);
    if (index != BaseListRef::npos) {
      AutoUndoEdit undo(this);
      get_user()->roles().remove(index);
      update_change_date();
      undo.end(strfmt(_("Revoke Role '%s' from User '%s'"), role_name.c_str(), get_name().c_str()));
    }
  }
}

//--------------------------------------------------------------------------------------------------

std::vector<std::string> UserEditorBE::get_roles() {
  std::vector<std::string> roles;

  for (size_t c = get_user()->roles().count(), i = 0; i < c; i++)
    roles.push_back(*get_user()->roles()[i]->name());

  return roles;
}

//--------------------------------------------------------------------------------------------------

std::string UserEditorBE::get_title() {
  return base::strfmt("%s - User", get_name().c_str());
}

//--------------------------------------------------------------------------------------------------

bool UserEditorBE::can_close() {
  return true; // There's nothing that can prevent closing the editor.
}

//--------------------------------------------------------------------------------------------------
