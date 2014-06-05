/* 
 * Copyright (c) 2008, 2014, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include "ModelWrappers.h"

namespace MySQL {
namespace Grt {
namespace Db {

ref class ObjectPrivilegeListBE;
ref class DBObjectEditorWrapper;

public ref class ObjectRoleListWrapper : public MySQL::Grt::ListModelWrapper
{
public:
  enum class Columns {
    Name = ::bec::ObjectRoleListBE::Name
  };

public:
  ObjectRoleListWrapper(DBObjectEditorWrapper^ editor);

  ~ObjectRoleListWrapper()
  {
    delete inner;
  }

  ::bec::ObjectRoleListBE *get_unmanaged_object()
  { return static_cast<::bec::ObjectRoleListBE *>(inner); }


  void add_role_for_privileges(GrtValue^ role)
  {
    get_unmanaged_object()->add_role_for_privileges(db_RoleRef::cast_from(role->get_unmanaged_object()));
  }

  void remove_role_from_privileges(GrtValue^ role)
  {
    get_unmanaged_object()->remove_role_from_privileges(db_RoleRef::cast_from(role->get_unmanaged_object()));
  }


  ObjectPrivilegeListBE^ get_privilege_list();

  void set_selected(NodeIdWrapper^ node)
  {
    get_unmanaged_object()->select_role(*node->get_unmanaged_object());
  }
};



public ref class ObjectPrivilegeListBE : public MySQL::Grt::ListModelWrapper
{
public:
  enum class Columns {
    Enabled = ::bec::ObjectPrivilegeListBE::Enabled,
    Name = ::bec::ObjectPrivilegeListBE::Name
  };

public:
  ObjectPrivilegeListBE(::bec::ObjectPrivilegeListBE *inn)
    : MySQL::Grt::ListModelWrapper(inn)
  {}

  ::bec::ObjectPrivilegeListBE *get_unmanaged_object()
  { return static_cast<::bec::ObjectPrivilegeListBE *>(inner); }
};
} // namespace Db
} // namespace Grt
} // namespace MySQL
