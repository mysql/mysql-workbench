/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __USER_EDITOR_H__
#define __USER_EDITOR_H__

#include "DBObjectEditorWrapper.h"
#include "RoleTreeBE.h"
#include "GrtTemplates.h"
#include "grtdb/editor_user.h"

namespace MySQL {
  namespace Grt {
    namespace Db {

    public
      ref class UserEditorBE : public BaseEditorWrapper {
      protected:
        UserEditorBE(::bec::UserEditorBE *inn);
        ~UserEditorBE();

      public:
        UserEditorBE::UserEditorBE(MySQL::Grt::GrtValue ^ arglist);
        ::bec::UserEditorBE *get_unmanaged_object();
        void set_name(String ^ name);
        String ^ get_name();
        void set_password(String ^ pass);
        String ^ get_password();
        void set_comment(String ^ comment);
        String ^ get_comment();
        RoleTreeBE ^ get_role_tree();
        void add_role(String ^ pass);
        void remove_role(String ^ pass);
        ;
        List<String ^> ^ get_roles();
      };

    } // namespace Db
  }   // namespace Grt
} // namespace MySQL

#endif // __USER_EDITOR_H__