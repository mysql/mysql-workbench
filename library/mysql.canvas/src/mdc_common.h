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

#pragma once

#ifndef _MSC_VER

#include <glib.h>
#include <list>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <stdexcept>
#include <assert.h>
#include <algorithm>
#include <typeinfo>
#include <stdio.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include "cairo/cairo.h"

#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "base/geometry.h"
#include "base/drawing.h"
#include "base/string_utilities.h"

#include "mdc_canvas_public.h"

#ifdef _MSC_VER
#define DEFAULT_FONT_FACE "Arial"
#elif defined(__APPLE__)
#define DEFAULT_FONT_FACE "Lucida Grande"
#else
#define DEFAULT_FONT_FACE "Helvetica"
#endif

#define MM_TO_PT(m) ((m) / (25.4 / 72.0))

#ifndef GL_BGRA
#define GL_BGRA GL_BGRA_EXT
#endif

namespace mdc {

  typedef unsigned int Count;
  typedef double Timestamp;

  enum FontSlant {
    SNormal = CAIRO_FONT_SLANT_NORMAL,
    SOblique = CAIRO_FONT_SLANT_OBLIQUE,
    SItalic = CAIRO_FONT_SLANT_ITALIC
  };

  enum FontWeight { WNormal = CAIRO_FONT_WEIGHT_NORMAL, WBold = CAIRO_FONT_WEIGHT_BOLD };

  struct MYSQLCANVAS_PUBLIC_FUNC FontSpec {
    std::string family;
    FontSlant slant;
    FontWeight weight;
    float size;

    inline FontSpec &operator=(const FontSpec &font) {
      family = font.family;
      slant = font.slant;
      weight = font.weight;
      size = font.size;

      return *this;
    }

    inline bool operator!=(const FontSpec &font) const {
      return (family != font.family || slant != font.slant || weight != font.weight) || size != font.size;
    }

    inline bool operator==(const FontSpec &font) const {
      return (family == font.family && slant == font.slant && weight == font.weight && size == font.size);
    }

    FontSpec(const FontSpec &other) : family(other.family), slant(other.slant), weight(other.weight), size(other.size) {
    }

    FontSpec() : family(DEFAULT_FONT_FACE), slant(SNormal), weight(WNormal), size(12) {
    }

    FontSpec(const std::string &afamily, FontSlant aslant = SNormal, FontWeight aweight = WNormal, float asize = 12.0)
      : family(afamily), slant(aslant), weight(aweight), size(asize) {
    }

    void toggle_bold(bool flag) {
      weight = flag ? WBold : WNormal;
    }
    void toggle_italic(bool flag) {
      slant = flag ? SItalic : SNormal;
    }

    static FontSpec from_string(const std::string &spec) {
      std::string font;
      float size;
      bool bold;
      bool italic;
      if (base::parse_font_description(spec, font, size, bold, italic))
        return FontSpec(font, italic ? SItalic : SNormal, bold ? WBold : WNormal, size);
      else
        return FontSpec();
    }
  };

  class canvas_error : public std::runtime_error {
  public:
    canvas_error(const std::string &msg) : std::runtime_error(msg){};
  };

  class MYSQLCANVAS_PUBLIC_FUNC Surface {
  protected:
    cairo_surface_t *surface;

    Surface() : surface(0) {
    }

  public:
    Surface(const Surface &other);
    Surface(cairo_surface_t *surface);

    virtual ~Surface();

    Surface &operator=(const Surface &s) {
      if (this != &s) {
        if (surface != NULL)
          cairo_surface_destroy(surface);
        surface = cairo_surface_reference(s.surface);
      }

      return *this;
    }

    cairo_surface_t *get_surface() const {
      return surface;
    }
  };

  class MYSQLCANVAS_PUBLIC_FUNC PDFSurface : public Surface {
  public:
    PDFSurface(cairo_surface_t *surface) : Surface(surface) {
    }
    PDFSurface(const std::string &path, double width, double height);
  };

  class MYSQLCANVAS_PUBLIC_FUNC PSSurface : public Surface {
  public:
    PSSurface(cairo_surface_t *surface) : Surface(surface) {
    }
    PSSurface(const std::string &path, double width, double height);
  };

  class MYSQLCANVAS_PUBLIC_FUNC ImageSurface : public Surface {
  public:
    ImageSurface(double width, double height, cairo_format_t format);
    void save_to_png(const std::string &destination) const;
  };

#ifdef _MSC_VER
  class MYSQLCANVAS_PUBLIC_FUNC Win32Surface : public Surface {
  public:
    Win32Surface(HDC hdc, bool printing = false);
  };
#endif

  class FontManager;

  struct MYSQLCANVAS_PUBLIC_FUNC CairoCtx {
  private:
    cairo_t *cr;

    FontManager *fm;

    bool _free_cr;

  public:
    CairoCtx();
    CairoCtx(cairo_t *cr);
    CairoCtx(cairo_surface_t *surf);
    CairoCtx(const Surface &surf);
    ~CairoCtx();

    void check_state() const;

    void update_cairo_backend(cairo_surface_t *surface);
    inline cairo_t *get_cr() {
      return cr;
    }

    inline void save() const {
      cairo_save(cr);
      check_state();
    }
    inline void restore() const {
      cairo_restore(cr);
      check_state();
    }
    inline void show_page() {
      cairo_show_page(cr);
    }

    inline void translate(const base::Point &p) {
      cairo_translate(cr, p.x, p.y);
    }
    inline void translate(double x, double y) {
      cairo_translate(cr, x, y);
    }
    inline void scale(const base::Point &p) {
      cairo_scale(cr, p.x, p.y);
    }
    inline void scale(double x, double y) {
      cairo_scale(cr, x, y);
    }
    inline void rotate(double rad) {
      cairo_rotate(cr, rad);
    }

    inline void set_line_width(double width) {
      cairo_set_line_width(cr, width);
    }
    inline void set_line_cap(cairo_line_cap_t t) {
      cairo_set_line_cap(cr, t);
    }
    inline void set_line_join(cairo_line_join_t t) {
      cairo_set_line_join(cr, t);
    }
    inline void set_miter_limit(double l) {
      cairo_set_miter_limit(cr, l);
    }

    inline void user_to_device(double *x, double *y) {
      cairo_user_to_device(cr, x, y);
    };
    inline void device_to_user(double *x, double *y) {
      cairo_device_to_user(cr, x, y);
    };
    inline void set_dash(double dashes[], int ndashes, double offset) {
      cairo_set_dash(cr, dashes, ndashes, offset);
    }
    inline void set_operator(cairo_operator_t oper) {
      cairo_set_operator(cr, oper);
    }

    inline void set_color(const base::Color &color) const {
      if (color.alpha == 1.0)
        cairo_set_source_rgb(cr, color.red, color.green, color.blue);
      else
        cairo_set_source_rgba(cr, color.red, color.green, color.blue, color.alpha);
    }

    inline void set_color(const base::Color &color, double alpha) const {
      cairo_set_source_rgba(cr, color.red, color.green, color.blue, alpha);
    }

    void set_font(const FontSpec &font) const;
    void get_text_extents(const FontSpec &font, const std::string &text, cairo_text_extents_t &extents);
    void get_text_extents(const FontSpec &font, const char *text, cairo_text_extents_t &extents);
    bool get_font_extents(const FontSpec &font, cairo_font_extents_t &extents);

    inline void set_source_surface(cairo_surface_t *srf, double x, double y) {
      cairo_set_source_surface(cr, srf, x, y);
    }

    inline void set_mask(cairo_pattern_t *pat) {
      cairo_mask(cr, pat);
    }

    inline void set_mask_surface(cairo_surface_t *surf, double x, double y) {
      cairo_mask_surface(cr, surf, x, y);
    }

    inline void set_pattern(cairo_pattern_t *pat) {
      cairo_set_source(cr, pat);
    }

    inline void paint() {
      cairo_paint(cr);
    }
    inline void paint_with_alpha(double a) {
      cairo_paint_with_alpha(cr, a);
    }

    inline void clip() {
      cairo_clip(cr);
    }

    inline void stroke() {
      cairo_stroke(cr);
    }
    inline void fill() {
      cairo_fill(cr);
    }
    inline void stroke_preserve() {
      cairo_stroke_preserve(cr);
    }
    inline void fill_preserve() {
      cairo_fill_preserve(cr);
    }

    inline void move_to(const base::Point &pt) {
      cairo_move_to(cr, pt.x, pt.y);
    }
    inline void move_to(double x, double y) {
      cairo_move_to(cr, x, y);
    }
    inline void rel_move_to(double x, double y) {
      cairo_rel_move_to(cr, x, y);
    }

    inline void line_to(const base::Point &pt) {
      cairo_line_to(cr, pt.x, pt.y);
    }
    inline void line_to(double x, double y) {
      cairo_line_to(cr, x, y);
    }

    inline void arc(double cx, double cy, double r, double start, double end) {
      cairo_arc(cr, cx, cy, r, start, end);
    }

    inline void show_text(const std::string &text) {
      cairo_show_text(cr, text.c_str());
    }

    inline void new_path() {
      cairo_new_path(cr);
    }
    inline void close_path() {
      cairo_close_path(cr);
    }

    inline void rectangle(const base::Rect &rect) {
      cairo_rectangle(cr, rect.left(), rect.top(), rect.width(), rect.height());
    }
    inline void rectangle(double x, double y, double w, double h) {
      cairo_rectangle(cr, x, y, w, h);
    }
  };

#define DOUBLE_CLICK_DELAY 0.400

  MYSQLCANVAS_PUBLIC_FUNC Timestamp get_time();

  cairo_status_t write_to_surface(void *closure, const unsigned char *data, unsigned int length);

} // End of mdc namespace
