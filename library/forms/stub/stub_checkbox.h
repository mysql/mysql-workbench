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
#ifndef _STUB_CHECKBOX_H_
#define _STUB_CHECKBOX_H_

#include "stub_button.h"

namespace mforms {
  namespace stub {

    class CheckBoxWrapper : public ButtonWrapper {
    protected:
      static bool create(::mforms::CheckBox *self, bool square) {
        return true;
      }

      static void set_active(::mforms::CheckBox *self, bool flag) {
      }

      static bool get_active(::mforms::CheckBox *self) {
        return false;
      }

      CheckBoxWrapper(::mforms::CheckBox *self) : ButtonWrapper(self) {
      }

      static void callback(::mforms::CheckBox *self) {
      }

    public:
      static void init() {
        ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

        f->_checkbox_impl.create = &CheckBoxWrapper::create;
        f->_checkbox_impl.set_active = &CheckBoxWrapper::set_active;
        f->_checkbox_impl.get_active = &CheckBoxWrapper::get_active;
      }
    };
  };
};

#endif
