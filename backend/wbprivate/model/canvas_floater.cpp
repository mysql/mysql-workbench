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

#include "base/drawing.h"
#include "wbcanvas/figure_common.h"
#include "canvas_floater.h"

using namespace wb;
using namespace base;

Button::Button(mdc::Layer *layer) : mdc::Button(layer, mdc::ActionButton) {
  set_padding(6, 6);
  set_pen_color(Color(0.9, 0.9, 0.9));
  set_fill_background(false);
  set_text_alignment(mdc::AlignCenter);
}

void Button::draw_contents(mdc::CairoCtx *cr) {
  cairo_pattern_t *pat;

  pat = cairo_pattern_create_linear(0.0, get_position().y, 0.0, get_position().y + 20.0);
  if (_pressed) {
    cairo_pattern_add_color_stop_rgb(pat, 1, 0.2, 0.2, 0.2);
    cairo_pattern_add_color_stop_rgb(pat, 0, 0.1, 0.1, 0.1);
  } else {
    cairo_pattern_add_color_stop_rgb(pat, 1, 0.1, 0.1, 0.1);
    cairo_pattern_add_color_stop_rgb(pat, 0, 0.2, 0.2, 0.2);
  }
  mdc::stroke_rounded_rectangle(cr, get_bounds(), mdc::CAll, 4);
  cr->set_pattern(pat);
  cr->fill_preserve();
  cr->set_line_width(1);
  cr->set_color(Color::black());
  cr->stroke();
  cairo_pattern_destroy(pat);

  mdc::Button::draw_contents(cr);
}

Floater::Floater(mdc::Layer *layer, const std::string &title)
  : mdc::Box(layer, mdc::Box::Vertical), _title(0), _content_box(layer, mdc::Box::Vertical) {
  set_draggable(true);

  set_background_corners(mdc::CAll, 8.0);
  set_background_color(Color(0, 0, 0, 0.6));
  set_draw_background(true);

  if (!title.empty()) {
    _title = new wbfig::Titlebar(layer, 0, 0, false);
    _title->set_title(title);
    _title->set_padding(6, 6);
    _title->set_font(mdc::FontSpec("helvetica", mdc::SNormal, mdc::WBold));
    _title->set_color(Color::black());
    _title->set_text_color(Color(0.7, 0.7, 0.7));
    _title->set_rounded(mdc::CTop);
    add(_title, false, false);
  }

  _content_box.set_spacing(6);
  _content_box.set_padding(8, 8);
  add(&_content_box, true, true);

  scoped_connect(get_view()->signal_viewport_changed(), std::bind(&Floater::update_position, this));
}

Floater::~Floater() {
  delete _title;
}

void Floater::set_title(const std::string &title) {
  if (_title)
    _title->set_title(title);
}

void Floater::update_position() {
}

bool Floater::on_button_press(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button,
                              mdc::EventState state) {
  if (button == mdc::ButtonLeft) {
    _dragging = true;
    _drag_offset = convert_point_to(point, 0) - get_root_position();

    return true;
  }
  return super::on_button_press(target, point, button, state);
}

bool Floater::on_button_release(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button,
                                mdc::EventState state) {
  if (button == mdc::ButtonLeft) {
    _dragging = false;
    return true;
  }
  return super::on_button_release(target, point, button, state);
}

bool Floater::on_drag(mdc::CanvasItem *target, const Point &point, mdc::EventState state) {
  if (_dragging) {
    mdc::Group *group = dynamic_cast<mdc::Group *>(get_parent());

    if (group) {
      group->move_item(this, group->convert_point_from(point, this) - _drag_offset);
    }
    return true;
  }

  return super::on_drag(target, point, state);
}
