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

#include "mdc_figure.h"
#include "mdc_algorithms.h"
#include "mdc_canvas_view.h"
#include "mdc_draw_util.h"

using namespace mdc;
using namespace base;

Figure::Figure(Layer *layer)
  : CanvasItem(layer), _pen_color(Color::black()), _fill_color(Color::white()), _line_width(1.0) {
}

void Figure::render(CairoCtx *cr) {
  draw_state(cr);
  draw_contents(cr);
}

void Figure::draw_contents_gl() {
  throw std::logic_error("draw_contents_gl() not implemented for this figure");
}

void Figure::render_gl(mdc::CairoCtx *cr) {
  draw_state_gl();
  draw_contents_gl();
}

//--------------------------------------------------------------------------------------------------

/**
 * Draws a hollow rectangle given by the bounds of this figure and a certain offset.
 * This version uses Cairo to draw it.
 */
void Figure::stroke_outline(CairoCtx *cr, float offset) const {
  Rect bounds = get_bounds();

  cr->rectangle(bounds.left() - offset, bounds.top() - offset, bounds.width() + offset * 2,
                bounds.height() + offset * 2);
}

//--------------------------------------------------------------------------------------------------

/**
 * Draws a hollow rectangle given by the bounds of this figure and a certain offset.
 * This version uses OpenGL to draw it.
 */
void Figure::stroke_outline_gl(float offset) const {
  Rect bounds = get_bounds();

  gl_rectangle(bounds.left() - offset, bounds.top() - offset, bounds.width() + 2 * offset, bounds.height() + 2 * offset,
               false);
}

//--------------------------------------------------------------------------------------------------

Point Figure::get_intersection_with_line_to(const Point &p) {
  // this will return the intersection point of the line from the
  // center of the object to the given point.
  Point center = get_root_position();
  Point p1;
  Point p2;

  center.x += get_size().width / 2;
  center.y += get_size().height / 2;

  // brute-force, subclasses should reimplement with their own version

  if (intersect_rect_to_line(get_bounds(), center, p, p1, p2)) {
    /*
    cairo_surface_t *crs= cairo_image_surface_create(CAIRO_FORMAT_RGB24,
                                                     (int)get_size().width,
                                                     (int)get_size().height);
    {
      CairoCtx cr(crs);
      cr.rectangle(0, 0, get_size().width, get_size().height);
      cr.set_color(Color::black());
      cr.fill();

      cr.translate(-get_position().x, -get_position().y);
      stroke_outline(&cr, 0);
      cr.set_color(Color::white());
      cr.fill();

      unsigned char *data= cairo_image_surface_get_data(crs);
      int stride= cairo_image_surface_get_stride(crs);
#define GET_PIXEL(x, y) data[((x+(y)*stride)*3)]

      //XXX
    }
    cairo_surface_destroy(crs);
     */
  }
  return center;
}

void Figure::set_pen_color(const Color &color) {
  _pen_color = color;
  set_needs_render();
}

void Figure::set_fill_color(const Color &color) {
  _fill_color = color;
  set_needs_render();
}

void Figure::set_line_width(float width) {
  _line_width = width;
  set_needs_render();
}
