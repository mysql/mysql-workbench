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

#include "RoleTreeBE.h"

using namespace MySQL::Grt;
using namespace MySQL::Grt::Db;

//--------------------------------------------------------------------------------------------------

RoleTreeBE::RoleTreeBE(::bec::RoleTreeBE *inn) : MySQL::Grt::TreeModelWrapper(inn), free_inner(false) {
}

//--------------------------------------------------------------------------------------------------

RoleTreeBE::RoleTreeBE(GrtValue ^ catalog)
  : MySQL::Grt::TreeModelWrapper(new ::bec::RoleTreeBE(db_CatalogRef::cast_from(catalog->get_unmanaged_object()))),
    free_inner(true) {
}

//--------------------------------------------------------------------------------------------------

RoleTreeBE::~RoleTreeBE() {
  if (free_inner)
    delete inner;
}

//--------------------------------------------------------------------------------------------------

::bec::RoleTreeBE *RoleTreeBE::get_unmanaged_object() {
  return static_cast<::bec::RoleTreeBE *>(inner);
}

//--------------------------------------------------------------------------------------------------

MySQL::Grt::GrtValue ^ RoleTreeBE::get_role_with_id(MySQL::Grt::NodeIdWrapper ^ node) {
  return gcnew MySQL::Grt::GrtValue(get_unmanaged_object()->get_role_with_id(*node->get_unmanaged_object()));
}

//--------------------------------------------------------------------------------------------------

void RoleTreeBE::erase_node(MySQL::Grt::NodeIdWrapper ^ node) {
  get_unmanaged_object()->erase_node(*node->get_unmanaged_object());
}

//--------------------------------------------------------------------------------------------------

void RoleTreeBE::insert_node_before(MySQL::Grt::NodeIdWrapper ^ before, MySQL::Grt::NodeIdWrapper ^ node) {
  get_unmanaged_object()->insert_node_before(*before->get_unmanaged_object(), *node->get_unmanaged_object());
}

//--------------------------------------------------------------------------------------------------

void RoleTreeBE::insert_node_after(MySQL::Grt::NodeIdWrapper ^ after, MySQL::Grt::NodeIdWrapper ^ node) {
  get_unmanaged_object()->insert_node_after(*after->get_unmanaged_object(), *node->get_unmanaged_object());
}

//--------------------------------------------------------------------------------------------------

void RoleTreeBE::append_child(MySQL::Grt::NodeIdWrapper ^ parent, MySQL::Grt::NodeIdWrapper ^ child) {
  get_unmanaged_object()->append_child(*parent->get_unmanaged_object(), *child->get_unmanaged_object());
}

//--------------------------------------------------------------------------------------------------

void RoleTreeBE::move_to_top_level(MySQL::Grt::NodeIdWrapper ^ node) {
  get_unmanaged_object()->move_to_top_level(*node->get_unmanaged_object());
}

//--------------------------------------------------------------------------------------------------
