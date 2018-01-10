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

#ifndef _EDITOR_USER_ROLE_H_
#define _EDITOR_USER_ROLE_H_

#include "grtdb/editor_dbobject.h"
#include "role_tree_model.h"

#include "grts/structs.db.h"

#include "wbpublic_public_interface.h"

namespace bec {

  class RoleEditorBE;

  class WBPUBLICBACKEND_PUBLIC_FUNC RolePrivilegeListBE : public ListModel {
  public:
    enum Columns { Name, Enabled };

    RolePrivilegeListBE(RoleEditorBE *owner);

    virtual void refresh();

    virtual size_t count();

    void add_all();
    void remove_all();

    virtual bool set_field(const NodeId &node, ColumnId column, ssize_t value);

  protected:
    RoleEditorBE *_owner;
    db_RolePrivilegeRef _role_privilege;
    grt::StringListRef _privileges;

    virtual bool get_field_grt(const NodeId &node, ColumnId column, grt::ValueRef &value);
  };

  //!
  //! Wraps in ListModel way operations with db_Role::privileges()
  //! Role is obtained from RoleEditorBE owner.
  //!
  class WBPUBLICBACKEND_PUBLIC_FUNC RoleObjectListBE : public ListModel {
  public:
    enum Columns { Name };

    RoleObjectListBE(RoleEditorBE *owner);

    void set_selected_node(const NodeId &node);
    db_RolePrivilegeRef get_selected_object_info();

    virtual size_t count();
    virtual void refresh(){};

    virtual MenuItemList get_popup_items_for_nodes(const std::vector<NodeId> &nodes);
    virtual bool activate_popup_item_for_nodes(const std::string &name, const std::vector<NodeId> &nodes);

  protected:
    RoleEditorBE *_owner;
    NodeId _selection;

    virtual IconId get_field_icon(const NodeId &node, ColumnId column, IconSize size);
    virtual bool get_field_grt(const NodeId &node, ColumnId column, grt::ValueRef &value);
  };

  //!
  //! Represents Roles, their objects and assigned privileges.
  //! The set of classes: RoleEditorBE, RoleObjectListBE and RolePrivilegeListBE
  //! works using RoleEditorBE::_role field.
  class WBPUBLICBACKEND_PUBLIC_FUNC RoleEditorBE : public BaseEditor {
  protected:
    db_RoleRef _role; //!< Selected role
    db_mgmt_RdbmsRef _rdbms;
    RoleTreeBE _tree;                    //!< List of roles in a schema
    RolePrivilegeListBE _privilege_list; //!< Serves as a source of role's objects privileges for the UIs
    RoleObjectListBE _object_list;       //!< Serves as a source of role's objects for the UI.

  public:
    RoleEditorBE(const db_RoleRef &role, const db_mgmt_RdbmsRef &rdbms);

    db_RoleRef get_role() {
      return _role;
    }

    const db_mgmt_RdbmsRef &get_rdbms() {
      return _rdbms;
    }

    virtual std::string get_title();

    void set_name(const std::string &name);
    std::string get_name();

    void set_parent_role(const std::string &name);
    std::string get_parent_role();
    std::vector<std::string> get_role_list();

    RoleTreeBE *get_role_tree() {
      return &_tree;
    }

    RolePrivilegeListBE *get_privilege_list() {
      return &_privilege_list;
    }
    RoleObjectListBE *get_object_list() {
      return &_object_list;
    }

    bool add_dropped_objectdata(const std::string &data);
    bool add_object(const std::string &type, const std::string &name);
    bool add_object(db_DatabaseObjectRef object);
    void remove_object(const bec::NodeId &object_node_id);
  };
};

#endif /* _EDITOR_USER_ROLE_H_ */
