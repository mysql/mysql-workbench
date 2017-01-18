/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "mdc_rectangle.h"
#include "mdc_draw_util.h"

using namespace mdc;

RectangleFigure::RectangleFigure(Layer *layer) : Figure(layer) {
  _corner_radius = 0.0;
  _corners = CNone;
  _filled = false;
}

void RectangleFigure::stroke_outline(CairoCtx *cr, float offset) const {
  stroke_rounded_rectangle(cr, get_bounds(), _corners, _corner_radius, offset);
}

void RectangleFigure::stroke_outline_gl(float offset) const {
  stroke_rounded_rectangle_gl(get_bounds(), _corners, _corner_radius, offset);
}

void RectangleFigure::draw_contents(CairoCtx *cr) {
  cr->set_line_width(_line_width);
  stroke_outline(cr);

  if (_filled) {
    if (_fill_color.alpha != 1.0)
      cr->set_operator(CAIRO_OPERATOR_SOURCE);
    cr->set_color(_fill_color);
    cr->fill_preserve();
    cr->set_color(_pen_color);
    cr->stroke();
  } else {
    cr->set_color(_pen_color);
    cr->stroke();
  }
}

void RectangleFigure::draw_contents_gl() {
  glLineWidth(_line_width);
  if (_filled) {
    gl_setcolor(_fill_color);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }

  gl_setcolor(_pen_color);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  stroke_outline_gl();
}

void RectangleFigure::set_rounded_corners(float radius, CornerMask corners) {
  _corner_radius = radius;
  _corners = corners;
  set_needs_render();
}

void RectangleFigure::set_filled(bool flag) {
  _filled = flag;
  set_needs_render();
}
