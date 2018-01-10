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

#include "mdc_canvas_view.h"
#include "mdc_layer.h"
#include "mdc_icon_text.h"

using namespace mdc;
using namespace base;

IconTextFigure::IconTextFigure(Layer *layer) : TextFigure(layer), _icon(0) {
  _spacing = 5;
}

IconTextFigure::~IconTextFigure() {
  if (_icon)
    cairo_surface_destroy(_icon);
}

Size IconTextFigure::calc_min_size() {
  Size size = TextFigure::calc_min_size();

  if (_icon) {
    double w = cairo_image_surface_get_width(_icon);
    double h = cairo_image_surface_get_height(_icon);

    size.width = w + size.width + _spacing;
    size.height = std::max(size.height, h);
  }

  return size;
}

void IconTextFigure::auto_size() {
  Size size = get_text_size();
  size.width += _xpadding * 2;
  size.height += _ypadding * 2;

  if (_icon)
    size.width += cairo_image_surface_get_width(_icon) + _spacing;

  resize_to(size);
}

void IconTextFigure::draw_contents(CairoCtx *cr) {
  Rect bounds = get_bounds();

  if (_fill_background) {
    cr->set_color(_fill_color);
    cr->rectangle(bounds);
    cr->fill();
  }
  if (_icon) {
    double w = cairo_image_surface_get_width(_icon);
    double h = cairo_image_surface_get_height(_icon);
    double x;
    Point pos = get_position();

    pos.x += _xpadding;
    pos.y += _ypadding + (get_size().height - h - 2 * _ypadding) / 2;

    x = w + _spacing;
    cr->save();
    cr->set_source_surface(_icon, pos.x, pos.y);
    cr->scale(1.0 / w, 1.0 / h);
    cr->paint();
    cr->restore();
    cr->translate(x, 0);
    bounds.size.width -= x;
  }
  bool fill_bg = _fill_background;
  _fill_background = false;
  TextFigure::draw_contents(cr, bounds);
  _fill_background = fill_bg;
}

void IconTextFigure::set_icon(cairo_surface_t *icon) {
  if (icon != _icon) {
    if (_icon)
      cairo_surface_destroy(_icon);
    if (icon)
      _icon = cairo_surface_reference(icon);
    else
      _icon = NULL;
    set_needs_relayout();
  }
}

void IconTextFigure::set_spacing(double space) {
  _spacing = space;
  set_needs_relayout();
}
