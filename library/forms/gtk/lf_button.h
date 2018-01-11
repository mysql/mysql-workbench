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
