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
#ifndef _STUB_BUTTON_H_
#define _STUB_BUTTON_H_

#include "stub_view.h"

namespace mforms {
  namespace stub {

    class ButtonWrapper : public ViewWrapper {
    protected:
      ButtonWrapper(::mforms::Button *self, bool concrete = false) : ViewWrapper(self) {
      }

      static void callback(::mforms::Button *self) {
      }

      static bool create(::mforms::Button *self, ::mforms::ButtonType) {
        return true;
      }

      static void set_text(::mforms::Button *self, const std::string &text) {
      }

      static void enable_internal_padding(Button *self, bool enabled) {
      }

      static void set_icon(Button *self, const std::string &path) {
      }

    public:
      static void init() {
        ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

        f->_button_impl.create = &ButtonWrapper::create;
        f->_button_impl.set_text = &ButtonWrapper::set_text;
        f->_button_impl.set_icon = &ButtonWrapper::set_icon;
        f->_button_impl.enable_internal_padding = &ButtonWrapper::enable_internal_padding;
      }
    };
  }
}

#endif
