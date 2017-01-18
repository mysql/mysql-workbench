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

#ifndef _WB_HISTORY_TREE_H_
#define _WB_HISTORY_TREE_H_

#include "mforms/treeview.h"
#include <grtpp_undo_manager.h>

namespace bec {
  class GRTManager;
};

namespace wb {
  class HistoryTree : public mforms::TreeView {
    grt::UndoManager *_undom;
    std::string _icon;
    bool _refresh_pending;

    void handle_redo(grt::UndoAction *);
    void handle_undo(grt::UndoAction *);
    void handle_change();

    void activate_node(mforms::TreeNodeRef node, int column);

  public:
    HistoryTree(grt::UndoManager *undom);

    void refresh();
  };
};

#endif
