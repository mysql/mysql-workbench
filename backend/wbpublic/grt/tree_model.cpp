/* 
 * Copyright (c) 2007, 2010, Oracle and/or its affiliates. All rights reserved.
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

#include "stdafx.h"

#include "tree_model.h"
#include <grtpp_util.h>

#ifdef _WIN32 
#include <windows.h> // for OutputDebugString
#endif

using namespace grt;
using namespace bec;

Pool<std::vector<int> >   *NodeId::_pool = 0;

//----------------------------------------------------------------------

Type ListModel::get_field_type(const NodeId &node, int column)
{
  throw std::logic_error("not implemented");
}


bool ListModel::get_field_grt(const NodeId &node, int column, grt::ValueRef &value)
{
  return false;
}


bool ListModel::get_field(const NodeId &node, int column, std::string &value)
{
  ValueRef v;

  if (!get_field_grt(node, column, v))
    return false;

  value= v.repr();

  return true;
}


bool ListModel::get_field(const NodeId &node, int column, int &value)
{
  ValueRef v;

  if (!get_field_grt(node, column, v))
    return false;

  switch (v.type())
  {
  case IntegerType:
    value= IntegerRef::cast_from(v);
    return true;

  default:
    value= 999999999;
    return false;
  }
}


bool ListModel::get_field(const NodeId &node, int column, long long &value)
{
  ValueRef v;

  if (!get_field_grt(node, column, v))
    return false;

  switch (v.type())
  {
  case IntegerType:
    value= IntegerRef::cast_from(v);
    return true;

  default:
    value= 999999999;
    return false;
  }
}


bool ListModel::get_field(const NodeId &node, int column, bool &value)
{
  int i;
  if (!get_field(node, column, i))
    return false;
  
  value= i!=0;
  return true;
}

bool ListModel::get_field(const NodeId &node, int column, double &value)
{
  ValueRef v;

  if (!get_field_grt(node, column, v))
    return false;

  switch (v.type())
  {
  case DoubleType:
    value= DoubleRef::cast_from(v);
    return true;

  case IntegerType:
    value= (double)(int)IntegerRef::cast_from(v);
    return true;

  default:
    value= 999999999.99999;
    return false;
  }
}


ValueRef ListModel::get_grt_value(const NodeId &node, int column)
{
  ValueRef value;

  get_field_grt(node, column, value);

  return value;
}


std::string ListModel::get_field_description(const NodeId &node, int column)
{
  return "";
}


IconId ListModel::get_field_icon(const NodeId &node, int column, IconSize size)
{
  return 0;
}


bool ListModel::set_field(const NodeId &node, int column, const std::string &value)
{ 
  return false;
}


bool ListModel::set_field(const NodeId &node, int column, int value) 
{
  return false; 
}


bool ListModel::set_field(const NodeId &node, int column, long long value) 
{
  return false; 
}


bool ListModel::set_field(const NodeId &node, int column, double value)
{
  return false; 
}

bool ListModel::set_convert_field(const NodeId &node, int column, const std::string &value)
{
  switch (get_field_type(node, column))
  {
  case IntegerType:
    return set_field(node, column, atoi(value.c_str()));

  case DoubleType:
    return set_field(node, column, atof(value.c_str()));

  case StringType:
    return set_field(node, column, value);

  default:
    return false;
  }
  
  return true;
}


NodeId ListModel::get_node(int index)
{
  return index;
}


bool ListModel::has_next(const NodeId &node)
{
  return node[0] + 1 < count();
}


NodeId ListModel::get_next(const NodeId &node)
{
  if (node[0] + 1 < count())
    return node[0] + 1;
  throw std::out_of_range("invalid child");
}


ValueRef ListModel::parse_value(Type type, const std::string &value)
{
  switch (type)
  {
  case IntegerType:
    return IntegerRef(atoi(value.c_str()));

  case DoubleType:
    return DoubleRef(atof(value.c_str()));

  case AnyType:
  case StringType:
    return StringRef(value);

  default:
    break;
  }
  return ValueRef();
}

//--------------------------------------------------------------------------------------------------

/**
 * Move the given node one index down in this list.
 */
void ListModel::reorder_up(const NodeId &node)
{
  if (node.end() > 0)
    reorder(node, node.end() - 1);
}

//--------------------------------------------------------------------------------------------------

/**
 * Move the given node one index up in this list.
 */
void ListModel::reorder_down(const NodeId &node)
{
  reorder(node, node.end() + 1);
}

//--------------------------------------------------------------------------------------------------

void ListModel::dump(int show_field)
{
  g_print("\nDumping list model:\n");
  for (int i= 0, c= count(); i < c; i++)
  {
    NodeId child(i);
    std::string value;
    
    if (!get_field(child, show_field, value))
      value= "???";
    
    g_print("- %s\n", value.c_str());
  }  
  g_print("\nFinished dumping list model.");
}


//---------------------------------------------------------------------------


int TreeModel::count()
{
  return count_children(get_root());
}


NodeId TreeModel::get_node(int index)
{
  return get_child(get_root(), index);
}


bool TreeModel::has_next(const NodeId &node)
{
  NodeId parent(get_parent(node));
    
  return node.end() < count_children(parent)-1;
}


NodeId TreeModel::get_next(const NodeId &node)
{
  if (node.depth() < 2)
    return ListModel::get_next(node);
  else
  {
    NodeId parent(get_parent(node));
    
    if (node.end() < count_children(parent)-1)
      return parent.append(node.end()+1);

    throw std::out_of_range("last node");
  }
}

int TreeModel::get_node_depth(const NodeId &node)
{
  return node.depth();
}


NodeId TreeModel::get_root() const
{
  return NodeId();
}


bool TreeModel::expand_node(const NodeId &node)
{
  return false;
}


void TreeModel::collapse_node(const NodeId &node)
{
}


bool TreeModel::is_expanded(const NodeId &node)
{
  return false;
}


bool TreeModel::is_expandable(const NodeId &node_id)
{
  return count_children(node_id) > 0;
}


static void dump_node(TreeModel *model, int show_field, const NodeId &node_id)
{
  for (int i= 0, c= model->count_children(node_id); i < c; i++)
  {
    NodeId child= model->get_child(node_id, i);
    std::string value;
    char* left= (char*) "-"; // Cast needed to avoid deprecation warning with GCC 4.3

    if (!model->get_field(child, show_field, value))
      value= "???";

    if (model->is_expandable(node_id))
      left= (char*) "+";

    g_print("%*s %s\n", child.depth(), left, value.c_str());

    dump_node(model, show_field, child);
  }
}


void TreeModel::dump(int show_field)
{
  g_print("\nDumping tree model:\n");
  dump_node(this, show_field, NodeId());
  g_print("\nFinished dumping tree model.");
}

