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

#pragma once

#include <mforms/base.h>
#include <mforms/button.h>

namespace mforms {
  class CheckBox;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct CheckBoxImplPtrs {
    bool (*create)(CheckBox *self, bool square);
    void (*set_active)(CheckBox *self, bool flag);
    bool (*get_active)(CheckBox *self);
  };
#endif
#endif

  /** A checkbox toggle control.
   */
  class MFORMS_EXPORT CheckBox : public Button {
  public:
    CheckBox(bool square = false);

    /** Sets state of checkbox. */
    void set_active(bool flag);

    /** Gets the state of the checkbox */
    bool get_active();

    virtual int get_int_value() {
      return get_active() ? 1 : 0;
    }
    virtual bool get_bool_value() {
      return get_active();
    }
    virtual std::string get_string_value() {
      return get_active() ? "1" : "0";
    }

  protected:
    CheckBoxImplPtrs *_checkbox_impl;
  };
};
