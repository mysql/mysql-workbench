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

#ifndef _MDC_CANVAS_VIEW_WINDOWS_H_
#define _MDC_CANVAS_VIEW_WINDOWS_H_

#include "mdc_canvas_view_opengl.h"

namespace mdc {

  class MYSQLCANVAS_PUBLIC_FUNC WindowsGLCanvasView : public OpenGLCanvasView {
  public:
    WindowsGLCanvasView(HWND window, int width, int height);
    virtual ~WindowsGLCanvasView();

    virtual bool initialize();

    virtual void make_current();
    virtual void remove_current();
    virtual void swap_buffers();

    virtual void update_view_size(int width, int height);

  protected:
    HWND _window;
    HGLRC _glrc;
    HDC _hdc;

    cairo_surface_t *_offline_surface;
  };

  class MYSQLCANVAS_PUBLIC_FUNC WindowsCanvasView : public CanvasView {
  public:
    WindowsCanvasView(int width, int height);
    virtual ~WindowsCanvasView();

    virtual bool initialize();
    virtual bool has_gl() const {
      return false;
    }
    void set_target_context(HDC hdc);

  protected:
    HDC _hdc;

    cairo_surface_t *_offline_surface;

    virtual void update_view_size(int width, int height);
    virtual void begin_repaint(int x, int y, int w, int h);
    virtual void end_repaint();
  };
};

#endif /* _MDC_CANVAS_VIEW_WINDOWS_H_ */
