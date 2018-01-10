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

#pragma once

#include <mforms/base.h>
#include <mforms/button.h>

namespace mforms {

  class RadioButton;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct RadioButtonImplPtrs {
    bool (*create)(RadioButton *self, int group_id);
    void (*set_active)(RadioButton *self, bool flag);
    bool (*get_active)(RadioButton *self);
  };
#endif
#endif

  /** A radio button that will unselect other buttons in the same group when selected. */
  class MFORMS_EXPORT RadioButton : public Button {
  protected:
    RadioButtonImplPtrs *_radio_impl;

    boost::signals2::signal<void()> _signal_toggled;

    int _group_id;

    void radio_activated(int group_id);

  public:
    /** Create a new unique group id */
    static int new_id();

    /** Constructor.

     The group_id defines the radiogroup this button belongs to.
     When a radio group is activated, all others in the same group will be deactivated.
     It must be unique in the application, you can use new_id() to create one.

     Note: on Windows exists a limitation due to an implicit handling of radio groups there.
           All radio buttons sharing the same window parent automatically belong to one group and hence only one
           of them can be active. This effectively prohibits more than one radio group on a single parent container.
     */
    RadioButton(int group_id);

    /** Returns group_id of this radio button */
    int group_id() const {
      return _group_id;
    }

    /** Sets state of the radio button */
    void set_active(bool flag);

    /** Gets state */
    bool get_active();

#ifndef SWIG
#ifndef DOXYGEN_SHOULD_SKIP_THIS
    virtual void callback();
#endif
#endif
  };
};
