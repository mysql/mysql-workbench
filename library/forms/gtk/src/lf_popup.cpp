/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "../lf_mforms.h"
#include "../lf_popup.h"
#include "mforms.h"
#include "gtk_helpers.h"
#include <typeinfo>
#include <gdkmm/devicemanager.h>

//#define d(...) {fprintf(stderr, "%s:%i: ", __PRETTY_FUNCTION__, __LINE__); fprintf(stderr,__VA_ARGS__);}
#define d(...)

namespace mforms {
  namespace gtk {

    //------------------------------------------------------------------------------

    PopupImpl::PopupImpl(::mforms::Popup *self, mforms::PopupStyle style)
      : ObjectImpl(self), _width(-1), _height(-1), _have_rgba(false), _inside(false), _result(-1), _style(style) {
      d("\n");
      //  mforms::Popup* selfc = dynamic_cast<mforms::Popup*>(owner);
      _wnd.set_app_paintable(true);

      _wnd.signal_draw().connect(sigc::mem_fun(this, &PopupImpl::handle_draw_event));
      _wnd.signal_key_press_event().connect(sigc::mem_fun(this, &PopupImpl::key_press_event));
      _wnd.signal_button_press_event().connect(sigc::mem_fun(this, &PopupImpl::mouse_button_event));
      _wnd.signal_button_release_event().connect(sigc::mem_fun(this, &PopupImpl::mouse_button_event));
      _wnd.signal_enter_notify_event().connect(sigc::mem_fun(this, &PopupImpl::mouse_cross_event));
      _wnd.signal_leave_notify_event().connect(sigc::mem_fun(this, &PopupImpl::mouse_cross_event));
      _wnd.signal_motion_notify_event().connect(sigc::mem_fun(this, &PopupImpl::mouse_move_event));

      // request mouse moved events
      _wnd.add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::POINTER_MOTION_MASK |
                      Gdk::KEY_PRESS_MASK);
      _wnd.set_gravity(Gdk::GRAVITY_NORTH_WEST);

      _wnd.property_skip_taskbar_hint() = true;
      _wnd.property_skip_pager_hint() = true;
      _wnd.property_decorated() = false;

      _wnd.override_background_color(color_to_rgba(Gdk::Color("black")), Gtk::STATE_FLAG_NORMAL);

      set_size(self, 825, 351);
      auto wnd = get_mainwindow();
      if (wnd != nullptr)
        _wnd.set_transient_for(*wnd);
      _wnd.set_modal(true);

      auto w = (Gtk::Widget*)&_wnd;
      gtk_widget_set_visual(w->gobj(), _wnd.get_screen()->get_rgba_visual()->gobj());
    }
    //------------------------------------------------------------------------------
    PopupImpl::~PopupImpl() {
      if (!_idleClose.empty())
        _idleClose.disconnect();
    }

    //------------------------------------------------------------------------------

    //------------------------------------------------------------------------------
    bool PopupImpl::handle_draw_event(const ::Cairo::RefPtr< ::Cairo::Context> &context) {
      d("\n");
      mforms::Popup *self = dynamic_cast<mforms::Popup *>(owner);
      if (self) {
        cairo_t *cr = context->cobj();

        if (cr) {
          // Draw round corners
          if (_width > 0 && _height > 0 && _style == mforms::PopupBezel) {
            // cairo_save(cr);

            const int W = _width;
            const int H = _height;

            if (_have_rgba)
              cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.0);
            else
              cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);

            cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
            cairo_paint(cr);

            cairo_new_path(cr);
            cairo_move_to(cr, R, 0);                  // 1
            cairo_line_to(cr, W - R, 0);              // 2
            cairo_curve_to(cr, W, 0, W, 0, W, R);     // 3
            cairo_line_to(cr, W, H - R);              // 4
            cairo_curve_to(cr, W, H, W, H, W - R, H); // 5
            cairo_line_to(cr, R, H);                  // 6
            cairo_curve_to(cr, 0, H, 0, H, 0, H - R); // 7
            cairo_line_to(cr, 0, R);                  // 8
            cairo_curve_to(cr, 0, 0, 0, 0, R, 0);     // 9
            cairo_close_path(cr);

            cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.92);
            cairo_fill_preserve(cr);

            cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
            self->repaint(cr, R, R, _width - R, _height - R);
          } else {
            double x1, y1, x2, y2;
            context->get_clip_extents(x1, y1, x2, y2);
            self->repaint(cr, x1, y1, x2 - x1, y2 - y1);
          }
        }
      }
      return true;
    }

    //------------------------------------------------------------------------------
    bool PopupImpl::key_press_event(GdkEventKey *event) {
      if (event->keyval == GDK_KEY_Escape) {
        set_modal_result(dynamic_cast<mforms::Popup *>(owner), 0);
      }
      return true;
    }

    //------------------------------------------------------------------------------
    bool PopupImpl::mouse_cross_event(GdkEventCrossing *event) {
      mforms::Popup *self = dynamic_cast<mforms::Popup *>(owner);
      if (self && _wnd.get_window()->gobj() == event->window) {
        d("\n");
        if (event->type == GDK_ENTER_NOTIFY) {
          _inside = true;
          self->mouse_enter();
        } else {
          _inside = false;
          self->mouse_leave();
        }
      }
      return true;
    }

    //------------------------------------------------------------------------------
    bool PopupImpl::mouse_button_event(GdkEventButton *event) {
      mforms::Popup *self = dynamic_cast<mforms::Popup *>(owner);
      d("\n");
      if (self && _wnd.get_window()->gobj() == event->window) {
        if (!_inside) {
          set_modal_result(self, 0);

          return false;
        }

        mforms::MouseButton mbtn = MouseButtonOther;
        switch (event->button) // button number assumptions from starter icon code
        {
          case 1:
            mbtn = MouseButtonLeft;
            break;
          case 2:
            mbtn = MouseButtonOther;
            break;
          case 3:
            mbtn = MouseButtonRight;
            break;
        }

        if (event->type == GDK_BUTTON_PRESS) {
          self->mouse_down(mbtn, (int)event->x, (int)event->y);
        } else if (event->type == GDK_BUTTON_RELEASE) {
          self->retain();
          self->mouse_up(mbtn, (int)event->x, (int)event->y);
          self->mouse_click(mbtn, (int)event->x, (int)event->y); // Click must be called after mouse was up!
          self->release();
        } else if (event->type == GDK_2BUTTON_PRESS)
          self->mouse_double_click(mbtn, (int)event->x, (int)event->y);
      } else
        set_modal_result(self, 0);
      return false;
    }

    //------------------------------------------------------------------------------
    bool PopupImpl::mouse_move_event(GdkEventMotion *event) {
      d("\n");
      mforms::Popup *self = dynamic_cast<mforms::Popup *>(owner);
      if (_inside && self && _wnd.get_window()->gobj() == event->window) {
        self->mouse_move(MouseButtonLeft, (int)event->x, (int)event->y);
      }
      return true;
    }

    //------------------------------------------------------------------------------
    bool PopupImpl::create(::mforms::Popup *self, ::mforms::PopupStyle style) {
      d("\n");
      return new PopupImpl(self, style) != 0;
    }

    //------------------------------------------------------------------------------

    void PopupImpl::destroy(::mforms::Popup *self) {
      d("\n");
      PopupImpl *impl = self->get_data<PopupImpl>();
      self->set_data(NULL, NULL);
      delete impl;
    }

    //------------------------------------------------------------------------------
    void PopupImpl::set_needs_repaint(::mforms::Popup *self) {
      d("\n");
      // request a repaint so that this can be called from any thread
      PopupImpl *impl = self->get_data<PopupImpl>();
      impl->_wnd.queue_draw();
    }

    //------------------------------------------------------------------------------
    void PopupImpl::set_size(::mforms::Popup *self, int w, int h) {
      PopupImpl *impl = self->get_data<PopupImpl>();
      d("\n");

      impl->_width = w;  // + 2*R;
      impl->_height = h; // + 2*R;
      impl->_wnd.set_size_request(impl->_width, impl->_height);
    }

    //------------------------------------------------------------------------------
    int PopupImpl::show(::mforms::Popup *self, int x, int y) {
      PopupImpl *impl = self->get_data<PopupImpl>();
      d("x=%i, y=%i\n", x, y);
      if (impl->_wnd.is_visible())
        impl->_wnd.hide();

      {
        // Gtk::Window* main_window = get_mainwindow();

        //    const Gtk::Requisition req = impl->_wnd.size_request();
        impl->_wnd.show();
        impl->_wnd.move(x, y);

        if (impl->_style == mforms::PopupBezel) {
          impl->_wnd.get_window()->get_display()->get_device_manager()->get_client_pointer()->grab(
            impl->_wnd.get_window(), Gdk::OWNERSHIP_NONE, true,
            Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::POINTER_MOTION_MASK, 0);

          //      impl->_wnd.get_window()->pointer_grab(true,
          //      Gdk::BUTTON_PRESS_MASK|Gdk::BUTTON_RELEASE_MASK|Gdk::POINTER_MOTION_MASK, 0);

          impl->_loop.run();
          impl->_wnd.set_modal(false);
          impl->_wnd.hide();
        }
      }

      return impl->_result;
    }

    //------------------------------------------------------------------------------
    base::Rect PopupImpl::get_content_rect(::mforms::Popup *self) {
      d("\n");
      PopupImpl *impl = self->get_data<PopupImpl>();
      if (impl->_style == mforms::PopupBezel)
        return base::Rect(R, R, impl->_width - R, impl->_height - R);
      else
        return base::Rect(0, 0, impl->_width, impl->_height);
    }

    //------------------------------------------------------------------------------
    void PopupImpl::set_modal_result(Popup *self, int result) {
      d("\n");
      PopupImpl *impl = self->get_data<PopupImpl>();
      impl->_result = result;
      impl->_wnd.hide();

      if (result > -1 && impl->_style == mforms::PopupBezel)
        impl->_loop.quit();

      // must call closed() cb when idle, because it can delete the popup
      // in the middle of an event handler
      if (!impl->_idleClose.empty())
        impl->_idleClose.disconnect();
      impl->_idleClose = Glib::signal_idle().connect(sigc::bind_return(sigc::mem_fun(self, &Popup::closed), false));
    }

    //------------------------------------------------------------------------------
    void PopupImpl::init() {
      ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

      f->_popup_impl.create = &PopupImpl::create;
      f->_popup_impl.destroy = &PopupImpl::destroy;
      f->_popup_impl.set_needs_repaint = &PopupImpl::set_needs_repaint;
      f->_popup_impl.set_size = &PopupImpl::set_size;
      f->_popup_impl.show = &PopupImpl::show;
      f->_popup_impl.get_content_rect = &PopupImpl::get_content_rect;
      f->_popup_impl.set_modal_result = &PopupImpl::set_modal_result;
    }
  };
};
