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

#ifndef _ACTION_LIST_H_
#define _ACTION_LIST_H_

#include "wbpublic_public_interface.h"
#include "grt/tree_model.h"

class WBPUBLICBACKEND_PUBLIC_FUNC ActionList {
public:
  ActionList();
  void reset();

  /* actions that doesn't require context node(s) */
public:
  typedef std::function<void()> ActionSlot;
  void register_action(const std::string &name, const ActionSlot &slot);
  void unregister_action(const std::string &name);
  bool trigger_action(const std::string &name);

private:
  typedef std::map<std::string, ActionSlot> ActionSlots;
  ActionSlots _actions;

  /* actions for single node */
public:
  typedef std::function<void(const bec::NodeId &)> NodeActionSlot;
  void register_node_action(const std::string &name, const NodeActionSlot &slot);
  void unregister_node_action(const std::string &name);
  bool trigger_action(const std::string &name, const bec::NodeId &node);

private:
  typedef std::map<std::string, NodeActionSlot> NodeActionSlots;
  NodeActionSlots _node_actions;

  /* actions for multiple nodes */
public:
  typedef std::function<void(const std::vector<bec::NodeId> &)> NodesActionSlot;
  void register_nodes_action(const std::string &name, const NodesActionSlot &slot);
  void unregister_nodes_action(const std::string &name);
  bool trigger_action(const std::string &name, const std::vector<bec::NodeId> &nodes);

private:
  typedef std::map<std::string, NodesActionSlot> NodesActionSlots;
  NodesActionSlots _nodes_actions;

  /* actions for multiple row indexes plus column index */
public:
  typedef std::function<void(const std::vector<int> &, int)> RowsColActionSlot;
  void register_rows_col_action(const std::string &name, const RowsColActionSlot &slot);
  void unregister_rows_col_action(const std::string &name);
  bool trigger_action(const std::string &name, const std::vector<int> &rows, int column);

private:
  typedef std::map<std::string, RowsColActionSlot> RowsColActionSlots;
  RowsColActionSlots _rows_col_actions;

  /* aux templates */
private:
  template <typename Slots, typename Slot>
  void register_action_(const std::string &name, Slots &slots, const Slot &slot);

  template <typename Slots>
  void unregister_action_(const std::string &name, Slots &slots);

  template <typename Slots>
  bool trigger_action_(const std::string &name, Slots &slots);

  template <typename Slots, typename Context>
  bool trigger_action_(const std::string &name, Slots &slots, const Context &context);
  template <typename Slots, typename Context1, typename Context2>
  bool trigger_action_(const std::string &name, Slots &slots, const Context1 &context1, const Context2 &context2);
};

#endif /* _ACTION_LIST_H_ */
