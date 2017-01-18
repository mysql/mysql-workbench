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

#ifndef __ROLE_TREE_H__
#define __ROLE_TREE_H__

#include "DBObjectEditorWrapper.h"
#include "GrtTemplates.h"
#include "grtdb/role_tree_model.h"
#include "ModelWrappers.h"

namespace MySQL {
  namespace Grt {
    namespace Db {

    public
      ref class RoleTreeBE : public MySQL::Grt::TreeModelWrapper {
      public:
        enum class Columns { Enabled = ::bec::RoleTreeBE::Enabled, Name = ::bec::RoleTreeBE::Name };

        RoleTreeBE(::bec::RoleTreeBE *inn);
        RoleTreeBE(GrtValue ^ catalog);
        ~RoleTreeBE();

        ::bec::RoleTreeBE *get_unmanaged_object();
        MySQL::Grt::GrtValue ^ get_role_with_id(MySQL::Grt::NodeIdWrapper ^ node);
        void erase_node(MySQL::Grt::NodeIdWrapper ^ node);
        void insert_node_before(MySQL::Grt::NodeIdWrapper ^ before, MySQL::Grt::NodeIdWrapper ^ node);
        void insert_node_after(MySQL::Grt::NodeIdWrapper ^ after, MySQL::Grt::NodeIdWrapper ^ node);
        void append_child(MySQL::Grt::NodeIdWrapper ^ parent, MySQL::Grt::NodeIdWrapper ^ child);
        void move_to_top_level(MySQL::Grt::NodeIdWrapper ^ node);

      private:
        bool free_inner;
      };

    } // namespace Db
  }   // namespace Grt
} // namespace MySQL

#endif // __ROLE_TREE_H__
