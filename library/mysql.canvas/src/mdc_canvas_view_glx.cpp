/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mdc_canvas_view_glx.h"

#include <cairo-xlib.h>

using namespace mdc;

GLXCanvasView::GLXCanvasView(Display *dpy, Window win, Visual *visual, int width, int height)
  : OpenGLCanvasView(width, height), _glxcontext(0), _display(dpy), _window(win), _visual(visual) {
  _crsurface = cairo_xlib_surface_create(_display, _window, _visual, _view_width, _view_height);

  _cairo = new CairoCtx(_crsurface);
  cairo_set_tolerance(_cairo->get_cr(), 0.1);
}

bool GLXCanvasView::initialize() {
  XVisualInfo *visinfo;
  XWindowAttributes xwa;
  int attribs[] = {GLX_RGBA, GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1,
                   //    GLX_ALPHA_SIZE, 1,
                   GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 1, None};

  if (!XGetWindowAttributes(_display, _window, &xwa)) {
    printf("error: could not get window attributes\n");
    return false;
  }

  visinfo = glXChooseVisual(_display, XScreenNumberOfScreen(xwa.screen), attribs);
  if (!visinfo) {
    throw canvas_error("could not determine an appropriate GLX visual");
  }

  _glxcontext = glXCreateContext(_display, visinfo, NULL, GL_TRUE);
  XSync(_display, False);
  if (!_glxcontext) {
    XFree(visinfo);
    throw canvas_error("could not initialize GLX context");
  }
  XFree(visinfo);

  make_current();

  if (!OpenGLCanvasView::initialize())
    return false;

  return true;
}

GLXCanvasView::~GLXCanvasView() {
  if (_glxcontext) {
    if (_glxcontext == glXGetCurrentContext()) {
      glXWaitGL();

      glXMakeCurrent(_display, None, NULL);
    }

    glXDestroyContext(_display, _glxcontext);
  }
}

void GLXCanvasView::make_current() {
  glXMakeCurrent(_display, _window, _glxcontext);
}

void GLXCanvasView::remove_current() {
  // glXMakeCurrent(_display, 0, 0);
}

void GLXCanvasView::swap_buffers() {
  glXSwapBuffers(_display, _window);
}

void GLXCanvasView::update_view_size(int width, int height) {
  if (_view_width != width || _view_height != height) {
    _view_width = width;
    _view_height = height;

    delete _cairo;
    if (_crsurface)
      cairo_surface_destroy(_crsurface);

    _crsurface = cairo_xlib_surface_create(_display, _window, _visual, _view_width, _view_height);

    _cairo = new CairoCtx(_crsurface);
    cairo_set_tolerance(_cairo->get_cr(), 0.1);

    update_offsets();
    queue_repaint();

    _viewport_changed_signal();
  }
}
