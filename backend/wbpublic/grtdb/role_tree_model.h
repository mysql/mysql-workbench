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

#ifndef _ROLE_TREE_MODEL_H_
#define _ROLE_TREE_MODEL_H_

#include "grt/tree_model.h"

#include "grts/structs.db.h"

#include "wbpublic_public_interface.h"

namespace bec {

  class WBPUBLICBACKEND_PUBLIC_FUNC RoleTreeBE : public TreeModel {
  public:
    enum Columns { Enabled, Name };

  protected:
    struct Node {
      Node *parent;
      db_RoleRef role;

      std::vector<Node *> children;

      Node() : parent(NULL) {
      }

      ~Node() {
        for (std::vector<Node *>::iterator iter = children.begin(); iter != children.end(); ++iter)
          delete *iter;
      }

      void insert_child_after(Node *after, Node *child) {
        if (after == NULL)
          children.push_back(child);
        else {
          std::vector<Node *>::iterator insert_point = std::find(children.begin(), children.end(), after);
          if (insert_point == children.end())
            children.push_back(child);
          else
            children.insert(insert_point, child);
        }
        child->parent = this;

        if (role.is_valid()) {
          if (after)
            role->childRoles().insert(child->role, role->childRoles().get_index(after->role));
          else
            role->childRoles().insert(child->role, role->childRoles().count() - 1);
        }

        child->role->parentRole(this->role);
      }

      void insert_child_before(Node *before, Node *child) {
        if (before == NULL) {
          children.push_back(child);
        } else {
          std::vector<Node *>::iterator insert_point = std::find(children.begin(), children.end(), before);
          if (insert_point == children.end())
            children.push_back(child);
          else
            children.insert(insert_point, child);
        }
        child->parent = this;

        if (role.is_valid()) {
          if (before)
            role->childRoles().insert(child->role, role->childRoles().get_index(before->role));
          else
            role->childRoles().insert(child->role);
        }
        child->role->parentRole(this->role);
      }

      void append_child(Node *child) {
        children.push_back(child);
        child->parent = this;

        // If this is the hidden root node then no child roles list exists (the role is just a dummy).
        if (role.is_valid())
          role->childRoles().insert(child->role);
        child->role->parentRole(this->role);
      }

      void erase_child(Node *child) {
        std::vector<Node *>::iterator erase_point = std::find(children.begin(), children.end(), child);
        if (erase_point != children.end()) {
          children.erase(erase_point);
          child->parent = NULL;
        }

        // If the current parent is the hidden root node then there is no childRoles collection.
        if (role.is_valid())
          role->childRoles().remove_value(child->role);
        child->role->parentRole(db_RoleRef());
      }
    };

    db_CatalogRef _catalog;
    Node *_root;
    std::string _object_id;

    virtual bool get_field_grt(const NodeId &node, ColumnId column, grt::ValueRef &value);
    virtual grt::Type get_field_type(const NodeId &node, ColumnId column);
    virtual bool set_field(const NodeId &node, ColumnId column, const std::string &value);

    Node *get_node_with_id(const NodeId &node);
    bool find_role(const RoleTreeBE::Node *node, const db_RoleRef &role, bec::NodeId &path);

    void add_role_children_to_node(Node *parent_node);
    bool is_parent_child(Node *parent, Node *child);

  public:
    RoleTreeBE(const db_CatalogRef &catalog);

    void set_object(const db_DatabaseObjectRef &object);

    virtual ~RoleTreeBE();

    virtual void refresh();
    virtual size_t count_children(const NodeId &parent);
    virtual NodeId get_child(const NodeId &parent, size_t index);

    db_RoleRef get_role_with_id(const NodeId &node) {
      Node *n = get_node_with_id(node);
      return (n ? n->role : db_RoleRef());
    }

    NodeId node_id_for_role(const db_RoleRef &role);

    void erase_node(const NodeId &node);
    void insert_node_after(const NodeId &after, const NodeId &node);
    void insert_node_before(const NodeId &before, const NodeId &node);
    void append_child(const NodeId &parent, const NodeId &child);
    void move_to_top_level(const NodeId &node);
  };
};

#endif
