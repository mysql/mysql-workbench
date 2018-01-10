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
#include "mdc_canvas_view_windows.h"
#include "mdc_canvas_view_image.h"
#include "mdc_rectangle.h"

#ifndef GL_BGRA
#define GL_BGRA GL_BGRA_EXT
#endif

DEFAULT_LOG_DOMAIN(DOMAIN_CANVAS_BE)

using namespace mdc;

//--------------------------------------------------------------------------------------------------

/**
 * Determines a proper pixel format needed for creating an OpenGL rendering context.
 */
bool FindPixelFormatForDeviceContext(HDC context) {
  logDebug("Determine a proper pixel format\n");

  PIXELFORMATDESCRIPTOR pfd = {
    sizeof(PIXELFORMATDESCRIPTOR), // size of this pfd
    1,                             // version number
    PFD_DRAW_TO_WINDOW |           // support window
      PFD_SUPPORT_OPENGL |         // support OpenGL
      // PFD_SUPPORT_GDI |                 // support GDI
      PFD_DOUBLEBUFFER, // double buffered
    PFD_TYPE_RGBA,      // RGBA type
    32,                 // color with alpha
    0,
    0, 0, 0, 0, 0,  // no explicit color bits assignment
    0, 0,           // no explicit alpha bits assignment
    0,              // no accumulation buffer
    0, 0, 0, 0,     // accum bits ignored
    0,              // no z-buffer (depth testing disabled for 2D drawing)
    0,              // no stencil buffer
    0,              // no auxiliary buffer
    PFD_MAIN_PLANE, // main layer
    0,              // reserved
    0, 0, 0         // layer masks ignored
  };

  // Get the device context's best available pixel format match.
  int pixelFormat = ChoosePixelFormat(context, &pfd);
  if (pixelFormat == 0)
    return false;

  // Make that match the device context's current pixel format .
  return SetPixelFormat(context, pixelFormat, &pfd) != 0;
}

//--------------------------------------------------------------------------------------------------

WindowsGLCanvasView::WindowsGLCanvasView(HWND window, int width, int height)
  : OpenGLCanvasView(width, height), _glrc(0), _window(window) {
  logDebug("Creating OpenGL canvas view (%i x %i pixels)\n", width, height);

  // A surface used to get a cairo context outside of a paint cycle.
  _offline_surface = cairo_win32_surface_create_with_dib(CAIRO_FORMAT_RGB24, 1, 1);
}

//--------------------------------------------------------------------------------------------------

WindowsGLCanvasView::~WindowsGLCanvasView() {
  logDebug("Destroying OpenGL canvas view\n");

  if (_glrc != 0)
    wglDeleteContext(_glrc);
  if (_offline_surface != NULL)
    cairo_surface_destroy(_offline_surface);
}

//--------------------------------------------------------------------------------------------------

bool WindowsGLCanvasView::initialize() {
  // Find a proper pixel format.
  _hdc = GetDC(_window);
  if (!FindPixelFormatForDeviceContext(_hdc)) {
    ReleaseDC(_window, _hdc);
    _hdc = 0;

    logError("Could not set up a proper pixel format for OpenGL\n");
    return false;
  }

  _glrc = wglCreateContext(_hdc);
  if (_glrc == 0) {
    ReleaseDC(_window, _hdc);
    _hdc = 0;

    logError("Could not create WGL context\n");
    return false;
  }

  // The make_current() call will set a proper surface.
  // We need an active context now, as the base initializer will set a few default values.
  _cairo = new CairoCtx();
  make_current();

  const GLubyte* temp = glGetString(GL_VERSION);
  if (temp != NULL)
    logInfo("Found OpenGL version for this view: %s\n", temp);
  else
    logWarning("Could not get OpenGL version info\n");

  bool result = OpenGLCanvasView::initialize();
  remove_current(); // Will also release the allocated dc.

  return result;
}

//--------------------------------------------------------------------------------------------------

void WindowsGLCanvasView::make_current() {
  if (_hdc != 0)
    ReleaseDC(_window, _hdc); // Shouldn't happen actually.

  _hdc = GetDC(_window);
  wglMakeCurrent(_hdc, _glrc);

  _crsurface = cairo_win32_surface_create(_hdc);
  _cairo->update_cairo_backend(_crsurface);
  cairo_set_tolerance(_cairo->get_cr(), 0.1);
}

//--------------------------------------------------------------------------------------------------

void WindowsGLCanvasView::remove_current() {
  cairo_surface_destroy(_crsurface);
  _crsurface = NULL;
  _cairo->update_cairo_backend(_offline_surface);

  wglMakeCurrent(0, 0);
  if (_hdc != 0) {
    ReleaseDC(_window, _hdc);
    _hdc = 0;
  }
}

//--------------------------------------------------------------------------------------------------

void WindowsGLCanvasView::swap_buffers() {
  SwapBuffers(_hdc);
}

//--------------------------------------------------------------------------------------------------

void WindowsGLCanvasView::update_view_size(int width, int height) {
  logDebug2("Updating OpenGL canvas view size (%i x %i pixels)\n", width, height);

  if (_view_width != width || _view_height != height) {
    _view_width = width;
    _view_height = height;

    update_offsets();
    queue_repaint();
    _viewport_changed_signal();
  }
}

//----------------- WindowsCanvasView --------------------------------------------------------------

WindowsCanvasView::WindowsCanvasView(int width, int height) : CanvasView(width, height) {
  logDebug("Creating GDI canvas view (%i x %i pixels)\n", width, height);
  _hdc = 0;
  _crsurface = 0;

  // A surface used to get a cairo context outside of a paint cycle (usually for font measurement).
  _offline_surface = cairo_win32_surface_create_with_dib(CAIRO_FORMAT_RGB24, 1, 1);
  _cairo = new CairoCtx(_offline_surface);
}

//--------------------------------------------------------------------------------------------------

WindowsCanvasView::~WindowsCanvasView() {
  logDebug("Destroying GDI canvas view\n");

  if (_offline_surface)
    cairo_surface_destroy(_offline_surface);

  if (_crsurface)
    cairo_surface_destroy(_crsurface);

  // _cairo is deleted in the ancestor's d-tor.
}

//--------------------------------------------------------------------------------------------------

bool WindowsCanvasView::initialize() {
  return CanvasView::initialize();
}

//--------------------------------------------------------------------------------------------------

void WindowsCanvasView::update_view_size(int width, int height) {
  logDebug2("Updating GDI canvas view size (%i x %i pixels)\n", width, height);

  if (_view_width != width || _view_height != height) {
    _view_width = width;
    _view_height = height;

    update_offsets();
    queue_repaint();

    _viewport_changed_signal();
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * For drawing we need the current GDI device context (which might change between calls).
 * As the base class does not allow to pass it in the repaint() function an additional one is needed
 * to set the context for the next paint cycle.
 */
void WindowsCanvasView::set_target_context(HDC hdc) {
  _hdc = hdc;
}

//--------------------------------------------------------------------------------------------------

void WindowsCanvasView::begin_repaint(int x, int y, int w, int h) {
  _crsurface = cairo_win32_surface_create(_hdc);
  _cairo->update_cairo_backend(_crsurface);
  cairo_set_tolerance(_cairo->get_cr(), 0.1);
}

//--------------------------------------------------------------------------------------------------

void WindowsCanvasView::end_repaint() {
  _hdc = 0;

  cairo_surface_destroy(_crsurface);
  _crsurface = NULL;

  _cairo->update_cairo_backend(_offline_surface);
}

//--------------------------------------------------------------------------------------------------
