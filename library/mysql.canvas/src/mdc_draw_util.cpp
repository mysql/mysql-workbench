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

#ifndef _MSC_VER
#include <string.h>
#endif

#include "mdc_draw_util.h"

using namespace base;

namespace mdc {

  void draw_shadow(CairoCtx *cr, const Rect &around_rect, const Color &color) {
    cairo_pattern_t *pat;

    // right
    cr->save();
    pat = cairo_pattern_create_linear(0, 0, 1.0, 0);
    cairo_pattern_add_color_stop_rgba(pat, 0, color.red, color.green, color.blue, 1.0);
    cairo_pattern_add_color_stop_rgba(pat, 1, color.red, color.green, color.blue, 0.0);

    cr->translate(around_rect.right(), around_rect.top() + 2);
    cr->scale(5, around_rect.size.height - 2);
    cr->rectangle(0, 0, 1, 1);
    cr->set_pattern(pat);
    cr->fill();
    cairo_pattern_destroy(pat);
    cr->restore();

    // corner
    cr->save();
    pat = cairo_pattern_create_linear(0, 0, 1.0, 1.0);
    cairo_pattern_add_color_stop_rgba(pat, 0, color.red, color.green, color.blue, 0.6);
    cairo_pattern_add_color_stop_rgba(pat, 1, color.red, color.green, color.blue, 0.0);

    cr->translate(around_rect.right(), around_rect.bottom());
    cr->scale(5, 5);
    cr->move_to(0, 0);
    cr->line_to(0, 1);
    cr->line_to(1, 0);
    cr->set_pattern(pat);
    cr->fill();
    cairo_pattern_destroy(pat);
    cr->restore();

    // bottom
    cr->save();
    pat = cairo_pattern_create_linear(0, 0, 0, 1);
    cairo_pattern_add_color_stop_rgba(pat, 0, color.red, color.green, color.blue, 1.0);
    cairo_pattern_add_color_stop_rgba(pat, 1, color.red, color.green, color.blue, 0.0);

    cr->translate(around_rect.left(), around_rect.bottom());
    cr->scale(around_rect.size.width, 5);
    cr->rectangle(0, 0, 1, 1);
    cr->set_pattern(pat);
    cr->fill();
    cairo_pattern_destroy(pat);
    cr->restore();

    // corner
    cr->save();
    pat = cairo_pattern_create_linear(0, 1.0, 1.0, 0.0);
    cairo_pattern_add_color_stop_rgba(pat, 1, color.red, color.green, color.blue, 0.6);
    cairo_pattern_add_color_stop_rgba(pat, 0, color.red, color.green, color.blue, 0.0);

    cr->translate(around_rect.left() - 4, around_rect.bottom());
    cr->scale(4, 5);
    cr->move_to(0, 0);
    cr->line_to(1, 1);
    cr->line_to(1, 0);
    cr->close_path();
    cr->set_pattern(pat);
    cr->fill();
    cairo_pattern_destroy(pat);
    cr->restore();

    // left
    cr->save();
    pat = cairo_pattern_create_linear(0, 0, 1.0, 0);
    cairo_pattern_add_color_stop_rgba(pat, 1, color.red, color.green, color.blue, 1.0);
    cairo_pattern_add_color_stop_rgba(pat, 0, color.red, color.green, color.blue, 0.0);

    cr->translate(around_rect.left() - 4, around_rect.top() + 2);
    cr->scale(5, around_rect.size.height - 1);
    cr->rectangle(0, 0, 1, 1);
    cr->set_pattern(pat);
    cr->fill();
    cairo_pattern_destroy(pat);
    cr->restore();
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Draws a rectangular shadow around the given rectangle using OpenGL.
   */
  void draw_shadow_gl(const Rect &bounds, const Color &color) {
#ifndef __APPLE__
    double small_offset = 15;
    double large_offset = 50;
    glBegin(GL_QUADS);

    // Top part.
    gl_setcolor(color, 1);
    glVertex2d(bounds.left(), bounds.top());

    gl_setcolor(color, 0);
    glVertex2d(bounds.left() - small_offset, bounds.top() - small_offset);

    gl_setcolor(color, 0);
    glVertex2d(bounds.right() + large_offset, bounds.top() - small_offset);

    gl_setcolor(color, 1);
    glVertex2d(bounds.right(), bounds.top());

    // Right part.
    gl_setcolor(color, 1);
    glVertex2d(bounds.right(), bounds.top());

    gl_setcolor(color, 0);
    glVertex2d(bounds.right() + large_offset, bounds.top() - small_offset);

    gl_setcolor(color, 0);
    glVertex2d(bounds.right() + large_offset, bounds.bottom() + large_offset);

    gl_setcolor(color, 1);
    glVertex2d(bounds.right(), bounds.bottom());

    // Bottom part.
    gl_setcolor(color, 1);
    glVertex2d(bounds.right(), bounds.bottom());

    gl_setcolor(color, 0);
    glVertex2d(bounds.right() + large_offset, bounds.bottom() + large_offset);

    gl_setcolor(color, 0);
    glVertex2d(bounds.left() - small_offset, bounds.bottom() + large_offset);

    gl_setcolor(color, 1);
    glVertex2d(bounds.left(), bounds.bottom());

    // Left part.
    gl_setcolor(color, 1);
    glVertex2d(bounds.left(), bounds.bottom());

    gl_setcolor(color, 0);
    glVertex2d(bounds.left() - small_offset, bounds.bottom() + large_offset);

    gl_setcolor(color, 0);
    glVertex2d(bounds.left() - small_offset, bounds.top() - small_offset);

    gl_setcolor(color, 1);
    glVertex2d(bounds.left(), bounds.top());

    glEnd();
#endif
  }

  //--------------------------------------------------------------------------------------------------

  void draw_glow(CairoCtx *cr, const Rect &around_rect, const Color &color) {
    cr->save();
    cr->set_color(color, 0.6);
    cr->set_line_width(5);
    cr->rectangle(around_rect.left() - 2.5, around_rect.top() - 2.5, around_rect.width() + 6, around_rect.height() + 6);
    cr->stroke();
    cr->restore();
  }

  void fill_hollow_rectangle(CairoCtx *cr, const Rect &outer_rect, const Rect &inner_rect) {
    cr->rectangle(outer_rect.left(), outer_rect.top(), outer_rect.width(), inner_rect.top() - outer_rect.top());

    cr->rectangle(outer_rect.left(), inner_rect.bottom(), outer_rect.width(),
                  outer_rect.bottom() - inner_rect.bottom());

    cr->rectangle(outer_rect.left(), inner_rect.top(), inner_rect.left() - outer_rect.left(), inner_rect.height());

    cr->rectangle(inner_rect.right() + 1, inner_rect.top(), outer_rect.right() - inner_rect.left(),
                  inner_rect.height());

    cr->fill();
  }

  void stroke_rounded_rectangle(CairoCtx *cr, const Rect &rect, CornerMask corners, float corner_radius, float offset) {
    Rect bounds = rect;

    bounds.pos.x += 0.5 - offset;
    bounds.pos.y += 0.5 - offset;
    bounds.size.width += offset * 2;
    bounds.size.height += offset * 2;

    if (corner_radius > 0 && corners) {
      double r = corner_radius;
      int dtl = (corners & CTopLeft) != 0;
      int dtr = (corners & CTopRight) != 0;
      int dbl = (corners & CBottomLeft) != 0;
      int dbr = (corners & CBottomRight) != 0;

      cr->new_path();

      if (dtl) {
        cr->arc(bounds.left() + r, bounds.top() + r, r, 180 * M_PI / 180.0, 270 * M_PI / 180.0);
      }

      cr->line_to(bounds.right() - r * dtr, bounds.top());

      if (dtr) {
        cr->arc(bounds.right() - r, bounds.top() + r, r, 270 * M_PI / 180.0, 0 * M_PI / 180.0);
      }

      cr->line_to(bounds.right(), bounds.bottom() - r * dbr);

      if (dbr) {
        cr->arc(bounds.right() - r, bounds.bottom() - r, r, 0 * M_PI / 180.0, 90 * M_PI / 180.0);
      }

      cr->line_to(bounds.left() + r * dbl, bounds.bottom());

      if (dbl) {
        cr->arc(bounds.left() + r, bounds.bottom() - r, r, 90 * M_PI / 180.0, 180 * M_PI / 180.0);
      }

      cr->line_to(bounds.left(), bounds.top() + r * dtl);

      cr->close_path();
    } else
      cr->rectangle(bounds);
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Blurs the content of the given surface, e.g. to use it as shadow.
   */
  void cairo_image_surface_blur(cairo_surface_t *surface, double radius) {
    // Steve Hanov, 2009
    // Released into the public domain.

    // get width, height
    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
    unsigned char *dst = (unsigned char *)malloc(width * height * 4);
    unsigned *precalc = (unsigned *)malloc(width * height * sizeof(unsigned));
    unsigned char *src = cairo_image_surface_get_data(surface);
    double mul = 1.f / ((radius * 2) * (radius * 2));
    int channel;

    // The number of times to perform the averaging. According to wikipedia,
    // three iterations is good enough to pass for a gaussian.
    const int MAX_ITERATIONS = 3;
    int iteration;

    memcpy(dst, src, width * height * 4);

    for (iteration = 0; iteration < MAX_ITERATIONS; iteration++) {
      for (channel = 0; channel < 4; channel++) {
        double x, y;

        // Pre-computation step.
        unsigned char *pix = src;
        unsigned *pre = precalc;

        pix += channel;
        for (y = 0; y < height; y++) {
          for (x = 0; x < width; x++) {
            int tot = pix[0];
            if (x > 0)
              tot += pre[-1];
            if (y > 0)
              tot += pre[-width];
            if (x > 0 && y > 0)
              tot -= pre[-width - 1];
            *pre++ = tot;
            pix += 4;
          }
        }

        // Blur step.
        pix = dst + (int)radius * width * 4 + (int)radius * 4 + channel;
        for (y = radius; y < height - radius; y++) {
          for (x = radius; x < width - radius; x++) {
            double l = x < radius ? 0 : x - radius;
            double t = y < radius ? 0 : y - radius;
            double r = x + radius >= width ? width - 1 : x + radius;
            double b = y + radius >= height ? height - 1 : y + radius;
            double tot = precalc[(int)(r + b * width)] + precalc[(int)(l + t * width)] - precalc[(int)(l + b * width)] -
                         precalc[(int)(r + t * width)];
            *pix = (unsigned char)(tot * mul);
            pix += 4;
          }
          pix += (int)radius * 2 * 4;
        }
      }
      memcpy(src, dst, width * height * 4);
    }

    free(dst);
    free(precalc);
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Draws a rounded rectangle in OpenGL.
   */
  void stroke_rounded_rectangle_gl(const Rect &rect, CornerMask corners, float corner_radius, float offset) {
    Rect bounds = rect;

    double x = (double)bounds.pos.x + offset;
    double y = (double)bounds.pos.y + offset;
    double w = (double)bounds.size.width + 2 * offset;
    double h = (double)bounds.size.height + 2 * offset;

    if (corner_radius > 0 && corners) {
      double r = corner_radius;
      int dtl = (corners & CTopLeft) != 0;
      int dtr = (corners & CTopRight) != 0;
      int dbl = (corners & CBottomLeft) != 0;
      int dbr = (corners & CBottomRight) != 0;

      glBegin(GL_POLYGON);

      if (dtr) {
        for (double t = M_PI * 1.5f; t < 2 * M_PI; t += 0.1f) {
          double sx = x + w - r + cos(t) * r;
          double sy = y + r + sin(t) * r;
          glVertex2d(sx, sy);
        }
      }
      glVertex2d(x + w, y + r * dtr);
      glVertex2d(x + w, y + h - r * dbr);

      if (dbr) {
        for (double t = 0; t < 0.5f * M_PI; t += 0.1f) {
          double sx = x + w - r + cos(t) * r;
          double sy = y + h - r + sin(t) * r;
          glVertex2d(sx, sy);
        }
      }
      glVertex2d(x + w - r * dbr, y + h);
      glVertex2d(x + r * dbl, y + h);

      if (dbl) {
        for (double t = 0.5f * M_PI; t < M_PI; t += 0.1f) {
          double sx = x + r + cos(t) * r;
          double sy = y + h - r + sin(t) * r;
          glVertex2d(sx, sy);
        }
      }
      glVertex2d(x, y + h - r * dbl);
      glVertex2d(x, y + r * dtl);

      if (dtl) {
        for (double t = M_PI; t < 1.5f * M_PI; t += 0.1f) {
          double sx = x + r + cos(t) * r;
          double sy = y + r + sin(t) * r;
          glVertex2d(sx, sy);
        }
      }
      glVertex2d(x + r * dtl, y);
      glVertex2d(x + w - r * dtr, y);

      glEnd();
    } else
      gl_rectangle(bounds, false);
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Convenience function to set an OpenGL color.
   */
  void gl_setcolor(const Color &color) {
    glColor4d(color.red, color.green, color.blue, color.alpha);
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Overload that allows to override a color's alpha value.
   */
  void gl_setcolor(const Color &color, double alpha) {
    glColor4d(color.red, color.green, color.blue, alpha);
  }

  //--------------------------------------------------------------------------------------------------

  void gl_rectangle(double x, double y, double w, double h, bool filled) {
    if (filled)
      glBegin(GL_QUADS);
    else
      glBegin(GL_LINE_LOOP);
    glVertex2d(x, y);
    glVertex2d(x + w, y);
    glVertex2d(x + w, y + h);
    glVertex2d(x, y + h);
    glEnd();
  }

  //--------------------------------------------------------------------------------------------------

  void gl_rectangle(const Rect &rect, bool filled) {
    if (filled)
      glBegin(GL_QUADS);
    else
      glBegin(GL_LINE_LOOP);
    glVertex2d(rect.left(), rect.top());
    glVertex2d(rect.right(), rect.top());
    glVertex2d(rect.right(), rect.bottom());
    glVertex2d(rect.left(), rect.bottom());
    glEnd();
  }

  //--------------------------------------------------------------------------------------------------

  void gl_box(const Rect &rect, Color &border_color, Color &fill_color) {
    // Interior first.
    gl_setcolor(fill_color);
    gl_rectangle(rect, true);

    // Now the border.
    glEnable(GL_POLYGON_OFFSET_FILL); // avoid depth fighting
    glPolygonOffset(1.0, 1.0);

    gl_setcolor(border_color);
    gl_rectangle(rect, false);
    glDisable(GL_POLYGON_OFFSET_FILL);
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Draws a polygon from the given vertices, either filled or not.
   */
  void gl_polygon(const Point vertices[], int size, bool filled) {
    if (filled)
      glBegin(GL_POLYGON);
    else
      glBegin(GL_LINE_LOOP);

    for (int i = 0; i < size; i++)
      glVertex2d(vertices[i].x, vertices[i].y);
    glEnd();
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Draws a filled polygon with a border.
   * Note: due to restrictions in OpenGL the given points must form a convex polygon or the output is wrong.
   *       If we ever need any type of polygons we have to implement polygon splitting (e.g. "ear clipping").
   */
  void gl_polygon(const Point vertices[], int size, const Color &border_color, const Color &fill_color) {
    gl_setcolor(fill_color);
    gl_polygon(vertices, size, true);

    gl_setcolor(border_color);
    gl_polygon(vertices, size, false);
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Draws an arc at the given position and radius. The parameters start and end must be given in radians
   * and specify at which angle the arc starts and ends (measured from the positive x axis).
   * The arc is drawn clockwise and optionally filled. A filled arc is implicitly closed.
   */
  void gl_arc(double x, double y, double radius, double start, double end, bool filled) {
    if (filled)
      glBegin(GL_POLYGON);
    else
      glBegin(GL_LINE_STRIP);

    for (double t = start; t < end; t += 0.2f) // TODO: optimize step count, make it dependent on radius.
    {
      double sx = x + cos(t) * radius;
      double sy = y - sin(t) * radius;
      glVertex2d(sx, sy);
    }
    glEnd();
  }

  //--------------------------------------------------------------------------------------------------
};
