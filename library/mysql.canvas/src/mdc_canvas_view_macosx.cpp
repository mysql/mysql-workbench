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

#include "base/log.h"
#include "mdc_canvas_view_macosx.h"

namespace mdc {
  std::string detect_opengl_version() {
    return "2.0"; // XXX
  }
};

DEFAULT_LOG_DOMAIN(DOMAIN_CANVAS_BE)

using namespace mdc;

//--------------------------------------------------------------------------------------------------

QuartzCanvasView::QuartzCanvasView(int width, int height) : CanvasView(width, height) {
  logDebug("Creating quartz canvas view\n");

  // A surface used to get a cairo context outside of a paint cycle (usually for font measurement).
  _offlineSurface = cairo_quartz_surface_create(CAIRO_FORMAT_RGB24, 1, 1);
  _crsurface = NULL;
  _context = NULL;
  _cairo = new CairoCtx(_offlineSurface);
}

//--------------------------------------------------------------------------------------------------

QuartzCanvasView::~QuartzCanvasView() {
  logDebug("Destroying quartz canvas view\n");

  if (_offlineSurface != NULL)
    cairo_surface_destroy(_offlineSurface);

  if (_crsurface != NULL)
    cairo_surface_destroy(_crsurface);

  // _cairo is deleted in the ancestor's d-tor.
}

//--------------------------------------------------------------------------------------------------

/**
 * For drawing we need the current core graphics context (which might change between calls).
 * As the base class does not allow to pass it in the repaint() function an additional call is needed
 * to set the context for the next paint cycle.
 */
void QuartzCanvasView::set_target_context(CGContextRef cgContext) {
  _context = cgContext;
}

//--------------------------------------------------------------------------------------------------

void QuartzCanvasView::update_view_size(int width, int height) {
  if (_view_width != width || _view_height != height) {
    _view_width = width;
    _view_height = height;

    update_offsets();
    queue_repaint();

    _viewport_changed_signal();
  }
}

//--------------------------------------------------------------------------------------------------

void QuartzCanvasView::begin_repaint(int, int, int, int) {
  _crsurface = cairo_quartz_surface_create_for_cg_context(_context, _view_width, _view_height);
  _cairo->update_cairo_backend(_crsurface);
}

//--------------------------------------------------------------------------------------------------

void QuartzCanvasView::end_repaint() {
  _context = NULL;

  cairo_surface_destroy(_crsurface);
  _crsurface = NULL;

  _cairo->update_cairo_backend(_offlineSurface);
}

//--------------------------------------------------------------------------------------------------
