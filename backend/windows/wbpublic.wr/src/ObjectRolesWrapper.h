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

#pragma once

#include "ModelWrappers.h"

namespace MySQL {
  namespace Grt {
    namespace Db {

      ref class ObjectPrivilegeListBE;
      ref class DBObjectEditorWrapper;

    public
      ref class ObjectRoleListWrapper : public MySQL::Grt::ListModelWrapper {
      public:
        enum class Columns { Name = ::bec::ObjectRoleListBE::Name };

      public:
        ObjectRoleListWrapper(DBObjectEditorWrapper ^ editor);
        ~ObjectRoleListWrapper();

        ::bec::ObjectRoleListBE *get_unmanaged_object();
        void add_role_for_privileges(GrtValue ^ role);
        void remove_role_from_privileges(GrtValue ^ role);
        ObjectPrivilegeListBE ^ get_privilege_list();
        void set_selected(NodeIdWrapper ^ node);
      };

    public
      ref class ObjectPrivilegeListBE : public MySQL::Grt::ListModelWrapper {
      public:
        enum class Columns {
          Enabled = ::bec::ObjectPrivilegeListBE::Enabled,
          Name = ::bec::ObjectPrivilegeListBE::Name
        };

      public:
        ObjectPrivilegeListBE(::bec::ObjectPrivilegeListBE *inn);
        ::bec::ObjectPrivilegeListBE *get_unmanaged_object();
      };

    } // namespace Db
  }   // namespace Grt
} // namespace MySQL
