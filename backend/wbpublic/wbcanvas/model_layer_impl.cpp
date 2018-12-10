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

#include "model_layer_impl.h"
#include "model_diagram_impl.h"
#include "model_model_impl.h"
#include "model_figure_impl.h"

#include "layer_figure.h"

#include "grt/common.h"
#include "grt/grt_manager.h"

#include <grtpp_util.h>
#include <grtpp_undo_manager.h>
#include "base/string_utilities.h"

#define LAYER_ALPHA 1
#define LAYER_BORDER_COLOR Color(0.8, 0.8, 0.8)

using namespace std;
using namespace base;

model_Layer::ImplData::ImplData(model_Layer *self) : model_Object::ImplData(self), _area_group(0) {
  scoped_connect(self->signal_changed(),
                 std::bind(&ImplData::member_changed, this, std::placeholders::_1, std::placeholders::_2));
}

model_Layer::ImplData::~ImplData() {
  unrealize();
}

void model_Layer::ImplData::unrealize() {
  if (_area_group) {
    if (is_canvas_view_valid())
      get_canvas_view()->lock();

    for (size_t c = self()->_figures.count(), i = 0; i < c; i++) {
      model_Figure::ImplData *fig = self()->_figures[i]->get_data();
      if (fig)
        fig->unrealize();
    }

    _area_group->set_visible(false);

    _area_group->get_view()->remove_item(_area_group);

    if (_area_group != _area_group->get_layer()->get_root_area_group()) {
      delete _area_group;
      _area_group = 0;
    } else
      _area_group = 0;

    if (is_canvas_view_valid())
      get_canvas_view()->unlock();
  }
}

void model_Layer::ImplData::raise_figure(const model_FigureRef &figure) {
  _area_group->raise_item(figure->get_data()->get_canvas_item());

  self()->_figures.reorder(self()->_figures.get_index(figure), 0);

  figure->get_data()->get_canvas_item()->set_needs_render();
}

void model_Layer::ImplData::lower_figure(const model_FigureRef &figure) {
  // QQQ amek sure that the stacking for this is correct
  _area_group->lower_item(figure->get_data()->get_canvas_item());

  self()->_figures.reorder(self()->_figures.get_index(figure), self()->_figures.count() - 1);

  figure->get_data()->get_canvas_item()->set_needs_render();
}

void model_Layer::ImplData::member_changed(const std::string &name, const grt::ValueRef &ovalue) {
  if (!_area_group) {
    if (name == "name" && _in_view)
      (*self()->owner()->signal_refreshDisplay())(self());
  } else {
    if (name == "name") {
      if (dynamic_cast<wbfig::LayerAreaGroup *>(_area_group)) {
        dynamic_cast<wbfig::LayerAreaGroup *>(_area_group)->set_title(self()->_name);
        dynamic_cast<wbfig::LayerAreaGroup *>(_area_group)->set_needs_render();
      }
    } else if (name == "height") {
      if (*self()->_height < 1)
        self()->_height = 1;

      Size size = _area_group->get_size();
      if (*self()->_height != size.height) {
        size.height = *self()->_height;
        _area_group->resize_to(size);
      }
    } else if (name == "width") {
      if (*self()->_width < 1)
        self()->_width = 1;

      Size size = _area_group->get_size();
      if (*self()->_width != size.width) {
        size.width = *self()->_width;
        _area_group->resize_to(size);
      }
    } else if (name == "left") {
      Point pos = _area_group->get_position();
      if (*self()->_left != pos.x) {
        pos.x = *self()->_left;
        _area_group->move_to(pos);
      }
    } else if (name == "top") {
      if (*self()->_top < 0)
        self()->_top = 0;

      Point pos = _area_group->get_position();
      if (*self()->_top != pos.y) {
        pos.y = *self()->_top;
        _area_group->move_to(pos);
      }
    } else if (name == "visible") {
      if (_area_group) {
        if (*self()->_visible != 0)
          _area_group->set_visible(true);
        else
          _area_group->set_visible(false);
      }
    } else if (name == "color") {
      if (_area_group) {
        Color color = Color::parse(*self()->_color);
        color.alpha = LAYER_ALPHA;
        _area_group->set_background_color(color);
        _area_group->set_needs_render();
      }
    }
  }

  try_realize();
}

//--------------------------------------------------------------------------------------------------

mdc::CanvasView *model_Layer::ImplData::get_canvas_view() const {
  model_Diagram::ImplData *view = self()->owner()->get_data();

  if (view)
    return view->get_canvas_view();

  return 0;
}

//--------------------------------------------------------------------------------------------------

bool model_Layer::ImplData::is_canvas_view_valid() {
  if (self()->owner().is_valid()) {
    model_Diagram::ImplData *view = self()->owner()->get_data();

    if (view)
      return view->is_canvas_view_valid();
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

bool model_Layer::ImplData::is_realizable() {
  return _in_view && *self()->_width > 0 && *self()->_height > 0 && is_canvas_view_valid();
}

//--------------------------------------------------------------------------------------------------

bool model_Layer::ImplData::realize() {
  if (!is_realizable() || _area_group)
    return false;

  if (!is_main_thread()) {
    run_later(std::bind(&model_Layer::ImplData::realize, this));
    return true;
  }

  if (!_area_group) {
    mdc::CanvasView *cview = self()->owner()->get_data()->get_canvas_view();

    // we're the root layer
    if (self()->owner()->rootLayer().valueptr() == self()) {
      _area_group = cview->get_current_layer()->get_root_area_group();
      return true;
    }

    cview->lock();

    mdc::Layer *layer = cview->get_current_layer();

    wbfig::LayerAreaGroup *figure = new wbfig::LayerAreaGroup(layer, self()->owner()->get_data(), self());
    _area_group = figure;
    _area_group->set_tag(self()->id());

    Color color(Color::parse(*self()->_color));
    color.alpha = LAYER_ALPHA;
    _area_group->set_border_color(LAYER_BORDER_COLOR);
    _area_group->set_background_color(color);
    _area_group->set_draw_background(true);
    std::string font = self()->owner()->owner()->get_data()->get_string_option(
      base::strfmt("%s:TitleFont", self()->class_name().c_str()), "");
    dynamic_cast<wbfig::LayerAreaGroup *>(_area_group)->set_font(mdc::FontSpec::from_string(font));

    dynamic_cast<wbfig::LayerAreaGroup *>(_area_group)->set_title(*self()->_name);

    _area_group->set_position(Point(max(0.0, *self()->_left), max(0.0, *self()->_top)));
    _area_group->resize_to(Size(*self()->_width, *self()->_height));

    layer->add_item(_area_group);

    scoped_connect(figure->signal_bounds_changed(),
                   std::bind(&model_Layer::ImplData::layer_bounds_changed, this, std::placeholders::_1));
    scoped_connect(figure->signal_interactive_resize(),
                   std::bind(&model_Layer::ImplData::interactive_layer_resized, this, std::placeholders::_1));

    _area_group->set_needs_render();

    // reparent the objects that are supposed to be inside this layer
    for (size_t c = self()->_figures.count(), i = 0; i < c; i++) {
      model_Figure::ImplData *fig = self()->_figures[i]->get_data();
      mdc::CanvasItem *item;
      if (fig && (item = fig->get_canvas_item())) {
        Point pos(item->get_root_position());
        pos.x -= *self()->_left;
        pos.y -= *self()->_top;
        _area_group->add(item);
        item->move_to(pos);
        _area_group->raise_item(item);
      } else if (fig)
        fig->realize();
    }

    self()->owner()->get_data()->stack_layer(model_LayerRef(self()), _area_group);

    cview->unlock();
  }

  return true;
}

void model_Layer::ImplData::layer_bounds_changed(const Rect &rect) {
  Rect bounds = _area_group->get_bounds();
  bool moved = false;
  bool resized = false;

  // should update internal state if resize is not interactive

  if (*self()->_left != bounds.left() || *self()->_top != bounds.top()) {
    moved = true;
  }

  if (*self()->_width != bounds.width() || *self()->_height != bounds.height()) {
    resized = true;
  }

  if (!dynamic_cast<wbfig::LayerAreaGroup *>(_area_group)->in_user_resize() && (moved || resized)) {
    //    grt::MetaClass *mc= self()->get_metaclass();
    if (moved && !resized) {
      //      grt::AutoUndo undo;

      self()->_left = grt::DoubleRef(bounds.left());
      self()->_top = grt::DoubleRef(bounds.top());

      //      undo.end(strfmt("Move %s", mc->get_attribute("caption").c_str()));
    } else if (resized && !moved) {
      //      grt::AutoUndo undo;

      self()->_width = grt::DoubleRef(bounds.width());
      self()->_height = grt::DoubleRef(bounds.height());

      //      undo.end(strfmt("Resize %s", mc->get_attribute("caption").c_str()));
    } else if (moved && resized) {
      //      grt::AutoUndo undo;

      self()->_left = grt::DoubleRef(bounds.left());
      self()->_top = grt::DoubleRef(bounds.top());
      self()->_width = grt::DoubleRef(bounds.width());
      self()->_height = grt::DoubleRef(bounds.height());

      //      undo.end(bec::fmt("Resize %s", mc->get_attribute("caption").c_str()));
    }
  }
}

void model_Layer::ImplData::interactive_layer_resized(const Rect &rect) {
  Rect bounds = get_canvas_item()->get_bounds();

  model_Model::ImplData *model = self()->owner()->owner()->get_data();
  bool skip_undo = true;
  if (model && rect != bounds)
    skip_undo = false;

  grt::AutoUndo undo(skip_undo);

  self()->left(grt::DoubleRef(bounds.left()));
  self()->top(grt::DoubleRef(bounds.top()));
  self()->width(grt::DoubleRef(bounds.width()));
  self()->height(grt::DoubleRef(bounds.height()));

  undo.end(base::strfmt("Resize '%s'", self()->_name.c_str()));
}

void model_Layer::ImplData::render_mini(mdc::CairoCtx *cr) {
  cr->save();
  cr->set_operator(CAIRO_OPERATOR_OVER);
  cr->set_color(Color::parse(*self()->_color), LAYER_ALPHA);
  cr->rectangle(floor(*self()->_left) + 0.5, floor(*self()->_top) + 0.5, ceil(*self()->_width), ceil(*self()->_height));
  cr->fill_preserve();
  cr->set_color(Color::black());
  cr->stroke();
  cr->restore();
}
