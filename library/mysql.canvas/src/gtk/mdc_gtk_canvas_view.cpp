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

#include "mdc_gtk_canvas_view.h"
#include <gdk/gdkx.h>
#include <sys/time.h>

#define WHEEL_SCROLL_STEP 12

using namespace mdc;

std::string mdc::detect_opengl_version() {
  int major, minor;
  Display *dpy = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
  if (!glXQueryVersion(dpy, &major, &minor))
    return ""; // glx not supported
  return "";
}

GtkCanvas::GtkCanvas(CanvasType type)
  : Gtk::Layout(), _canvas(0), _canvas_type(type), _reentrance(false), _initialized(false) {
  Gdk::Color c("white");
  Gdk::RGBA rgba;
  rgba.set_rgba(c.get_red_p(), c.get_green_p(), c.get_blue_p());

  override_background_color(rgba, Gtk::STATE_FLAG_NORMAL);

  set_double_buffered(false);
  add_events(Gdk::POINTER_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::KEY_PRESS_MASK |
             Gdk::KEY_RELEASE_MASK | Gdk::ENTER_NOTIFY_MASK | Gdk::LEAVE_NOTIFY_MASK);
  signal_draw().connect(sigc::mem_fun(this, &GtkCanvas::redraw));

  unset_vadjustment(); // we don't need this as we will set our own
  unset_hadjustment(); // we don't need this as we will set our own
  set_can_focus(true);
}

GtkCanvas::~GtkCanvas() {
  if (_canvas != 0)
    delete _canvas;
}

CanvasView *GtkCanvas::get_canvas() {
  return _canvas;
}

void GtkCanvas::create_canvas() {
  if (_canvas != 0)
    return;

  Display *dpy = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());

  switch (_canvas_type) {
    case OpenGLCanvasType:
      _canvas = new GLXCanvasView(dpy, gdk_x11_window_get_xid(get_bin_window()->gobj()),
                                  gdk_x11_visual_get_xvisual(get_visual()->gobj()), get_width(), get_height());
      break;
    case XlibCanvasType:
      _canvas = new XlibCanvasView(dpy, gdk_x11_window_get_xid(get_bin_window()->gobj()),
                                   gdk_x11_visual_get_xvisual(get_visual()->gobj()), get_width(), get_height());
      break;
    case BufferedXlibCanvasType:
      _canvas = new BufferedXlibCanvasView(
        dpy, gdk_x11_window_get_xid(get_bin_window()->gobj()), gdk_x11_visual_get_xvisual(get_visual()->gobj()),
        gdk_visual_get_depth(gdk_window_get_visual(get_bin_window()->gobj())), get_width(), get_height());
      break;
  }
  _initialized = false;
}

void GtkCanvas::set_vadjustment(const Glib::RefPtr<Gtk::Adjustment> &vadjustment) {
  Scrollable::set_vadjustment(vadjustment);
  get_vadjustment()->set_lower(0);
  get_vadjustment()->signal_value_changed().connect(sigc::mem_fun(*this, &GtkCanvas::scroll_canvas));

  int ret = g_signal_handlers_disconnect_matched(get_vadjustment()->gobj(), G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, gobj());
  g_assert(ret == 1);
}

void GtkCanvas::set_hadjustment(const Glib::RefPtr<Gtk::Adjustment> &hadjustment) {
  Scrollable::set_hadjustment(hadjustment);
  get_hadjustment()->set_lower(0);
  get_hadjustment()->signal_value_changed().connect(sigc::mem_fun(*this, &GtkCanvas::scroll_canvas));
  int ret = g_signal_handlers_disconnect_matched(get_hadjustment()->gobj(), G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, gobj());
  g_assert(ret == 1);
}

void GtkCanvas::on_realize() {
  super::on_realize();
  this->create_canvas();
}

void GtkCanvas::on_unrealize() {
  if (_canvas != 0) {
    delete _canvas;
    _canvas = 0;
  }

  super::on_unrealize();
}

void GtkCanvas::on_map() {
  super::on_map();

  if (_initialized)
    return;

  if (!_canvas->initialize()) {
    g_warning("could not initialize canvas");
    delete _canvas;
    _canvas = 0;
    return;
  }

  scoped_connect(_canvas->signal_viewport_changed(), sigc::mem_fun(this, &GtkCanvas::canvas_view_viewport_changed));
  scoped_connect(_canvas->signal_repaint(), sigc::mem_fun(this, &GtkCanvas::canvas_view_needs_repaint));

  _canvas->repaint();
}

void GtkCanvas::on_size_allocate(Gtk::Allocation &alloc) {
  if (_reentrance)
    return;
  _reentrance = true;

  super::on_size_allocate(alloc);

  if (_canvas)
    _canvas->update_view_size(alloc.get_width(), alloc.get_height());
  // set_size(alloc.get_width(), alloc.get_height());
  _reentrance = false;
}

mdc::EventState GtkCanvas::get_event_state(int event_state) {
  mdc::EventState state = ((mdc::EventState)0);

  if (event_state & GDK_CONTROL_MASK)
    state = state | mdc::SControlMask;
  if (event_state & GDK_SHIFT_MASK)
    state = state | mdc::SShiftMask;
  if (event_state & GDK_MOD1_MASK)
    state = state | mdc::SAltMask;

  if (event_state & GDK_BUTTON1_MASK)
    state = state | mdc::SLeftButtonMask;
  if (event_state & GDK_BUTTON2_MASK)
    state = state | mdc::SMiddleButtonMask;
  if (event_state & GDK_BUTTON3_MASK)
    state = state | mdc::SRightButtonMask;

  return state;
}

bool GtkCanvas::on_button_press_event(GdkEventButton *event) {
  MouseButton button = ButtonLeft;

  grab_focus();

  switch (event->button) {
    case 1:
      button = ButtonLeft;
      break;
    case 2:
      button = ButtonMiddle;
      break;
    case 3:
      button = ButtonRight;
      break;
  }
  if (event->type == GDK_2BUTTON_PRESS)
    _canvas->handle_mouse_double_click(button, (int)event->x, (int)event->y, get_event_state(event->state));

  else
    _canvas->handle_mouse_button(button, true, (int)event->x, (int)event->y, get_event_state(event->state));

  return true;
}

bool GtkCanvas::on_button_release_event(GdkEventButton *event) {
  MouseButton button = ButtonLeft;

  switch (event->button) {
    case 1:
      button = ButtonLeft;
      break;
    case 2:
      button = ButtonMiddle;
      break;
    case 3:
      button = ButtonRight;
      break;
  }

  _canvas->handle_mouse_button(button, false, (int)event->x, (int)event->y, get_event_state(event->state));

  return true;
}

bool GtkCanvas::on_motion_notify_event(GdkEventMotion *event) {
  _canvas->handle_mouse_move((int)event->x, (int)event->y, get_event_state(event->state));

  return true;
}

bool GtkCanvas::on_event(GdkEvent *event) {
  if (event->type == GDK_ENTER_NOTIFY)
    _canvas->handle_mouse_enter((int)event->motion.x, (int)event->motion.y, get_event_state(event->motion.state));
  else if (event->type == GDK_LEAVE_NOTIFY)
    _canvas->handle_mouse_leave((int)event->motion.x, (int)event->motion.y, get_event_state(event->motion.state));

  return false;
}

bool GtkCanvas::on_scroll_event(GdkEventScroll *event) {
  double x, y;
  base::Rect rect;
  rect = _canvas->get_viewport();
  guint modifiers = gtk_accelerator_get_default_mod_mask();

  x = rect.pos.x;
  y = rect.pos.y;

  switch (event->direction) {
    case GDK_SCROLL_DOWN:
      if ((event->state & modifiers) == GDK_SHIFT_MASK) {
        x += WHEEL_SCROLL_STEP;
        break;
      } else if ((event->state & modifiers) == GDK_CONTROL_MASK) {
        on_zoom_out_event();
        return true;
      } else {
        y += WHEEL_SCROLL_STEP;
        break;
      }
    case GDK_SCROLL_RIGHT:
      x += WHEEL_SCROLL_STEP;
      break;

    case GDK_SCROLL_UP:
      if ((event->state & modifiers) == GDK_SHIFT_MASK) {
        x -= WHEEL_SCROLL_STEP;
        break;
      } else if ((event->state & modifiers) == GDK_CONTROL_MASK) {
        on_zoom_in_event();
        return true;
      } else {
        y -= WHEEL_SCROLL_STEP;
        break;
      }
    case GDK_SCROLL_LEFT:
      x -= WHEEL_SCROLL_STEP;
      break;
    default:
      break;
  }

  if (get_vadjustment()) {
    if (y < 0)
      y = 0;
    else if (y > get_vadjustment()->get_upper())
      y = get_vadjustment()->get_upper();

    if (get_vadjustment()->get_value() != y)
      get_vadjustment()->set_value(y);
  }

  if (get_hadjustment()) {
    if (x < 0)
      x = 0;
    else if (x > get_hadjustment()->get_upper())
      x = get_hadjustment()->get_upper();

    if (get_hadjustment()->get_value() != x)
      get_hadjustment()->set_value(x);
  }

  return true;
}

void GtkCanvas::on_zoom_in_event() {
}

void GtkCanvas::on_zoom_out_event() {
}

bool GtkCanvas::on_key_press_event(GdkEventKey *event) {
  return false;
}

bool GtkCanvas::on_key_release_event(GdkEventKey *event) {
  return false;
}

bool GtkCanvas::redraw(::Cairo::RefPtr< ::Cairo::Context> context) {
  if (should_draw_window(context, this->get_bin_window())) {
    struct timeval tv, tv2;

    gettimeofday(&tv, NULL);

    double x1, y1, x2, y2;
    context->get_clip_extents(x1, y1, x2, y2);
    _canvas->repaint(x1, y1, x2 - x1, y2 - y1);
    gettimeofday(&tv2, NULL);

    static const char *debug_canvas = getenv("DEBUG_CANVAS");
    if (debug_canvas)
      printf("rendertime= %.4f (%.1ffps)\n", (tv2.tv_sec - tv.tv_sec) + (tv2.tv_usec - tv.tv_usec) / 1000000.0,
             1.0 / ((tv2.tv_sec - tv.tv_sec) + (tv2.tv_usec - tv.tv_usec) / 1000000.0));
  }

  return true;
}

void GtkCanvas::canvas_view_needs_repaint(int, int, int, int) {
  queue_draw();
}

void GtkCanvas::canvas_view_viewport_changed() {
  update_scrollers();
}

void GtkCanvas::scroll_canvas() {
  if (_canvas) {
    float xpos = get_hadjustment()->get_value();
    float ypos = get_vadjustment()->get_value();

    _canvas->set_offset(base::Point(xpos, ypos));
  }
}

void GtkCanvas::update_scrollers() {
  base::Size size = _canvas->get_total_view_size();
  base::Rect vp = _canvas->get_viewport();
  Glib::RefPtr<Gtk::Adjustment> hadjustment = get_hadjustment();
  Glib::RefPtr<Gtk::Adjustment> vadjustment = get_vadjustment();

  set_size(size.width, size.height);

  if (hadjustment) {
    if (hadjustment->get_upper() != size.width)
      hadjustment->set_upper(size.width);

    if (hadjustment->get_page_increment() != vp.size.width / 2)
      hadjustment->set_page_increment(vp.size.width / 2);
    if (hadjustment->get_page_size() != vp.size.width)
      hadjustment->set_page_size(vp.size.width);
    if (hadjustment->get_step_increment() != 10)
      hadjustment->set_step_increment(10);
    if (hadjustment->get_value() != vp.pos.x)
      hadjustment->set_value(vp.pos.x);
  }
  if (vadjustment) {
    if (vadjustment->get_upper() != size.height)
      vadjustment->set_upper(size.height);

    if (vadjustment->get_page_increment() != vp.size.height / 2)
      vadjustment->set_page_increment(vp.size.height / 2);
    if (vadjustment->get_page_size() != vp.size.height)
      vadjustment->set_page_size(vp.size.height);
    if (vadjustment->get_step_increment() != 10)
      vadjustment->set_step_increment(10);
    if (vadjustment->get_value() != vp.pos.y)
      vadjustment->set_value(vp.pos.y);
  }
}
