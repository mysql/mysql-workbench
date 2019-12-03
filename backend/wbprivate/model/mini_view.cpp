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

#include "mdc.h"

#include "mini_view.h"

#include "wbcanvas/model_figure_impl.h"
#include "wbcanvas/model_layer_impl.h"

using namespace mdc;
using namespace wb;
using namespace base;

//----------------------------------------------------------------------------------------------------------------------

MiniView::MiniView(mdc::Layer *layer) : mdc::Figure(layer), _canvas_view(0), _viewport_figure(0) {
  _updating_viewport = false;
  _skip_viewport_update = false;
  _backgroundColor = Color::getSystemColor(base::TextBackgroundColor);

#ifndef __APPLE__
  set_cache_toplevel_contents(true);
#endif
  layer->get_view()->set_event_callbacks(
    std::bind(&MiniView::view_button_cb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
              std::placeholders::_4, std::placeholders::_5),
    std::bind(&MiniView::view_motion_cb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
    std::function<bool(CanvasView *, KeyInfo, EventState, bool)>());
}

//----------------------------------------------------------------------------------------------------------------------

MiniView::~MiniView() {
  if (_view_repaint_connection.connected())
    _view_repaint_connection.disconnect();

  if (_view_viewport_change_connection.connected())
    _view_viewport_change_connection.disconnect();

  delete _viewport_figure; // not added to layer, so delete it by hand
}

//----------------------------------------------------------------------------------------------------------------------

bool MiniView::view_button_cb(mdc::CanvasView *view, mdc::MouseButton btn, bool press, Point pos, mdc::EventState) {
  if (btn == mdc::ButtonLeft && _viewport_figure) {
    if (press)
      _click_pos = pos;
    else {
      if (_click_pos == pos) {
        double scale;
        Rect bounds = get_scaled_target_bounds(scale);
        Rect rect = _viewport_figure->get_bounds();
        Rect nrect;

        nrect.pos.x = pos.x - rect.width() / 2;
        nrect.pos.y = pos.y - rect.height() / 2;
        nrect.size = rect.size;

        if (nrect.left() < bounds.left())
          nrect.pos.x = bounds.left();
        if (nrect.top() < bounds.top())
          nrect.pos.y = bounds.top();
        if (nrect.right() > bounds.right())
          nrect.pos.x = bounds.right() - nrect.width();
        if (nrect.bottom() > bounds.bottom())
          nrect.pos.y = bounds.bottom() - nrect.height();

        _viewport_figure->set_bounds(nrect);
        _viewport_figure->set_needs_render();
        viewport_dragged(rect);
      }
    }
  }

  return false;
}

//----------------------------------------------------------------------------------------------------------------------

bool MiniView::view_motion_cb(mdc::CanvasView *, Point, mdc::EventState) {
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

void MiniView::update_size() {
  Size size = get_layer()->get_view()->get_total_view_size();

  set_fixed_size(size);
  resize_to(size);

  viewport_changed();
}

//----------------------------------------------------------------------------------------------------------------------

void MiniView::render_figure(CairoCtx *cr, const model_FigureRef &elem) {
  model_Figure::ImplData *e = elem->get_data();

  if (e)
    e->render_mini(cr);
}

//----------------------------------------------------------------------------------------------------------------------

void MiniView::render_layer(CairoCtx *cr, const model_LayerRef &layer) {
  model_Layer::ImplData *l = layer->get_data();

  if (l)
    l->render_mini(cr);
}

//----------------------------------------------------------------------------------------------------------------------

void MiniView::render_layer_figures(mdc::CairoCtx *cr, const model_LayerRef &layer) {
  for (size_t c = layer->figures().count(), i = 0; i < c; i++) {
    model_FigureRef figure(layer->figures()[i]);
    mdc::CanvasItem *figure_layer;

    if (figure->get_data()->get_canvas_item()) {
      cr->save();

      figure_layer = figure->get_data()->get_canvas_item()->get_parent();
      cr->translate(figure_layer->get_position());

      render_figure(cr, figure);

      cr->restore();
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

Rect MiniView::get_scaled_target_bounds(double &scale) {
  Rect rect;

  scale = 1.0;

  if (_canvas_view) {
    Size full_size = _canvas_view->get_total_view_size();
    Size mini_size = get_layer()->get_view()->get_total_view_size();

    // Add a small padding to make the output more visually appealing.
    double padding = 3;

    // calculate a scaling for fitting the whole view
    if (full_size.width / full_size.height < mini_size.width / mini_size.height)
      scale = (mini_size.height - 2 * padding) / full_size.height;
    else
      scale = (mini_size.width - 2 * padding) / full_size.width;

    rect.pos.x = (mini_size.width - full_size.width * scale) / 2;
    rect.pos.y = (mini_size.height - full_size.height * scale) / 2;
    rect.size.width = full_size.width * scale;
    rect.size.height = full_size.height * scale;
  }
  return rect;
}

//----------------------------------------------------------------------------------------------------------------------

void MiniView::draw_contents(CairoCtx *cr) {
  cr->set_operator(CAIRO_OPERATOR_SOURCE);
  cr->set_color(Color(0.5, 0.5, 0.5));
  cr->paint();

  if (!_canvas_view || !_model_diagram.is_valid() || !_model_diagram->rootLayer().is_valid())
    return;

  double scale;
  Rect bounds = get_scaled_target_bounds(scale);

  cr->save();

  cr->set_line_width(1);
  cr->set_color(_backgroundColor);
  cr->rectangle(bounds);
  cr->fill_preserve();
  cr->set_color(_backgroundColor.invert());
  cr->stroke();

  Size page_size(_canvas_view->get_page_size());

  if (page_size.width <= 0 || page_size.height <= 0 || scale == 0)
    return;

  mdc::Count xpages, ypages;

  cr->set_color(Color(0.5, 0.5, 0.5));
  page_size.width *= scale;
  page_size.height *= scale;
  page_size = page_size.round();

  _canvas_view->get_page_layout(xpages, ypages);

  for (mdc::Count y = 1; y < ypages; y++) {
    cr->move_to(bounds.left() + 0.5, floor(bounds.top() + y * page_size.height) + 0.5);
    cr->line_to(bounds.right() + 0.5, floor(bounds.top() + y * page_size.height) + 0.5);
    cr->stroke();
  }

  for (mdc::Count x = 1; x < xpages; x++) {
    cr->move_to(floor(bounds.left() + x * page_size.width) + 0.5, bounds.top() + 0.5);
    cr->line_to(floor(bounds.left() + x * page_size.width) + 0.5, bounds.bottom() + 0.5);
    cr->stroke();
  }

  cr->translate(bounds.pos);
  cr->scale(scale, scale);

  // first draw layers only
  for (size_t c = _model_diagram->layers().count(), i = 0; i < c; i++)
    render_layer(cr, _model_diagram->layers()[i]);

  // now draw figures only
  render_layer_figures(cr, _model_diagram->rootLayer());
  for (size_t c = _model_diagram->layers().count(), i = 0; i < c; i++)
    render_layer_figures(cr, _model_diagram->layers()[i]);

  cr->restore();
}

//----------------------------------------------------------------------------------------------------------------------

void MiniView::viewport_changed() {
  if (_viewport_figure && _canvas_view && !_updating_viewport) {
    Rect vp = _canvas_view->get_viewport();
    double scale;
    Rect bounds = get_scaled_target_bounds(scale);

    vp.pos.x = vp.pos.x * scale + bounds.left();
    vp.pos.y = vp.pos.y * scale + bounds.top();
    vp.size.width *= scale;
    vp.size.height *= scale;

    _skip_viewport_update = true;
    _viewport_figure->set_bounds(vp);
    _viewport_figure->set_needs_render();
    _skip_viewport_update = false;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void MiniView::viewport_dragged(const Rect &orect) {
  if (!_skip_viewport_update) {
    double scale;
    Rect bounds = get_scaled_target_bounds(scale);
    Rect rect = _viewport_figure->get_bounds();
    Rect nrect;

    _updating_viewport = true;

    nrect = rect;
    if (nrect.left() < bounds.left())
      nrect.pos.x = bounds.left();
    if (nrect.top() < bounds.top())
      nrect.pos.y = bounds.top();
    if (nrect.right() > bounds.right())
      nrect.pos.x = bounds.right() - nrect.width();
    if (nrect.bottom() > bounds.bottom())
      nrect.pos.y = bounds.bottom() - nrect.height();

    if (nrect != rect) {
      _viewport_figure->set_bounds(nrect);
      _viewport_figure->set_needs_render();
    }

    if (_canvas_view) {
      Point p;

      p.x = (nrect.left() - bounds.left()) / scale;
      p.y = (nrect.top() - bounds.top()) / scale;

      _canvas_view->set_offset(p);
    }
    _updating_viewport = false;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void MiniView::set_active_view(mdc::CanvasView *canvas_view, const model_DiagramRef &model_diagram) {
  _canvas_view = canvas_view;
  _model_diagram = model_diagram;

  if (!_viewport_figure) {
    _viewport_figure = new mdc::RectangleFigure(get_layer());
    _viewport_figure->set_filled(false);
    _viewport_figure->set_pen_color(base::Color::getSystemColor(base::TextBackgroundColor).invert());
    get_layer()->get_view()->get_current_layer()->add_item(_viewport_figure);

    _viewport_figure->set_accepts_selection(true);
    _viewport_figure->set_accepts_focus(false);
    _viewport_figure->set_state_drawing(false);
    _viewport_figure->set_auto_sizing(false);
    _viewport_figure->set_draggable(true);
    _viewport_figure->set_needs_render();

    scoped_connect(_viewport_figure->signal_bounds_changed(),
                   std::bind(&MiniView::viewport_dragged, this, std::placeholders::_1));
  }

  if (_view_repaint_connection.connected())
    _view_repaint_connection.disconnect();

  if (_view_viewport_change_connection.connected())
    _view_viewport_change_connection.disconnect();

  if (_canvas_view) {
    _view_viewport_change_connection =
      _canvas_view->signal_viewport_changed()->connect(std::bind(&MiniView::viewport_changed, this));

    _view_repaint_connection =
      _canvas_view->signal_repaint()->connect((std::bind(&CanvasItem::set_needs_render, this)));

    _viewport_figure->set_visible(true);

    get_layer()->get_view()->set_page_size(get_layer()->get_view()->get_viewable_size());

    resize_to(get_layer()->get_view()->get_viewable_size());
    viewport_changed();
  } else {
    _view_viewport_change_connection.disconnect();
    _view_repaint_connection.disconnect();
    _viewport_figure->set_visible(false);
  }

  set_needs_render();
}

//----------------------------------------------------------------------------------------------------------------------

void MiniView::setBackgroundColor(base::Color const& color) {
  // The same applies here like for the mdc back layer: drawing anything else but a white background produces
  // problems with other elements. In the mini view this is not that apparent, but we have to follow the color
  // of the main canvas.
  _backgroundColor = base::Color::white(); // color;
  if (_viewport_figure != nullptr)
    _viewport_figure->set_pen_color(base::Color::black());
  
  set_needs_render();
}

//----------------------------------------------------------------------------------------------------------------------
