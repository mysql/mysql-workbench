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

#pragma once

#include "mforms/treeview.h"

namespace MySQL {
  namespace Forms {
    // class TreeViewNode : mforms::Node
    //{
    // private:
    //  std::string* _mytag;
    //  mforms::TreeNodeData *_data;
    //  std::vector<std::string> _text;
    //  std::vector<mforms::TreeNodeTextAttributes>* _attributes;
    //  std::map<int, void*> _icon;

    // public:
    //  TreeViewNode();
    //  ~TreeViewNode();
    //};
  };
};

class TreeNodeWrapper : public mforms::TreeNode {
  friend struct mforms::TreeNodeRef;

  mforms::TreeNodeRef _parent;
  std::string _tag;
  std::vector<std::string> _values;
  std::vector<mforms::TreeNodeTextAttributes> _attributes;
  std::vector<std::string> _icons;
  std::vector<TreeNodeWrapper *> _children;
  mforms::TreeNodeData *pdata;
  bool _expanded;

  int _index;

protected:
  bool is_root() const;

public:
  TreeNodeWrapper();

  int node_index() const {
    return _index;
  }

  virtual void release();
  virtual void retain();

  virtual bool equals(const mforms::TreeNode &other);
  virtual bool is_valid() const;
  virtual int level() const;

  virtual void set_icon_path(int column, const std::string &icon);

  virtual void set_attributes(int column, const mforms::TreeNodeTextAttributes &attrs);
  virtual void set_string(int column, const std::string &value);
  virtual void set_int(int column, int value);
  virtual void set_long(int column, std::int64_t value);
  virtual void set_bool(int column, bool value);
  virtual void set_float(int column, double value);

  virtual std::string get_string(int column) const;
  virtual int get_int(int column) const;
  virtual std::int64_t get_long(int column) const;
  virtual bool get_bool(int column) const;
  virtual double get_float(int column) const;

  virtual int count() const;
  virtual mforms::TreeNodeRef insert_child(int index);
  virtual void insert_child(int index, const mforms::TreeNode &node);
  virtual void move_child(mforms::TreeNodeRef child, int new_index);
  virtual void remove_from_parent();
  virtual mforms::TreeNodeRef get_child(int index) const;
  virtual int get_child_index(mforms::TreeNodeRef child) const;
  virtual mforms::TreeNodeRef get_parent() const;
  virtual mforms::TreeNodeRef previous_sibling() const;
  virtual mforms::TreeNodeRef next_sibling() const;
  virtual void remove_children();
  virtual void move_node(mforms::TreeNodeRef node, bool before);

  virtual std::vector<mforms::TreeNodeRef> add_node_collection(const mforms::TreeNodeCollectionSkeleton &nodes,
                                                               int position = -1);
  void add_children_from_skeletons(std::vector<TreeNodeWrapper *> &parents,
                                   const std::vector<mforms::TreeNodeSkeleton> &children);

  virtual void expand();
  virtual void collapse();
  virtual bool is_expanded();

  virtual void toggle();

  virtual void set_tag(const std::string &tag);
  virtual std::string get_tag() const;

  virtual void set_data(mforms::TreeNodeData *data);
  virtual mforms::TreeNodeData *get_data() const;
};
