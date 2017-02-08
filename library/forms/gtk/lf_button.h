/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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
#ifndef _LF_BUTTON_H_
#define _LF_BUTTON_H_

#include "mforms/button.h"

#include "lf_view.h"

namespace mforms {
  namespace gtk {

    class ButtonImpl : public ViewImpl {
      Gtk::Box *_holder;

    protected:
      Gtk::Label *_label;
      Gtk::Button *_button;
      Gtk::Image *_icon;

      virtual Gtk::Widget *get_outer() const {
        return _button;
      }

      ButtonImpl(::mforms::Button *self, ::mforms::ButtonType btype = ::mforms::PushButton, bool concrete = false);
      static void callback(::mforms::Button *self);
      static bool create(::mforms::Button *self, ::mforms::ButtonType btype);
      static void set_text(::mforms::Button *self, const std::string &text);
      static void set_icon(::mforms::Button *self, const std::string &path);
      static void enable_internal_padding(Button *self, bool enabled);

      virtual void set_text(const std::string &text);

    public:
      static void init();
    };
  }
}

#endif
