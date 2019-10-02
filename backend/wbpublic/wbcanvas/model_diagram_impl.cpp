/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "grts/structs.workbench.model.h"

#include "model_diagram_impl.h"
#include "model_model_impl.h"
#include "model_layer_impl.h"
#include "model_figure_impl.h"
#include "model_connection_impl.h"

#include "layer_figure.h"
#include "badge_figure.h"

#include "grt/grt_manager.h"
#include "base/string_utilities.h"
#include "base/log.h"

DEFAULT_LOG_DOMAIN(DOMAIN_CANVAS_BE)

using namespace grt;
using namespace base;

// this subclass allows repainting to happen in 3 steps, layer only, connection only and figure only
// to follow a repaint ordering that's independent from logical ordering
class RootAreaGroup : public mdc::AreaGroup {
public:
  RootAreaGroup(mdc::Layer *owner) : mdc::AreaGroup(owner) {
  }

  virtual void repaint(const Rect &clipArea, bool direct) {
    mdc::CairoCtx *cr = _layer->get_view()->cairoctx();

    std::list<mdc::AreaGroup *> layers;
    std::list<mdc::Line *> connections;
    std::list<mdc::CanvasItem *> figures;

    cr->save();

    for (auto end = _contents.rend(), iter = _contents.rbegin(); iter != end; ++iter) {
      mdc::CanvasItem *item = *iter;

      if (item->get_visible() && item->intersects(clipArea)) {
        if (dynamic_cast<mdc::Line *>(item))
          connections.push_back(static_cast<mdc::Line *>(item));
        else if (dynamic_cast<mdc::AreaGroup *>(item))
          layers.push_back(static_cast<mdc::AreaGroup *>(item));
        else
          figures.push_back(item);
      }
    }

    // 1st paint all backgrounds of area groups
    for (std::list<mdc::AreaGroup *>::iterator end = layers.end(), iter = layers.begin(); iter != end; ++iter) {
      ((mdc::Layouter *)(*iter))->repaint(clipArea, direct);
    }

    // 2nd paint all connections
    for (std::list<mdc::Line *>::iterator end = connections.end(), iter = connections.begin(); iter != end; ++iter) {
      (*iter)->repaint(clipArea, direct);
    }

    // 3rd paint figures in root layer
    for (std::list<mdc::CanvasItem *>::iterator end = figures.end(), iter = figures.begin(); iter != end; ++iter) {
      (*iter)->repaint(clipArea, direct);
    }

    // 4th paint figures inside area groups
    for (std::list<mdc::AreaGroup *>::iterator end = layers.end(), iter = layers.begin(); iter != end; ++iter) {
      Rect localClipArea(clipArea);
      localClipArea.pos = localClipArea.pos - (*iter)->get_position();
      (*iter)->repaint_contents(localClipArea, direct);
    }

    cr->restore();
  }
};

model_Diagram::ImplData::ImplData(model_Diagram *self) : _self(self), _canvas_view(0) {
  _updating_selection = 0;
  _connected_update = false;

  scoped_connect(self->signal_changed(), std::bind(&model_Diagram::ImplData::member_changed, this,
                                                   std::placeholders::_1, std::placeholders::_2));
  scoped_connect(self->signal_list_changed(),
                 std::bind(&model_Diagram::ImplData::member_list_changed, this, std::placeholders::_1,
                           std::placeholders::_2, std::placeholders::_3));
}

model_Diagram::ImplData::~ImplData() {
  unrealize();
}

bool model_Diagram::ImplData::is_realizable() {
  return true;
}

void model_Diagram::ImplData::set_page_counts(int x, int y) {
  Size pageSize(get_size_for_page(_self->owner()->get_data()->get_page_settings()));

  _self->width(pageSize.width * x);
  _self->height(pageSize.height * y);

  if (_self->_rootLayer.is_valid()) {
    _self->_rootLayer->width(_self->_width);
    _self->_rootLayer->height(_self->_height);
  }

  update_size();
}

//--------------------------------------------------------------------------------------------------

mdc::CanvasView *model_Diagram::ImplData::get_canvas_view() {
  // Create a canvas view if none exists yet.
  if (_canvas_view == NULL)
    realize();
  return _canvas_view;
}

//--------------------------------------------------------------------------------------------------

Size model_Diagram::ImplData::get_size_for_page(const app_PageSettingsRef &page) {
  Size size;

  if (page.is_valid() && page->paperType().is_valid()) {
    std::string name = page->paperType()->name();
    std::string id = page->paperType().id();

    size.width = (page->paperType()->width() - (page->marginLeft() + page->marginRight())) * page->scale();

    size.height = (page->paperType()->height() - (page->marginTop() + page->marginBottom())) * page->scale();
  } else {
    size.width = 1000;
    size.height = 1000;
  }

  if (page.is_valid() && page->orientation() == "landscape") {
    std::swap(size.width, size.height);
  }

  return size;
}

void model_Diagram::ImplData::block_updates(bool flag) {
}

void model_Diagram::ImplData::notify_object_realize(const model_ObjectRef &object) {
  _realize_object_signal(object);
}

void model_Diagram::ImplData::notify_object_will_unrealize(const model_ObjectRef &object) {
  _will_unrealize_object_signal(object);
}

void model_Diagram::ImplData::update_from_page_size() {
  if (_canvas_view) {
    Size pageSize(get_size_for_page(_self->owner()->get_data()->get_page_settings()));
    _canvas_view->set_page_size(pageSize);

    mdc::Count xc, yc;
    _canvas_view->get_page_layout(xc, yc);

    _self->_width = xc * pageSize.width;
    _self->_height = yc * pageSize.height;

    _self->_rootLayer->width(_self->_width);
    _self->_rootLayer->height(_self->_height);
  }
}

void model_Diagram::ImplData::update_size() {
  if (_canvas_view) {
    Size pageSize(get_size_for_page(_self->owner()->get_data()->get_page_settings()));

    if (!is_main_thread())
      run_later(std::bind(&mdc::CanvasView::set_page_size, _canvas_view, pageSize));
    else
      _canvas_view->set_page_size(pageSize);

    mdc::Count xc, yc;

    xc = (mdc::Count)ceil(*_self->_width / pageSize.width);
    yc = (mdc::Count)ceil(*_self->_height / pageSize.height);

    if (xc < 1)
      xc = 1;
    if (yc < 1)
      yc = 1;

    if (!is_main_thread())
      run_later(std::bind(&mdc::CanvasView::set_page_layout, _canvas_view, xc, yc));
    else
      _canvas_view->set_page_layout(xc, yc);
  }
  if (_self->_rootLayer.is_valid()) {
    _self->_rootLayer->width(_self->_width);
    _self->_rootLayer->height(_self->_height);

    _self->_rootLayer->get_data()->try_realize();
  }
}

void model_Diagram::ImplData::realize_selection() {
  // apply selection state that was previously saved
  begin_selection_update();

  for (size_t i = _self->_selection.count(); i > 0; --i) {
    model_ObjectRef object(_self->_selection.get(i - 1));

    if (object.is_instance<model_Figure>()) {
      model_Figure::ImplData *elem = dynamic_cast<model_Figure::ImplData *>(object->get_data());

      if (elem && elem->get_canvas_item())
        _canvas_view->get_selection()->add(elem->get_canvas_item());
      else // unselect items that don't exist in the canvas
        self()->unselectObject(object);
    } else if (object.is_instance<model_Connection>()) {
      model_Connection::ImplData *conn = dynamic_cast<model_Connection::ImplData *>(object->get_data());

      if (conn && conn->get_canvas_item())
        _canvas_view->get_selection()->add(conn->get_canvas_item());
      else // unselect items that don't exist in the canvas
        self()->unselectObject(object);
    } else if (object.is_instance<model_Layer>()) {
      model_Layer::ImplData *layer = dynamic_cast<model_Layer::ImplData *>(object->get_data());

      if (layer && layer->get_area_group())
        _canvas_view->get_selection()->add(layer->get_area_group());
      else // unselect items that don't exist in the canvas
        self()->unselectObject(object);
    }
  }

  end_selection_update();

  if (_canvas_view)
    g_return_if_fail(_canvas_view->get_selection()->get_contents().size() == _self->_selection.count());
}

void model_Diagram::ImplData::update_options(const std::string &key) {
  if (key == "workbench.physical.Diagram:DrawLineCrossings" || key.empty()) {
    model_Model::ImplData *model = _self->owner()->get_data();
    if (_canvas_view)
      _canvas_view->set_draws_line_hops(model->get_int_option("workbench.physical.Diagram:DrawLineCrossings", 1) == 1);
  }
}

void model_Diagram::ImplData::realize_contents() {
  _self->_rootLayer->get_data()->realize();

  for (size_t c = _self->_layers.count(), i = 0; i < c; i++) {
    _self->_layers[i]->get_data()->realize();
  }
  for (size_t c = _self->_figures.count(), i = 0; i < c; i++) {
    _self->_figures[i]->get_data()->realize();
  }
  for (size_t c = _self->_connections.count(), i = 0; i < c; i++) {
    _self->_connections[i]->get_data()->realize();
  }
}

bool model_Diagram::ImplData::realize() {
  if (!is_realizable())
    return false;

  if (!is_main_thread()) {
    run_later(std::bind(&model_Diagram::ImplData::realize, this));
    return true;
  }

  if (!_canvas_view) {
    model_Model::ImplData *model = _self->owner()->get_data();

    if (!_connected_update)
      scoped_connect(model->signal_options_changed(),
                     std::bind(&model_Diagram::ImplData::update_options, this, std::placeholders::_1));
    _connected_update = true;

    _canvas_view = model->get_delegate()->create_diagram(model_DiagramRef(_self));

    _canvas_view->get_current_layer()->set_root_area(new RootAreaGroup(_canvas_view->get_current_layer()));

    update_options("");

    _selection_signal_conn = _canvas_view->get_selection()->signal_changed()->connect(std::bind(
      &model_Diagram::ImplData::canvas_selection_changed, this, std::placeholders::_1, std::placeholders::_2));

    update_size();

    if (*_self->_zoom < 0.1)
      _self->_zoom = 0.1;

    _canvas_view->set_zoom((float)*_self->_zoom);

    realize_contents();

    run_later(std::bind(&model_Diagram::ImplData::realize_selection, this));
  }
  if (!_canvas_view) {
    if (!_self->owner().is_valid())
      throw std::logic_error("Owner model of view not specified");
    else
      throw std::logic_error("Could not get bridge for owner model of view");
  }
  return true;
}

void model_Diagram::ImplData::unrealize() {
  if (_selection_signal_conn.connected())
    _selection_signal_conn.disconnect();

  for (size_t c = _self->_figures.count(), i = 0; i < c; i++) {
    _self->_figures[i]->get_data()->unrealize();
  }
  for (size_t c = _self->_connections.count(), i = 0; i < c; i++) {
    _self->_connections[i]->get_data()->unrealize();
  }
  for (size_t c = _self->_layers.count(), i = 0; i < c; i++) {
    _self->_layers[i]->get_data()->unrealize();
  }
  if (_self->_rootLayer.is_valid() && _self->_rootLayer->get_data())
    _self->_rootLayer->get_data()->unrealize();

  if (_canvas_view) {
    _canvas_view->pre_destroy();
    if (_self->owner()->get_data()->get_delegate() != NULL)
      _self->owner()->get_data()->get_delegate()->free_canvas_view(_canvas_view);

    _canvas_view = NULL;
  }
}

void model_Diagram::ImplData::member_changed(const std::string &name, const grt::ValueRef &ovalue) {
  if (name == "zoom") {
    if (*_self->_zoom <= 0.1)
      _self->_zoom = 0.1;
    else if (*_self->_zoom > 2.0)
      _self->_zoom = 2.0;

    if (_canvas_view)
      _canvas_view->set_zoom((float)*_self->_zoom);
  } else if (name == "x" || name == "y") {
    if (_canvas_view)
      _canvas_view->set_offset(base::Point(_self->_x, _self->_y));
  } else if (name == "pageSettings") {
    update_size();
  } else if (name == "width" || name == "height") {
    update_size();
  }
}

void model_Diagram::ImplData::member_list_changed(grt::internal::OwnedList *alist, bool added,
                                                  const grt::ValueRef &value) {
  grt::BaseListRef list(alist);

  if (list == self()->_figures) {
    model_FigureRef figure(model_FigureRef::cast_from(value));
    figure->get_data()->set_in_view(added);
  } else if (list == self()->_connections) {
    model_ConnectionRef conn(model_ConnectionRef::cast_from(value));
    conn->get_data()->set_in_view(added);
  } else if (list == self()->_layers) {
    if (value != self()->_rootLayer) {
      model_LayerRef layer(model_LayerRef::cast_from(value));
      layer->get_data()->set_in_view(added);
    }
  } else if (list == self()->_selection) {
    // consistency check, selection changes shouldn't be tracked in undo history
    if (grt::GRT::get()->get_undo_manager()->is_enabled() && grt::GRT::get()->tracking_changes())
      logWarning("Undo tracking was enabled during selection change\n");
  }
}

void model_Diagram::ImplData::add_figure(const model_FigureRef &figure) {
  _self->_figures.insert(figure);
  if (figure->layer().is_valid())
    figure->layer()->figures().insert(figure);
  else
    _self->rootLayer()->figures().insert(figure);
}

void model_Diagram::ImplData::add_connection(const model_ConnectionRef &conn) {
  _self->_connections.insert(conn);
}

void model_Diagram::ImplData::remove_figure(const model_FigureRef &figure) {
  _self->_figures.remove_value(figure);
  if (figure->layer().is_valid()) {
    figure->layer()->figures().remove_value(figure);
  }
}

void model_Diagram::ImplData::remove_connection(const model_ConnectionRef &conn) {
  _self->_connections.remove_value(conn);
}

void model_Diagram::ImplData::delete_layer(const model_LayerRef &layer) {
  grt::AutoUndo undo(!self()->is_global());

  model_LayerRef root(self()->rootLayer());

  // remove objects contained in this
  for (size_t c = layer->figures().count(), i = 0; i < c; i++) {
    model_FigureRef object(layer->figures().get(c - i - 1));

    layer->figures().remove(c - i - 1);
    root->figures().insert(object);
    object->layer(root);
  }

  _self->_layers.remove_value(layer);

  undo.end(_("Delete Layer from View"));
}

void model_Diagram::ImplData::select_object(const model_ObjectRef &object) {
  if (_self->_selection.get_index(object) != grt::BaseListRef::npos)
    return;

  if (object.is_instance<model_Figure>()) {
    model_Figure::ImplData *elem = dynamic_cast<model_Figure::ImplData *>(object->get_data());

    begin_selection_update();
    if (elem && elem->get_canvas_item())
      _canvas_view->get_selection()->add(elem->get_canvas_item());

    grt::GRT::get()->get_undo_manager()->disable();
    _self->_selection.insert(object);
    grt::GRT::get()->get_undo_manager()->enable();
  } else if (object.is_instance<model_Connection>()) {
    model_Connection::ImplData *conn = dynamic_cast<model_Connection::ImplData *>(object->get_data());

    begin_selection_update();
    if (conn && conn->get_canvas_item())
      _canvas_view->get_selection()->add(conn->get_canvas_item());

    grt::GRT::get()->get_undo_manager()->disable();
    _self->_selection.insert(object);
    grt::GRT::get()->get_undo_manager()->enable();
  } else if (object.is_instance<model_Layer>()) {
    model_Layer::ImplData *layer = dynamic_cast<model_Layer::ImplData *>(object->get_data());

    begin_selection_update();
    if (layer && layer->get_area_group())
      _canvas_view->get_selection()->add(layer->get_area_group());

    grt::GRT::get()->get_undo_manager()->disable();
    _self->_selection.insert(object);
    grt::GRT::get()->get_undo_manager()->enable();
  } else
    throw std::runtime_error("trying to select invalid object");

  end_selection_update();
}

void model_Diagram::ImplData::unselect_object(const model_ObjectRef &object) {
  if (object.is_instance<model_Figure>()) {
    model_Figure::ImplData *elem = dynamic_cast<model_Figure::ImplData *>(object->get_data());

    begin_selection_update();
    if (elem && elem->get_canvas_item())
      _canvas_view->get_selection()->remove(elem->get_canvas_item());

    grt::GRT::get()->get_undo_manager()->disable();
    _self->_selection.remove_value(object);
    grt::GRT::get()->get_undo_manager()->enable();
  } else if (object.is_instance<model_Connection>()) {
    model_Connection::ImplData *conn = dynamic_cast<model_Connection::ImplData *>(object->get_data());

    begin_selection_update();
    if (conn && conn->get_canvas_item())
      _canvas_view->get_selection()->remove(conn->get_canvas_item());

    grt::GRT::get()->get_undo_manager()->disable();
    _self->_selection.remove_value(object);
    grt::GRT::get()->get_undo_manager()->enable();
  } else if (object.is_instance<model_Layer>()) {
    model_Layer::ImplData *layer = dynamic_cast<model_Layer::ImplData *>(object->get_data());

    begin_selection_update();
    if (layer && layer->get_area_group())
      _canvas_view->get_selection()->remove(layer->get_area_group());

    grt::GRT::get()->get_undo_manager()->disable();
    _self->_selection.remove_value(object);
    grt::GRT::get()->get_undo_manager()->enable();
  } else
    throw std::runtime_error("trying to deselect invalid object");

  end_selection_update();
}

void model_Diagram::ImplData::unselect_all() {
  begin_selection_update();

  _canvas_view->get_selection()->clear();

  grt::GRT::get()->get_undo_manager()->disable();
  while (_self->_selection.count() > 0)
    _self->_selection.remove(0);
  grt::GRT::get()->get_undo_manager()->enable();

  end_selection_update();
}

/**
 * Increases the selection lock count and returns true if the caller can continue.
 */
bool model_Diagram::ImplData::begin_selection_update() {
  return ++_updating_selection == 1;
}

/**
 * Removes one lock level from the selection lock and triggers the change event if
 * it reaches 0.
 */
void model_Diagram::ImplData::end_selection_update() {
  _updating_selection--;
  if (_updating_selection == 0)
    _selection_changed_signal(model_DiagramRef(_self));
}

void model_Diagram::ImplData::canvas_selection_changed(bool added, mdc::CanvasItem *item) {
  if (begin_selection_update()) {
    if (added) {
      model_ObjectRef object;

      if (item) {
        object = find_object_in_list(_self->_figures, item->get_tag());
        if (!object.is_valid())
          object = find_object_in_list(_self->_connections, item->get_tag());
        if (!object.is_valid())
          object = find_object_in_list(_self->_layers, item->get_tag());

        if (object.is_valid()) {
          grt::GRT::get()->get_undo_manager()->disable();
          if (!find_object_in_list(_self->_selection, item->get_tag()).is_valid())
            _self->_selection.insert(object);
          grt::GRT::get()->get_undo_manager()->enable();
        }
      } else
        abort();
    } else // removed
    {
      if (item) {
        model_ObjectRef object(find_object_in_list(_self->_selection, item->get_tag()));

        grt::GRT::get()->get_undo_manager()->disable();
        if (object.is_valid())
          _self->_selection.remove_value(object);
        grt::GRT::get()->get_undo_manager()->enable();
      } else {
        grt::GRT::get()->get_undo_manager()->disable();
        while (_self->_selection.count() > 0)
          _self->_selection.remove(0);
        grt::GRT::get()->get_undo_manager()->enable();
      }
    }
  }

  end_selection_update();
}

static mdc::CanvasItem *get_first_realized_layer_under(const grt::ListRef<model_Layer> &list,
                                                       const model_LayerRef &layer) {
  bool found = false;
  if (!layer.is_valid())
    found = true;

  for (grt::ListRef<model_Layer>::const_reverse_iterator it = list.rbegin(); it != list.rend(); ++it) {
    if (found) {
      model_Layer::ImplData *layer = (*it)->get_data();
      if (layer && layer->get_area_group())
        return layer->get_area_group();
    }
    if (!found && *it == layer)
      found = true;
  }
  return 0;
}

static mdc::CanvasItem *get_first_realized_connection_under(const grt::ListRef<model_Connection> &list,
                                                            const model_ConnectionRef &connection) {
  bool found = false;
  if (!connection.is_valid())
    found = true;
  for (grt::ListRef<model_Connection>::const_reverse_iterator it = list.rbegin(); it != list.rend(); ++it) {
    if (found) {
      model_Connection::ImplData *conn = (*it)->get_data();
      if (conn && conn->get_canvas_item())
        return conn->get_canvas_item();
    }
    if (!found && *it == connection)
      found = true;
  }
  return 0;
}

static mdc::CanvasItem *get_first_realized_figure_under(const grt::ListRef<model_Figure> &list,
                                                        const model_FigureRef &figure) {
  bool found = false;
  if (!figure.is_valid())
    found = true;
  for (grt::ListRef<model_Figure>::const_reverse_iterator it = list.rbegin(); it != list.rend(); ++it) {
    if (found) {
      model_Figure::ImplData *fig = (*it)->get_data();
      if (fig && fig->get_canvas_item())
        return fig->get_canvas_item();
    }
    if (!found && *it == figure)
      found = true;
  }
  return 0;
}

void model_Diagram::ImplData::stack_layer(const model_LayerRef &layer, mdc::CanvasItem *layer_item) {
  mdc::CanvasItem *item_under;

  item_under = get_first_realized_layer_under(_self->_layers, layer);

  if (item_under)
    _canvas_view->get_current_layer()->get_root_area_group()->raise_item(layer_item, item_under);
  else
    _canvas_view->get_current_layer()->get_root_area_group()->lower_item(layer_item);
}

void model_Diagram::ImplData::stack_connection(const model_ConnectionRef &conn, mdc::CanvasItem *connection_item) {
  mdc::CanvasItem *item_under;

  item_under = get_first_realized_connection_under(_self->_connections, conn);
  if (!item_under)
    item_under = get_first_realized_layer_under(_self->_layers, model_LayerRef());

  if (item_under)
    _canvas_view->get_current_layer()->get_root_area_group()->raise_item(connection_item, item_under);
  else
    _canvas_view->get_current_layer()->get_root_area_group()->lower_item(connection_item);
}

void model_Diagram::ImplData::stack_figure(const model_FigureRef &figure, mdc::CanvasItem *figure_item) {
  mdc::CanvasItem *item_under;

  item_under = get_first_realized_figure_under(figure->layer()->figures(), figure);

  _canvas_view->get_current_layer()->get_root_area_group()->raise_item(figure_item, item_under);
}

model_LayerRef model_Diagram::ImplData::get_layer_under_figure(const model_FigureRef &figure) {
  Rect bounds;

  mdc::CanvasItem *item = figure->get_data()->get_canvas_item();
  if (item)
    bounds = item->get_root_bounds();
  else {
    model_LayerRef currentLayer(figure->layer());
    if (currentLayer.is_valid()) {
      bounds.pos.x = figure->left() + currentLayer->left();
      bounds.pos.y = figure->top() + currentLayer->top();
    } else {
      bounds.pos.x = figure->left();
      bounds.pos.y = figure->top();
    }
    bounds.size.width = figure->width();
    bounds.size.height = figure->height();
  }

  for (grt::ListRef<model_Layer>::const_reverse_iterator it = _self->layers().rbegin(); it != _self->layers().rend();
       ++it) {
    model_LayerRef layer(*it);
    Rect layerBounds;

    layerBounds.pos.x = layer->left();
    layerBounds.pos.y = layer->top();
    layerBounds.size.width = layer->width();
    layerBounds.size.height = layer->height();
    if (mdc::bounds_contain_bounds(layerBounds, bounds))
      return layer;
  }

  return _self->rootLayer();
}

bool model_Diagram::ImplData::update_layer_of_figure(const model_FigureRef &figure) {
  bool relocated = false;
  grt::AutoUndo undo;

  model_LayerRef layer(get_layer_under_figure(figure));

  if (layer != _self->rootLayer()) {
    // figure is in some layer
    if (layer != figure->layer()) {
      if (figure->layer().is_valid())
        figure->layer()->figures().remove_value(figure);

      figure->layer(layer);
      layer->figures().insert(figure);
      relocated = true;
    } else {
      mdc::CanvasItem *item = figure->get_data()->get_canvas_item();
      mdc::AreaGroup *parent = layer->get_data()->get_area_group();
      // if figure stays in same layer as before, make sure that it has correct canvas parent
      if (item && parent && item->get_parent() != parent) {
        Point pos = item->get_root_position() - parent->get_root_position();
        parent->add(item);
        item->move_to(pos);
      }
    }
  } else {
    // figure is outside of any layer
    if (figure->layer() != _self->rootLayer()) {
      if (figure->layer().is_valid())
        figure->layer()->figures().remove_value(figure);

      figure->layer(_self->rootLayer());
      _self->rootLayer()->figures().insert(figure);
      relocated = true;
    }
  }

  if (relocated)
    undo.end(base::strfmt(_("Updated Layer for '%s'"), figure->name().c_str()));
  else
    undo.cancel();

  return relocated;
}

//--------------------------------------------------------------------------------------------------

bool model_Diagram::ImplData::figure_click(const model_ObjectRef &owner, mdc::CanvasItem *target, const Point &point,
                                           mdc::MouseButton button, mdc::EventState state) {
  _item_click_signal(owner, target, point, button, state);

  return false;
}

//--------------------------------------------------------------------------------------------------

bool model_Diagram::ImplData::figure_double_click(const model_ObjectRef &owner, mdc::CanvasItem *target,
                                                  const Point &point, mdc::MouseButton button, mdc::EventState state) {
  _item_double_click_signal(owner, target, point, button, state);

  return false;
}

//--------------------------------------------------------------------------------------------------

bool model_Diagram::ImplData::figure_button_press(const model_ObjectRef &owner, mdc::CanvasItem *target,
                                                  const Point &point, mdc::MouseButton button, mdc::EventState state) {
  _item_mouse_button_signal(owner, target, true, point, button, state);

  return false;
}

bool model_Diagram::ImplData::figure_button_release(const model_ObjectRef &owner, mdc::CanvasItem *target,
                                                    const Point &point, mdc::MouseButton button,
                                                    mdc::EventState state) {
  _item_mouse_button_signal(owner, target, false, point, button, state);

  return false;
}

bool model_Diagram::ImplData::figure_enter(const model_ObjectRef &owner, mdc::CanvasItem *target, const Point &point) {
  _item_crossed_signal(owner, target, true, point);

  return false;
}

bool model_Diagram::ImplData::figure_leave(const model_ObjectRef &owner, mdc::CanvasItem *target, const Point &point) {
  _item_crossed_signal(owner, target, false, point);

  return false;
}

static void update_badge(const std::string &name, const ValueRef &ovalue, const meta_TagRef &tag, BadgeFigure *badge) {
  if (name == "label") {
    badge->set_text(tag->label());
  } else if (name == "color") {
    badge->set_gradient_from_color(Color::parse(tag->color()));
  }
}

void model_Diagram::ImplData::add_tag_badge_to_figure(const model_FigureRef &figure, const meta_TagRef &tag) {
  BadgeFigure *badge = new BadgeFigure(get_canvas_view()->get_current_layer());

  badge->set_badge_id(tag->id());
  badge->set_text(tag->label());
  badge->set_gradient_from_color(Color::parse(*tag->color()));
  badge->set_text_color(Color::parse("#ffffff"));

  badge->updater_connection =
    tag->signal_changed()->connect(std::bind(&update_badge, std::placeholders::_1, std::placeholders::_2, tag, badge));

  get_canvas_view()->get_current_layer()->add_item(badge,
                                                   get_canvas_view()->get_current_layer()->get_root_area_group());

  figure->get_data()->add_badge(badge);
}

void model_Diagram::ImplData::remove_tag_badge_from_figure(const model_FigureRef &figure, const meta_TagRef &tag) {
  BadgeFigure *badge = figure->get_data()->get_badge_with_id(tag->id());

  if (badge) {
    figure->get_data()->remove_badge(badge);
    get_canvas_view()->get_current_layer()->remove_item(badge);
    delete badge;
  }
}
