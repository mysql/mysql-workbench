/* 
 * Copyright (c) 2011, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "grt/grt_value_tree.h"
#include "wb_helpers.h"

using namespace grt;
using namespace bec;

#include "grt_values_test_data.h"


BEGIN_TEST_DATA_CLASS(be_inspector_value_tree)
public:
  GRT grt;

TEST_DATA_CONSTRUCTOR(be_inspector_value_tree)
{
}

END_TEST_DATA_CLASS;


TEST_MODULE(be_inspector_value_tree, "grt value tree inspector backend");


// Tests the Value Tree
// --------------------

TEST_FUNCTION(1)
{
  // Load test meta classes.
  grt.load_metaclasses("data/structs.test.xml");

  // Consolidate the loaded classes.
  grt.end_loading_metaclasses();

  ValueRef tree= create_grt_tree1(&grt);
  grt.set_root(tree);
}


TEST_FUNCTION(2)
{ // test node traversal
  std::string name;
  
  ValueTreeBE *tree= new ValueTreeBE(&grt);
  tree->set_displayed_global_value("/");

  ensure_equals("root items", tree->count(), 1U);

  name= tree->get_path_for_node(tree->get_root());
  ensure_equals("path", name, "");

  name= tree->get_path_for_node(tree->get_child(tree->get_root(), 0));
  ensure_equals("path", name, "/");

  delete tree;
}


TEST_FUNCTION(5)
{ // display dict
  NodeId node, node1, node2, tnode;
  bool flag;
  std::string name, type, icon;
  size_t i;

  ValueTreeBE *tree= new ValueTreeBE(&grt);

  tree->set_displayed_global_value("/");

  // counts
  ensure_equals("root items", tree->count(), 1U);

  tnode= tree->get_child(tree->get_root(), 0);
  
  ensure_equals("root child items", tree->count_children(tnode), 0U);
  
  flag= tree->is_expandable(tnode);
  ensure("root is expandable", flag);
  
  flag= tree->expand_node(tnode);
  ensure("expand root child", flag);

  ensure_equals("root child items after expand", tree->count_children(tnode), 3U);

  grt.set("/test", DictRef(&grt));
  ensure_equals("root child items after add", tree->count_children(tnode), 3U);
  tree->refresh();
  ensure_equals("root child items after add + refresh", tree->count_children(tnode), 4U);

  flag= tree->get_row(tnode, name, type);
  ensure("root node", flag);
  ensure_equals("root node name", name, "/");

  

  ensure_equals("get child",
                tree->get_node_depth(tree->get_child(tree->get_child(tree->get_root(), 0), 0)),
                2U);


  node= tree->get_child(tnode, 0);
  ensure("get node 0", node.is_valid());

  flag= tree->get_row(node, name, type);
  ensure("get row 0", flag);
  ensure_equals("row 0 name", name, "books");
  ensure_equals("row 0 type", type, "list [object:test.Book]");

  ValueRef v= tree->get_grt_value(node, 0);
  ensure("get row 0 value", v.is_valid());


  flag= tree->get_row(tree->get_child(tnode, 1), name, type);
  ensure("get row 1", flag);
  ensure_equals("row 1 name", name, "somedict");
  ensure_equals("row 1 type", type, "dict");

  flag= tree->get_row(tree->get_child(tnode, 2), name, type);
  ensure("get row 2", flag);
  ensure_equals("row 2 name", name, "somelist");
  ensure_equals("row 2 type", type, "list");


  node= tree->get_child(tnode, 0);
  i= tree->count_children(node);
  ensure_equals("row 0 children (collapsed)", i, 0U);
  tree->expand_node(node);
  i= tree->count_children(node);
  ensure_equals("row 0 children", i, 2U);

  node1= tree->get_child(node, 0);
  flag= tree->get_field(node1, ValueTreeBE::Name, name);
  ensure("get field", flag);
  flag= tree->get_field(node1, ValueTreeBE::Type, type);
  ensure("get field", flag);
  ensure_equals("row 0.0 name", name, "[0]");
  ensure_equals("row 0.0 type", type, "object:test.Book");

  tree->expand_node(node1);
  
  node2= tree->get_child(node1, 0);
  flag= tree->get_field(node2, ValueTreeBE::Name, name);
  ensure("get field", flag);
  flag= tree->get_field(node2, ValueTreeBE::Type, type);
  ensure("get field", flag);
  ensure_equals("row 0.0.0 name", name, "authors");
  ensure_equals("row 0.0.0 type", type, "list [object:test.Author]");

  

  node1= tree->get_child(node, 1);
  flag= tree->get_field(node1, ValueTreeBE::Name, name);
  ensure("get field", flag);
  flag= tree->get_field(node1, ValueTreeBE::Type, type);
  ensure("get field", flag);
  ensure_equals("row 0.1 name", name, "[1]");
  ensure_equals("row 0.1 type", type, "object:test.Book");



  node= tree->get_child(tnode, 1);
  i= tree->count_children(node);
  ensure_equals("row 1 children (collapsed)", i, 0U);
  tree->expand_node(node);
  i= tree->count_children(node);
  ensure_equals("row 1 children", i, 1U);

  node1= tree->get_child(node, 0);
  flag= tree->get_field(node1, ValueTreeBE::Name, name);
  ensure("get field", flag);
  flag= tree->get_field(node1, ValueTreeBE::Type, type);
  ensure("get field", flag);
  ensure_equals("row 1.0 name", name, "k6");
  ensure_equals("row 1.0 type", type, "list [int]");



  node= tree->get_child(tnode, 2);
  i= tree->count_children(node);
  ensure_equals("row 2 children (collapsed)", i, 0U);
  tree->expand_node(node);
  i= tree->count_children(node);
  ensure_equals("row 2 children", i, 2U);

  node1= tree->get_child(node, 0);
  flag= tree->get_field(node1, ValueTreeBE::Name, name);
  ensure("get field", flag);
  flag= tree->get_field(node1, ValueTreeBE::Type, type);
  ensure("get field", flag);
  ensure_equals("row 2.0 name", name, "5");
  ensure_equals("row 2.0 type", type, "list [int]");

  node1= tree->get_child(node, 1);
  flag= tree->get_field(node1, ValueTreeBE::Name, name);
  ensure("get field", flag);
  flag= tree->get_field(node1, ValueTreeBE::Type, type);
  ensure("get field", flag);
  ensure_equals("row 2.1 name", name, "6");
  ensure_equals("row 2.1 type", type, "dict");

  
  
  delete tree;
}


TEST_FUNCTION(6)
{ // display list
  NodeId node, node1;
  bool flag;
  std::string name, type, icon;

  ValueTreeBE *tree= new ValueTreeBE(&grt);

  tree->set_displayed_global_value("/somelist");

  // counts
  ensure_equals("root items", tree->count(), 1U);

  node= tree->get_child(tree->get_root(), 0);
  tree->expand_node(node);
  
  flag= tree->get_field(node, ValueTreeBE::Name, name);
  ensure("get root field name", flag);
  ensure_equals("root fileld name", name, "/somelist");

  
  node1= tree->get_child(node, 0);
  flag= tree->get_field(node1, ValueTreeBE::Name, name);
  ensure("get field", flag);
  flag= tree->get_field(node1, ValueTreeBE::Type, type);
  ensure("get field", flag);
  ensure_equals("row 0 name", name, "5");
  ensure_equals("row 0 type", type, "list [int]");

  node1= tree->get_child(node, 1);
  flag= tree->get_field(node1, ValueTreeBE::Name, name);
  ensure("get field", flag);
  flag= tree->get_field(node1, ValueTreeBE::Type, type);
  ensure("get field", flag);
  ensure_equals("row 1 name", name, "6");
  ensure_equals("row 1 type", type, "dict");

  

  delete tree;
}


TEST_FUNCTION(7)
{ // display object
  NodeId node;

  ValueTreeBE *tree= new ValueTreeBE(&grt);

  tree->set_displayed_global_value("/books");

  // counts
  ensure_equals("root items", tree->count(), 1U);

  


  delete tree;
}



END_TESTS

