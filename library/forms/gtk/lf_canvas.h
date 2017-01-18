/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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
#ifndef _LF_CANVAS_H_
#define _LF_CANVAS_H_

#include "mforms/canvas.h"

#include "lf_view.h"

#include "gtk/mdc_gtk_canvas_view.h"
#include "gtk/mdc_gtk_canvas_scroller.h"

namespace mforms {
  namespace gtk {

    class CanvasImpl : public ViewImpl {
      mdc::GtkCanvasScroller *_scroller;
      mdc::GtkCanvas *_canvas;

    protected:
      virtual Gtk::Widget *get_outer() const {
        return _scroller;
      }

      virtual Gtk::Widget *get_inner() const {
        return _canvas;
      }

      CanvasImpl(::mforms::Canvas *self) : ViewImpl(self) {
        _scroller = Gtk::manage(new mdc::GtkCanvasScroller());
        _canvas = Gtk::manage(new mdc::GtkCanvas(mdc::GtkCanvas::BufferedXlibCanvasType));
        _scroller->add(*_canvas);
        _scroller->show_all();
      }

      static bool create(::mforms::Canvas *self) {
        return new CanvasImpl(self) != 0;
      }

      static mdc::CanvasView *canvas(::mforms::Canvas *self) {
        CanvasImpl *impl = self->get_data<CanvasImpl>();
        if (impl)
          return impl->_canvas->get_canvas();
        return NULL;
      }

    public:
      static void init() {
        ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

        f->_canvas_impl.create = &CanvasImpl::create;
        f->_canvas_impl.canvas = &CanvasImpl::canvas;
      }
    };
  };
};

#endif
