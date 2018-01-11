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

#include "grt/action_list.h"

ActionList::ActionList() {
}

void ActionList::reset() {
  _actions.clear();
  _node_actions.clear();
  _nodes_actions.clear();
  _rows_col_actions.clear();
}

template <typename Slots, typename Slot>
void ActionList::register_action_(const std::string &name, Slots &slots, const Slot &slot) {
  slots[name] = slot;
}

template <typename Slots>
void ActionList::unregister_action_(const std::string &name, Slots &slots) {
  typename Slots::iterator action_iter = slots.find(name);
  if (slots.end() != action_iter)
  slots.erase(action_iter);
}

template <typename Slots>
bool ActionList::trigger_action_(const std::string &name, Slots &slots) {
  typename Slots::iterator action_iter = slots.find(name);
  if (slots.end() != action_iter) {
    action_iter->second();
    return true;
  } else {
    return false;
  }
}

template <typename Slots, typename Context>
bool ActionList::trigger_action_(const std::string &name, Slots &slots, const Context &context) {
  typename Slots::iterator action_iter = slots.find(name);
  if (slots.end() != action_iter) {
    action_iter->second(context);
    return true;
  } else {
    return false;
  }
}

template <typename Slots, typename Context1, typename Context2>
bool ActionList::trigger_action_(const std::string &name, Slots &slots, const Context1 &context1,
                                 const Context2 &context2) {
  typename Slots::iterator action_iter = slots.find(name);
  if (slots.end() != action_iter) {
    action_iter->second(context1, context2);
    return true;
  } else {
    return false;
  }
}

void ActionList::register_action(const std::string &name, const ActionSlot &slot) {
  register_action_(name, _actions, slot);
}

void ActionList::register_node_action(const std::string &name, const NodeActionSlot &slot) {
  register_action_(name, _node_actions, slot);
}

void ActionList::register_nodes_action(const std::string &name, const NodesActionSlot &slot) {
  register_action_(name, _nodes_actions, slot);
}

void ActionList::register_rows_col_action(const std::string &name, const RowsColActionSlot &slot) {
  register_action_(name, _rows_col_actions, slot);
}

void ActionList::unregister_action(const std::string &name) {
  unregister_action_(name, _actions);
}

void ActionList::unregister_node_action(const std::string &name) {
  unregister_action_(name, _node_actions);
}

void ActionList::unregister_nodes_action(const std::string &name) {
  unregister_action_(name, _nodes_actions);
}

void ActionList::unregister_rows_col_action(const std::string &name) {
  unregister_action_(name, _rows_col_actions);
}

bool ActionList::trigger_action(const std::string &name) {
  return trigger_action_(name, _actions);
}

bool ActionList::trigger_action(const std::string &name, const bec::NodeId &node) {
  return trigger_action_(name, _node_actions, node);
}

bool ActionList::trigger_action(const std::string &name, const std::vector<bec::NodeId> &nodes) {
  return trigger_action_(name, _nodes_actions, nodes);
}

bool ActionList::trigger_action(const std::string &name, const std::vector<int> &rows, int column) {
  return trigger_action_(name, _rows_col_actions, rows, column);
}
