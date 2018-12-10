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

#include "wb_history_tree.h"
#include "workbench/wb_context.h"
#include "base/string_utilities.h"

using namespace wb;
using namespace bec;
using namespace grt;

HistoryTree::HistoryTree(UndoManager *undom)
  : mforms::TreeView(mforms::TreeFlatList | mforms::TreeSidebar | mforms::TreeNoHeader | mforms::TreeTranslucent),
    _undom(undom),
    _refresh_pending(false) {
  add_column(mforms::IconStringColumnType, "Action", 200);
  end_columns();

  _icon = bec::IconManager::get_instance()->get_icon_path("history.png");

  scoped_connect(undom->signal_redo(), std::bind(&HistoryTree::handle_redo, this, std::placeholders::_1));
  scoped_connect(undom->signal_undo(), std::bind(&HistoryTree::handle_undo, this, std::placeholders::_1));
  scoped_connect(undom->signal_changed(), std::bind(&HistoryTree::handle_change, this));
  scoped_connect(signal_node_activated(),
                 std::bind(&HistoryTree::activate_node, this, std::placeholders::_1, std::placeholders::_2));
}

void HistoryTree::refresh() {
  _undom->lock();
  std::deque<UndoAction *> undostack(_undom->get_undo_stack());
  std::deque<UndoAction *> redostack(_undom->get_redo_stack());

  _refresh_pending = false;

  int new_count = (int)(undostack.size() + redostack.size());
  while (count() < new_count)
    add_node();
  while (count() > new_count)
    node_at_row(count() - 1)->remove_from_parent();

  // just update the captions of the existing nodes

  int row = 0;
  for (std::deque<UndoAction *>::const_iterator iter = undostack.begin(); iter != undostack.end(); ++iter) {
    mforms::TreeNodeRef node = node_at_row(row++);
    node->set_icon_path(0, _icon);
    node->set_string(0, (*iter)->description());
  }

  for (std::deque<UndoAction *>::const_reverse_iterator iter = redostack.rbegin(); iter != redostack.rend(); ++iter) {
    mforms::TreeNodeRef node = node_at_row(row++);
    node->set_icon_path(0, _icon);
    node->set_string(0, "(" + (*iter)->description() + ")");
  }
  _undom->unlock();
}

void HistoryTree::activate_node(mforms::TreeNodeRef node, int column) {
  std::deque<UndoAction *> &undostack(_undom->get_undo_stack());
  std::deque<UndoAction *> &redostack(_undom->get_redo_stack());
  if (!node)
    return;
  int row = row_for_node(node);

  // undo
  if (row < (int)undostack.size()) {
    while ((int)undostack.size() > row)
      _undom->undo();
  }
  // redo
  else if (row <= (int)(undostack.size() + redostack.size())) {
    while ((int)undostack.size() <= row)
      _undom->redo();
  }
}

void HistoryTree::handle_redo(grt::UndoAction *action) {
  refresh();
}

void HistoryTree::handle_undo(grt::UndoAction *action) {
  refresh();
}

void HistoryTree::handle_change() {
  if (!_refresh_pending) {
    _refresh_pending = true;
    bec::GRTManager::get()->run_once_when_idle(this, std::bind(&HistoryTree::refresh, this));
  }
}
