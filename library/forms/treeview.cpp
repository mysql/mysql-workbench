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

#include "mforms/mforms.h"

using namespace mforms;

TreeNodeSkeleton::TreeNodeSkeleton(const std::string &strcaption, const std::string &stricon,
                                   const std::string &strtag) {
  caption = strcaption;
  icon = stricon;
  tag = strtag;
}

TreeNodeCollectionSkeleton::TreeNodeCollectionSkeleton(const std::string &stricon) {
  icon = stricon;
}

TreeNodeRef::TreeNodeRef(TreeNode *anode) : node(anode) {
  if (node)
    node->retain();
}

TreeNodeRef::TreeNodeRef(const TreeNodeRef &other) {
  node = other.node;
  if (node)
    node->retain();
}

TreeNodeRef::~TreeNodeRef() {
  if (node)
    node->release();
}

TreeNodeRef &TreeNodeRef::operator=(const TreeNodeRef &other) {
  if (node != other.node) {
    if (other.node)
      other.node->retain();
    if (node)
      node->release();
    node = other.node;
  }
  return *this;
}

TreeNode *TreeNodeRef::operator->() const {
  if (!node)
    throw std::logic_error("Attempt to dereference NULL TreeNode");
  return node;
}

TreeNode *TreeNodeRef::operator->() {
  if (!node)
    throw std::logic_error("Attempt to dereference NULL TreeNode");
  return node;
}

bool TreeNodeRef::operator==(const TreeNodeRef &other) const {
  if (node == other.node)
    return true;

  if (other.node && node)
    return node->equals(*other.node);

  return false;
}

bool TreeNodeRef::operator!=(const TreeNodeRef &other) const {
  if (node == other.node)
    return false;

  if (other.node && node)
    return !node->equals(*other.node);

  return true;
}

bool mforms::TreeNodeRef::is_valid() {
  return node != NULL;
}

// -------------------------------------------------------------------------------------------------

void TreeNode::remove_children() {
  if (is_valid()) {
    for (int i = count() - 1; i >= 0; --i) {
      TreeNodeRef child(get_child(i));
      if (child)
        child->remove_from_parent();
    }
  }
}

TreeNodeRef TreeNode::find_child_with_tag(const std::string &tag) {
  for (int c = count(), i = 0; i < c; i++) {
    TreeNodeRef child(get_child(i));
    if (child && child->get_tag() == tag)
      return child;
  }
  return TreeNodeRef();
}

//--------------------------------------------------------------------------------------------------

void TreeNode::toggle() {
  if (can_expand()) {
    if (!is_expanded())
      expand();
    else
      collapse();
  }
}

//----------------- TreeView -----------------------------------------------------------------------

TreeView::TreeView(TreeOptions options) // yep
  : _context_menu(0),
    _header_menu(0),
    _update_count(0),
    _clicked_header_column(0),
    _end_column_called(false) {
  _treeview_impl = &ControlFactory::get_instance()->_treeview_impl;
  _index_on_tag = (options & TreeIndexOnTag) ? true : false;

  _treeview_impl->create(this, options);
}

//--------------------------------------------------------------------------------------------------

TreeView::~TreeView() {
  _update_count++; // Avoid any callbacks/events.
}

//--------------------------------------------------------------------------------------------------

void TreeView::set_context_menu(ContextMenu *menu) {
  _context_menu = menu;
}

//--------------------------------------------------------------------------------------------------

void TreeView::set_header_menu(ContextMenu *menu) {
  _header_menu = menu;
}

//--------------------------------------------------------------------------------------------------

void TreeView::header_clicked(int column) {
  _clicked_header_column = column;
}

//--------------------------------------------------------------------------------------------------

int TreeView::add_column(TreeColumnType type, const std::string &name, int initial_width, bool editable,
                         bool attributed) {
  if (_end_column_called)
    throw std::logic_error("Add column called, after end_columns has been called");
  _column_types.push_back(type);
#if defined(_MSC_VER)
  return _treeview_impl->add_column(this, type, name, initial_width, editable);
#else
  return _treeview_impl->add_column(this, type, name, initial_width, editable, attributed);
#endif
}

void TreeView::set_column_title(int column, const std::string &title) {
  _treeview_impl->set_column_title(this, column, title);
}

TreeColumnType TreeView::get_column_type(int column) {
  if (column >= 0 && column < (int)_column_types.size())
    return _column_types[column];
  return StringColumnType;
}

void TreeView::set_allow_sorting(bool flag) {
  if (!_end_column_called)
    throw std::logic_error("TreeView::set_allow_sorting() must be called after end_columns()");
  _treeview_impl->set_allow_sorting(this, flag);
}

void TreeView::end_columns() {
  _end_column_called = true;
  _treeview_impl->end_columns(this);
}

void TreeView::clear() {
  _treeview_impl->clear(this);
}

TreeNodeRef TreeView::root_node() {
  return _treeview_impl->root_node(this);
}

TreeNodeRef TreeView::add_node() {
  return root_node()->add_child();
}

TreeNodeRef TreeView::get_selected_node() {
  return _treeview_impl->get_selected_node(this);
}

void TreeView::set_selection_mode(TreeSelectionMode mode) {
  _treeview_impl->set_selection_mode(this, mode);
}

TreeSelectionMode TreeView::get_selection_mode() {
  return _treeview_impl->get_selection_mode(this);
}

int TreeView::get_selected_row() {
  TreeNodeRef node(get_selected_node());
  return row_for_node(node);
}

std::list<TreeNodeRef> TreeView::get_selection() {
  return _treeview_impl->get_selection(this);
}

void TreeView::clear_selection() {
  _treeview_impl->clear_selection(this);
}

void TreeView::select_node(TreeNodeRef node) {
  if (node.is_valid()) {
    _update_count++;
    clear_selection();
    _treeview_impl->set_selected(this, node, true);
    _update_count--;
  }
}

void TreeView::set_node_selected(TreeNodeRef node, bool flag) {
  if (node.is_valid()) {
    _update_count++;
    _treeview_impl->set_selected(this, node, flag);
    _update_count--;
  }
}

void TreeView::scrollToNode(TreeNodeRef node) {
  _treeview_impl->scrollToNode(this, node);
}

void TreeView::set_row_height(int height) {
  if (_treeview_impl->set_row_height)
    _treeview_impl->set_row_height(this, height);
}

//--------------------------------------------------------------------------------------------------

void TreeView::set_cell_edit_handler(const std::function<void(TreeNodeRef, int, std::string)> &handler) {
  _cell_edited = handler;
}

//--------------------------------------------------------------------------------------------------

bool TreeView::cell_edited(TreeNodeRef row, int column, const std::string &value) {
  if (_cell_edited) {
    _cell_edited(row, column, value);
    return false;
  }
  return true;
}

//--------------------------------------------------------------------------------------------------

bool TreeView::get_drag_data(DragDetails &details, void **data, std::string &format) {
  return false;
}

//--------------------------------------------------------------------------------------------------

void TreeView::drag_finished(DragOperation operation) {
}

//--------------------------------------------------------------------------------------------------

void TreeView::column_resized(int column) {
  _signal_column_resized(column);
}

//--------------------------------------------------------------------------------------------------

void TreeView::changed() {
  if (_update_count == 0)
    _signal_changed();
}

//--------------------------------------------------------------------------------------------------

void TreeView::node_activated(TreeNodeRef row, int column) {
  _signal_activated(row, column);
}

//--------------------------------------------------------------------------------------------------

void TreeView::set_row_overlay_handler(
  const std::function<std::vector<std::string>(TreeNodeRef)> &overlay_icons_for_node) {
  _overlay_icons_for_node = overlay_icons_for_node;
}

//--------------------------------------------------------------------------------------------------

std::vector<std::string> TreeView::overlay_icons_for_node(TreeNodeRef row) {
  if (_overlay_icons_for_node)
    return _overlay_icons_for_node(row);
  return std::vector<std::string>();
}

//--------------------------------------------------------------------------------------------------

void TreeView::overlay_icon_for_node_clicked(TreeNodeRef row, int index) {
  node_activated(row, -(index + 1));
}

//--------------------------------------------------------------------------------------------------

/**
 * Descendants can override this method to indicate expandability depending on other information.
 */
bool TreeView::can_expand(TreeNodeRef row) {
  return row->count() > 0;
}

//--------------------------------------------------------------------------------------------------

void TreeView::expand_toggle(TreeNodeRef row, bool expanded) {
  _signal_expand_toggle(row, expanded);
}

//--------------------------------------------------------------------------------------------------

void TreeView::freeze_refresh() {
  _treeview_impl->freeze_refresh(this, true);
}

void TreeView::thaw_refresh() {
  _treeview_impl->freeze_refresh(this, false);
}

int TreeView::row_for_node(TreeNodeRef node) {
  return _treeview_impl->row_for_node(this, node);
}

TreeNodeRef TreeView::node_at_row(int row) {
  return _treeview_impl->node_at_row(this, row);
}

mforms::TreeNodeRef mforms::TreeView::node_at_position(base::Point position) {
  return _treeview_impl->node_at_position(this, position);
}

TreeNodeRef TreeView::node_with_tag(const std::string &tag) {
  if (!_index_on_tag)
    throw std::logic_error("Tree was not created with TreeIndexOnTag");

  return _treeview_impl->node_with_tag(this, tag);
}

void TreeView::set_column_visible(int column, bool flag) {
  if (_treeview_impl->set_column_visible)
    _treeview_impl->set_column_visible(this, column, flag);
}

bool TreeView::get_column_visible(int column) {
  if (_treeview_impl->get_column_visible)
    return _treeview_impl->get_column_visible(this, column);
  return true;
}

void TreeView::set_column_width(int column, int width) {
  if (_treeview_impl->set_column_width)
    _treeview_impl->set_column_width(this, column, width);
}

int TreeView::get_column_width(int column) {
  if (_treeview_impl->get_column_width)
    return _treeview_impl->get_column_width(this, column);
  return 0;
}

double TreeView::parse_string_with_unit(const char *s) {
  char *end = NULL;
  double value = strtod(s, &end);

  if (*end == ' ')
    ++end;

  switch (*end) {
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
      if (*(end + 1) == 'i')
        value *= 1024.0;
      else
        value *= 1000.0;
      break;
    case 'M': // mega
      if (*(end + 1) == 'i')
        value *= 1048576.0;
      else
        value *= 1000000.0;
      break;
    case 'G': // giga
      if (*(end + 1) == 'i')
        value *= 1073741824.0;
      else
        value *= 1000000000.0;
      break;
    case 'T': // tera
      if (*(end + 1) == 'i')
        value *= 1099511627776.0;
      else
        value *= 1000000000000.0;
      break;
    case 'P': // peta
      if (*(end + 1) == 'i')
        value *= 1125899906842624.0;
      else
        value *= 1000000000000000.0;
      break;
  }
  return value;
}

void TreeView::BeginUpdate() {
  if (_treeview_impl->BeginUpdate)
    _treeview_impl->BeginUpdate(this);
}

void TreeView::EndUpdate() {
  if (_treeview_impl->EndUpdate)
    _treeview_impl->EndUpdate(this);
}
