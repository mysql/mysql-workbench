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

#include "editor_dbobject.h"

namespace bec {

  class ObjectRoleListBE;

  class WBPUBLICBACKEND_PUBLIC_FUNC ObjectPrivilegeListBE : public ListModel {
  public:
    enum Columns { Name, Enabled };

    ObjectPrivilegeListBE(ObjectRoleListBE *owner, const db_mgmt_RdbmsRef &rdbms);

    virtual void refresh();
    virtual size_t count();

    virtual bool set_field(const NodeId &node, ColumnId column, ssize_t value);

  protected:
    ObjectRoleListBE *_owner;
    db_mgmt_RdbmsRef _rdbms;
    grt::StringListRef _privileges;

    virtual bool get_field_grt(const NodeId &node, ColumnId column, grt::ValueRef &value);
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC ObjectRoleListBE : public ListModel {
  public:
    enum Columns { Name };

    ObjectRoleListBE(DBObjectEditorBE *owner, const db_mgmt_RdbmsRef &rdbms);

    virtual size_t count();
    virtual void refresh();

    void select_role(const NodeId &node);

    void add_role_for_privileges(const db_RoleRef &role);
    void remove_role_from_privileges(const db_RoleRef &role);

    ObjectPrivilegeListBE *get_privilege_list() {
      return &_privilege_list;
    }
    db_RolePrivilegeRef get_selected();
    DBObjectEditorBE *get_owner() {
      return _owner;
    }

  protected:
    DBObjectEditorBE *_owner;
    db_mgmt_RdbmsRef _rdbms;
    std::vector<db_RolePrivilegeRef> _role_privs;

    ObjectPrivilegeListBE _privilege_list;

    NodeId _selected_node;

    virtual bool get_field_grt(const NodeId &node, ColumnId column, grt::ValueRef &value);
  };
};
