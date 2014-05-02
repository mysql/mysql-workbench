/*
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _WB_CATALOG_TREE_VIEW_H_
#define _WB_CATALOG_TREE_VIEW_H_

#include "workbench/wb_backend_public_interface.h"

#include "mforms/treenodeview.h"

namespace mforms {
  class ContextMenu;
}

namespace be {
  class ValueTreeBE;
}

namespace wb {
  class WBContext;
  class CatalogTreeBE;

  class MYSQLWBBACKEND_PUBLIC_FUNC CatalogTreeView : public mforms::TreeNodeView
  {
  private:
    mforms::ContextMenu *_menu;
    CatalogTreeBE *_model;
    std::list<GrtObjectRef> _dragged_objects;
    bool _initialized;

    void context_menu_will_show(mforms::MenuItem *parent_item);
    void fill_node(mforms::TreeNodeRef parent_node, bec::NodeId parent_id);
    void model_changed(bec::NodeId id, int old_child_count);
    void update_parent_node(mforms::TreeNodeRef parent_node, bec::NodeId parent_id);
    void update_captions();

  protected:
    virtual bool get_drag_data(mforms::DragDetails &details, void **data, std::string &format);

  public:
    CatalogTreeView(CatalogTreeBE *model);
    virtual ~CatalogTreeView();
    
    void refresh();

    virtual void node_activated(mforms::TreeNodeRef row, int column);
    virtual bool can_expand(mforms::TreeNodeRef row);
    virtual void expand_toggle(mforms::TreeNodeRef row, bool expanded);
  };
};


#endif // _WB_CATALOG_TREE_VIEW_H_
