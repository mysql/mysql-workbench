/*
 * Copyright (c) 2012, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "../stub_treenode.h"

#include "base/string_utilities.h"

bool TreeNodeWrapper::is_root() const {
  return true;
};

void TreeNodeWrapper::release() {
}
void TreeNodeWrapper::retain() {
}

bool TreeNodeWrapper::equals(const mforms::TreeNode &other) {
  return true;
}

bool TreeNodeWrapper::is_valid() const {
  return true;
}

int TreeNodeWrapper::level() const {
  return 1;
}

void TreeNodeWrapper::set_icon_path(int column, const std::string &icon) {
  set_string(column + 1, icon);
}

void TreeNodeWrapper::set_attributes(int column, const mforms::TreeNodeTextAttributes &attrs) {
  mforms::TreeNodeTextAttributes attributes;

  while (_attributes.size() < (size_t)(column + 1))
    _attributes.push_back(attributes);

  attributes = attrs;
  _attributes[column] = attributes;
}

void TreeNodeWrapper::set_string(int column, const std::string &value) {
  while (_values.size() < (size_t)(column + 1))
    _values.push_back("");

  _values[column] = value;
}

void TreeNodeWrapper::set_int(int column, int value) {
}
void TreeNodeWrapper::set_long(int column, std::int64_t value) {
}
void TreeNodeWrapper::set_bool(int column, bool value) {
}
void TreeNodeWrapper::set_float(int column, double value) {
}

std::string TreeNodeWrapper::get_string(int column) const {
  return _values.size() > (size_t)column ? _values[column] : "";
}

int TreeNodeWrapper::get_int(int column) const {
  return 0;
}

std::int64_t TreeNodeWrapper::get_long(int column) const {
  return 0;
}

bool TreeNodeWrapper::get_bool(int column) const {
  return true;
}

double TreeNodeWrapper::get_float(int column) const {
  return 0;
}

int TreeNodeWrapper::count() const {
  return (int)_children.size();
}

std::vector<mforms::TreeNodeRef> TreeNodeWrapper::add_node_collection(const mforms::TreeNodeCollectionSkeleton &nodes,
                                                                      int position) {
  std::vector<TreeNodeWrapper *> added_nodes;
  std::vector<mforms::TreeNodeRef> result;

  for (std::vector<std::string>::const_iterator it = nodes.captions.begin(); it != nodes.captions.end(); ++it) {
    TreeNodeWrapper *child = new TreeNodeWrapper();
    child->_parent = mforms::TreeNodeRef(this);

    mforms::TreeNodeRef tmp(child);
    tmp->set_string(0, *it);

    added_nodes.push_back(child);
  }

  // If the new nodes will have children as well, inserts them
  if (nodes.children.size())
    add_children_from_skeletons(added_nodes, nodes.children);

  bool at_end = (position == -1);
  for (size_t index = 0; index < added_nodes.size(); index++) {
    added_nodes[index]->_parent = mforms::TreeNodeRef(this);

    if ((int)_children.size() > position && !at_end)
      _children.insert(_children.begin() + position, added_nodes[index]);
    else
      _children.push_back(added_nodes[index]);

    position++;

    result.push_back(mforms::TreeNodeRef(added_nodes[index]));
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeRef TreeNodeWrapper::insert_child(int index) {
  if (index < 0) {
    TreeNodeWrapper *child = new TreeNodeWrapper();
    child->_parent = mforms::TreeNodeRef(this);
    _children.push_back(child);

    return mforms::TreeNodeRef(child);
  }

  TreeNodeWrapper *child = new TreeNodeWrapper();
  child->_parent = mforms::TreeNodeRef(this);
  _children.insert(_children.begin() + index, child);

  return mforms::TreeNodeRef(child);
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::insert_child(int index, const mforms::TreeNode &node) {
  TreeNodeWrapper *child = (TreeNodeWrapper *)&node;
  child->_parent = mforms::TreeNodeRef(this);
  if (index < 0)
    _children.push_back(child);
  else
    _children.insert(_children.begin() + index, child);
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::move_child(mforms::TreeNodeRef node, int new_index) {
  TreeNodeWrapper *child = (TreeNodeWrapper *)node.ptr();

  std::vector<TreeNodeWrapper *>::iterator i = std::find(_children.begin(), _children.end(), child);
  if (i == _children.end())
    return;

  int old_index = int(i - _children.begin());
  if (old_index == new_index)
    return;

  _children.erase(i);
  if (old_index < new_index)
    --new_index;
  _children.insert(_children.begin() + new_index, child);
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::add_children_from_skeletons(std::vector<TreeNodeWrapper *> &parents,
                                                  const std::vector<mforms::TreeNodeSkeleton> &children) {
  for (size_t child_index = 0; child_index < children.size(); child_index++) {
    // Creates "this" child for each parent
    std::vector<TreeNodeWrapper *> added_nodes;

    for (size_t index = 0; index < parents.size(); index++) {
      TreeNodeWrapper *child = new TreeNodeWrapper();
      child->set_string(0, children[child_index].caption);
      child->set_tag(children[child_index].tag);
      added_nodes.push_back(child);
    }

    // If the new nodes will have childrens as well, inserts them
    if (children[child_index].children.size())
      add_children_from_skeletons(added_nodes, children[child_index].children);

    // Now inserts each children to it's corresponding parent
    for (size_t parent_index = 0; parent_index < parents.size(); parent_index++) {
      added_nodes[parent_index]->_parent = mforms::TreeNodeRef(parents[parent_index]);
      parents[parent_index]->_children.push_back(added_nodes[parent_index]);
    }
  }
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::remove_from_parent() {
  TreeNodeWrapper *inner_parent = dynamic_cast<TreeNodeWrapper *>(_parent.ptr());

  if (inner_parent) {
    if (std::find(inner_parent->_children.begin(), inner_parent->_children.end(), this) !=
        inner_parent->_children.end()) {
      inner_parent->_children.erase(std::find(inner_parent->_children.begin(), inner_parent->_children.end(), this));
    }
  }
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeRef TreeNodeWrapper::get_child(int index) const {
  return (_children.size() > (size_t)index) ? mforms::TreeNodeRef(_children[index]) : mforms::TreeNodeRef();
}

//--------------------------------------------------------------------------------------------------

int TreeNodeWrapper::get_child_index(mforms::TreeNodeRef node) const {
  TreeNodeWrapper *child = (TreeNodeWrapper *)node.ptr();
  std::vector<TreeNodeWrapper *>::const_iterator i = std::find(_children.begin(), _children.end(), child);
  if (i == _children.end())
    return -1;

  return (int)(i - _children.begin());
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeRef TreeNodeWrapper::get_parent() const {
  return _parent;
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeRef TreeNodeWrapper::previous_sibling() const {
  return mforms::TreeNodeRef();
}

mforms::TreeNodeRef TreeNodeWrapper::next_sibling() const {
  return mforms::TreeNodeRef();
}

void TreeNodeWrapper::remove_children() {
  mforms::TreeNode::remove_children();
}

void TreeNodeWrapper::move_node(mforms::TreeNodeRef node, bool before) {
}

void TreeNodeWrapper::expand() {
  _expanded = true;
}

void TreeNodeWrapper::collapse() {
  _expanded = false;
}

bool TreeNodeWrapper::is_expanded() {
  return _expanded;
}

void TreeNodeWrapper::toggle() {
  _expanded = !_expanded;
}

void TreeNodeWrapper::set_tag(const std::string &tag) {
  _tag = tag;
}
std::string TreeNodeWrapper::get_tag() const {
  return _tag;
}

void TreeNodeWrapper::set_data(mforms::TreeNodeData *data) {
  pdata = data;
}

mforms::TreeNodeData *TreeNodeWrapper::get_data() const {
  return pdata;
}

TreeNodeWrapper::TreeNodeWrapper() : pdata(NULL), _expanded(false) {
  _parent = mforms::TreeNodeRef();
}
//------------------------------------------------------------------------------
// void TreeNodeWrapper::init()
//{
//  ::mforms::ControlFactory *f= ::mforms::ControlFactory::get_instance();
//
//}
