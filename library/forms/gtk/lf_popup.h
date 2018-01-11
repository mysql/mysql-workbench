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

#ifndef _LF_POPUP_H_
#define _LF_POPUP_H_

#include "mforms/popup.h"

#include "lf_base.h"
#include "main_app.h"

namespace mforms {
  namespace gtk {

    class PopupImpl : public ObjectImpl {
      enum { R = 20 };
      int _width;
      int _height;
      Gtk::Window _wnd;
      runtime::loop _loop;
      bool _have_rgba;
      bool _inside;
      int _result;
      mforms::PopupStyle _style;
      sigc::connection _idleClose;

      bool handle_draw_event(const ::Cairo::RefPtr< ::Cairo::Context> &context);
      bool mouse_cross_event(GdkEventCrossing *event);
      bool mouse_button_event(GdkEventButton *event);
      bool mouse_move_event(GdkEventMotion *event);
      bool key_press_event(GdkEventKey *event);
      void set_size(int, int);

      static bool create(::mforms::Popup *self, mforms::PopupStyle style);
      static void destroy(::mforms::Popup *self);
      static void set_needs_repaint(::mforms::Popup *self);
      static void set_size(::mforms::Popup *, int, int);
      static int show(::mforms::Popup *, int, int);
      static base::Rect get_content_rect(::mforms::Popup *);
      static void set_modal_result(Popup *, int result);

    public:
      PopupImpl(::mforms::Popup *self, mforms::PopupStyle style);
      ~PopupImpl();
      static void init();
    }; // end of PopupImpl

  } // end of namespace gtk
} // end of namespace mforms

#endif /* _LF_POPUP_H_ */
