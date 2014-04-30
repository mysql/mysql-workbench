/* 
 * Copyright (c) 2007, 2012, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _DBOBJECT_ROLES_H_
#define _DBOBJECT_ROLES_H_

#include "editor_dbobject.h"

#include "wbpublic_public_interface.h"

namespace bec {

  class ObjectRoleListBE;

  class WBPUBLICBACKEND_PUBLIC_FUNC ObjectPrivilegeListBE : public ListModel
  {
  public:
    enum Columns {
      Name,
      Enabled
    };

    ObjectPrivilegeListBE(const db_mgmt_RdbmsRef &rdbms);

    virtual void refresh();

    virtual int count();

    virtual bool set_field(const NodeId &node, int column, int value);
    void set_owner(ObjectRoleListBE* owner) { _owner= owner; };

  protected:
    ObjectRoleListBE *_owner;
    db_mgmt_RdbmsRef _rdbms;
    grt::StringListRef _privileges;

    virtual bool get_field_grt(const NodeId &node, int column, grt::ValueRef &value);
  };


  
  class WBPUBLICBACKEND_PUBLIC_FUNC ObjectRoleListBE : public ListModel
  {
  public:
    enum Columns {
      Name
    };

    ObjectRoleListBE(DBObjectEditorBE *owner);

    virtual int count();
    virtual void refresh();

    void select_role(const NodeId &node);

    void add_role_for_privileges(const db_RoleRef &role);
    void remove_role_from_privileges(const db_RoleRef &role);

    ObjectPrivilegeListBE *get_privilege_list() { return &_privilege_list; }

    db_RolePrivilegeRef get_selected();

    DBObjectEditorBE *get_owner() { return _owner; }

  protected:
    DBObjectEditorBE *_owner;
    std::vector<db_RolePrivilegeRef> _role_privs;

    ObjectPrivilegeListBE _privilege_list;

    NodeId _selected_node;
    
    virtual bool get_field_grt(const NodeId &node, int column, grt::ValueRef &value);
  };
};

#endif /* _DBOBJECT_ROLES_H_ */
