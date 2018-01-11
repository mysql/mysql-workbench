/*
 * Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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
