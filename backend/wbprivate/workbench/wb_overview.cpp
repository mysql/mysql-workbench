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

#include <stack>

#include "wb_overview.h"
#include "model/wb_overview_physical.h"
#include "grt/icon_manager.h"
#include "grt/clipboard.h"
#include "grt/exceptions.h"

#include "model/wb_context_model.h"
#include "workbench/wb_context.h"
#include "workbench/wb_context_ui.h"
#include "grt/validation_manager.h"
#include "base/string_utilities.h"

/**
 * @file  wb_overview.cpp
 * @brief
 */

using namespace wb;
using namespace bec;
using namespace base;

int OverviewBE::Node::get_popup_menu_items(WBContext *wb, bec::MenuItemList &items) {
  return 0;
}

bool OverviewBE::ObjectNode::activate(WBContext *wb) {
  bec::GRTManager::get()->open_object_editor(object, bec::NoFlags);

  return true;
}

bool OverviewBE::ObjectNode::rename(WBContext *wb, const std::string &name) {
  db_DatabaseObjectRef dbobj(db_DatabaseObjectRef::cast_from(object));

  if (dbobj.is_valid()) {
    grt::AutoUndo undo;
    dbobj->name(name);
    undo.end(strfmt(_("Rename %s"), dbobj.get_metaclass()->get_attribute("caption").c_str()));
    bec::ValidationManager::validate_instance(object, CHECK_NAME);
  } else
    throw std::runtime_error("rename not implemented for this object");

  return true;
}

//----------------------------------------------------------------------

OverviewBE::OverviewBE(WBContext *wb) : _wb(wb), _root_node(0) {
}

OverviewBE::~OverviewBE() {
  delete _root_node;
}

std::string OverviewBE::get_title() {
  if (_root_node)
    return _root_node->label;

  return "";
}

NodeId OverviewBE::get_child(const NodeId &parent, size_t index) {
  if (!parent.is_valid() && index < count_children(parent))
    return index;

  return NodeId(parent).append(index);
}

size_t OverviewBE::count_children(const NodeId &parent) {
  if (!_root_node)
    return 0;

  if (!parent.is_valid())
    return (int)_root_node->children.size();

  Node *n = get_node_by_id(parent);
  if (n)
    return (int)n->count_children();
  return 0;
}

bec::NodeId OverviewBE::get_node_child_for_object(const bec::NodeId &node, const grt::ObjectRef &object) {
  ContainerNode *n;

  if (node.is_valid())
    n = dynamic_cast<ContainerNode *>(get_node_by_id(node));
  else
    n = _root_node;

  if (!n)
    return bec::NodeId();

  for (size_t c = n->count_children(), i = 0; i < c; i++) {
    Node *ch = n->get_child(i);
    if (ch && ch->object == object)
      return get_child(node, (int)i);
  }
  return NodeId();
}

bool OverviewBE::get_field(const NodeId &node, ColumnId column, std::string &value) {
  Node *n = get_node_by_id(node);
  if (!n)
    return false;

  switch (column) {
    case Label:
      value = n->label;
      return true;

    case NodeType:
    case Expanded:
    case Height:
    case DisplayMode:
      return false;

    default:
      if (column >= FirstDetailField) {
        value = n->get_detail((int)column - FirstDetailField);
        return true;
      }
  }
  return false;
}

grt::ValueRef OverviewBE::get_grt_value(const NodeId &node, ColumnId column) {
  Node *n = get_node_by_id(node);
  if (n)
    return n->object;
  return grt::ValueRef();
}

bool OverviewBE::get_field(const NodeId &node, ColumnId column, ssize_t &value) {
  Node *n = get_node_by_id(node);
  if (!n)
    return false;

  switch (column) {
    case Label:
      return false;

    case ChildNodeType:
      if (dynamic_cast<ContainerNode *>(n)) {
        value = (int)dynamic_cast<ContainerNode *>(n)->child_type;
        return true;
      } else {
        value = -1;
        return false;
      }

    case NodeType:
      value = (ssize_t)n->type;
      return true;

    case Expanded:
      if (n->type != OItem) {
        value = n->expanded;
        return true;
      }
      return false;

    case DisplayMode:
      value = (ssize_t)n->display_mode;
      return true;

    default:
      return false;
  }
  return false;
}

int OverviewBE::get_details_field_count(const bec::NodeId &node) {
  ContainerNode *n = dynamic_cast<ContainerNode *>(get_node_by_id(node));
  if (!n)
    return 0;

  return n->count_detail_fields();
}

std::string OverviewBE::get_field_name(const bec::NodeId &node, ColumnId column) {
  ContainerNode *n = dynamic_cast<ContainerNode *>(get_node_by_id(node));
  if (!n)
    return "";

  return n->get_detail_name((int)column - FirstDetailField);
}

bool OverviewBE::set_field(const NodeId &node, ColumnId column, const std::string &value) {
  Node *n = get_node_by_id(node);
  if (!n)
    return false;
  std::string title;

  try {
    switch (column) {
      case Label:
        if (n->label != value) {
          title = strfmt(_("Rename '%s'"), n->label.c_str());
          return n->rename(_wb, value);
        }
    }
  } catch (grt::grt_runtime_error &exc) {
    _wb->show_exception(title, exc);
  } catch (std::exception &exc) {
    _wb->show_exception(title, exc);
  }

  return false;
}

std::string OverviewBE::get_field_description(const NodeId &node, ColumnId column) {
  Node *n = get_node_by_id(node);
  if (!n)
    return "";

  return n->description;
}

IconId OverviewBE::get_field_icon(const NodeId &node, ColumnId column, bec::IconSize size) {
  Node *n = get_node_by_id(node);
  if (!n)
    return 0;

  if (size == Icon16)
    return n->small_icon;
  else
    return n->large_icon;
}

OverviewBE::Node *OverviewBE::do_get_node(const NodeId &node) const {
  Node *n = 0;
  size_t i;
  if (!node.is_valid())
    return _root_node;

  if (!_root_node || node[0] >= _root_node->children.size())
    return 0;

  for (i = 1, n = _root_node->children[node[0]]; i < node.depth(); i++) {
    if (n)
      n = n->get_child(node[i]);
    else {
      return 0;
    }
  }

  return n;
}

OverviewBE::Node *OverviewBE::get_deepest_focused() {
  ContainerNode *parent = _root_node;

  while (parent && dynamic_cast<ContainerNode *>(parent->focused))
    parent = dynamic_cast<ContainerNode *>(parent->focused);

  return parent;
}

/** Find an Item node in the subtree indicated by node that matches the given string by depth first search.
 *
 * Search will begin at starting_node until the last node of the tree. If it's the nil node, it will
 * start at the beginning of the tree.
 */
bec::NodeId OverviewBE::search_child_item_node_matching(const bec::NodeId &node, const bec::NodeId &starting_node,
                                                        const std::string &text) {
  bec::NodeId start_node = node;
  bec::NodeId parent;
  size_t start;
  gchar *tmp = g_utf8_strdown(text.c_str(), (gssize)text.size());
  std::string lower_text = tmp;
  g_free(tmp);

  if (starting_node.is_valid()) {
    start_node = starting_node;
    start = start_node.end() + 1;
    parent = get_parent(start_node);
  } else {
    parent = node;
    start = 0;
  }

  do {
    for (size_t i = start; i < count_children(parent); i++) {
      std::string label;
      bec::NodeId child(get_child(parent, i));
      ssize_t type;

      get_field(child, NodeType, type);

      if (type == OItem) {
        get_field(child, Label, label);

        tmp = g_utf8_strdown(label.c_str(), (gssize)label.size());
        if (strstr(tmp, lower_text.c_str())) {
          g_free(tmp);
          return child;
        }
        g_free(tmp);
      } else {
        if (count_children(child) > 0) {
          bec::NodeId found = search_child_item_node_matching(child, bec::NodeId(), text);
          if (found.is_valid())
            return found;
        }
      }
    }

    if (parent.is_valid()) {
      start = parent.end() + 1;
      parent = get_parent(parent);
    } else
      break;
  } while (parent.depth() > node.depth());

  return bec::NodeId();
}

void OverviewBE::refresh() {
  /* needs to be refactored so that the tree is refreshed not rebuilt

  workbench_DocumentRef document= _wb->get_document();
  std::list<int> focus_list;

  if (!document.is_valid())
    return;
    //throw std::logic_error("Document is not set");

  // save focused items
  ContainerNode *c= _root_node;
  while (c)
  {
    int i= c->get_focused_index();
    if (i < 0)
      break;
    focus_list.push_back(i);

    c= dynamic_cast<ContainerNode*>(c->focused);
  }

  unselect_all(NodeId());


  // restore focused items. the container nodes should be stable
  ContainerNode *node= _root_node;
  for (std::list<int>::const_iterator i= focus_list.begin(); i != focus_list.end(); ++i)
  {
    if (*i >= (int)node->children.size())
      break;
    node->focused= node->get_child(*i);
    node= dynamic_cast<ContainerNode*>(node->focused);
  }
  */
}

bool OverviewBE::activate_node(const NodeId &node) {
  Node *n = get_node_by_id(node);
  if (n)
    return n->activate(_wb);
  return false;
}

std::string OverviewBE::get_node_unique_id(const NodeId &node) {
  Node *n = get_node_by_id(node);
  if (n)
    return n->get_unique_id();
  return "";
}

static void unselect_all(OverviewBE::ContainerNode &node) {
  node.selected = false;

  for (std::vector<OverviewBE::Node *>::iterator i = node.children.begin(); i != node.children.end(); ++i) {
    OverviewBE::ContainerNode *cont = dynamic_cast<OverviewBE::ContainerNode *>(*i);
    if (cont)
      unselect_all(*cont);
    else
      (*i)->selected = false;
  }
}

void OverviewBE::unselect_all(const NodeId &node) {
  ContainerNode *container = dynamic_cast<ContainerNode *>(get_node_by_id(node));

  if (container)
    ::unselect_all(*container);
}

void OverviewBE::begin_selection_marking() {
  if (_root_node)
    ::unselect_all(*_root_node);
}

void OverviewBE::end_selection_marking() {
  _selection_change_signal();
}

void OverviewBE::select_node(const NodeId &node) {
  Node *n = get_node_by_id(node);

  if (n) {
    // XXX in the future we may allow selectionof container nodes (which would recursively select its contents)
    // if (dynamic_cast<ContainerNode*>(n))
    //   throw std::logic_error("selection only possible in leaf nodes");

    n->selected = true;

    focus_node(get_parent(node));
  }
}

grt::ListRef<GrtObject> OverviewBE::get_selection() {
  ContainerNode *node = dynamic_cast<ContainerNode *>(get_deepest_focused());
  grt::ListRef<GrtObject> selection(true);

  if (node) {
    for (std::vector<Node *>::iterator iter = node->children.begin(); iter != node->children.end(); ++iter) {
      if ((*iter)->selected)
        selection.insert((*iter)->object);
    }
  }
  return selection;
}

std::list<int> OverviewBE::get_selected_children(const bec::NodeId &node) {
  std::list<int> list;
  ContainerNode *n = dynamic_cast<ContainerNode *>(get_node_by_id(node));
  if (n) {
    int i = 0;
    for (std::vector<Node *>::iterator iter = n->children.begin(); iter != n->children.end(); ++iter) {
      if ((*iter)->selected)
        list.push_back(i);
      ++i;
    }
  }
  return list;
}

void OverviewBE::focus_node(const bec::NodeId &node) {
  NodeId parent_id = get_parent(node);

  ContainerNode *parent;

  if (parent_id.is_valid())
    parent = dynamic_cast<ContainerNode *>(get_node_by_id(parent_id));
  else
    parent = _root_node;
  if (!parent)
    throw std::logic_error("attempt to focus invalid node");

  parent->focused = get_node_by_id(node);
  if (parent->focused)
    parent->focused->focus(this);

  if (parent_id.is_valid())
    focus_node(parent_id);
}

bec::NodeId OverviewBE::get_focused_child(const bec::NodeId &node) {
  ContainerNode *parent = dynamic_cast<ContainerNode *>(get_node_by_id(node));
  if (parent && parent->focused) {
    size_t i = std::find(parent->children.begin(), parent->children.end(), parent->focused) - parent->children.begin();

    // std::find returns .end() if nothing is found, hence the difference would be the list size in that case.
    if (i < parent->children.size())
      return NodeId(node).append((int)i);
  }
  return bec::NodeId();
}

bool OverviewBE::request_add_object(const NodeId &node) {
  Node *n = get_node_by_id(node);
  if (n)
    return n->add_object(_wb);
  return false;
}

/*
 * @brief Delete the current selection
 *
 * Selection is defined by traversing the focused container nodes
 * until the individually selected leaf nodes.
 *
 */
int OverviewBE::request_delete_selected() {
  ContainerNode *parent = dynamic_cast<ContainerNode *>(get_deepest_focused());

  if (parent) {
    bool ok = false;
    int count = 0;
    for (std::vector<Node *>::const_iterator iter = parent->children.begin(); iter != parent->children.end(); ++iter) {
      if ((*iter)->selected) {
        if ((*iter)->is_deletable()) {
          (*iter)->delete_object(_wb);
          count++;
          ok = true;
        }
      }
    }
    if (ok)
      _wb->_frontendCallbacks->show_status_text(strfmt(_("%i object(s) deleted."), count));
    else
      _wb->_frontendCallbacks->show_status_text(_("Could not delete selection."));
    return count;
  }
  return 0;
}

bool OverviewBE::is_editable(const bec::NodeId &node) const {
  Node *n = get_node_by_id(node);

  return n ? n->is_renameable() : false;
}

bool OverviewBE::is_deletable(const bec::NodeId &node) const {
  Node *n = get_node_by_id(node);

  return n ? n->is_deletable() : false;
}

bool OverviewBE::is_copyable(const bec::NodeId &node) const {
  Node *n = get_node_by_id(node);

  return n ? n->is_copyable() : false;
}

bool OverviewBE::request_delete_object(const bec::NodeId &node) {
  Node *n = get_node_by_id(node);
  if (n) {
    n->delete_object(_wb);
    return true;
  }
  return false;
}

void OverviewBE::store_node_states(Node *node) {
  workbench_DocumentRef document = _wb->get_document();

  if (node->type != OItem) {
    workbench_OverviewPanelRef panel = node->get_state();
    if (panel.is_valid()) {
      panel->owner(document);
      document->overviewPanels().insert(panel);
    }
  }

  for (size_t c = node->count_children(), i = 0; i < c; i++) {
    Node *child = node->get_child(i);
    if (child)
      store_node_states(child);
  }
}

void OverviewBE::store_state() {
  while (_wb->get_document()->overviewPanels().count() > 0)
    _wb->get_document()->overviewPanels().remove(0);

  for (std::vector<Node *>::iterator iter = _root_node->children.begin(); iter != _root_node->children.end(); ++iter)
    store_node_states(*iter);
}

void OverviewBE::restore_state() {
  workbench_DocumentRef document = _wb->get_document();

  for (size_t c = document->overviewPanels().count(), i = 0; i < c; i++) {
    workbench_OverviewPanelRef panel = document->overviewPanels().get(i);

    Node *node = get_node_by_id(NodeId(panel->nodeId()));
    if (node)
      node->restore_state(panel);
  }
}

bool OverviewBE::can_cut() {
  return can_copy() && can_delete();
}

bool OverviewBE::can_copy() {
  ContainerNode *parent = dynamic_cast<ContainerNode *>(get_deepest_focused());

  if (parent && !parent->children.empty()) {
    bool ok = false;
    for (std::vector<Node *>::const_iterator iter = parent->children.begin(); iter != parent->children.end(); ++iter) {
      if ((*iter)->selected) {
        if (!(*iter)->is_copyable())
          return false;
        ok = true;
      }
    }
    return ok;
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

static OverviewBE::ContainerNode *get_pasteable_container(OverviewBE::ContainerNode *parent, bec::Clipboard *clip) {
  OverviewBE::ContainerNode *focused_container = dynamic_cast<OverviewBE::ContainerNode *>(parent->focused);
  if (focused_container != NULL) {
    OverviewBE::ContainerNode *pastable_child = get_pasteable_container(focused_container, clip);
    if (pastable_child != NULL)
      return pastable_child;
  }

  if (parent->is_pasteable(clip))
    return parent;
  return NULL;
}

//--------------------------------------------------------------------------------------------------

static bool check_focused_pasteable(OverviewBE::ContainerNode *container, bec::Clipboard *clip) {
  if (dynamic_cast<OverviewBE::ContainerNode *>(container->focused))
    if (check_focused_pasteable(dynamic_cast<OverviewBE::ContainerNode *>(container->focused), clip))
      return true;

  return container->is_pasteable(clip);
}

//--------------------------------------------------------------------------------------------------

bool OverviewBE::can_paste() {
  //  ContainerNode *parent= dynamic_cast<ContainerNode*>(get_deepest_focused());
  if (_root_node && _wb->get_clipboard())
    return check_focused_pasteable(_root_node, _wb->get_clipboard());
  return false;
}

//--------------------------------------------------------------------------------------------------

static int count_selection(OverviewBE::ContainerNode *node) {
  if (node && !node->children.empty()) {
    int count = 0;
    for (std::vector<OverviewBE::Node *>::const_iterator i = node->children.begin(); i != node->children.end(); ++i)
      if ((*i)->selected) {
        if (!(*i)->is_deletable())
          return 0;
        count++;
      }
    return count;
  }
  return 0;
}

bool OverviewBE::can_delete() {
  ContainerNode *node = dynamic_cast<ContainerNode *>(get_deepest_focused());
  if (node)
    return count_selection(node) > 0;
  return false;
}

std::string OverviewBE::get_edit_target_name() {
  ContainerNode *node = dynamic_cast<ContainerNode *>(get_deepest_focused());

  if (node) {
    std::string text;
    int count = 0;

    for (std::vector<OverviewBE::Node *>::const_iterator i = node->children.begin(); i != node->children.end(); ++i)
      if ((*i)->selected) {
        if (!(*i)->is_deletable())
          return "";
        text = "'" + (*i)->label + "'";
        count++;
      }
    if (count == 1)
      return text;
    else if (count > 1)
      return strfmt(_("%i Selected Objects"), count);
  }
  return "";
}

std::string OverviewBE::get_target_name_for_nodes(const std::vector<bec::NodeId> &nodes) {
  int count = 0;
  std::string text;
  for (std::vector<bec::NodeId>::const_iterator i = nodes.begin(); i != nodes.end(); ++i) {
    Node *node = get_node_by_id(*i);
    if (node) {
      if (!node->is_deletable())
        return "";
      text = "'" + node->label + "'";
      count++;
    }
  }
  if (count == 1)
    return text;
  else if (count > 1)
    return strfmt(_("%i Selected Objects"), count);
  return "";
}

void OverviewBE::cut() {
  grt::AutoUndo undo;

  copy();
  int count = request_delete_selected();
  undo.end(strfmt(_("Cut %s"), get_edit_target_name().c_str()));
  _wb->_frontendCallbacks->show_status_text(strfmt(_("%i object(s) cut."), count));
}

void OverviewBE::copy() {
  ContainerNode *parent = dynamic_cast<ContainerNode *>(get_deepest_focused());
  int count = 0;

  if (parent && !parent->children.empty()) {
    _wb->get_clipboard()->clear();
    for (std::vector<Node *>::const_iterator iter = parent->children.begin(); iter != parent->children.end(); ++iter) {
      if ((*iter)->selected) {
        (*iter)->copy_object(_wb, _wb->get_clipboard());
        count++;
      }
    }
    if (count > 0) {
      _wb->get_clipboard()->set_content_description(get_edit_target_name());
      _wb->get_clipboard()->changed();
    }
  }

  if (count > 0)
    _wb->_frontendCallbacks->show_status_text(strfmt(_("%i object(s) copied."), count));
}

void OverviewBE::paste() {
  std::stack<ContainerNode *> focused;

  ContainerNode *parent = _root_node;

  while (parent) {
    focused.push(parent);
    parent = dynamic_cast<ContainerNode *>(parent->focused);
  }

  while (!focused.empty()) {
    parent = focused.top();
    focused.pop();
    if (parent->is_pasteable(_wb->get_clipboard())) {
      grt::AutoUndo undo;

      parent->paste_object(_wb, _wb->get_clipboard());

      undo.end(strfmt(_("Paste %s"), _wb->get_clipboard()->get_content_description().c_str()));
      break;
    }
  }
}

void OverviewBE::delete_selection() {
  grt::AutoUndo undo;
  request_delete_selected();
  undo.end(strfmt(_("Delete %s"), get_edit_target_name().c_str()));
}

/** Returns the popup menu definition for the given node and selected related items.

 <li>If the given node is a container node, menu items for that container will be returned.
 <li>If the node is a leaf item, then items for that leaf item will be returned. If
 the container of that item has other selected items, it will return the items for the
 multiple items.
 */
bec::MenuItemList OverviewBE::get_popup_items_for_nodes(const std::vector<bec::NodeId> &nodes) {
  bec::MenuItemList items;

  // Don't go early out here if no node was passed in. We also have to add selection independent
  // menu items like paste.
  grt::ListRef<GrtObject> selection(true);

  bec::MenuItem item;
  std::list<Node *> tree_nodes;

  item.checked = false;
  item.type = bec::MenuAction;

  for (std::vector<bec::NodeId>::const_iterator node = nodes.begin(); node != nodes.end(); ++node) {
    Node *n = get_node_by_id(*node);
    if (n && n->object.is_valid()) {
      selection.insert(n->object);
      tree_nodes.push_back(n);
    }
  }

  bool is_catalog = false;
  bool is_model = false;

  for (std::list<Node *>::const_iterator iter = tree_nodes.begin(); iter != tree_nodes.end(); ++iter) {
    if ((*iter)->object.is_instance(db_Catalog::static_class_name()))
      is_catalog = true;

    if ((*iter)->object.is_instance(model_Model::static_class_name()))
      is_model = true;
  }

  // if (selection.count() > 0)
  {
    std::list<std::string> groups;

    if (!is_catalog) {
      groups.push_back("Catalog/*");
      groups.push_back("Overview/*");
      groups.push_back("Menu/Catalog");
      groups.push_back("Menu/Overview");
    }

    // model_Model will put lots of unrelated items
    if (!is_model)
      _wb->get_model_context()->get_object_list_popup_items(this, nodes, selection, get_target_name_for_nodes(nodes),
                                                            groups, items);

    bec::MenuItemList common_items;

    for (std::list<Node *>::const_iterator iter = tree_nodes.begin(); iter != tree_nodes.end(); ++iter) {
      bec::MenuItemList items_for_node;

      // get items for each node and only keep the ones that are common
      (*iter)->get_popup_menu_items(_wb, items_for_node);
      if (common_items.empty())
        common_items.swap(items_for_node);
      else {
        bec::MenuItemList::iterator ci, next = common_items.begin();
        while (next != common_items.end()) {
          bool strip_this_item = true;
          ci = next++;
          for (bec::MenuItemList::reverse_iterator ni = items_for_node.rbegin(); ni != items_for_node.rend(); ++ni) {
            if (ci->accessibilityName == ni->accessibilityName) {
              strip_this_item = false;
              break;
            }
          }
          if (strip_this_item)
            common_items.erase(ci);
        }
      }
    }

    // append everything
    items.insert(items.end(), common_items.begin(), common_items.end());
  }

  return items;
}

bool OverviewBE::activate_popup_item_for_nodes(const std::string &name, const std::vector<bec::NodeId> &nodes) {
  if (name == "builtin:paste") {
    Node *n = get_pasteable_container(_root_node, _wb->get_clipboard());

    if (n)
      n->paste_object(_wb, _wb->get_clipboard());
  } else if (name == "builtin:delete") {
    grt::AutoUndo undo;

    for (std::vector<bec::NodeId>::const_iterator end = nodes.end(), iter = nodes.begin(); iter != end; ++iter) {
      Node *n = get_node_by_id(*iter);
      if (n)
        n->delete_object(_wb);
    }

    undo.end_or_cancel_if_empty(strfmt(_("Delete %s"), get_target_name_for_nodes(nodes).c_str()));
  } else if (name == "builtin:cut") {
    grt::AutoUndo undo;

    _wb->get_clipboard()->clear();
    for (std::vector<bec::NodeId>::const_iterator end = nodes.end(), iter = nodes.begin(); iter != end; ++iter) {
      Node *n = get_node_by_id(*iter);
      if (n) {
        n->copy_object(_wb, _wb->get_clipboard());
        n->delete_object(_wb);
      }
    }
    undo.end_or_cancel_if_empty(strfmt(_("Cut %s"), get_target_name_for_nodes(nodes).c_str()));
    _wb->get_clipboard()->set_content_description(get_target_name_for_nodes(nodes));
  } else if (name == "builtin:copy") {
    _wb->get_clipboard()->clear();

    for (std::vector<bec::NodeId>::const_iterator end = nodes.end(), iter = nodes.begin(); iter != end; ++iter) {
      Node *n = get_node_by_id(*iter);
      if (n)
        n->copy_object(_wb, _wb->get_clipboard());
    }

    _wb->get_clipboard()->set_content_description(get_target_name_for_nodes(nodes));
  } else {
    for (std::vector<bec::NodeId>::const_iterator end = nodes.end(), iter = nodes.begin(); iter != end; ++iter) {
      Node *n = get_node_by_id(*iter);
      if (n) {
        bec::ArgumentPool argpool;

        argpool.add_entries_for_object("", n->object, "GrtObject");

        if (name == "plugin:wb.edit.editSelectedFigureInNewWindow")
          wb::WBContextUI::get()->get_command_ui()->activate_command("plugin:wb.edit.editObjectInNewWindow", argpool);
        else
          wb::WBContextUI::get()->get_command_ui()->activate_command(name, argpool);
      }
    }
  }

  return true;
}

bec::ToolbarItemList OverviewBE::get_toolbar_items(const bec::NodeId &node) {
  bec::ToolbarItemList items;
  return items;
}

bool OverviewBE::activate_toolbar_item(const bec::NodeId &node, const std::string &name) {
  return false;
}

void OverviewBE::send_refresh_node(const bec::NodeId &node) {
  UIForm *frm = dynamic_cast<UIForm *>(this);
  if (frm && _wb)
    _wb->request_refresh(RefreshOverviewNodeInfo, node.toString(), reinterpret_cast<NativeHandle>(frm));
}

void OverviewBE::send_refresh_children(const bec::NodeId &node) {
  UIForm *frm = dynamic_cast<UIForm *>(this);
  if (frm && _wb)
    _wb->request_refresh(RefreshOverviewNodeChildren, node.toString(), reinterpret_cast<NativeHandle>(frm));
}
