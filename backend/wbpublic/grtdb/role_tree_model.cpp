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

#include "role_tree_model.h"

#include "base/string_utilities.h"

#include "grtpp_undo_manager.h"

using namespace bec;
using namespace base;

RoleTreeBE::RoleTreeBE(const db_CatalogRef &catalog) : _catalog(catalog), _root(0) {
  refresh();
}

RoleTreeBE::~RoleTreeBE() {
  delete _root;
}

void RoleTreeBE::set_object(const db_DatabaseObjectRef &object) {
  _object_id = object.id();
}

void RoleTreeBE::add_role_children_to_node(Node *parent_node) {
  if (!parent_node->role->childRoles().is_valid())
    return;

  grt::ListRef<db_Role> roles(parent_node->role->childRoles());

  for (size_t c = roles.count(), i = 0; i < c; i++) {
    Node *n = new Node;
    n->role = roles.get(i);
    n->parent = parent_node;

    parent_node->children.push_back(n);
    add_role_children_to_node(n);
  }
}

void RoleTreeBE::refresh() {
  /*
    //delete _root;

    _root= new Node;

    grt::ListRef<db_Role> roles(_catalog.roles());
    _root->role= db_Role(_catalog.grt());
    _root->role.childRoles(grt::ListRef<db_Role>(_catalog.grt()));

    for (size_t c= roles.count(), i= 0; i < c; i++)
    {
      //if (!roles[i]->parentRole().is_valid()) // top-level role
      {
        Node *n= new Node;
        n->role= roles[i];
        n->parent= _root;

        _root->children.push_back(n);
        _root->role.childRoles().insert(n->role, -1);
        add_role_children_to_node(n);
      }
    }
  */

  std::map<std::string, Node *> nodes;

  delete _root;

  _root = new Node;

  grt::ListRef<db_Role> roles(_catalog->roles());

  for (size_t c = roles.count(), i = 0; i < c; i++) {
    Node *n = new Node;
    n->role = roles[i];

    nodes[n->role.id()] = n;
  }

  for (size_t c = roles.count(), i = 0; i < c; i++) {
    if (roles[i]->parentRole().is_valid()) {
      if (nodes.find(roles[i]->parentRole().id()) == nodes.end()) {
        Node *next_node = nodes[roles[i]->id()];
        _root->children.push_back(next_node);
        next_node->parent = _root;
      } else {
        Node *parent = nodes[roles[i]->parentRole().id()];
        Node *next_node = nodes[roles[i]->id()];
        parent->children.push_back(next_node);
        next_node->parent = parent;
      }
    } else {
      Node *next_node = nodes[roles[i]->id()];
      _root->children.push_back(nodes[roles[i]->id()]);
      next_node->parent = _root;
    }
  }
}

size_t RoleTreeBE::count_children(const NodeId &parent) {
  Node *node = get_node_with_id(parent);

  if (node)
    return (int)node->children.size();

  return 0;
}

NodeId RoleTreeBE::get_child(const NodeId &parent, size_t index) {
  Node *n = get_node_with_id(parent);
  if (n && index < n->children.size())
    return NodeId(parent).append(index);
  if (n)
    throw std::logic_error("invalid index");

  return NodeId();
}

bool RoleTreeBE::set_field(const NodeId &node, ColumnId column, const std::string &value) {
  Node *n;

  switch ((Columns)column) {
    case Enabled:
      return false;

    case Name:
      n = get_node_with_id(node);
      if (n) {
        // Switched to AutoUndoEdit allow for Live editors to not track undo, but
        // RoleTree is not used by live editors, so it's ok to keep AutoUndo here.
        grt::AutoUndo undo;
        //    AutoUndoEdit undo(this);

        n->role->name(value);

        undo.end(strfmt(_("Rename Role to '%s'"), value.c_str()));
        return true;
      }
      return false;
  }
  return false;
}

grt::Type RoleTreeBE::get_field_type(const NodeId &node, ColumnId column) {
  switch ((Columns)column) {
    case Enabled:
      return grt::IntegerType;
    case Name:
      return grt::StringType;
  }
  throw std::logic_error("Invalid column");
}

bool RoleTreeBE::get_field_grt(const NodeId &node, ColumnId column, grt::ValueRef &value) {
  Node *n;

  switch ((Columns)column) {
    case Enabled:
      n = get_node_with_id(node);
      if (n) {
        if (_object_id.empty()) { // returns whether there is anything in the privs list
          value = grt::IntegerRef(n->role->privileges().count() > 0 ? 1 : 0);
        } else {
          // returns whether the object is in the privs list of the role
          bool flag = false;
          for (size_t c = n->role->privileges().count(), i = 0; i < c; i++) {
            db_DatabaseObjectRef obj(n->role->privileges().get(i)->databaseObject());
            if (obj.is_valid() && obj.id() == _object_id) {
              flag = true;
              break;
            }
          }
          value = grt::IntegerRef(flag ? 1 : 0);
        }
        return true;
      }
      return false;

    case Name:
      n = get_node_with_id(node);
      if (n) {
        value = n->role->name();
        return true;
      }
      return false;
  }

  return false;
}

RoleTreeBE::Node *RoleTreeBE::get_node_with_id(const NodeId &node) {
  Node *n = _root;

  if (!n)
    return 0;

  if (node.depth() == 0)
    return n;

  for (size_t i = 0; i < node.depth(); i++) {
    if (node[i] >= n->children.size())
      throw std::logic_error("Invalid node id");

    n = n->children[node[i]];
  }
  return n;
}

bool RoleTreeBE::find_role(const RoleTreeBE::Node *node, const db_RoleRef &role, bec::NodeId &path) {
  int i = 0;

  if (node->role == role)
    return true;

  for (std::vector<RoleTreeBE::Node *>::const_iterator r = node->children.begin(); r != node->children.end();
       ++r, i++) {
    if (find_role(*r, role, path)) {
      path.prepend(i);
      return true;
    }
  }
  return false;
}

NodeId RoleTreeBE::node_id_for_role(const db_RoleRef &role) {
  NodeId node;
  if (find_role(_root, role, node))
    return node;
  return NodeId();
}

void RoleTreeBE::erase_node(const NodeId &node) {
  Node *n = get_node_with_id(node);
  if (!n)
    return;

  Node *parent = n->parent;
  if (!parent)
    return;

  parent->erase_child(n);
}

bool RoleTreeBE::is_parent_child(Node *parent, Node *child) {
  for (; child; child = child->parent) {
    if (child->parent == parent)
      return true;
  }
  return false;
}

void RoleTreeBE::append_child(const NodeId &parent, const NodeId &child) {
  Node *p = get_node_with_id(parent);
  Node *c = get_node_with_id(child);

  if (!p || !c || is_parent_child(c, p))
    return;

  erase_node(child);

  p->append_child(c);
}

void RoleTreeBE::insert_node_after(const NodeId &after, const NodeId &node) {
  Node *n = get_node_with_id(node);
  Node *a = get_node_with_id(after);

  if (!n || !a)
    return;

  erase_node(node);

  a->parent->insert_child_after(a, n);
}

void RoleTreeBE::insert_node_before(const NodeId &before, const NodeId &node) {
  Node *n = get_node_with_id(node);
  Node *b = get_node_with_id(before);

  if (!n || !b)
    return;

  erase_node(node);

  b->parent->insert_child_before(b, n);
}

void RoleTreeBE::move_to_top_level(const NodeId &node) {
  Node *n = get_node_with_id(node);
  if (n != NULL)
    n->role->parentRole(db_RoleRef());
}
