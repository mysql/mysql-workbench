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

#ifndef _WB_LAYER_TREE_H_
#define _WB_LAYER_TREE_H_

#include "mforms/treeview.h"
#include <set>

#include "grts/structs.workbench.physical.h"

namespace wb {
  class WBContextUI;
  class ModelDiagramForm;

  class LayerTree : public mforms::TreeView {
  private:
    class FigureNode : public mforms::TreeNodeData {
    public:
      model_ObjectRef object;
      boost::signals2::scoped_connection conn;
      bool is_layer;
    };

    ModelDiagramForm *_form;
    model_DiagramRef _diagram;
    std::multimap<std::string, workbench_physical_ConnectionRef> _figure_connections;
    bool _updating_selection;

    void activate_node(const mforms::TreeNodeRef &node, int column);
    void handle_menu_action(const std::string &name);

    void selection_changed();

    void add_figure_node(mforms::TreeNodeRef parent, model_FigureRef figure, int insertion_point = -1);

    void object_changed(const std::string &key, const grt::ValueRef &value, mforms::TreeNodeRef node);
    void diagram_objects_changed(grt::internal::OwnedList *list, bool added, const grt::ValueRef &value);

  public:
    LayerTree(ModelDiagramForm *_form, const model_DiagramRef &diagram);
    void refresh();

    void refresh_selection_status();
  };
};

#endif
