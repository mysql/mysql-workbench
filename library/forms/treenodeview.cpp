/* 
 * Copyright (c) 2012, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "mforms/mforms.h"

using namespace mforms;

TreeNodeSkeleton::TreeNodeSkeleton(const std::string& strcaption, const std::string& stricon, const std::string& strtag)
{
  caption = strcaption;
  icon = stricon;
  tag = strtag;
}

TreeNodeCollectionSkeleton::TreeNodeCollectionSkeleton(const std::string& stricon)
{
  icon = stricon;
}

TreeNodeRef::TreeNodeRef(TreeNode *anode)
: node(anode) 
{ 
  if (node)
    node->retain();
}
  
TreeNodeRef::TreeNodeRef(const TreeNodeRef &other)
{
  node = other.node;
  if (node)
    node->retain();
}
  
TreeNodeRef::~TreeNodeRef() 
{
  if (node)
    node->release(); 
}

TreeNodeRef &TreeNodeRef::operator= (const TreeNodeRef &other)
{
  if (node != other.node)
  {
    if (other.node)
      other.node->retain();
    if (node)
      node->release();
    node = other.node;
  }
  return *this;
}

TreeNode *TreeNodeRef::operator->() const
{ 
  if (!node) 
    throw std::logic_error("Attempt to dereference NULL TreeNode");
  return node;
}

TreeNode *TreeNodeRef::operator->()
{ 
  if (!node) 
    throw std::logic_error("Attempt to dereference NULL TreeNode");
  return node;
}

bool TreeNodeRef::operator == (const TreeNodeRef &other) const
{
  if (node == other.node)
    return true;
  
  if (other.node && node)
    return node->equals(*other.node);

  return false;
}

bool TreeNodeRef::operator != (const TreeNodeRef &other) const
{
  if (node == other.node)
    return false;
  
  if (other.node && node)
    return !node->equals(*other.node);

  return true;
}

bool mforms::TreeNodeRef::is_valid()
{
  return node != NULL;
}

// -------------------------------------------------------------------------------------------------

void TreeNode::remove_children()
{
  if (is_valid())
  {
    for (int i = count()-1; i >= 0; --i)
    {
      TreeNodeRef child(get_child(i));
      if (child)
        child->remove_from_parent();
    }
  }
}

TreeNodeRef TreeNode::find_child_with_tag(const std::string &tag)
{
  for (int c = count(), i = 0; i < c; i++)
  {
    TreeNodeRef child(get_child(i));
    if (child && child->get_tag() == tag)
      return child;
  }
  return TreeNodeRef();
}

//----------------- TreeNodeView -------------------------------------------------------------------

TreeNodeView::TreeNodeView(TreeOptions options)
: _context_menu(0), _update_count(0), _end_column_called(false)
{
  _treeview_impl= &ControlFactory::get_instance()->_treenodeview_impl;
  _index_on_tag = (options & TreeIndexOnTag) ? true : false;

  _treeview_impl->create(this, options);
}

//--------------------------------------------------------------------------------------------------

TreeNodeView::~TreeNodeView()
{
  _update_count++; // Avoid any callbacks/events.
}

//--------------------------------------------------------------------------------------------------

void TreeNodeView::set_context_menu(ContextMenu *menu)
{
  _context_menu = menu;
}

//--------------------------------------------------------------------------------------------------

int TreeNodeView::add_column(TreeColumnType type, const std::string &name, int initial_width, bool editable, bool attributed)
{
  if (_end_column_called)
    throw std::logic_error("Add column called, after end_columns has been called");
  _column_types.push_back(type);
#if defined(_WIN32)
  return _treeview_impl->add_column(this, type, name, initial_width, editable);
#else
  return _treeview_impl->add_column(this, type, name, initial_width, editable, attributed);
#endif
}


TreeColumnType TreeNodeView::get_column_type(int column)
{
  if (column >= 0 && column < (int)_column_types.size())
    return _column_types[column];
  return StringColumnType;
}


void TreeNodeView::set_allow_sorting(bool flag)
{
  if (!_end_column_called)
    throw std::logic_error("TreeNodeView::set_allow_sorting() must be called after end_columns()");
  _treeview_impl->set_allow_sorting(this, flag);
}


void TreeNodeView::end_columns()
{
  _end_column_called = true;
  _treeview_impl->end_columns(this);
}


void TreeNodeView::clear()
{
  _treeview_impl->clear(this);
}


TreeNodeRef TreeNodeView::root_node()
{
  return _treeview_impl->root_node(this);
}

TreeNodeRef TreeNodeView::add_node()
{
  return root_node()->add_child(); 
}

TreeNodeRef TreeNodeView::get_selected_node()
{
  return _treeview_impl->get_selected_node(this);
}

void TreeNodeView::set_selection_mode(TreeSelectionMode mode)
{
  _treeview_impl->set_selection_mode(this, mode);
}

TreeSelectionMode TreeNodeView::get_selection_mode()
{
  return _treeview_impl->get_selection_mode(this);
}

int TreeNodeView::get_selected_row()
{
  TreeNodeRef node(get_selected_node());
  return row_for_node(node);
}


std::list<TreeNodeRef> TreeNodeView::get_selection()
{
  return _treeview_impl->get_selection(this);
}

void TreeNodeView::clear_selection()
{
  _treeview_impl->clear_selection(this);
}


void TreeNodeView::select_node(TreeNodeRef node)
{
  _update_count++;
  clear_selection();
  _treeview_impl->set_selected(this, node, true);
  _update_count--;
}


void TreeNodeView::set_node_selected(TreeNodeRef node, bool flag)
{
  _update_count++;
  _treeview_impl->set_selected(this, node, flag);
  _update_count--;
}


void TreeNodeView::set_row_height(int height)
{
  if (_treeview_impl->set_row_height)
    _treeview_impl->set_row_height(this, height);
}

//--------------------------------------------------------------------------------------------------

void TreeNodeView::set_cell_edit_handler(const boost::function<void (TreeNodeRef, int, std::string)> &handler)
{
  _cell_edited = handler;
}

//--------------------------------------------------------------------------------------------------

bool TreeNodeView::cell_edited(TreeNodeRef row, int column, const std::string &value)
{
  if (_cell_edited)
  {
    _cell_edited(row, column, value);
    return false;
  }
  return true;
}

//--------------------------------------------------------------------------------------------------

bool TreeNodeView::get_drag_data(DragDetails &details, void **data, std::string &format)
{
  return false;
}

//--------------------------------------------------------------------------------------------------

void TreeNodeView::drag_finished(DragOperation operation)
{
}

//--------------------------------------------------------------------------------------------------

void TreeNodeView::column_resized(int column)
{
  _signal_column_resized(column);
}

//--------------------------------------------------------------------------------------------------

void TreeNodeView::changed()
{
  if (_update_count == 0)
    _signal_changed();
}

//--------------------------------------------------------------------------------------------------

void TreeNodeView::node_activated(TreeNodeRef row, int column)
{
  _signal_activated(row, column);
}

//--------------------------------------------------------------------------------------------------

/**
 * Descendants can override this method to indicate expandability depending on other information.
 */
bool TreeNodeView::can_expand(TreeNodeRef row)
{
  return row->count() > 0;
}

//--------------------------------------------------------------------------------------------------

void TreeNodeView::expand_toggle(TreeNodeRef row, bool expanded)
{
  _signal_expand_toggle(row, expanded);
}

//--------------------------------------------------------------------------------------------------

void TreeNodeView::freeze_refresh()
{
  _treeview_impl->freeze_refresh(this, true);
}


void TreeNodeView::thaw_refresh()
{
  _treeview_impl->freeze_refresh(this, false);
}


int TreeNodeView::row_for_node(TreeNodeRef node)
{
  return _treeview_impl->row_for_node(this, node);
}


TreeNodeRef TreeNodeView::node_at_row(int row)
{
  return _treeview_impl->node_at_row(this, row);
}


TreeNodeRef TreeNodeView::node_with_tag(const std::string &tag)
{
  if (!_index_on_tag)
    throw std::logic_error("Tree was not created with TreeIndexOnTag");

  return _treeview_impl->node_with_tag(this, tag);
}


void TreeNodeView::set_column_visible(int column, bool flag)
{
  if (_treeview_impl->set_column_visible)
    _treeview_impl->set_column_visible(this, column, flag);
}


bool TreeNodeView::get_column_visible(int column)
{
  if (_treeview_impl->get_column_visible)
    return _treeview_impl->get_column_visible(this, column);
  return true;
}


void TreeNodeView::set_column_width(int column, int width)
{
  if (_treeview_impl->set_column_width)
    _treeview_impl->set_column_width(this, column, width);
}


int TreeNodeView::get_column_width(int column)
{
  if (_treeview_impl->get_column_width)
    return _treeview_impl->get_column_width(this, column);
  return 0;
}


double TreeNodeView::parse_string_with_unit(const char *s)
{
  char *end = NULL;
  double value = strtod(s, &end);

  if (*end == ' ')
    ++end;

  switch (*end)
  {
    case 'p': // pico
      value /= 1000000000000.0;
      break;
    case 'n': // nano
      value /= 1000000000.0;
      break;
    case 'u': // micro
      value /= 1000000.0;
      break;
    case 'm': // milli
      value /= 1000.0;
      break;

    case 'h': // special case for hours
      value *= 3600.0;
      break;

    case 'K': // kilo
    case 'k':
      if (*(end+1) == 'i')
        value *= 1024.0;
      else
        value *= 1000.0;
      break;
    case 'M': // mega
      if (*(end+1) == 'i')
        value *= 1048576.0;
      else
        value *= 1000000.0;
      break;
    case 'G': // giga
      if (*(end+1) == 'i')
        value *= 1073741824.0;
      else
        value *= 1000000000.0;
      break;
    case 'T': // tera
      if (*(end+1) == 'i')
        value *= 1099511627776.0;
      else
        value *= 1000000000000.0;
      break;
    case 'P': // peta
      if (*(end+1) == 'i')
        value *= 1125899906842624.0;
      else
        value *= 1000000000000000.0;
      break;
  }
  return value;
}
