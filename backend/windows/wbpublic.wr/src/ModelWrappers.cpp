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

#include "stdafx.h"

#include "ConvUtils.h"
#include "Grt.h"
#include "GrtTemplates.h"
#include "DelegateWrapper.h"
#include "ModelWrappers.h"

#include "ModelWrappers.h"

using namespace MySQL::Grt;
using namespace MySQL::Workbench;

//--------------------------------------------------------------------------------------------------

NodeId::NodeId(const ::bec::NodeId *inn)
  : inner(new ::bec::NodeId(*inn))
{
}

//--------------------------------------------------------------------------------------------------

NodeId::NodeId()
  : inner(new ::bec::NodeId()) 
{
}

//--------------------------------------------------------------------------------------------------

NodeId::NodeId(int index)
  : inner(new ::bec::NodeId(index))
{
}

//--------------------------------------------------------------------------------------------------

NodeId::NodeId(String ^str)
  : inner(new ::bec::NodeId(NativeToCppString(str)))
{
}

//--------------------------------------------------------------------------------------------------

NodeId::~NodeId()
{
  delete inner;
}

//--------------------------------------------------------------------------------------------------

::bec::NodeId* NodeId::get_unmanaged_object()
{
  return inner;
}

//--------------------------------------------------------------------------------------------------

bool NodeId::operator == (NodeId^ node)
{
  return inner->operator == (*node->inner);
}

//--------------------------------------------------------------------------------------------------

bool NodeId::equals(NodeId^ node)
{
  return inner->operator == (*node->inner);
}

//--------------------------------------------------------------------------------------------------

int NodeId::operator[] (int i)
{
  return inner->operator[](i);
}

//--------------------------------------------------------------------------------------------------

int NodeId::get_by_index (int i)
{
  return inner->operator[](i);
}

//--------------------------------------------------------------------------------------------------

int NodeId::depth()
{
  return inner->depth();
}

//--------------------------------------------------------------------------------------------------

int NodeId::end()
{
  return inner->end();
}

//--------------------------------------------------------------------------------------------------

bool NodeId::previous()
{
  return inner->previous();
}

//--------------------------------------------------------------------------------------------------

bool NodeId::next()
{
  return inner->next();
}

//--------------------------------------------------------------------------------------------------

NodeId^ NodeId::append(int i)
{
  inner->append(i); return this;
}

//--------------------------------------------------------------------------------------------------

bool NodeId::is_valid()
{
  return inner->is_valid();
}

//--------------------------------------------------------------------------------------------------

String^ NodeId::repr()
{
  return CppStringToNative(inner->repr());
}

//----------------- ListModel ----------------------------------------------------------------------

ListModel::ListModel(::bec::ListModel *inn)
  : inner(inn)
{
  native_connections = new std::vector<boost::signals2::connection>();
}

//--------------------------------------------------------------------------------------------------

ListModel::~ListModel()
{
  if (!native_connections)
    return;

  for each (TreeRefreshSlot^ handler in tree_refresh_handlers)
    delete handler;
  tree_refresh_handlers.Clear();

  delete native_connections;
  native_connections = NULL;
}

//--------------------------------------------------------------------------------------------------

bool ListModel::is_valid()
{
  return inner != NULL;
}

//--------------------------------------------------------------------------------------------------

bool ListModel::equals(ListModel^ other)
{
  return (inner == other->inner);
}

//--------------------------------------------------------------------------------------------------

int ListModel::count()
{
  return inner->count();
}

//--------------------------------------------------------------------------------------------------

NodeId^ ListModel::get_node(int index)
{
  return gcnew NodeId(&inner->get_node(index));
}

//--------------------------------------------------------------------------------------------------

bool ListModel::get_field(NodeId^ node, int column, [Out] String^ %value)
{
  std::string str;
  bool retval= inner->get_field(*node->get_unmanaged_object(), column, str);
  if (retval && str.length() > 0)
    value= CppStringToNative(str);
  else
    value = "";
  return retval;
}

//--------------------------------------------------------------------------------------------------

bool ListModel::get_field(NodeId^ node, int column, [Out] int %value)      
{
  int v;
  bool retval= inner->get_field(*node->get_unmanaged_object(), column, v);
  value= v;
  return retval;
}

//--------------------------------------------------------------------------------------------------

bool ListModel::get_field(NodeId^ node, int column, [Out] double %value)
{
  double v;
  bool retval= inner->get_field(*node->get_unmanaged_object(), column, v);
  value= v;
  return retval;
}

//--------------------------------------------------------------------------------------------------

String^ ListModel::get_field_description(NodeId^ node, int column)
{
  return CppStringToNative(inner->get_field_description(*node->get_unmanaged_object(), column));
}

//--------------------------------------------------------------------------------------------------

IconId ListModel::get_field_icon(NodeId^ node, int column, IconSize size)
{
  return inner->get_field_icon(*node->get_unmanaged_object(), column, (bec::IconSize) size);
}

//--------------------------------------------------------------------------------------------------

GrtValue^ ListModel::get_grt_value(NodeId^ node, int column)
{
  return gcnew GrtValue(inner->get_grt_value(*node->get_unmanaged_object(), column));
}

//--------------------------------------------------------------------------------------------------

void ListModel::refresh()
{
  inner->refresh();
}

//--------------------------------------------------------------------------------------------------

void ListModel::reset()
{
  inner->reset();
}

//--------------------------------------------------------------------------------------------------

GrtValueType^ ListModel::get_field_type(NodeId^ node, int column)
{
  return static_cast<GrtValueType>(inner->get_field_type(*node->get_unmanaged_object(), column));
}

//--------------------------------------------------------------------------------------------------

// For editable lists only.
bool ListModel::set_field(NodeId^ node, int column, String^ value)
{
  return inner->set_field(*node->get_unmanaged_object(), column, NativeToCppString(value));
}

//--------------------------------------------------------------------------------------------------

bool ListModel::set_field(NodeId^ node, int column, double value)
{
  return inner->set_field(*node->get_unmanaged_object(), column, value);
}

//--------------------------------------------------------------------------------------------------

bool ListModel::set_field(NodeId^ node, int column, int value)
{
  return inner->set_field(*node->get_unmanaged_object(), column, value);
}

//--------------------------------------------------------------------------------------------------

bool ListModel::set_convert_field(NodeId^ node, int column, String^ value)
{
  return inner->set_convert_field(*node->get_unmanaged_object(), column, NativeToCppString(value));
}

//--------------------------------------------------------------------------------------------------

bool ListModel::activate_node(NodeId^ node)
{
  return inner->activate_node(*node->get_unmanaged_object());
}

//--------------------------------------------------------------------------------------------------

std::vector<bec::NodeId> ListModel::convert_node_list(List<NodeId^> ^nodes)
{
  return ObjectListToCppVector<NodeId, bec::NodeId>(nodes);
}

//--------------------------------------------------------------------------------------------------

List<MySQL::Base::MenuItem^>^ ListModel::get_popup_items_for_nodes(List<NodeId^> ^nodes)
{
  bec::MenuItemList item_list= inner->get_popup_items_for_nodes(convert_node_list(nodes));
  List<MySQL::Base::MenuItem^> ^items= gcnew List<MySQL::Base::MenuItem^>();

  for (bec::MenuItemList::const_iterator iter= item_list.begin(); iter != item_list.end(); ++iter)
    items->Add(gcnew MySQL::Base::MenuItem(*iter));

  return items;
}

//--------------------------------------------------------------------------------------------------

bool ListModel::activate_popup_item_for_nodes(String ^name, List<NodeId^> ^nodes)
{
  try
  {
    return inner->activate_popup_item_for_nodes(NativeToCppString(name), convert_node_list(nodes));
  }
  catch (std::exception &e)
  {
    Logger::LogError("model wrapper", String::Format("Internal exception occured: {0}", CppStringToNative(e.what())));
    throw;
  }
}

//--------------------------------------------------------------------------------------------------

bool ListModel::delete_node(NodeId^ node) 
{
  return inner->delete_node(*node->get_unmanaged_object());
}

//--------------------------------------------------------------------------------------------------

void ListModel::reorder(NodeId^ node, int index)
{
  inner->reorder(*node->get_unmanaged_object(), index);
}

//--------------------------------------------------------------------------------------------------

void ListModel::reorder_up(NodeId^ node)
{
  inner->reorder_up(*node->get_unmanaged_object());
}

//--------------------------------------------------------------------------------------------------

void ListModel::reorder_down(NodeId^ node)
{
  inner->reorder_down(*node->get_unmanaged_object());
}

//--------------------------------------------------------------------------------------------------

bool ListModel::is_editable(NodeId^ node)
{
  return inner->is_editable(*node->get_unmanaged_object());
}

//--------------------------------------------------------------------------------------------------

void ListModel::add_tree_refresh_handler(TreeRefreshSlot::ManagedDelegate^ slot)
{
  TreeRefreshSlot ^tree_refresh_handler= gcnew TreeRefreshSlot(slot);
  boost::signals2::connection connection = inner->tree_changed_signal()->connect(tree_refresh_handler->get_slot());
  tree_refresh_handlers.Add(tree_refresh_handler);
  native_connections->push_back(connection);
}

//--------------------------------------------------------------------------------------------------

void ListModel::remove_tree_refresh_handler(TreeRefreshSlot::ManagedDelegate^ slot)
{
  int i = 0;
  for each (TreeRefreshSlot^ handler in tree_refresh_handlers)
  {
    if (handler->wraps_delegate(slot))
    {
      tree_refresh_handlers.Remove(handler);

      std::vector<boost::signals2::connection>::iterator iterator = native_connections->begin() + i;
      iterator->disconnect();
      native_connections->erase(iterator);
      break;
    }
    i++;
  }
}

//--------------------------------------------------------------------------------------------------

TreeModel::TreeModel(::bec::TreeModel *inn)
  : ListModel(inn)
{
}

//--------------------------------------------------------------------------------------------------

::bec::TreeModel* TreeModel::get_unmanaged_object()
{
  return static_cast<::bec::TreeModel *>(inner);
}

//--------------------------------------------------------------------------------------------------

NodeId^ TreeModel::get_root()
{
  return gcnew NodeId(&get_unmanaged_object()->get_root());
}

//--------------------------------------------------------------------------------------------------

int TreeModel::get_node_depth(NodeId^ node)
{
  return get_unmanaged_object()->get_node_depth(*node->get_unmanaged_object());
}

//--------------------------------------------------------------------------------------------------

NodeId^ TreeModel::get_parent(NodeId^ node)
{
  return gcnew NodeId(&get_unmanaged_object()->get_parent(*node->get_unmanaged_object()));
}

//--------------------------------------------------------------------------------------------------

int TreeModel::count_children(NodeId^ parent)
{
  return get_unmanaged_object()->count_children(*parent->get_unmanaged_object());
}

//--------------------------------------------------------------------------------------------------

NodeId^ TreeModel::get_child(NodeId^ parent, int index)
{
  return gcnew NodeId(&get_unmanaged_object()->get_child(*parent->get_unmanaged_object(), index));
}

//--------------------------------------------------------------------------------------------------

bool TreeModel::expand_node(NodeId^ node)
{
  return get_unmanaged_object()->expand_node(*node->get_unmanaged_object());
}

//--------------------------------------------------------------------------------------------------

void TreeModel::collapse_node(NodeId^ node)
{
  return get_unmanaged_object()->collapse_node(*node->get_unmanaged_object());
}

//--------------------------------------------------------------------------------------------------

bool TreeModel::is_expandable(NodeId^ node)
{
  return get_unmanaged_object()->is_expandable(*node->get_unmanaged_object());
}

//--------------------------------------------------------------------------------------------------

GridModel::GridModel(::bec::GridModel *inn)
  : ListModel(inn)
{
}

//--------------------------------------------------------------------------------------------------

::bec::GridModel* GridModel::get_unmanaged_object()
{
  return static_cast<::bec::GridModel *>(inner);
}

//--------------------------------------------------------------------------------------------------

int GridModel::get_column_count()
{
  return get_unmanaged_object()->get_column_count();
}

//--------------------------------------------------------------------------------------------------

String^ GridModel::get_column_caption(int column)
{
  return CppStringToNative(get_unmanaged_object()->get_column_caption(column));
}

//--------------------------------------------------------------------------------------------------

GridModel::ColumnType GridModel::get_column_type(int column) 
{
  return (GridModel::ColumnType)get_unmanaged_object()->get_column_type(column);
}

//--------------------------------------------------------------------------------------------------

bool GridModel::is_readonly()
{
  return get_unmanaged_object()->is_readonly();
}

//--------------------------------------------------------------------------------------------------

String^ GridModel::readonly_reason()
{
  return CppStringToNative(get_unmanaged_object()->readonly_reason());
}

//--------------------------------------------------------------------------------------------------

bool GridModel::is_field_null(NodeId^ node, int column)
{
  return get_unmanaged_object()->is_field_null(*node->get_unmanaged_object(), column);
}

//--------------------------------------------------------------------------------------------------

bool GridModel::set_field_null(NodeId^ node, int column)
{
  return get_unmanaged_object()->set_field_null(*node->get_unmanaged_object(), column);
}

//--------------------------------------------------------------------------------------------------

bool GridModel::get_field_repr(NodeId^ node, int column, [Out] String^ %value)
{
  std::string str;
  bool retval= get_unmanaged_object()->get_field_repr(*node->get_unmanaged_object(), column, str);
  value= CppStringToNative(str);
  return retval;
}

//--------------------------------------------------------------------------------------------------

void GridModel::set_edited_field(int row_index, int col_index)
{
  get_unmanaged_object()->set_edited_field(row_index, col_index);
}

//--------------------------------------------------------------------------------------------------

void GridModel::sort_columns([Out] List<int>^ %indexes, [Out] List<int>^ %orders)
{
  indexes= gcnew List<int>();
  orders= gcnew List<int>();
  ::bec::GridModel::SortColumns sort_columns= get_unmanaged_object()->sort_columns();
  for (::bec::GridModel::SortColumns::const_iterator i= sort_columns.begin(), i_end= sort_columns.end(); i != i_end; ++i)
  {
    indexes->Add(i->first);
    orders->Add(i->second);
  }
}

//--------------------------------------------------------------------------------------------------
