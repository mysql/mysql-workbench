/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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
#include "../lf_drawbox.h"
#include "base/log.h"
#include <gtk/gtk-a11y.h>
#include <atk/atk.h>
#include <atkmm.h>
#include "../mforms_acc.h"

DEFAULT_LOG_DOMAIN(DOMAIN_MFORMS_GTK);

namespace mforms {
  namespace gtk {

    DrawBoxImpl::DrawBoxImpl(::mforms::DrawBox *self)
      : ViewImpl(self),  _fixed_width(-1), _fixed_height(-1), _fixed(0), _relayout_pending(false), _drag_in_progress(false) {

      auto widget = mforms_new(); //this is freed by gtk

      _darea = dynamic_cast<Gtk::EventBox*>(Glib::wrap(widget));
      _mformsGTK = MFORMSOBJECT(widget);
      _mformsGTK->pmforms->SetMFormsOwner(self);

      _padding._left = 0;
      _padding._right = 0;
      _padding._top = 0;
      _padding._bottom = 0;
      _last_btn = MouseButtonNone;
      _darea->signal_draw().connect(sigc::bind(sigc::mem_fun(this, &DrawBoxImpl::repaint), self));

      _darea->signal_size_allocate().connect_notify(
        sigc::bind(sigc::mem_fun(this, &DrawBoxImpl::on_size_allocate), self));
      _darea->signal_button_press_event().connect(
        sigc::bind(sigc::mem_fun(this, &DrawBoxImpl::mouse_button_event), self));
      _darea->signal_button_release_event().connect(
        sigc::bind(sigc::mem_fun(this, &DrawBoxImpl::mouse_button_event), self));
      _darea->signal_motion_notify_event().connect(
        sigc::bind(sigc::mem_fun(this, &DrawBoxImpl::mouse_move_event), self));

      _darea->set_size_request(10, 10); // set initial size to allow a repaint event to arrive

      // request mouse moved events
      _darea->add_events(Gdk::POINTER_MOTION_MASK);
      _darea->show();
      setup();
    }

    DrawBoxImpl::~DrawBoxImpl() {
      _sig_relayout.disconnect();
      _darea = nullptr;
      _mformsGTK = nullptr;
    }

    void *DrawBoxImpl::on_repaint() {
      _darea->queue_draw();
      return 0;
    }

    void DrawBoxImpl::set_size(int width, int height) {
      _darea->set_size_request(width, height);
      ViewImpl::set_size(width, height);

      _fixed_width = width;
      _fixed_height = height;
    }
    bool DrawBoxImpl::relayout(::mforms::DrawBox *self) {
      Glib::RefPtr<Gdk::Window> window = _darea->get_window();
      if (_fixed && window) {
        int ww, wh;
        ww = window->get_width();
        wh = window->get_height();

        for (std::map<Gtk::Widget *, AlignControl>::iterator it = _alignments.begin(); it != _alignments.end(); ++it) {
          if (it->second._align == mforms::NoAlign)
            continue;
          int x, y;
          switch (it->second._align) {
            case mforms::BottomLeft:
            case mforms::MiddleLeft:
            case mforms::TopLeft:
              x = _padding._left;
              break;

            case mforms::BottomCenter:
            case mforms::MiddleCenter:
            case mforms::TopCenter:
              x = (ww - it->first->get_width()) / 2;
              break;

            case mforms::BottomRight:
            case mforms::MiddleRight:
            case mforms::TopRight:
              x = ww - _padding._right - it->first->get_width();
              break;

            default:
              x = 0;
              break;
          }

          switch (it->second._align) {
            case mforms::BottomLeft:
            case mforms::BottomCenter:
            case mforms::BottomRight:
              y = wh - it->first->get_height() - _padding._bottom;
              break;

            case mforms::MiddleLeft:
            case mforms::MiddleCenter:
            case mforms::MiddleRight:
              y = (wh - it->first->get_height()) / 2;
              break;

            case mforms::TopLeft:
            case mforms::TopCenter:
            case mforms::TopRight:
            default:
              y = _padding._top;
              break;
          }

          if (it->second._x != x || it->second._y != y) {
            it->second._x = x;
            it->second._y = y;
            _fixed->move(*it->first, x, y);
          }
        }
      }
      _relayout_pending = false;
      return false;
    }

    void DrawBoxImpl::on_size_allocate(Gtk::Allocation &alloc, ::mforms::DrawBox *self) {
      if (!_relayout_pending) {
        _sig_relayout.disconnect();
        _sig_relayout = Glib::signal_idle().connect(sigc::bind(sigc::mem_fun(this, &DrawBoxImpl::relayout), self));
        _relayout_pending = true;
      }
    }

    bool DrawBoxImpl::repaint(const ::Cairo::RefPtr< ::Cairo::Context> &context, ::mforms::DrawBox *self) {
      // This vv needs improvment on linux. Maybe setup an event listener which is bound to resize
      Gtk::Requisition minimum, natural;
      _darea->get_preferred_size(minimum, natural);
      auto layoutSize = self->getLayoutSize(base::Size(minimum.width, minimum.height));

      if (_fixed_height >= 0)
        layoutSize.height = _fixed_height;
      if (_fixed_width >= 0)
        layoutSize.width = _fixed_width;
      _darea->set_size_request(layoutSize.width, layoutSize.height);


      mforms::gtk::draw_event_slot(context, _darea);
      double x1, y1, x2, y2;
      context->get_clip_extents(x1, y1, x2, y2);

      self->repaint(context->cobj(), x1, y1, x2 - x1, y2 - y1);

      //  Cairo::RefPtr<Cairo::Context> context(_darea->get_window()->create_cairo_context());
      //    self->repaint(context->cobj(), event->area.x, event->area.y, event->area.width, event->area.height);

      return true;
    }

    bool DrawBoxImpl::mouse_button_event(GdkEventButton *event, ::mforms::DrawBox *self) {
      mforms::MouseButton mbtn;
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
        default:
          mbtn = MouseButtonNone;
          logError("Unrecognised mouse button pressed");
          break;
      }

      if (event->type == GDK_BUTTON_PRESS) {
        // if there is some widget inside darea then we need to grab focus, so if there will be entry box,
        // then the focus can be moved
        if (_fixed) {
          _darea->grab_focus();
        }
        _last_btn = mbtn;
        return self->mouse_down(mbtn, (int)event->x, (int)event->y);
      } else if (event->type == GDK_BUTTON_RELEASE) {
        _last_btn = MouseButtonNone;
        // We must have click before up, because thet's how it's made on the other platforms.
        self->mouse_click(mbtn, (int)event->x, (int)event->y);
        self->mouse_up(mbtn, (int)event->x, (int)event->y);

        // We return false because otherwise when we do drag drop with Gtk::main::run wb will not be blocked and there
        // will be problem.
        return false;
      } else if (event->type == GDK_2BUTTON_PRESS)
        return self->mouse_double_click(mbtn, (int)event->x, (int)event->y);

      return false;
    }

    bool DrawBoxImpl::mouse_move_event(GdkEventMotion *event, ::mforms::DrawBox *self) {
      _mousePos.x = event->x;
      _mousePos.y = event->y;
      return self->mouse_move(_last_btn, (int)event->x, (int)event->y);
    }

    bool DrawBoxImpl::create(::mforms::DrawBox *self) {
      return new DrawBoxImpl(self) != 0;
    }

    void DrawBoxImpl::set_needs_repaint(::mforms::DrawBox *self) {
      // request a repaint so that this can be called from any thread
      DrawBoxImpl *impl = self->get_data<DrawBoxImpl>();

      mforms::Utilities::perform_from_main_thread(std::bind(&DrawBoxImpl::on_repaint, impl), false);
    }
    void DrawBoxImpl::add(::mforms::View *view, mforms::Alignment alignment) {
      if (!_fixed) {
        _fixed = Gtk::manage(new Gtk::Fixed);
        _darea->add(*_fixed);
        _darea->set_can_focus(true);
        _fixed->show();
      }
      std::map<Gtk::Widget *, AlignControl>::iterator it;
      it = _alignments.find(mforms::widget_for_view(view));
      if (it == _alignments.end()) {
        _fixed->add(*mforms::widget_for_view(view));

        AlignControl align;
        align._align = alignment;
        align._x = 0;
        align._y = 0;
        _alignments.insert(std::pair<Gtk::Widget *, AlignControl>(mforms::widget_for_view(view), align));
      }
    }
    //------------------------------------------------------------------------------
    void DrawBoxImpl::remove(::mforms::View *view) {
      if (_fixed) {
        std::map<Gtk::Widget *, AlignControl>::iterator it;
        it = _alignments.find(mforms::widget_for_view(view));
        if (it != _alignments.end()) {
          _fixed->remove(*mforms::widget_for_view(view));
          _alignments.erase(it);
        }
      }
    }
    //------------------------------------------------------------------------------
    void DrawBoxImpl::move(::mforms::View *view, int x, int y) {
      if (_fixed) {
        std::map<Gtk::Widget *, AlignControl>::iterator it;
        it = _alignments.find(mforms::widget_for_view(view));
        if (it != _alignments.end()) {
          it->second._align = mforms::NoAlign;
          it->second._x = 0;
          it->second._y = 0;
          _fixed->move(*mforms::widget_for_view(view), x, y);
        }
      }
    }
    //------------------------------------------------------------------------------
    void DrawBoxImpl::add(::mforms::DrawBox *self, ::mforms::View *view, mforms::Alignment alignment) {
      DrawBoxImpl *impl = self->get_data<DrawBoxImpl>();
      impl->add(view, alignment);
    }
    //------------------------------------------------------------------------------
    void DrawBoxImpl::remove(::mforms::DrawBox *self, ::mforms::View *view) {
      DrawBoxImpl *impl = self->get_data<DrawBoxImpl>();
      impl->remove(view);
    }
    //------------------------------------------------------------------------------
    void DrawBoxImpl::move(::mforms::DrawBox *self, ::mforms::View *view, int x, int y) {
      DrawBoxImpl *impl = self->get_data<DrawBoxImpl>();
      impl->move(view, x, y);
    }
    void DrawBoxImpl::set_padding_impl(int left, int top, int right, int bottom) {
      _padding._left = left;
      _padding._right = right;
      _padding._top = top;
      _padding._bottom = bottom;
    }

    //------------------------------------------------------------------------------

    void DrawBoxImpl::drawFocus(::mforms::DrawBox *self, cairo_t *cr, const base::Rect r) {
      auto bounds = r;
      bounds.use_inter_pixel = true;
      cairo_set_source_rgba(cr, 0.0, 0.6, 1.0, 1.0);
      cairo_rectangle(cr, bounds.left(), bounds.top(), bounds.width() - 2, bounds.height() - 2);
      cairo_set_line_width(cr, 1);
      cairo_stroke(cr);
    }

    void DrawBoxImpl::init() {
      ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

      f->_drawbox_impl.create = &DrawBoxImpl::create;
      f->_drawbox_impl.set_needs_repaint = &DrawBoxImpl::set_needs_repaint;
      f->_drawbox_impl.add = &DrawBoxImpl::add;
      f->_drawbox_impl.move = &DrawBoxImpl::move;
      f->_drawbox_impl.remove = &DrawBoxImpl::remove;
      f->_drawbox_impl.drawFocus = &DrawBoxImpl::drawFocus;
    }
  };
};
