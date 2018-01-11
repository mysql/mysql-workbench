/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "model_figure_impl.h"
#include "model_diagram_impl.h"
#include "model_layer_impl.h"
#include "model_model_impl.h"

#include "grt/common.h"
#include "grt/grt_manager.h"
#include <grtpp_undo_manager.h>
#include "base/string_utilities.h"

using namespace base;

model_Figure::ImplData::ImplData(model_Figure *owner) : model_Object::ImplData(owner), _realizing(false) {
  _connected_update_options = false;

  scoped_connect(owner->signal_changed(), std::bind(&model_Figure::ImplData::member_changed, this,
                                                    std::placeholders::_1, std::placeholders::_2));
}

void model_Figure::ImplData::update_options(const std::string &key) {
}

void model_Figure::ImplData::set_layer(const model_LayerRef &nlayer) {
  model_LayerRef oldLayer(self()->_layer);

  if (is_canvas_view_valid())
    get_canvas_view()->lock_redraw();

  self()->_layer = nlayer;

  if (self()->_layer.is_valid()) {
    mdc::CanvasItem *figure = get_canvas_item();

    model_Layer::ImplData *layer = nlayer->get_data();
    mdc::AreaGroup *ag = 0;
    if (layer)
      ag = layer->get_area_group();

    if (oldLayer.is_valid()) {
      // update the position of the figure
      self()->_top = *self()->_top - *self()->_layer->top() + *oldLayer->top();
      self()->_left = *self()->_left - *self()->_layer->left() + *oldLayer->left();
    } else {
      self()->_top = *self()->_top - *self()->_layer->top();
      self()->_left = *self()->_left - *self()->_layer->left();
    }

    if (figure && ag) {
      // will remove from old group if needed
      ag->add(figure);

      figure->move_to(Point(*self()->_left, *self()->_top));
    }
  }
  if (is_canvas_view_valid())
    get_canvas_view()->unlock_redraw();

  try_realize();
}

void model_Figure::ImplData::member_changed(const std::string &name, const grt::ValueRef &ovalue) {
  if (name == "name") {
    if (_in_view) {
      (*self()->owner()->signal_refreshDisplay())(self());
    }
  } else if (name == "color") {
    if (get_canvas_item()) {
      if (self()->_color.is_valid() && *self()->_color.c_str())
        ((wbfig::BaseFigure *)get_canvas_item())->set_color(Color::parse(*self()->_color));
      else
        ((wbfig::BaseFigure *)get_canvas_item())->unset_color();
    }
  } else if (get_canvas_item()) // only valid if realized
  {
    if (name == "height") {
      Size size(get_canvas_item()->get_size());
      Size min_size(get_canvas_item()->get_min_size());

      self()->_manualSizing = 1;
      dynamic_cast<wbfig::BaseFigure *>(get_canvas_item())->set_allow_manual_resizing(true);

      if (*self()->_height < min_size.height)
        self()->_height = min_size.height;

      if (is_canvas_view_valid())
        get_canvas_view()->lock();

      if (*self()->_height != size.height) {
        size.height = *self()->_height;
        if (*self()->_manualSizing)
          get_canvas_item()->set_fixed_size(size);
        else
          get_canvas_item()->resize_to(size);
      }

      if (is_canvas_view_valid())
        get_canvas_view()->unlock();
    } else if (name == "width") {
      Size size(get_canvas_item()->get_size());
      Size min_size(get_canvas_item()->get_min_size());

      // XXX remove this automatic forcing of manual resizing and move it to
      // the diagram code, so that it's only turned on explicitly when the user resizes
      self()->_manualSizing = 1;
      dynamic_cast<wbfig::BaseFigure *>(get_canvas_item())->set_allow_manual_resizing(true);

      if (*self()->_width < min_size.width)
        self()->_width = min_size.width;

      if (is_canvas_view_valid())
        get_canvas_view()->lock();

      if (*self()->_width != size.width) {
        size.width = *self()->_width;
        if (*self()->_manualSizing)
          get_canvas_item()->set_fixed_size(size);
        else
          get_canvas_item()->resize_to(size);
      }

      if (is_canvas_view_valid())
        get_canvas_view()->unlock();
    } else if (name == "left") {
      if (is_canvas_view_valid())
        get_canvas_view()->lock();
      Point pos = get_canvas_item()->get_position();
      if (*self()->_left != pos.x) {
        pos.x = *self()->_left;
        get_canvas_item()->move_to(pos);
      }
      if (is_canvas_view_valid())
        get_canvas_view()->unlock();
    } else if (name == "top") {
      if (is_canvas_view_valid())
        get_canvas_view()->lock();
      Point pos = get_canvas_item()->get_position();
      if (*self()->_top != pos.y) {
        pos.y = *self()->_top;
        get_canvas_item()->move_to(pos);
      }
      if (is_canvas_view_valid())
        get_canvas_view()->unlock();
    } else if (name == "manualSizing") {
      dynamic_cast<wbfig::BaseFigure *>(get_canvas_item())->set_allow_manual_resizing(*self()->_manualSizing != 0);
    } else if (name == "expanded") {
      if (get_canvas_item())
        ((wbfig::BaseFigure *)get_canvas_item())->toggle(*self()->_expanded != 0);
    } else if (name == "visible") {
      if (*self()->_visible)
        try_realize();
      else
        unrealize();
    }
  } else // !get_canvas_item()
  {
    // do some sanity checks
    if (name == "width") {
      if (*self()->_width < 20)
        self()->_width = 20;
    } else if (name == "height") {
      if (*self()->_height < 20)
        self()->_height = 20;
    }
  }
}

bool model_Figure::ImplData::is_realizable() {
  if (_in_view && *self()->_visible && self()->layer().is_valid()) {
    model_Layer::ImplData *layer = self()->_layer->get_data();
    model_Diagram::ImplData *view = self()->owner()->get_data();

    if (layer && layer->get_area_group() && view && view->is_canvas_view_valid())
      return true;
  }
  return false;
}

void model_Figure::ImplData::figure_bounds_changed(const Rect &rect) {
  Rect bounds(get_canvas_item()->get_bounds());

  self()->_left = grt::DoubleRef(bounds.left());
  self()->_top = grt::DoubleRef(bounds.top());
  self()->_width = grt::DoubleRef(bounds.width());
  self()->_height = grt::DoubleRef(bounds.height());

  relayout_badges();
}

//--------------------------------------------------------------------------------------------------

/**
 * Called by the interaction layer at the end of an interactive resize operation.
 * The figure already has its final bounds at this point. We mostly create proper undo
 * records here.
 */
void model_Figure::ImplData::figure_resized(const Rect &rect) {
  Rect bounds = get_canvas_item()->get_bounds();

  model_Model::ImplData *model = self()->owner()->owner()->get_data();
  bool skip_undo = true;
  if (model && !_realizing && rect != bounds)
    skip_undo = false;

  grt::AutoUndo undo(skip_undo);

  self()->left(grt::DoubleRef(bounds.left()));
  self()->top(grt::DoubleRef(bounds.top()));
  self()->width(grt::DoubleRef(bounds.width()));
  self()->height(grt::DoubleRef(bounds.height()));

  self()->manualSizing(1);
  dynamic_cast<wbfig::BaseFigure *>(get_canvas_item())->set_allow_manual_resizing(true);

  undo.end(strfmt("Resize '%s'", self()->_name.c_str()));
}

//--------------------------------------------------------------------------------------------------

void model_Figure::ImplData::finish_realize() {
  Size size;
  Point pos;
  wbfig::BaseFigure *figure = ((wbfig::BaseFigure *)get_canvas_item());

  _realizing = true;

  if (!_connected_update_options) {
    _connected_update_options = true;
    scoped_connect(self()->owner()->owner()->get_data()->signal_options_changed(),
                   std::bind(&model_Figure::ImplData::update_options, this, std::placeholders::_1));
  }

  figure->set_tag(self()->id());

  scoped_connect(figure->signal_bounds_changed(),
                 std::bind(&model_Figure::ImplData::figure_bounds_changed, this, std::placeholders::_1));
  scoped_connect(figure->signal_interactive_resize(),
                 std::bind(&model_Figure::ImplData::figure_resized, this, std::placeholders::_1));

  pos.x = *self()->_left;
  pos.y = *self()->_top;

  if (*self()->_manualSizing) {
    figure->set_allow_manual_resizing(true);
    size.width = *self()->_width;
    size.height = *self()->_height;
    figure->set_fixed_size(size);
  } else {
    if (*self()->_width > 0 && *self()->_height > 0) {
      size.width = *self()->_width;
      size.height = *self()->_height;
      figure->resize_to(size);
    } else {
      // force relayout and fetch size
      figure->relayout();
      self()->_width = figure->get_size().width;
      self()->_height = figure->get_size().height;
    }
  }
  figure->move_to(pos);

  // check if the figure is in the views selection list and select it if so
  if (self()->owner()->selection().get_index(model_FigureRef(self())) != grt::BaseListRef::npos) {
    if (self()->owner()->selection().count() == 1)
      figure->get_view()->get_selection()->set(figure);
    else
      figure->get_view()->get_selection()->add(figure);
  }

  model_Diagram::ImplData *view = self()->owner()->get_data();
  model_Model::ImplData *model = self()->owner()->owner()->get_data();
  if (model) {
    std::string font;

    font = model->get_string_option(strfmt("%s:TitleFont", self()->class_name().c_str()), "");
    if (!font.empty())
      figure->set_title_font(mdc::FontSpec::from_string(font));

    font = model->get_string_option(strfmt("%s:ItemsFont", self()->class_name().c_str()), "");
    if (font.empty())
      font = model->get_string_option(strfmt("%s:TextFont", self()->class_name().c_str()), "");
    if (!font.empty())
      figure->set_content_font(mdc::FontSpec::from_string(font));
  }

  view->stack_figure(self(), get_canvas_item());

  _realizing = false;
}

void model_Figure::ImplData::highlight(const Color *color) {
  if (get_canvas_item())
    dynamic_cast<wbfig::BaseFigure *>(get_canvas_item())->highlight(color);
}

void model_Figure::ImplData::unhighlight() {
  if (get_canvas_item())
    dynamic_cast<wbfig::BaseFigure *>(get_canvas_item())->unhighlight();
}

void model_Figure::ImplData::render_mini(mdc::CairoCtx *cr) {
  Rect rect = get_canvas_item()->get_bounds();

  cr->set_color(Color::parse(*self()->_color));
  cr->rectangle(rect);
  cr->fill();
}

void model_Figure::ImplData::unrealize() {
  if (get_canvas_item()) {
    get_canvas_item()->get_view()->remove_item(get_canvas_item());
  }
}

//--------------------------------------------------------------------------------------------------

mdc::CanvasView *model_Figure::ImplData::get_canvas_view() const {
  if (self()->owner().is_valid()) {
    model_Diagram::ImplData *view = self()->owner()->get_data();
    if (view)
      return view->get_canvas_view();
  }
  return 0;
}

//--------------------------------------------------------------------------------------------------

bool model_Figure::ImplData::is_canvas_view_valid() {
  if (self()->owner().is_valid()) {
    model_Diagram::ImplData *view = self()->owner()->get_data();
    if (view)
      return view->is_canvas_view_valid();
  }
  return 0;
}

//--------------------------------------------------------------------------------------------------

void model_Figure::ImplData::relayout_badges() {
  if (!_badges.empty() && get_canvas_item()) {
    Rect bounds(get_canvas_item()->get_root_bounds());
    Point pos;

    pos.x = bounds.right() - 4;
    pos.y = bounds.top() + 5;

    for (std::list<BadgeFigure *>::const_iterator iter = _badges.begin(); iter != _badges.end(); ++iter) {
      (*iter)->get_layer()->get_root_area_group()->raise_item(*iter);
      (*iter)->set_position(pos);
      (*iter)->set_visible(true);
      (*iter)->set_needs_relayout();

      pos.y += (*iter)->get_size().height + 5;
    }
  }
}

void model_Figure::ImplData::add_badge(BadgeFigure *badge) {
  badge->set_visible(false);
  _badges.push_back(badge);

  relayout_badges();
}

void model_Figure::ImplData::remove_badge(BadgeFigure *badge) {
  std::list<BadgeFigure *>::iterator iter = std::find(_badges.begin(), _badges.end(), badge);

  if (iter != _badges.end())
    _badges.erase(iter);

  relayout_badges();
}

BadgeFigure *model_Figure::ImplData::get_badge_with_id(const std::string &badge_id) {
  for (std::list<BadgeFigure *>::const_iterator iter = _badges.begin(); iter != _badges.end(); ++iter) {
    if ((*iter)->badge_id() == badge_id)
      return *iter;
  }
  return 0;
}
