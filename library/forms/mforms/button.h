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

#include <mforms/view.h>

namespace mforms {
  class Button;

  enum ButtonType {
    PushButton,
    ToolButton,        // A button for a tool bar.
    AdminActionButton, // A push button that will trigger an action which requires admin privileges.
                       // This will add an indicator to the button (a shield on Windows).
    SmallButton
  };

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct ButtonImplPtrs {
    bool (*create)(Button *self, ButtonType btype);
    void (*set_text)(Button *self, const std::string &text);
    void (*set_icon)(Button *self, const std::string &path);
    void (*enable_internal_padding)(Button *self, bool);
  };
#endif
#endif

  /** A clickable button control.
   */
  class MFORMS_EXPORT Button : public View {
  public:
    Button(ButtonType type = PushButton);

    /** Sets whether an internal padding should be added to the button.

     Buttons are sized so that its content text will show up untruncated plus an additional
     space that is added to give them a better appearance. You can disable that extra space
     by calling this method with false.
     */
    void enable_internal_padding(bool flag);

    /** Sets the text to be shown in the button */
    void set_text(const std::string &text);

    /** Sets the path to the icon to be shown in the button, if its a ToolButton */
    void set_icon(const std::string &icon);

#ifndef SWIG
    /** Sets callback for when the button is clicked.

     In Python, use add_clicked_callback()
     */
    boost::signals2::signal<void()> *signal_clicked() {
      return &_clicked;
    }

  public:
    virtual void callback();

    bool is_updating() {
      return _updating;
    }
#endif
  protected:
    boost::signals2::signal<void()> _clicked;
    ButtonImplPtrs *_button_impl;
    bool _updating;
  };
};
