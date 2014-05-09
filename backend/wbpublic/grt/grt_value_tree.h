/* 
 * Copyright (c) 2007, 2014, Oracle and/or its affiliates. All rights reserved.
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
#ifndef _GRT_VALUE_TREE_H_
#define _GRT_VALUE_TREE_H_

#include <grtpp.h>

#include "tree_model.h"
#include "wbpublic_public_interface.h"

namespace bec {

class WBPUBLICBACKEND_PUBLIC_FUNC ValueTreeBE : public TreeModel
{
protected:
  class Node
  {
  public:
    std::string name;
    std::string type;
    std::string path;

    IconId small_icon;  // Usually 16 x 16 pixel.
    IconId large_icon;  // Usually 48 x 48 pixels.
    bool expandable;
    bool expanded;
    std::vector<Node*> subnodes;

    Node() : small_icon(0), large_icon(0), expandable(false), expanded(false)
    {
    }

    virtual ~Node()
    {
      reset_children();
    }

    void reset_children()
    {
      for (std::vector<Node*>::iterator i= subnodes.begin(); i!= subnodes.end(); ++i)
        delete *i;
      subnodes.clear();
    }
  private:
    Node(Node&);
    Node& operator = (Node&);
  };

public:
  // The columns to display in the treeview.
  enum ValueTreeColumns
  {
    Name,
    Type
  };

  ValueTreeBE(grt::GRT *grt);
  ~ValueTreeBE();

  virtual NodeId get_root() const;

  // returns the number of children the given node has
  virtual size_t count_children(const NodeId &node_id);

  // returns the child node with the given index, starting with 0
  virtual NodeId get_child(const NodeId &parent_id, size_t index);
  
  // get_field() is used to get the node's field captions. column is ValueTreeColumns.Name or ValueTreeColumns.Type
  virtual bool get_field(const NodeId &node_id, ColumnId column, std::string &value);

  // get_field_icon() is used to get the node's field icon ids. column is ValueTreeColumns.Name or ValueTreeColumns.Type
  virtual IconId get_field_icon(const NodeId &node_id, ColumnId column, IconSize size);

  virtual bool is_expandable(const NodeId &node_id);
  virtual bool is_expanded(const NodeId &node_id);

  // needs to be called when a node gets expanded
  virtual bool expand_node(const NodeId &node_id);

  // needs to be called when a node is collapsed
  virtual void collapse_node(const NodeId &node_id);

  virtual bool activate_node(const NodeId &node);

  // triggers a complete refresh of the tree
  virtual void refresh();

  // function to set the tree's topLevel node to a given GRT value
  void set_displayed_value(const grt::ValueRef &value, const std::string &name);

  // function to set the tree's topLevel node to a given GRT global tree path
  void set_displayed_global_value(const std::string &path, bool show_root= true);

  void show_captions(bool flag); // displays attr:caption instead of member name

  // single function to get all row captions at once
  bool get_row(const NodeId &node, std::string &name, std::string &type);

  // returns the actual Value the node represents. not used in the UI directly.
  grt::ValueRef get_node_value(const NodeId &node_id);

  grt::ValueRef get_root_value() { return _root_value; }

  std::string get_path_for_node(const NodeId &node_id, bool full= false);
  
  void set_node_filter(const boost::function<bool (NodeId, std::string, grt::ValueRef, std::string&, IconId&)> &filter);
  
  void set_activate_callback(const boost::function<void (grt::ValueRef)> &active_callback);

private:
  grt::ValueRef _root_value;

protected:
  grt::GRT *_grt;

  boost::function<bool (NodeId, std::string, grt::ValueRef, std::string&, IconId&)> _node_filter;
  boost::function<void (grt::ValueRef)> _activate_callback;

  Node _root;

  bool _show_root_node;
  bool _show_captions;
  bool _is_global_path;

  virtual grt::Type get_field_type(const NodeId &node_id, ColumnId column);

  Node *get_node_for_id(const NodeId &node_id);

  virtual void fill_node_info(const grt::ValueRef &value, Node *info);
  virtual void rescan_node(const NodeId &node_id, Node *node, const std::string &path, const grt::DictRef &value);
  virtual void rescan_node(const NodeId &node_id, Node *node, const std::string &path, const grt::BaseListRef &value);
  virtual void rescan_node(const NodeId &node_id, Node *node, const std::string &path, const grt::ObjectRef &value);

  bool rescan_member(const grt::MetaClass::Member *mem, const NodeId &node_id, Node *node, const grt::ObjectRef &value);
   
  virtual bool get_field_grt(const NodeId &node, ColumnId column, grt::ValueRef &value);

  void get_expanded_nodes(std::vector<NodeId> &expanded, const NodeId &path, const Node *node);
};

};

#endif /* _GRT_VALUE_TREE_H_ */
