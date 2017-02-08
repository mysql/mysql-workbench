/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __ACTION_LIST_WR_H__
#define __ACTION_LIST_WR_H__

#include "grt/action_list.h"

namespace MySQL {
  namespace Grt {

  public
    ref class ActionList {
    public:
      ActionList(::ActionList *inner);
      void reset();

    private:
      ~ActionList();

    private:
      ::ActionList *_inner;

    public:
      typedef DelegateSlot0<void, void> ActionCallback;
      void register_action(String ^ name, ActionCallback::ManagedDelegate ^ cb);
      void unregister_action(String ^ name);
      bool trigger_action(String ^ name);

    private:
      Dictionary<String ^, ActionCallback ^> ^ _actions;

    public:
      typedef DelegateSlot1<void, void, bec::NodeId, NodeIdWrapper ^> NodeActionCallback;
      void register_node_action(String ^ name, NodeActionCallback::ManagedDelegate ^ cb);
      void unregister_node_action(String ^ name);
      bool trigger_action(String ^ name, NodeIdWrapper ^ node);

    private:
      Dictionary<String ^, NodeActionCallback ^> ^ _node_actions;

    public:
      typedef DelegateSlot1<void, void, std::vector<bec::NodeId>, List<NodeIdWrapper ^> ^> NodesActionCallback;
      void register_nodes_action(String ^ name, NodesActionCallback::ManagedDelegate ^ cb);
      void unregister_nodes_action(String ^ name);
      bool trigger_action(String ^ name, List<NodeIdWrapper ^> ^ node);

    private:
      Dictionary<String ^, NodesActionCallback ^> ^ _nodes_actions;

    public:
      typedef DelegateSlot2<void, void, std::vector<int>, List<int> ^, int, int> RowsColActionCallback;
      void register_rows_col_action(String ^ name, RowsColActionCallback::ManagedDelegate ^ cb);
      void unregister_rows_col_action(String ^ name);
      bool trigger_action(String ^ name, List<int> ^ rows, int column);

    private:
      Dictionary<String ^, RowsColActionCallback ^> ^ _rows_col_actions;
    };
  }
}

#endif
