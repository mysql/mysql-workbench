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

#include "mdc_canvas_view_opengl.h"

DEFAULT_LOG_DOMAIN(DOMAIN_CANVAS_BE)

using namespace mdc;

OpenGLCanvasView::OpenGLCanvasView(int width, int height) : CanvasView(width, height) {
}

OpenGLCanvasView::~OpenGLCanvasView() {
}

void OpenGLCanvasView::check_error() {
  GLenum err = glGetError();
  if (err != GL_NO_ERROR) {
    const char *msg = "unknown error";
    switch (err) {
      case GL_NO_ERROR:
        msg = "no error";
        break;
      case GL_INVALID_ENUM:
        msg = "invalid enum";
        break;
      case GL_INVALID_VALUE:
        msg = "invalid value";
        break;
      case GL_INVALID_OPERATION:
        msg = "invalid operation";
        break;
      case GL_STACK_OVERFLOW:
        msg = "stack overflow";
        break;
      case GL_STACK_UNDERFLOW:
        msg = "stack underflow";
        break;
      case GL_OUT_OF_MEMORY:
        msg = "out of memory";
        break;
#ifdef GL_TABLE_TOO_LARGE
      case GL_TABLE_TOO_LARGE:
        msg = "table too large";
        break;
#endif
    }
    logError("OpenGL error: %s\n", msg);
  }
}

bool OpenGLCanvasView::initialize() {
  if (!CanvasView::initialize())
    return false;

  // Use a weird color (cyan) here. This should never be visible anywhere as we
  // completely draw the entire canvas. So this serves as indicator if something
  // in the paint code is wrong.
  glClearColor(0, 1.0f, 1.0f, 1.0f);

  glFrontFace(GL_CW);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DITHER);
  glDisable(GL_DEPTH_TEST);

  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  glDisable(GL_POLYGON_SMOOTH);
  glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  glEnable(GL_AUTO_NORMAL);
  glEnable(GL_NORMALIZE);

  glDisable(GL_FOG);
  glDisable(GL_LOGIC_OP);
  glDisable(GL_STENCIL_TEST);
  glDisable(GL_TEXTURE_1D);
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_LIGHTING);

  check_error();

  return true;
}

void OpenGLCanvasView::begin_repaint(int, int, int, int) {
  make_current();

  // Start clean.
  glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGLCanvasView::end_repaint() {
  swap_buffers();
  check_error();
  remove_current();
}
