/*
 * Copyright (c) 2010, 2017, Oracle and/or its affiliates. All rights reserved.
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
