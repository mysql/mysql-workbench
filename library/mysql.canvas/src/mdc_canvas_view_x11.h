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

#ifndef _MDC_CANVAS_MANAGER_X11_H_
#define _MDC_CANVAS_MANAGER_X11_H_

#include "mdc_canvas_view.h"
#include "base/geometry.h"

#include <X11/Xlib.h>

namespace mdc {

  class XlibCanvasView : public CanvasView {
  public:
    XlibCanvasView(Display *dpy, Window win, Visual *visual, int width, int height);

    virtual bool initialize();

    virtual void update_view_size(int width, int height);

  private:
    virtual bool has_gl() const {
      return false;
    }
    virtual void begin_repaint(int x, int y, int w, int h);
    virtual void end_repaint();
  };

  class BufferedXlibCanvasView : public CanvasView {
  public:
    BufferedXlibCanvasView(Display *dpy, Window win, Visual *visual, int depth, int width, int height);
    virtual ~BufferedXlibCanvasView();

    virtual bool initialize();

    virtual bool has_gl() const {
      return false;
    }

    // virtual Surface *create_temp_surface(const Size &size) const;

  protected:
    Display *_display;
    Window _window;
    Pixmap _back_buffer;
    Visual *_visual;
    GC _copy_gc;
    int _depth;
    int _clip_x, _clip_y, _clip_w, _clip_h;

    virtual void scroll_to(const base::Point &offs);
    virtual void update_view_size(int width, int height);
    virtual void make_current();
    virtual void begin_repaint(int x, int y, int w, int h);
    virtual void end_repaint();
  };
};

#endif /* _MDC_CANVAS_MANAGER_X11_H_ */
