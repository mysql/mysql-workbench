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

#include "mdc_canvas_view_x11.h"
#include "base/geometry.h"

using namespace mdc;

#ifdef ___TRACE
#define _GNU_SOURCE
#include <dlfcn.h>
#include <cxxabi.h>
#include <sys/time.h>

static struct timeval start_time;
static int trace_depth = 0;
static bool trace_on = false;
static bool start_tracing = false;
static FILE *trace_file = 0;

void ___enable_tracing(bool flag) {
  start_tracing = flag;
  if (flag) {
    if (!trace_file)
      trace_file = fopen("trace.txt", "w+");
    puts("ENABLE TRACE");
  } else {
    if (trace_file) {
      fclose(trace_file);
      trace_file = 0;
    }
    puts("DISABLE TRACE");
  }
}

extern "C" {

// instrumentation code for tracing
void __cyg_profile_func_enter(void *func_address, void *call_site) __attribute__((no_instrument_function));
void __cyg_profile_func_exit(void *func_address, void *call_site) __attribute__((no_instrument_function));
static char *resolve_function(void *addr) __attribute__((no_instrument_function));

static char *resolve_function(void *addr) {
  Dl_info info;
  int s;

  dladdr(addr, &info);

  return __cxxabiv1::__cxa_demangle(info.dli_sname, NULL, NULL, &s);
}

void __cyg_profile_func_enter(void *func_address, void *call_site) {
  if (trace_on) {
    char *s = resolve_function(func_address);
    struct timeval t;
    gettimeofday(&t, NULL);

    ++trace_depth;

    fprintf(trace_file ?: stdout, "TRACE:%.4f:%*senter %s\n",
            (t.tv_sec + t.tv_usec / 1000000.0) - (start_time.tv_sec + start_time.tv_usec / 1000000.0), trace_depth, "",
            s);
    free(s);
  }
}

void __cyg_profile_func_exit(void *func_address, void *call_site) {
  if (trace_on) {
    char *s = resolve_function(func_address);
    struct timeval t;
    gettimeofday(&t, NULL);

    fprintf(trace_file ?: stdout, "TRACE:%.4f:%*sleave %s\n",
            (t.tv_sec + t.tv_usec / 1000000.0) - (start_time.tv_sec + start_time.tv_usec / 1000000.0), trace_depth, "",
            s);

    --trace_depth;

    free(s);
  }
}
};

#endif

#include <cairo-xlib.h>

XlibCanvasView::XlibCanvasView(Display *dpy, Window win, Visual *visual, int width, int height)
  : CanvasView(width, height) {
  _crsurface = cairo_xlib_surface_create(dpy, win, visual, width, height);

  _cairo = new CairoCtx(_crsurface);
  cairo_set_tolerance(_cairo->get_cr(), 0.1);
}

void XlibCanvasView::update_view_size(int width, int height) {
  if (_view_width != width || _view_height != height) {
    _view_width = width;
    _view_height = height;

    cairo_xlib_surface_set_size(_crsurface, width, height);

    update_offsets();
    queue_repaint();

    _viewport_changed_signal();
  }
}

bool XlibCanvasView::initialize() {
  return CanvasView::initialize();
}

void XlibCanvasView::begin_repaint(int x, int y, int w, int h) {
}

void XlibCanvasView::end_repaint() {
}

//-------------------------------------------------------------------------------------

BufferedXlibCanvasView::BufferedXlibCanvasView(Display *dpy, Window win, Visual *visual, int depth, int width,
                                               int height)
  : CanvasView(width, height), _display(dpy), _window(win), _visual(visual), _depth(depth) {
  _back_buffer = XCreatePixmap(_display, _window, width, height, _depth);
  _crsurface = cairo_xlib_surface_create(_display, _back_buffer, _visual, width, height);

  _cairo = new CairoCtx(_crsurface);
  cairo_set_tolerance(_cairo->get_cr(), 0.1);

  XGCValues gcv;
  gcv.function = GXcopy;
  _copy_gc = XCreateGC(_display, _window, GCFunction, &gcv);
}

BufferedXlibCanvasView::~BufferedXlibCanvasView() {
  XFreePixmap(_display, _back_buffer);
  XFreeGC(_display, _copy_gc);
}

bool BufferedXlibCanvasView::initialize() {
  return CanvasView::initialize();
}

void BufferedXlibCanvasView::update_view_size(int width, int height) {
  if (_view_width != width || _view_height != height) {
    _view_width = width;
    _view_height = height;

    delete _cairo;
    if (_crsurface)
      cairo_surface_destroy(_crsurface);

    if (_back_buffer != None)
      XFreePixmap(_display, _back_buffer);
    _back_buffer = XCreatePixmap(_display, _window, _view_width, _view_height, _depth);

    _crsurface = cairo_xlib_surface_create(_display, _back_buffer, _visual, _view_width, _view_height);

    _cairo = new CairoCtx(_crsurface);
    cairo_set_tolerance(_cairo->get_cr(), 0.1);

    update_offsets();
    queue_repaint();

    _viewport_changed_signal();
  }
}

void BufferedXlibCanvasView::scroll_to(const base::Point &offs) {
  base::Point new_offset;
  base::Size viewable_size(get_viewable_size());
  base::Size total_size(get_total_view_size());

  new_offset = offs.round();
  new_offset.x = std::max(0.0, std::min(new_offset.x, total_size.width - viewable_size.width));
  new_offset.y = std::max(0.0, std::min(new_offset.y, total_size.height - viewable_size.height));

  if (new_offset != _offset) {
    base::Rect vrefresh, hrefresh;
    base::Rect copy_area;
    base::Point copy_target;
    bool blit = false;

    copy_target = _offset;
    copy_area.pos = _offset;
    copy_area.size = viewable_size;

    if (new_offset.x > _offset.x) {
      if (new_offset.x - _offset.x < viewable_size.width) {
        copy_area.pos.x = new_offset.x;
        copy_area.size.width = _offset.x + viewable_size.width - new_offset.x;
        copy_target.x = _offset.x;
        blit = true;

        vrefresh.pos.x = copy_area.right();
        vrefresh.pos.y = _offset.y;
        vrefresh.size.width = viewable_size.width - copy_area.width();
        vrefresh.size.height = viewable_size.height;
      }
    } else if (new_offset.x < _offset.x) {
      if (_offset.x - new_offset.x < viewable_size.width) {
        copy_area.pos.x = _offset.x;
        copy_area.size.width = viewable_size.width - (_offset.x - new_offset.x);
        copy_target.x = _offset.x + (_offset.x - new_offset.x);
        blit = true;

        vrefresh.pos.x = new_offset.x;
        vrefresh.pos.y = _offset.y;
        vrefresh.size.width = viewable_size.width - copy_area.width();
        vrefresh.size.height = viewable_size.height;
      }
    }

    if (new_offset.y > _offset.y) {
      if (new_offset.y - _offset.y < viewable_size.height) {
        copy_area.pos.y = new_offset.y;
        copy_area.size.height = _offset.y + viewable_size.height - new_offset.y;
        copy_target.y = _offset.y;
        blit = true;

        hrefresh.pos.y = copy_area.bottom();
        hrefresh.pos.x = _offset.x;
        hrefresh.size.height = viewable_size.height - copy_area.height();
        hrefresh.size.width = viewable_size.width;
      }
    } else if (new_offset.y < _offset.y) {
      if (_offset.y - new_offset.y < viewable_size.height) {
        copy_area.pos.y = _offset.y;
        copy_area.size.height = viewable_size.height - (_offset.y - new_offset.y);
        copy_target.y = _offset.y + (_offset.y - new_offset.y);
        blit = true;

        hrefresh.pos.y = new_offset.y;
        hrefresh.pos.x = _offset.x;
        hrefresh.size.height = viewable_size.height - copy_area.height();
        hrefresh.size.width = viewable_size.width;
      }
    }

    if (blit) {
      int x, y, w, h;
      int tx, ty;

      canvas_to_window(copy_area, x, y, w, h);
      canvas_to_window(copy_target, tx, ty);

      XCopyArea(_display, _window, _window, _copy_gc, x, y, w, h, tx, ty);

      _offset = new_offset;

      if (hrefresh.width() > 0 && hrefresh.height() > 0) {
        canvas_to_window(hrefresh, x, y, w, h);

        repaint_area(hrefresh, x, y, w, h);
      }
      if (vrefresh.width() > 0 && vrefresh.height() > 0) {
        canvas_to_window(vrefresh, x, y, w, h);

        repaint_area(vrefresh, x, y, w, h);
      }
    } else {
      _offset = new_offset;
      queue_repaint();
    }

    update_offsets();

    _viewport_changed_signal();
  }
}

void BufferedXlibCanvasView::make_current() {
}

/*
Surface *BufferedXlibCanvasView::create_temp_surface(const Size &size) const
{
  cairo_surface_t *surf= cairo_xlib_surface_create_with_dib(CAIRO_FORMAT_ARGB32,
    (int)size.width, (int)size.height);
  Surface *s= new Surface(surf);
  cairo_surface_destroy(surf);

  return s;
}*/

void BufferedXlibCanvasView::begin_repaint(int x, int y, int w, int h) {
#ifdef ___TRACE
  if (start_tracing) {
    gettimeofday(&start_time, NULL);

    fprintf(trace_file, "TRACE:0.0000: ============= BEGIN REPAINT =================\n");

    trace_on = true;
  }
#endif
  _clip_x = x;
  _clip_y = y;
  _clip_w = w;
  _clip_h = h;
}

void BufferedXlibCanvasView::end_repaint() {
  XCopyArea(_display, _back_buffer, _window, _copy_gc, _clip_x, _clip_y, _clip_w, _clip_h, _clip_x, _clip_y);

#ifdef ___TRACE
  if (start_tracing) {
    struct timeval t;
    gettimeofday(&t, NULL);

    trace_on = false;

    fprintf(trace_file, "TRACE:%.4f: ============= END REPAINT =================\n",
            (t.tv_sec + t.tv_usec / 1000000.0) - (start_time.tv_sec + start_time.tv_usec / 1000000.0));
  }
#endif
}
