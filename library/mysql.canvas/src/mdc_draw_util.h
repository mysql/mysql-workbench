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

#ifndef _MDC_DRAW_UTIL_H_
#define _MDC_DRAW_UTIL_H_

#include "mdc_canvas_public.h"
#include "mdc_common.h"

namespace mdc {

  enum CornerMask {
    CNone = 0,
    CTopLeft = (1 << 0),
    CTopRight = (1 << 1),
    CBottomLeft = (1 << 2),
    CBottomRight = (1 << 3),
    CTop = (CTopLeft | CTopRight),
    CBottom = (CBottomLeft | CBottomRight),
    CAll = (CTop | CBottom)
  };

  inline CornerMask operator|(CornerMask a, CornerMask b) {
    return (CornerMask)((int)a | (int)b);
  }

  MYSQLCANVAS_PUBLIC_FUNC void cairo_image_surface_blur(cairo_surface_t *surface, double radius);

  MYSQLCANVAS_PUBLIC_FUNC void draw_shadow(CairoCtx *cr, const base::Rect &around_rect, const base::Color &color);
  MYSQLCANVAS_PUBLIC_FUNC void draw_shadow_gl(const base::Rect &bounds, const base::Color &color);

  MYSQLCANVAS_PUBLIC_FUNC void draw_glow(CairoCtx *cr, const base::Rect &around_rect, const base::Color &color);

  MYSQLCANVAS_PUBLIC_FUNC void fill_hollow_rectangle(CairoCtx *cr, const base::Rect &outer_rect,
                                                     const base::Rect &inner_rect);

  MYSQLCANVAS_PUBLIC_FUNC void stroke_rounded_rectangle(CairoCtx *cr, const base::Rect &bounds, CornerMask corners,
                                                        float corner_radius, float offset = 0);
  MYSQLCANVAS_PUBLIC_FUNC void stroke_rounded_rectangle_gl(const base::Rect &rect, CornerMask corners,
                                                           float corner_radius, float offset);

  MYSQLCANVAS_PUBLIC_FUNC void gl_setcolor(const base::Color &color);
  MYSQLCANVAS_PUBLIC_FUNC void gl_setcolor(const base::Color &color, double alpha);
  MYSQLCANVAS_PUBLIC_FUNC void gl_rectangle(double x, double y, double w, double h, bool filled);
  MYSQLCANVAS_PUBLIC_FUNC void gl_rectangle(const base::Rect &rect, bool filled);
  MYSQLCANVAS_PUBLIC_FUNC void gl_box(const base::Rect &rect, base::Color &border_color, base::Color &fill_color);
  MYSQLCANVAS_PUBLIC_FUNC void gl_polygon(const base::Point vertices[], int size, bool filled);
  MYSQLCANVAS_PUBLIC_FUNC void gl_polygon(const base::Point vertices[], int size, const base::Color &border_color,
                                          const base::Color &fill_color);
  MYSQLCANVAS_PUBLIC_FUNC void gl_arc(double x, double y, double radius, double start, double end, bool filled);
};

#endif /* _MDC_DRAW_UTIL_H_ */
