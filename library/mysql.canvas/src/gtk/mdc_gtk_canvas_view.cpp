/* 
 * Copyright (c) 2007, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "mdc_gtk_canvas_view.h"
#include <gdk/gdkx.h>
#include <sys/time.h>

#define WHEEL_SCROLL_STEP 12

using namespace mdc;





std::string mdc::detect_opengl_version()
{
  int major, minor;
  
  if (!glXQueryVersion(gdk_display, &major, &minor))
    return "";  // glx not supported

  /*
  char buffer[100];
  
  sprintf(buffer, "Vendor: %s, Renderer: %s, OpenGL version: %s, GLX version: %i.%i",
          glGetString(GL_VENDOR),
          glGetString(GL_RENDERER),
          glGetString(GL_VERSION),
          major, minor);*/

//  return (const char*)glGetString(GL_VERSION) ?:"";

  return "";
}




GtkCanvas::GtkCanvas(CanvasType type)
  : _canvas(0), _canvas_type(type)
{
  set_flags(get_flags()|Gtk::CAN_FOCUS|Gtk::APP_PAINTABLE);
  modify_bg(Gtk::STATE_NORMAL, get_style()->get_white());
  set_double_buffered(false);
  add_events(Gdk::POINTER_MOTION_MASK|Gdk::BUTTON_PRESS_MASK|Gdk::BUTTON_RELEASE_MASK|Gdk::KEY_PRESS_MASK|Gdk::KEY_RELEASE_MASK|Gdk::ENTER_NOTIFY_MASK|Gdk::LEAVE_NOTIFY_MASK);
  signal_expose_event().connect(sigc::mem_fun(this, &GtkCanvas::redraw));
}


GtkCanvas::~GtkCanvas()
{
  delete _canvas;
}


void GtkCanvas::on_realize()
{ 
  super::on_realize();

  switch (_canvas_type)
  {
  case OpenGLCanvasType:
    _canvas= new GLXCanvasView(gdk_display, gdk_x11_drawable_get_xid(get_bin_window()->gobj()),
                               gdk_x11_visual_get_xvisual(get_visual()->gobj()),
                               get_width(), get_height());
    break;
  case XlibCanvasType:
    _canvas= new XlibCanvasView(gdk_display, gdk_x11_drawable_get_xid(get_bin_window()->gobj()),
                                gdk_x11_visual_get_xvisual(get_visual()->gobj()), 
                                get_width(), get_height());
    break;
  case BufferedXlibCanvasType:
    _canvas= new BufferedXlibCanvasView(gdk_display, gdk_x11_drawable_get_xid(get_bin_window()->gobj()),
                                        gdk_x11_visual_get_xvisual(get_visual()->gobj()), 
                                        gdk_drawable_get_depth(get_bin_window()->gobj()),
                                        get_width(), get_height());
    break;
  }
  _initialized= false;
}


void GtkCanvas::on_unrealize()
{
  delete _canvas;
  _canvas = 0;

  super::on_unrealize();
}


void GtkCanvas::on_map()
{
  super::on_map();

  if (_initialized)
    return;
  
  if (!_canvas->initialize())
  {
    g_warning("could not initialize canvas");
    delete _canvas;
    _canvas= 0;
    return;
  }

  scoped_connect(_canvas->signal_viewport_changed(),sigc::mem_fun(this, &GtkCanvas::canvas_view_viewport_changed));
  scoped_connect(_canvas->signal_repaint(),sigc::mem_fun(this, &GtkCanvas::canvas_view_needs_repaint));

  _canvas->repaint();
}


void GtkCanvas::on_size_allocate(Gtk::Allocation &alloc)
{
  super::on_size_allocate(alloc);

  if (_canvas)
    _canvas->update_view_size(alloc.get_width(), alloc.get_height());
  //set_size(alloc.get_width(), alloc.get_height());
}


mdc::EventState GtkCanvas::get_event_state(int event_state)
{
  mdc::EventState state= ((mdc::EventState)0);

  if (event_state & GDK_CONTROL_MASK)
    state= state | mdc::SControlMask;
  if (event_state & GDK_SHIFT_MASK)
    state= state | mdc::SShiftMask;
  if (event_state & GDK_MOD1_MASK)
    state= state | mdc::SAltMask;
  
  if (event_state & GDK_BUTTON1_MASK)
    state= state | mdc::SLeftButtonMask;
  if (event_state & GDK_BUTTON2_MASK)
    state= state | mdc::SMiddleButtonMask;
  if (event_state & GDK_BUTTON3_MASK)
    state= state | mdc::SRightButtonMask;
  
  return state;
}


bool GtkCanvas::on_button_press_event(GdkEventButton* event)
{
  MouseButton button= ButtonLeft;
  
  grab_focus();

  switch (event->button)
  {
  case 1: button= ButtonLeft; break;
  case 2: button= ButtonMiddle; break;
  case 3: button= ButtonRight; break;
  }
  if (event->type == GDK_2BUTTON_PRESS)
    _canvas->handle_mouse_double_click(button,
                               (int)event->x, (int)event->y,
                               get_event_state(event->state));

  else
    _canvas->handle_mouse_button(button, true,
                               (int)event->x, (int)event->y,
                               get_event_state(event->state));

  return true;
}


bool GtkCanvas::on_button_release_event(GdkEventButton* event)
{
  MouseButton button= ButtonLeft;

  switch (event->button)
  {
  case 1: button= ButtonLeft; break;
  case 2: button= ButtonMiddle; break;
  case 3: button= ButtonRight; break;
  }

  _canvas->handle_mouse_button(button, false,
                               (int)event->x, (int)event->y,
                               get_event_state(event->state));

  return true;
}


bool GtkCanvas::on_motion_notify_event(GdkEventMotion* event)
{
  _canvas->handle_mouse_move((int)event->x, (int)event->y,
                             get_event_state(event->state));
  
  return true;
}

bool GtkCanvas::on_event(GdkEvent *event)
{
  if(event->type == GDK_ENTER_NOTIFY)
    _canvas->handle_mouse_enter((int)event->motion.x, (int)event->motion.y, get_event_state(event->motion.state));
  else if(event->type == GDK_LEAVE_NOTIFY)
    _canvas->handle_mouse_leave((int)event->motion.x, (int)event->motion.y, get_event_state(event->motion.state));

  return false;
}


bool GtkCanvas::on_scroll_event(GdkEventScroll *event)
{
  double x, y;
  base::Rect rect;
  rect= _canvas->get_viewport();
  guint modifiers = gtk_accelerator_get_default_mod_mask();

  x= rect.pos.x;
  y= rect.pos.y;
  
  switch (event->direction)
  {
  case GDK_SCROLL_DOWN:
    if ((event->state & modifiers) == GDK_SHIFT_MASK)
    {
      x += WHEEL_SCROLL_STEP;
      break;
    }
    else if ((event->state & modifiers) == GDK_CONTROL_MASK)
    {
      on_zoom_out_event();
      return true;
    }
    else
    {
      y += WHEEL_SCROLL_STEP;
      break;
    }
  case GDK_SCROLL_RIGHT:
    x += WHEEL_SCROLL_STEP;
    break;
    
  case GDK_SCROLL_UP:
    if ((event->state & modifiers) == GDK_SHIFT_MASK)
    {
      x -= WHEEL_SCROLL_STEP;
      break;
    }
    else if ((event->state & modifiers) == GDK_CONTROL_MASK)
    {
      on_zoom_in_event();
     return true;
    }
    else
    {
      y -= WHEEL_SCROLL_STEP;
      break;
    }
  case GDK_SCROLL_LEFT:
    x -= WHEEL_SCROLL_STEP;
    break;
  }
  
  if (get_vadjustment())
  {
    if (y < 0) y = 0; else if (y > get_vadjustment()->get_upper()) y = get_vadjustment()->get_upper();
    
    if (get_vadjustment()->get_value() != y)
      get_vadjustment()->set_value(y);
  }
   
  if (get_hadjustment())
  {
    if (x < 0) x = 0; else if (x > get_hadjustment()->get_upper()) x = get_hadjustment()->get_upper();
    
    if (get_hadjustment()->get_value() != x)
      get_hadjustment()->set_value(x);
  }
  
  return true;
}

void GtkCanvas::on_zoom_in_event()
{
}

void GtkCanvas::on_zoom_out_event()
{
}


bool GtkCanvas::on_key_press_event(GdkEventKey *event)
{
  return false;
}


bool GtkCanvas::on_key_release_event(GdkEventKey *event)
{
  return false;
}


bool GtkCanvas::redraw(GdkEventExpose *ev)
{
  if (ev->window == get_bin_window()->gobj())
  {
    struct timeval tv, tv2;

    gettimeofday(&tv, NULL);

    _canvas->repaint(ev->area.x, ev->area.y, ev->area.width, ev->area.height);
    gettimeofday(&tv2, NULL);

    static const char *debug_canvas= getenv("DEBUG_CANVAS");
    if (debug_canvas)
      printf("rendertime= %.4f (%.1ffps)\n", (tv2.tv_sec - tv.tv_sec) + (tv2.tv_usec - tv.tv_usec)/1000000.0,
             1.0/((tv2.tv_sec - tv.tv_sec) + (tv2.tv_usec - tv.tv_usec)/1000000.0));
  }

  return true;
}



void GtkCanvas::canvas_view_needs_repaint(int,int,int,int)
{
  queue_draw();
}


void GtkCanvas::canvas_view_viewport_changed()
{
  update_scrollers();
}


void GtkCanvas::on_set_scroll_adjustments(Gtk::Adjustment* hadjustment,
                                          Gtk::Adjustment* vadjustment)
{
  super::on_set_scroll_adjustments(hadjustment, vadjustment);

  hadjustment->set_lower(0);
  vadjustment->set_lower(0);

  // disconnect all signal handlers from GtkLayout because
  // we don't want the default scrolling of the GtkLayout as it
  // moves the bin_window around and the canvas expects it to be static
  int ret;
  ret= g_signal_handlers_disconnect_matched(hadjustment->gobj(),
                                       G_SIGNAL_MATCH_DATA,
                                       0, 0, 0, 0, gobj());
  g_assert(ret == 1);

  ret= g_signal_handlers_disconnect_matched(vadjustment->gobj(),
                                       G_SIGNAL_MATCH_DATA,
                                       0, 0, 0, 0, gobj());
  g_assert(ret == 1);

  hadjustment->signal_value_changed().connect(sigc::mem_fun(*this, &GtkCanvas::scroll_canvas));
  vadjustment->signal_value_changed().connect(sigc::mem_fun(*this, &GtkCanvas::scroll_canvas));
    
  if (_canvas)
    update_scrollers();
}


void GtkCanvas::scroll_canvas()
{
  if (_canvas)
  {
    float xpos= get_hadjustment()->get_value();
    float ypos= get_vadjustment()->get_value();

    _canvas->set_offset(base::Point(xpos, ypos));
  }
}


void GtkCanvas::update_scrollers()
{
  base::Size size= _canvas->get_total_view_size();
  base::Rect vp= _canvas->get_viewport();
  Gtk::Adjustment *hadjustment= get_hadjustment();
  Gtk::Adjustment *vadjustment= get_vadjustment();

  set_size(size.width, size.height);

  if (hadjustment)
  {
    if (hadjustment->get_upper() != size.width)
      hadjustment->set_upper(size.width);
    
    if (hadjustment->get_page_increment() != vp.size.width/2)
      hadjustment->set_page_increment(vp.size.width/2);
    if (hadjustment->get_page_size() != vp.size.width)
      hadjustment->set_page_size(vp.size.width);
    if (hadjustment->get_step_increment() != 10)
      hadjustment->set_step_increment(10);
    if (hadjustment->get_value() != vp.pos.x)
      hadjustment->set_value(vp.pos.x);
  }
  if (vadjustment)
  {
    if (vadjustment->get_upper() != size.height)
      vadjustment->set_upper(size.height);

    if (vadjustment->get_page_increment() != vp.size.height/2)
      vadjustment->set_page_increment(vp.size.height/2);
    if (vadjustment->get_page_size() != vp.size.height)
      vadjustment->set_page_size(vp.size.height);
    if (vadjustment->get_step_increment() != 10)
      vadjustment->set_step_increment(10);
    if (vadjustment->get_value() != vp.pos.y)
      vadjustment->set_value(vp.pos.y);
  }
}
