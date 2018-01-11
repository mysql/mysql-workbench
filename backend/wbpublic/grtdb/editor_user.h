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
