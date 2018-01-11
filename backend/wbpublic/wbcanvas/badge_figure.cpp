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

#include "badge_figure.h"
#include "figure_common.h"

using namespace base;

BadgeFigure::BadgeFigure(mdc::Layer *layer) : mdc::Figure(layer) {
  _font = mdc::FontSpec::from_string("Helvetica Bold 11");
  _xpadding = 8;
  _ypadding = 3;
  _line_width = 2;
  _pen_color = Color(0.9, 0.9, 0.9);
  _text_color = Color(1, 1, 1);

  set_cache_toplevel_contents(false);

  _gradient = 0;
}

BadgeFigure::~BadgeFigure() {
  cairo_pattern_destroy(_gradient);
}

void BadgeFigure::set_badge_id(const std::string &bid) {
  _badge_id = bid;
}

void BadgeFigure::set_text(const std::string &text) {
  _text = text;
  set_needs_relayout();
}

void BadgeFigure::set_gradient_from_color(const Color &color) {
  HSVColor hsv(color);

  hsv.v /= 1.4;

  set_fill_color(Color(hsv));
  set_fill_color2(color);
  set_needs_render();
}

void BadgeFigure::set_fill_color2(const Color &color) {
  _fill_color2 = color;
  if (_gradient)
    cairo_pattern_destroy(_gradient);
  _gradient = 0;
}

void BadgeFigure::set_text_color(const Color &color) {
  _text_color = color;
}

Size BadgeFigure::calc_min_size() {
  Size size;
  cairo_text_extents_t extents;

  get_view()->cairoctx()->get_text_extents(_font, _text.c_str(), extents);

  size.width = extents.x_advance;
  size.height = extents.height;
  _text_size = size;
  //  size.width+= 2*_xpadding;
  //  size.height+= 2*_ypadding;

  return size;
}

void BadgeFigure::draw_contents(mdc::CairoCtx *cr) {
  if (!_gradient) {
    _gradient = cairo_pattern_create_linear(0.0, 0.0, 0.0, get_size().height);
    cairo_pattern_add_color_stop_rgba(_gradient, 1, _fill_color.red, _fill_color.green, _fill_color.blue,
                                      _fill_color.alpha);
    cairo_pattern_add_color_stop_rgba(_gradient, 0, _fill_color2.red, _fill_color2.green, _fill_color2.blue,
                                      _fill_color2.alpha);
  }

  cr->save();

  stroke_rounded_rectangle(cr, get_bounds(), mdc::CAll, 4.0, 0.0);
  cairo_set_source(cr->get_cr(), _gradient);
  cr->fill_preserve();
  cr->set_line_width(_line_width);
  cr->set_color(_pen_color);
  cr->stroke();

  cr->move_to(get_position().x + _xpadding, get_position().y + (get_size().height + _text_size.height) / 2);
  cr->set_color(_text_color);
  cr->show_text(_text);

  cr->restore();
}
