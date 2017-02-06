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

#pragma once

#include "grtdb/editor_dbobject.h"
#include "grt/tree_model.h"

#include "grts/structs.db.h"

#include "role_tree_model.h"

#include "wbpublic_public_interface.h"

namespace bec {

  //--------------------------------------------------------------------------------------------------

  class WBPUBLICBACKEND_PUBLIC_FUNC UserEditorBE : public DBObjectEditorBE {
  protected:
    db_UserRef _user;
    NodeId _selected_user;
    RoleTreeBE _role_tree;

  public: // editor interface
    UserEditorBE(const db_UserRef &user);

    virtual std::string get_title();

    virtual db_DatabaseObjectRef get_dbobject() {
      return get_user();
    }

    db_UserRef get_user() {
      return _user;
    }

    RoleTreeBE *get_role_tree();
    void add_role(const std::string &role_name);
    void remove_role(const std::string &role_name);
    std::vector<std::string> get_roles();

    void set_password(const std::string &pass);
    std::string get_password();

    virtual bool can_close();
  };
};
