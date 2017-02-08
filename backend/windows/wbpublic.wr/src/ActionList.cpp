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

#include "ConvUtils.h"
#include "GrtWrapper.h"
#include "GrtTemplates.h"
#include "DelegateWrapper.h"
#include "ModelWrappers.h"

#include "grt/tree_model.h"

#include "ActionList.h"

namespace MySQL {
  namespace Grt {

    ActionList::ActionList(::ActionList *inner)
      : _inner(inner),
        _actions(gcnew Dictionary<String ^, ActionCallback ^>()),
        _node_actions(gcnew Dictionary<String ^, NodeActionCallback ^>()),
        _nodes_actions(gcnew Dictionary<String ^, NodesActionCallback ^>()),
        _rows_col_actions(gcnew Dictionary<String ^, RowsColActionCallback ^>()) {
    }

    ActionList::~ActionList() {
      if (!_inner)
        return;

  for each(KeyValuePair<String ^, RowsColActionCallback ^> ^ pair in _rows_col_actions) delete pair->Value;
  delete _rows_col_actions;

  for each(KeyValuePair<String ^, NodesActionCallback ^> ^ pair in _nodes_actions) delete pair->Value;
  delete _nodes_actions;

  for each(KeyValuePair<String ^, NodeActionCallback ^> ^ pair in _node_actions) delete pair->Value;
  delete _node_actions;

  for each(KeyValuePair<String ^, ActionCallback ^> ^ pair in _actions) delete pair->Value;
  delete _actions;

  _inner = NULL;
    }

    void ActionList::reset() {
      _actions->Clear();
      _node_actions->Clear();
      _nodes_actions->Clear();
      _rows_col_actions->Clear();
      _inner->reset();
    }

    void ActionList::register_action(String ^ name, ActionCallback::ManagedDelegate ^ cb) {
      ActionCallback ^ actionCallback = gcnew ActionCallback(cb);
      _actions->Add(name, actionCallback);
      _inner->register_action(NativeToCppString(name), actionCallback->get_slot());
    }

    void ActionList::unregister_action(String ^ name) {
      _inner->unregister_action(NativeToCppString(name));
    }

    bool ActionList::trigger_action(String ^ name) {
      return _inner->trigger_action(NativeToCppString(name));
    }

    void ActionList::register_node_action(String ^ name, NodeActionCallback::ManagedDelegate ^ cb) {
      NodeActionCallback ^ actionCallback = gcnew NodeActionCallback(cb);
      _node_actions->Add(name, actionCallback);
      _inner->register_node_action(NativeToCppString(name), actionCallback->get_slot());
    }

    void ActionList::unregister_node_action(String ^ name) {
      return _inner->unregister_node_action(NativeToCppString(name));
    }

    bool ActionList::trigger_action(String ^ name, NodeIdWrapper ^ node) {
      return _inner->trigger_action(NativeToCppString(name), *node->get_unmanaged_object());
    }

    void ActionList::register_nodes_action(String ^ name, NodesActionCallback::ManagedDelegate ^ cb) {
      NodesActionCallback ^ actionCallback = gcnew NodesActionCallback(cb);
      _nodes_actions->Add(name, actionCallback);
      _inner->register_nodes_action(NativeToCppString(name), actionCallback->get_slot());
    }

    void ActionList::unregister_nodes_action(String ^ name) {
      return _inner->unregister_nodes_action(NativeToCppString(name));
    }

    bool ActionList::trigger_action(String ^ name, List<NodeIdWrapper ^> ^ nodes) {
      return _inner->trigger_action(NativeToCppString(name), ObjectListToCppVector<NodeIdWrapper, bec::NodeId>(nodes));
    }

    void ActionList::register_rows_col_action(String ^ name, RowsColActionCallback::ManagedDelegate ^ cb) {
      RowsColActionCallback ^ actionCallback = gcnew RowsColActionCallback(cb);
      _rows_col_actions->Add(name, actionCallback);
      _inner->register_rows_col_action(NativeToCppString(name), actionCallback->get_slot());
    }

    void ActionList::unregister_rows_col_action(String ^ name) {
      return _inner->unregister_rows_col_action(NativeToCppString(name));
    }

    bool ActionList::trigger_action(String ^ name, List<int> ^ rows, int column) {
      return _inner->trigger_action(NativeToCppString(name), NativeListToCppVector<int, int>(rows), column);
    }
  }
}
