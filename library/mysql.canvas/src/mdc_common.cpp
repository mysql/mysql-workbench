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

#include <string.h>

#ifndef _MSC_VER
#include <sys/time.h>
#include <time.h>
#else
#include <cairo/cairo-win32.h>
#endif

#include <errno.h>

#include <cairo/cairo-ps.h>
#include <cairo/cairo-pdf.h>

#include "mdc_common.h"
#include "base/file_utilities.h"

using namespace mdc;

struct ScaledFont {
  FontSpec spec;
  cairo_scaled_font_t *font;
  cairo_font_face_t *face;
  cairo_font_options_t *options;

  ScaledFont(const FontSpec &aspec, cairo_scaled_font_t *sf, cairo_font_face_t *fc, cairo_font_options_t *opt)
    : spec(aspec), font(sf), face(fc), options(opt) {
  }

  ScaledFont(const ScaledFont &other) {
    spec = other.spec;
    if (other.font)
      font = cairo_scaled_font_reference(other.font);
    else
      font = 0;
    if (other.face)
      face = cairo_font_face_reference(other.face);
    else
      face = 0;
    if (other.options)
      options = cairo_font_options_copy(other.options);
    else
      options = 0;
  }
  ~ScaledFont() {
    cairo_scaled_font_destroy(font);
    cairo_font_face_destroy(face);
    cairo_font_options_destroy(options);
  }

  inline ScaledFont &operator=(const ScaledFont &other) {
    spec = other.spec;
    if (other.font)
      font = cairo_scaled_font_reference(other.font);
    else
      font = 0;
    if (other.face)
      face = cairo_font_face_reference(other.face);
    else
      face = 0;
    if (other.options)
      options = cairo_font_options_copy(other.options);
    else
      options = 0;
    return *this;
  }
};

class mdc::FontManager {
  std::map<std::string, std::list<ScaledFont> > _cache;
  CairoCtx *_cairo;

  cairo_scaled_font_t *lookup(const FontSpec &spec) {
    if (_cache.find(spec.family) != _cache.end()) {
      std::list<ScaledFont> &flist(_cache[spec.family]);

      for (std::list<ScaledFont>::iterator iter = flist.begin(); iter != flist.end(); ++iter) {
        if (iter->spec == spec)
          return iter->font;
      }
    }
    return 0;
  }

  cairo_scaled_font_t *create(const FontSpec &spec) {
    cairo_font_face_t *face;
    cairo_scaled_font_t *sfont;
    cairo_matrix_t matrix;
    cairo_matrix_t ctm;
    cairo_font_options_t *options;
    cairo_t *cr = _cairo->get_cr();

#ifdef _DEBUG
    static int i = 0;

    i++;
    if (i % 100 == 0) {
      fprintf(stderr, "create font %s %i %i %f (%i)\n", spec.family.c_str(), spec.slant, spec.weight, spec.size, i);
    }
#endif

#if CAIRO_VERSION_MAJOR == 1 && CAIRO_VERSION_MINOR < 8
    _cairo->save();
    cairo_select_font_face(cr, spec.family.c_str(), (cairo_font_slant_t)spec.slant, (cairo_font_weight_t)spec.weight);

    cairo_set_font_size(cr, spec.size);
    face = cairo_get_font_face(cr);
    cairo_font_face_reference(face);
#else
    face =
      cairo_toy_font_face_create(spec.family.c_str(), (cairo_font_slant_t)spec.slant, (cairo_font_weight_t)spec.weight);
    cairo_set_font_size(cr, spec.size);
#endif
    if (cairo_font_face_status(face) != CAIRO_STATUS_SUCCESS) {
      cairo_font_face_destroy(face);
#if CAIRO_VERSION_MAJOR == 1 && CAIRO_VERSION_MINOR < 8
      _cairo->restore();
#endif
      return 0;
    }

    cairo_get_font_matrix(cr, &matrix);

    cairo_matrix_init_identity(&ctm);

    options = cairo_font_options_create();
    cairo_font_options_set_hint_metrics(options, CAIRO_HINT_METRICS_OFF);
    cairo_font_options_set_hint_style(options, CAIRO_HINT_STYLE_NONE);

    sfont = cairo_scaled_font_create(face, &matrix, &ctm, options);

    if (cairo_scaled_font_status(sfont) != CAIRO_STATUS_SUCCESS) {
      cairo_scaled_font_destroy(sfont);
      cairo_font_face_destroy(face);
#if CAIRO_VERSION_MAJOR == 1 && CAIRO_VERSION_MINOR < 8
      _cairo->restore();
#endif
      return 0;
    }
#if CAIRO_VERSION_MAJOR == 1 && CAIRO_VERSION_MINOR < 8
    _cairo->restore();
#endif

    _cache[spec.family].push_back(ScaledFont(spec, sfont, face, options));

    return sfont;
  }

public:
  FontManager(CairoCtx *cr) : _cairo(cr) {
  }

  cairo_scaled_font_t *get_font(const FontSpec &spec) {
    cairo_scaled_font_t *font;

    font = lookup(spec);
    if (!font)
      font = create(spec);

    if (!font)
      font = create(FontSpec("helvetica", SNormal, WNormal, spec.size));

    if (!font)
      throw canvas_error("Could not create font");

    return font;
  }
};

Surface::Surface(const Surface &other) : surface(cairo_surface_reference(other.surface)) {
}

Surface::Surface(cairo_surface_t *surface_) : surface(surface_) {
}

Surface::~Surface() {
  if (surface)
    cairo_surface_destroy(surface);
}

ImageSurface::ImageSurface(double width, double height, cairo_format_t format) {
  surface = cairo_image_surface_create(format, (int)width, (int)height);
}

void ImageSurface::save_to_png(const std::string &destination) const {
  cairo_status_t status = cairo_surface_write_to_png(surface, destination.c_str());
  if (status != CAIRO_STATUS_SUCCESS)
    throw canvas_error("cairo error: " + std::string(cairo_status_to_string(status)));
}

PDFSurface::PDFSurface(const std::string &path, double width, double height) {
  surface = cairo_pdf_surface_create(path.c_str(), width, height);
}

PSSurface::PSSurface(const std::string &path, double width, double height) {
  surface = cairo_ps_surface_create(path.c_str(), width, height);
}

#ifdef _MSC_VER
Win32Surface::Win32Surface(HDC hdc, bool printing) {
  if (printing)
    surface = cairo_win32_printing_surface_create(hdc);
  else
    surface = cairo_win32_surface_create(hdc);
}
#endif

//--------------------------------------------------------------------------------------------------

/**
 * Constructor to decouple object creation and surface creation.
 * Needs a call to update_cairo_backend!
 */
CairoCtx::CairoCtx() : _free_cr(false) {
  cr = NULL;
  fm = new FontManager(this);
}

//--------------------------------------------------------------------------------------------------

CairoCtx::CairoCtx(cairo_t *context) : _free_cr(false) {
  cr = context;

  fm = new FontManager(this);
}

//--------------------------------------------------------------------------------------------------

CairoCtx::CairoCtx(cairo_surface_t *surf) : _free_cr(true) {
  cairo_status_t st;
  cr = cairo_create(surf);

  if ((st = cairo_status(cr)) != CAIRO_STATUS_SUCCESS)
    throw canvas_error("Error creating cairo context: " + std::string(cairo_status_to_string(st)));

  fm = new FontManager(this);
}

//--------------------------------------------------------------------------------------------------

CairoCtx::CairoCtx(const Surface &surf) : _free_cr(true) {
  cr = cairo_create(surf.get_surface());

  if (cairo_status(cr) != CAIRO_STATUS_SUCCESS)
    throw canvas_error("Error creating cairo context: " + std::string(cairo_status_to_string(cairo_status(cr))));

  fm = new FontManager(this);
}

//--------------------------------------------------------------------------------------------------

CairoCtx::~CairoCtx() {
  if (cr && _free_cr)
    cairo_destroy(cr);
  delete fm;
}

//--------------------------------------------------------------------------------------------------

/**
 * Recreates the internal cairo context based on the (new) surface given.
 */
void CairoCtx::update_cairo_backend(cairo_surface_t *surface) {
  cairo_status_t st;

  if (cr != NULL && _free_cr)
    cairo_destroy(cr);
  if (surface == NULL)
    cr = NULL;
  else {
    cr = cairo_create(surface);
    _free_cr = true;

    if ((st = cairo_status(cr)) != CAIRO_STATUS_SUCCESS)
      throw canvas_error("Error creating cairo context: " + std::string(cairo_status_to_string(st)));
  }
}

//--------------------------------------------------------------------------------------------------

void CairoCtx::check_state() const {
  cairo_status_t status = cairo_status(cr);
  if (status != CAIRO_STATUS_SUCCESS)
    throw canvas_error("cairo error: " + std::string(cairo_status_to_string(cairo_status(cr))));
}

void CairoCtx::set_font(const FontSpec &font) const {
  cairo_set_scaled_font(cr, fm->get_font(font));
}

void CairoCtx::get_text_extents(const FontSpec &font, const std::string &text, cairo_text_extents_t &extents) {
  cairo_scaled_font_text_extents(fm->get_font(font), text.c_str(), &extents);
}

void CairoCtx::get_text_extents(const FontSpec &font, const char *text, cairo_text_extents_t &extents) {
  cairo_scaled_font_text_extents(fm->get_font(font), text, &extents);
}

bool CairoCtx::get_font_extents(const FontSpec &font, cairo_font_extents_t &extents) {
  cairo_scaled_font_t *fontp = fm->get_font(font);
  if (fontp) {
    cairo_scaled_font_extents(fontp, &extents);
    return true;
  }
  return false;
}

Timestamp mdc::get_time() {
#ifdef _MSC_VER
  unsigned __int64 t = 0;

  GetSystemTimeAsFileTime((FILETIME *)&t);

  return (double)t / 1e+7;
#else
  struct timeval t;

  gettimeofday(&t, NULL);

  return t.tv_sec + t.tv_usec / 1000000.0;
#endif
}

//-----------------
// mdc::write_to_surface
//-----------------
cairo_status_t mdc::write_to_surface(void *closure, const unsigned char *data, unsigned int length) {
  FILE *file = static_cast<FILE *>(closure);
  size_t res = fwrite(data, sizeof(data[0]), length, file);
  cairo_status_t ret = (res == length) ? CAIRO_STATUS_SUCCESS : CAIRO_STATUS_WRITE_ERROR;
  return ret;
}
