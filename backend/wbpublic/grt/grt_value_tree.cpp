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

#include "grt_value_tree.h"

#include <grtpp_util.h>

using namespace grt;
using namespace bec;

/*
 * The value tree backend holds a GRT value tree and provides methods
 * for accessing its contents in a way that's convenient for a tree view.
 * 
 * We do not hold references to displayed values, to avoid defering the
 * release/destruction of objects just because it's shown in the tree.
 * 
 * Instead we cache the grt value info that's to be displayed. When a tree
 * node is expanded, the tree is scanned and the values cached. When 
 * a node is collapsed, all the cached info is freed.
 * 
 */

//--------------------------------------------------------------------------------------------------

inline bool is_null_node(const NodeId &id)
{
  return id.depth() == 0;
}

//--------------------------------------------------------------------------------------------------

static bool count_simple_members(const MetaClass::Member *member, int *c)
{
  if (is_simple_type(member->type.base.type))
    (*c)++;
  return true;
}

//--------------------------------------------------------------------------------------------------

static int count_container_nodes(const ValueRef &value)
{
  int count= 0;

  if (!value.is_valid())
    return 0;

  switch (value.type())
  {
  case ListType:
    {
      BaseListRef l(BaseListRef::cast_from(value));

      for (size_t c= l.count(), i= 0; i < c; i++)
      {
        if (!is_simple_type(l[i].type()))
          count++;
      }
    }
    break;
  case DictType:
    {
      DictRef d(DictRef::cast_from(value));

      for (DictRef::const_iterator iter= d.begin(); iter != d.end(); ++iter)
      {
        if (!is_simple_type(iter->second.type()))
          count++;
      }
    }
    break;
  case ObjectType:
    {
      ObjectRef o(ObjectRef::cast_from(value));
      MetaClass *meta= o.get_metaclass();

      meta->foreach_member(boost::bind(&count_simple_members, _1, &count));
    }
    break;

  default:
    break;
  }
  
  return count;
}

//--------------------------------------------------------------------------------------------------

ValueTreeBE::ValueTreeBE(GRT *grt)
  : _grt(grt)
{
  _show_captions= false;
  _is_global_path= false;
}

//--------------------------------------------------------------------------------------------------

ValueTreeBE::~ValueTreeBE()
{
  _root.reset_children();
}

//--------------------------------------------------------------------------------------------------

void ValueTreeBE::show_captions(bool flag)
{
  _show_captions= true;
}

//--------------------------------------------------------------------------------------------------

void ValueTreeBE::set_displayed_global_value(const std::string &path, bool show_root)
{
  ValueRef value;
  
  if (!path.empty())
    value= _grt->get(path);
  
  _show_root_node= show_root;
  _is_global_path= true;
  _root.name= path;
  _root.path= path;
  _root.reset_children();
  if (path.empty())
  {
    _root_value.clear();
    _root.expandable= false;
  }
  else
  {
    _root_value= value;
    _root.expandable= count_container_nodes(_root_value)>0;
  }
  expand_node(get_root());

  refresh();
}

//--------------------------------------------------------------------------------------------------

void ValueTreeBE::set_displayed_value(const ValueRef &value, const std::string &name)
{
  _show_root_node= !name.empty();
  _is_global_path= false;
  if (value.is_valid())
  {
    _root.name= name;
    _root.path= "/";
    _root.reset_children();
    _root_value= value;
    _root.expandable= count_container_nodes(_root_value)>0;
    expand_node(get_root());
  }
  else
  {
    _root.name= name+" (invalid)";
    _root.path= "/";
    _root.reset_children();
    _root.expandable= true;
    _root_value.clear();
  }

  refresh();
}

//--------------------------------------------------------------------------------------------------

void ValueTreeBE::set_node_filter(const boost::function<bool(NodeId, std::string, ValueRef, std::string&, IconId&)> &filter)
{
  _node_filter= filter;
}

//--------------------------------------------------------------------------------------------------

NodeId ValueTreeBE::get_root() const
{
  if (_show_root_node)
    return TreeModel::get_root();
  
  return NodeId(0);
}

//--------------------------------------------------------------------------------------------------

size_t ValueTreeBE::count_children(const NodeId &node_id)
{
  if (is_null_node(node_id))
    return 1; // root node

  Node *node = get_node_for_id(node_id);

  if (node == NULL)
    return 0;

  return node->subnodes.size();
}

//--------------------------------------------------------------------------------------------------

Type ValueTreeBE::get_field_type(const NodeId &node_id, ColumnId column)
{
  switch (column)
  {
  case Name:
    return StringType;
  case Type:
    return IntegerType;
  }
  return AnyType;
}

//--------------------------------------------------------------------------------------------------

NodeId ValueTreeBE::get_child(const NodeId &parent_id, size_t index)
{
  Node *node;

  node= get_node_for_id(parent_id);
  if (!node)
  {
    if (is_null_node(parent_id))
      return NodeId(index);
    throw std::out_of_range("Invalid parent node");
  }

  if (index < node->subnodes.size())
    return NodeId(parent_id).append(index);

  throw std::out_of_range("Attempt to access invalid child");
}

//--------------------------------------------------------------------------------------------------

bool ValueTreeBE::is_expandable(const NodeId &node_id)
{
  Node *node = get_node_for_id(node_id);
  if (!node && is_null_node(node_id))
    return true;
  else if (!node)
    throw std::out_of_range("Invalid node");

  return node->expandable;
}

//--------------------------------------------------------------------------------------------------

bool ValueTreeBE::is_expanded(const NodeId &node_id)
{
  Node *node = get_node_for_id(node_id);
  if (node == NULL)
    return false;

  return node->expanded;
}

//--------------------------------------------------------------------------------------------------

bool ValueTreeBE::get_row(const NodeId &node_id,
                          std::string &name,
                          std::string &type)
{
  Node *info;
  
  if (!(info= get_node_for_id(node_id)))
    return false;

  name= info->name;
  type= info->type;

  return true;
}

//--------------------------------------------------------------------------------------------------

ValueRef ValueTreeBE::get_node_value(const NodeId &node_id)
{
  std::string path;

  if (!_root_value.is_valid())
    return ValueRef();

  path= get_path_for_node(node_id);
  if (path.empty())
    return ValueRef();

  return get_value_by_path(_root_value, path);
}

//--------------------------------------------------------------------------------------------------

bool ValueTreeBE::get_field_grt(const NodeId &node, ColumnId column, ValueRef &value)
{
  Node *info= get_node_for_id(node);
  
  if (!info)
    return false;
  
  switch ((ValueTreeColumns)column)
  {
  case Name:
    value= StringRef(info->name);
    return true;
  case Type:
    value= StringRef(info->type);
    return true;
  default:
    return false;
  }
}

//--------------------------------------------------------------------------------------------------

IconId ValueTreeBE::get_field_icon(const NodeId &node_id, ColumnId column, IconSize size)
{
  if ((ValueTreeColumns)column == Name)
  {
    Node *info= get_node_for_id(node_id);
    if (info)
    {
      if (size == Icon16)
        return info->small_icon;
      else
        return info->large_icon;
    }
  }
  return 0;
}

//--------------------------------------------------------------------------------------------------

bool ValueTreeBE::get_field(const NodeId &node_id, ColumnId column, std::string &value)
{
  Node *info;
  
  if (!(info= get_node_for_id(node_id)))
    return false;

  switch (column)
  {
  case Name:
    value= info->name;
    return true;
  case Type:
    value= info->type;
    return true;
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

void ValueTreeBE::fill_node_info(const ValueRef &value,
                                 Node *info)
{
  info->type = type_to_str(value.type());
  info->expandable = count_container_nodes(value) > 0;

  switch (value.type())
  {
  case ListType:
    {
      BaseListRef l(BaseListRef::cast_from(value));
      std::string struct_name;
      if (l.content_type() != AnyType)
      {
        info->type+= " [";
        if (l.content_type() == ObjectType)
        {
          if (l.content_class_name().empty())
          {
            info->type+= "object";
            struct_name= "";
          }
          else
          {
            info->type+= "object:"+l.content_class_name();
            struct_name= l.content_class_name();
          }
        }
        else
        {
          if (l.content_type() == AnyType)
            info->type+= "*";
          else
            info->type+= type_to_str(l.content_type());
        }
        info->type+= "]";
      }
      if (struct_name.empty())
        info->small_icon= IconManager::get_instance()->get_icon_id("grt_list.png", Icon16);
      else
        info->small_icon= IconManager::get_instance()->get_icon_id(_grt->get_metaclass(struct_name),
                                                             Icon16, "many_$");
    }
    break;
    
  case DictType:
    {
      DictRef d(DictRef::cast_from(value));
      if (d.content_type() != AnyType)
      {
        info->type+= " [";
        if (d.content_type() == ObjectType)
        {
          info->type+= "object:"+d.content_class_name();
          info->small_icon= IconManager::get_instance()->get_icon_id(_grt->get_metaclass(d.content_class_name()), Icon16);
        }
        else
          info->type+= type_to_str(d.content_type());
        info->type+= "]";
      }
      if (info->small_icon == 0)
        info->small_icon= IconManager::get_instance()->get_icon_id("grt_dict.png", Icon16);
    }
    break;

  case ObjectType:
    {
      ObjectRef o(ObjectRef::cast_from(value));
      info->type+= ":" + std::string(o.class_name());

      info->small_icon= IconManager::get_instance()->get_icon_id(o, Icon16);
      if (info->small_icon == 0)
        info->small_icon= IconManager::get_instance()->get_icon_id("grt_object.png", Icon16);
      info->large_icon= IconManager::get_instance()->get_icon_id(o, Icon48);
      if (info->large_icon == 0)
        info->large_icon= IconManager::get_instance()->get_icon_id("grt_object.png", Icon48);
    }
    break;
    
  default:
    info->small_icon= IconManager::get_instance()->get_icon_id("grt_simple_type.png", Icon16);
    info->large_icon= IconManager::get_instance()->get_icon_id("grt_simple_type.png", Icon48);
    break;
  }
}

//--------------------------------------------------------------------------------------------------

void ValueTreeBE::rescan_node(const NodeId &node_id, Node *node,
                              const std::string &path,
                              const BaseListRef &value)
{
  node->reset_children();
  for (size_t i= 0; i < value->count(); i++)
  {
    ValueRef v= value.get(i);
    std::string label;
    IconId icon= 0;

    std::string local_path = base::to_string(i);
    if (v.is_valid() && !is_simple_type(v.type()) &&
      (!_node_filter || _node_filter(node_id, local_path, v, label, icon)))
    {
      Node *child= new Node();

      fill_node_info(v, child);
      child->path = local_path;
      child->name= label.empty() ? child->path : label;
      child->small_icon= icon != 0 ? icon : child->small_icon;
      child->large_icon= icon != 0 ? icon : child->large_icon;
      if (v.type() == ObjectType && label.empty())
      {
        ObjectRef o(ObjectRef::cast_from(v));
        if (o.has_member("name") && o.get_string_member("name") != "")
          child->name= o.get_string_member("name");
        else        
          child->name= "["+child->path+"]";
      }
      node->subnodes.push_back(child);
    }
  }
}

//--------------------------------------------------------------------------------------------------

void ValueTreeBE::rescan_node(const NodeId &node_id, Node *node,
                              const std::string &path,
                              const DictRef &value)
{
  node->reset_children();
  for (DictRef::const_iterator item= value.begin(); item != value.end(); ++item)
  {
    std::string key(item->first);
    ValueRef v(item->second);
    std::string label;
    IconId icon= 0;

    if (v.is_valid() && !is_simple_type(v.type()) &&
        (!_node_filter || _node_filter(node_id, key, v, label, icon)))
    {
      Node *child= new Node();

      fill_node_info(v, child);
      child->path= key;
      child->name= label.empty() ? child->path : label;
      child->small_icon= icon != 0 ? icon : child->small_icon;
      child->large_icon= icon != 0 ? icon : child->large_icon;
      if (v.type() == ObjectType && label.empty())
      {
        ObjectRef o(ObjectRef::cast_from(v));
        if (o.has_member("name") && o.get_string_member("name") != "")
          child->name= o.get_string_member("name");
        else
          child->name= "["+child->path+"]";
      }
      node->subnodes.push_back(child);
    }
  }
}

//--------------------------------------------------------------------------------------------------

bool ValueTreeBE::rescan_member(const MetaClass::Member *mem, const NodeId &node_id, Node *node, const ObjectRef &value)
{
  std::string name(mem->name);
  ValueRef v(value.get_member(name));
  std::string label;
  IconId icon= 0;

  if (v.is_valid() && !is_simple_type(v.type()) && 
      (!_node_filter || _node_filter(node_id, name, v, label, icon)))
  {
    Node *child= new Node();
    
    fill_node_info(v, child);
    child->path= name;
    child->name= label.empty() ? child->path : label;
    child->small_icon= icon != 0 ? icon : child->small_icon;
    child->large_icon= icon != 0 ? icon : child->large_icon;
    node->subnodes.push_back(child);
  }
  
  return true;
}

//--------------------------------------------------------------------------------------------------

void ValueTreeBE::rescan_node(const NodeId &node_id, Node *node,
                              const std::string &path,
                              const ObjectRef &value)
{
  MetaClass *meta= value.get_metaclass();
  
  node->reset_children();
  meta->foreach_member(boost::bind(&ValueTreeBE::rescan_member, this, _1, node_id, node, value));
}

//--------------------------------------------------------------------------------------------------

bool ValueTreeBE::expand_node(const NodeId &node_id)
{
  Node *node;
  
  if (is_null_node(node_id))
    return true;

  if (!(node= get_node_for_id(node_id)))
    return false;

  node->expanded = true;

  ValueRef value(get_node_value(node_id));
  if (!value.is_valid())
    return false;

  switch (value.type())
  {
  case ListType:
    rescan_node(node_id, node, node->path, BaseListRef::cast_from(value));
    break;
  case DictType:
    rescan_node(node_id, node, node->path, DictRef::cast_from(value));
    break;
  case ObjectType:
    rescan_node(node_id, node, node->path, ObjectRef::cast_from(value));
    break;
  default:
    return false;
  }
  return true;
}

//--------------------------------------------------------------------------------------------------

void ValueTreeBE::collapse_node(const NodeId &node_id)
{
  Node *node;

  if (!(node= get_node_for_id(node_id)))
    return;

  node->expanded = false;
  node->reset_children();
}

//--------------------------------------------------------------------------------------------------

void ValueTreeBE::set_activate_callback(const boost::function<void(ValueRef)> &activate_callback)
{
  _activate_callback= activate_callback;
}

//--------------------------------------------------------------------------------------------------

bool ValueTreeBE::activate_node(const NodeId &node_id)
{
  ValueRef value(get_node_value(node_id));
  if (value.is_valid())
  {
    _activate_callback(value);
    return true;
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

ValueTreeBE::Node* ValueTreeBE::get_node_for_id(const NodeId &id)
{
  if (is_null_node(id))
    return 0;

  Node *node= &_root;
  for (size_t c = get_node_depth(id), i = 1; i < c; i++)
  {
    if (node->subnodes.size() <= id[i])
      return 0;

    node= node->subnodes[id[i]];
  }
  return node;
}

//--------------------------------------------------------------------------------------------------

std::string ValueTreeBE::get_path_for_node(const NodeId &id, bool full)
{
  if (is_null_node(id))
    return "";

  Node *node= &_root;
  std::string path;

  if (full)
    path= node->path;
  else
    path= "";
  for (size_t i= 1; i < get_node_depth(id); i++)
  {
    if (node->subnodes.size() <= id[i])
      return "";

    node= node->subnodes[id[i]];
    if (!node->path.empty() && node->path[0] == '/')
      path= node->path;
    else
    {
      if (path == "/")
        path.append(node->path);
      else
        path.append("/").append(node->path);
    }
  }

  if (path.empty())
    path= "/";
  
  return path;
}

//--------------------------------------------------------------------------------------------------

void ValueTreeBE::get_expanded_nodes(std::vector<NodeId> &expanded, const NodeId &path, const Node *node)
{
  NodeId id= path;
  int i= 0;

  id.append(0);
  for (std::vector<Node*>::const_iterator iter= node->subnodes.begin();
       iter != node->subnodes.end(); ++iter, ++i)
  {
    if ((*iter)->expanded)
    {
      id[id.depth()-1] = i;
      expanded.push_back(path);
    }
  }

  for (std::vector<Node*>::const_iterator iter= node->subnodes.begin();
       iter != node->subnodes.end(); ++iter, ++i)
  {
    if ((*iter)->subnodes.size() > 0)
    {
      id[id.depth()-1]= i;
      get_expanded_nodes(expanded, id, *iter);
    }
  }
}

//--------------------------------------------------------------------------------------------------

void ValueTreeBE::refresh()
{
  if (_root.path.empty())
    _root_value.clear();
  else
  {
    if (_is_global_path)
      _root_value= _grt->get(_root.path);
  }
  std::vector<NodeId> expanded_nodes;
  NodeId id= get_root();

  id.append(0);

  // 1st record what nodes are expanded
  expanded_nodes.push_back(get_root());

  if (_root.subnodes.size() > 0)
    expanded_nodes.push_back(id);
  get_expanded_nodes(expanded_nodes, id, &_root);

  // collapse root node
  _root.reset_children();
  _root.expandable = count_container_nodes(_root_value) > 0;
  
  // re-expand nodes
  for (std::vector<NodeId>::const_iterator i= expanded_nodes.begin();
       i != expanded_nodes.end(); ++i)
    expand_node(*i);

  tree_changed();
}

//--------------------------------------------------------------------------------------------------
