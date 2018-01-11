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
