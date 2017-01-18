/*
 * Copyright (c) 2013, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "mforms/treeview.h"
#include "grtpp_value.h"

namespace mforms {
  class ContextMenu;
}

namespace be {
  class ValueTreeBE;
}

namespace wb {
  class WBContext;
  class CatalogTreeBE;
  class ModelDiagramForm;

  enum NodeIcons {
    IconTablesMany,
    IconTable,
    IconViewsMany,
    IconView,
    IconRoutineGroupsMany,
    IconRoutineGroup,
    IconSchema
  };

  class MYSQLWBBACKEND_PUBLIC_FUNC CatalogTreeView : public mforms::TreeView {
  private:
    class ObjectNodeData : public mforms::TreeNodeData {
    private:
      grt::ObjectRef _ref;

    public:
      ObjectNodeData(grt::ObjectRef obj_ref);
      inline grt::ObjectRef get_object_ref();
    };

    enum ObjectType { ObjSchema, ObjTable, ObjView, ObjRoutineGrp, ObjNone };
    ModelDiagramForm *_owner;
    mforms::ContextMenu *_menu;
    std::list<GrtObjectRef> _dragged_objects;
    bool _initialized;

    void context_menu_will_show(mforms::MenuItem *parent_item);
    std::function<void(grt::ValueRef)> _activate_callback;

  protected:
    virtual bool get_drag_data(mforms::DragDetails &details, void **data, std::string &format);
    void menu_action(const std::string &name, grt::ValueRef val);
    mforms::TreeNodeRef create_new_node(const ObjectType &otype, mforms::TreeNodeRef parent, const std::string &name,
                                        grt::ObjectRef val);

  public:
    CatalogTreeView(ModelDiagramForm *owner);
    virtual ~CatalogTreeView();
    virtual void node_activated(mforms::TreeNodeRef row, int column);
    void refill(bool force = false);
    void set_activate_callback(const std::function<void(grt::ValueRef)> &active_callback);
    void mark_node(grt::ValueRef val, bool mark = true);
    void add_update_node_caption(grt::ValueRef val);
    void remove_node(grt::ValueRef val);
  };
};

#endif // _WB_CATALOG_TREE_VIEW_H_
