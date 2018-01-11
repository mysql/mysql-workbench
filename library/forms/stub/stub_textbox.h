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

#ifndef _STUB_TEXTBOX_H_
#define _STUB_TEXTBOX_H_

#include "stub_view.h"

namespace mforms {
  namespace stub {

    class TextBoxWrapper : public ViewWrapper {
      TextBoxWrapper(::mforms::TextBox *self, mforms::ScrollBars scroll_type);

      static bool create(::mforms::TextBox *self, mforms::ScrollBars scroll_type);
      static void set_text(::mforms::TextBox *self, const std::string &text);
      static void append_text(::mforms::TextBox *self, const std::string &text, bool scroll_to_end);
      static std::string get_text(::mforms::TextBox *self);
      static void set_read_only(::mforms::TextBox *self, bool flag);
      static void set_padding(::mforms::TextBox *self, int pad);
      static void set_bordered(::mforms::TextBox *self, bool flag);
      static void clear(::mforms::TextBox *self);
      static void set_monospaced(::mforms::TextBox *self, bool flag);
      static void get_selected_range(TextBox *self, int &start, int &end);

    public:
      static void init();
    };
  };
};

#endif
