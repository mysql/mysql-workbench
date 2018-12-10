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

#include "wb_layer_tree.h"

#include "workbench/wb_context.h"
#include "workbench/wb_context_ui.h"

#include "model/wb_context_model.h"
#include "model/wb_model_diagram_form.h"
#include "grts/structs.model.h"
#include "grts/structs.workbench.physical.h"

#include <algorithm>

using namespace wb;

LayerTree::LayerTree(ModelDiagramForm *form, const model_DiagramRef &diagram)
  : mforms::TreeView(mforms::TreeSidebar | mforms::TreeNoHeader | mforms::TreeIndexOnTag | mforms::TreeTranslucent),
    _form(form),
    _diagram(diagram),
    _updating_selection(false) {
  add_column(mforms::IconStringColumnType, "Object", 200, false);
  end_columns();

  set_selection_mode(mforms::TreeSelectMultiple);

  signal_node_activated()->connect(
    std::bind(&LayerTree::activate_node, this, std::placeholders::_1, std::placeholders::_2));
  signal_changed()->connect(std::bind(&LayerTree::selection_changed, this));

  scoped_connect(diagram->signal_list_changed(),
                 std::bind(&LayerTree::diagram_objects_changed, this, std::placeholders::_1, std::placeholders::_2,
                           std::placeholders::_3));
}

void LayerTree::add_figure_node(mforms::TreeNodeRef parent, model_FigureRef figure, int insertion_point) {
  bec::IconManager *im = bec::IconManager::get_instance();
  mforms::TreeNodeRef fnode;
  FigureNode *fchild = new FigureNode();
  fchild->object = figure;
  fchild->is_layer = false;

  if (insertion_point < 0)
    fnode = parent ? parent->add_child() : add_node();
  else
    fnode = parent ? parent->insert_child(insertion_point) : root_node()->insert_child(insertion_point);

  fnode->set_data(fchild);
  fnode->set_string(0, figure->name());
  fnode->set_tag(figure->id());
  fnode->set_icon_path(0, im->get_icon_path(im->get_icon_id(figure, bec::Icon16)));

  fchild->conn = figure->signal_changed()->connect(
    std::bind(&LayerTree::object_changed, this, std::placeholders::_1, std::placeholders::_2, fnode));

  std::string conn_icon = im->get_icon_path("db.Column.fknn.16x16.png");
  std::string id = figure->id();
  std::multimap<std::string, workbench_physical_ConnectionRef>::iterator iter;
  for (iter = _figure_connections.find(id); iter != _figure_connections.end() && iter->first == id; ++iter) {
    mforms::TreeNodeRef cnode = fnode->add_child();
    FigureNode *cchild = new FigureNode();
    cchild->object = iter->second->endFigure();
    cchild->is_layer = false;
    cnode->set_data(cchild);
    cnode->set_string(0, iter->second->caption());
    cnode->set_tag(iter->second.id());
    cnode->set_icon_path(0, conn_icon);
  }
}

void LayerTree::refresh() {
  bec::IconManager *im = bec::IconManager::get_instance();
  model_LayerRef layer = _diagram->rootLayer();

  // create mapping of figures to connections
  grt::ListRef<model_Connection> connections(_diagram->connections());
  for (size_t c = connections.count(), i = 0; i < c; i++) {
    if (connections[i]->startFigure().is_valid())
      _figure_connections.insert(std::pair<std::string, workbench_physical_ConnectionRef>(
        connections[i]->startFigure().id(), workbench_physical_ConnectionRef::cast_from(connections[i])));
  }

  clear();

  mforms::TreeNodeRef node;
  size_t l = 0;
  for (;;) {
    FigureNode *child = NULL;

    for (size_t fc = layer->figures().count(), f = 0; f < fc; f++) {
      model_FigureRef figure(layer->figures().get(f));

      add_figure_node(node, figure);
    }
    if (node)
      node->expand();

    if (l < _diagram->layers().count())
      layer = _diagram->layers()[l++];
    else
      break;

    child = new FigureNode();
    child->object = layer;
    child->is_layer = true;

    node = add_node();

    node->set_data(child);
    node->set_string(0, layer->name());
    node->set_tag(layer->id());

    node->set_icon_path(0, im->get_icon_path(im->get_icon_id(layer, bec::Icon16)));
  }
  refresh_selection_status();
}

void LayerTree::object_changed(const std::string &key, const grt::ValueRef &value, mforms::TreeNodeRef node) {
  FigureNode *fn = dynamic_cast<FigureNode *>(node->get_data());

  if (key == "name")
    node->set_string(0, fn->object->name());
  else if (key == "layer") {
    model_FigureRef figure(model_FigureRef::cast_from(fn->object));
    if (figure->layer() != value) {
      fn->conn.disconnect();
      node->remove_from_parent();

      if (figure->layer() == _diagram->rootLayer()) {
        int insertion_point = 0;
        // find the last node that's not a layer
        for (int c = count(); insertion_point < c; ++insertion_point) {
          mforms::TreeNodeRef n = node_at_row(insertion_point);
          FigureNode *nn = dynamic_cast<FigureNode *>(n->get_data());
          if (!nn)
            continue;
          if (nn->is_layer)
            break;
        }
        add_figure_node(mforms::TreeNodeRef(), figure, insertion_point);
      } else {
        mforms::TreeNodeRef layer_node = node_with_tag(figure->layer().id());

        add_figure_node(layer_node, figure);
      }
    }
  }
}

void LayerTree::selection_changed() {
  std::vector<model_ObjectRef> new_selection;
  std::vector<model_ObjectRef> old_selection;
  {
    std::list<mforms::TreeNodeRef> selection(get_selection());

    for (std::list<mforms::TreeNodeRef>::const_iterator iter = selection.begin(); iter != selection.end(); ++iter) {
      FigureNode *fnode = dynamic_cast<FigureNode *>((*iter)->get_data());
      if (fnode)
        new_selection.push_back(fnode->object);
    }

    for (size_t c = _diagram->selection().count(), i = 0; i < c; i++)
      old_selection.push_back(_diagram->selection().get(i));
  }

  std::sort(new_selection.begin(), new_selection.end());
  std::sort(old_selection.begin(), old_selection.end());

  std::vector<model_ObjectRef>::const_iterator end;
  _updating_selection = true;
  std::vector<model_ObjectRef> to_unselect(old_selection.size());
  end = std::set_difference(old_selection.begin(), old_selection.end(), new_selection.begin(), new_selection.end(),
                            to_unselect.begin());
  for (std::vector<model_ObjectRef>::const_iterator it = to_unselect.begin(); it != end; ++it)
    _diagram->unselectObject(*it);

  std::vector<model_ObjectRef> to_select(new_selection.size());
  end = std::set_difference(new_selection.begin(), new_selection.end(), old_selection.begin(), old_selection.end(),
                            to_select.begin());
  for (std::vector<model_ObjectRef>::const_iterator it = to_select.begin(); it != end; ++it)
    _diagram->selectObject(*it);

  _updating_selection = false;
}

void LayerTree::refresh_selection_status() {
  clear_selection();
  for (size_t c = _diagram->selection().count(), i = 0; i < c; i++) {
    std::string id = _diagram->selection().get(i)->id();
    mforms::TreeNodeRef node = node_with_tag(id);
    if (node)
      set_node_selected(node, true);
  }
}

void LayerTree::diagram_objects_changed(grt::internal::OwnedList *list, bool added, const grt::ValueRef &value) {
  if (list == _diagram->figures().valueptr()) {
    model_FigureRef figure(model_FigureRef::cast_from(value));
    if (added) {
      {
        _figure_connections.clear();
        grt::ListRef<model_Connection> connections(_diagram->connections());
        for (size_t c = connections.count(), i = 0; i < c; i++) {
          if (connections[i]->startFigure() == figure)
            _figure_connections.insert(std::pair<std::string, workbench_physical_ConnectionRef>(
              connections[i]->startFigure().id(), workbench_physical_ConnectionRef::cast_from(connections[i])));
        }
      }
      model_LayerRef layer(figure->layer());
      mforms::TreeNodeRef lnode = node_with_tag(layer->id());

      int insertion_point;
      if (lnode)
        insertion_point = -1;
      else {
        int c = count();
        // find the last node that's not a layer
        for (insertion_point = 0; insertion_point < c; ++insertion_point) {
          mforms::TreeNodeRef n = node_at_row(insertion_point);
          FigureNode *nn = dynamic_cast<FigureNode *>(n->get_data());
          if (nn == nullptr)
            continue;
          if (nn->is_layer)
            break;
        }
      }
      add_figure_node(lnode, figure, insertion_point);
    } else {
      mforms::TreeNodeRef node = node_with_tag(figure->id());
      if (node)
        node->remove_from_parent();
    }
  } else if (list == _diagram->layers().valueptr()) {
    if (added) {
      model_LayerRef layer(model_LayerRef::cast_from(value));
      if (layer->figures().count() == 0) {
        FigureNode *child = new FigureNode();
        child->object = layer;
        child->is_layer = true;

        bec::IconManager *im = bec::IconManager::get_instance();
        mforms::TreeNodeRef node = add_node();
        node->set_string(0, layer->name());
        node->set_icon_path(0, im->get_icon_path(im->get_icon_id(layer, bec::Icon16)));
        node->set_tag(layer->id());
        node->set_data(child);
        node->expand();
      } else
        refresh();
    } else
      refresh();
  } else if (list == _diagram->selection().valueptr()) {
    if (!_updating_selection) {
      mforms::TreeNodeRef node = node_with_tag(GrtObjectRef::cast_from(value)->id());
      if (!node)
        return;

      set_node_selected(node, added);
    }
  }
}

void LayerTree::activate_node(const mforms::TreeNodeRef &node, int column) {
  FigureNode *fnode = dynamic_cast<FigureNode *>(node->get_data());
  if (fnode)
    _form->focus_and_make_visible(fnode->object, true);
}

void LayerTree::handle_menu_action(const std::string &name) {
  /*
  std::vector<bec::NodeId> nodes(unsorted_nodes);
  std::sort(nodes.begin(), nodes.end());

  if (name == "select")
  {
    for (std::vector<bec::NodeId>::const_iterator iter= nodes.begin(); iter != nodes.end(); ++iter)
    {
      activate_node(*iter);
    }
  }
  else if (name == "edit")
  {
    if (!nodes.empty())
    {
      grt::ValueRef value(get_node_value(nodes[0]));
      if (model_ObjectRef::can_wrap(value))
        _wb->get_model_context()->activate_canvas_object(model_ObjectRef::cast_from(value), 0);
    }
  }
  else if (name == "reveal")
  {
    for (int i = nodes.size() - 1; i >= 0; --i)
    {
      grt::ValueRef value(get_node_value(nodes[i]));

      if (model_ConnectionRef::can_wrap(value))
      {
        model_ConnectionRef conn = model_ConnectionRef::cast_from(value);
        conn->visible(1);
        conn->drawSplit(0);
      }
    }
  }
*/
}
