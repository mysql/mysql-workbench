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
#ifndef _STUB_TEXTENTRY_H_
#define _STUB_TEXTENTRY_H_

#include "stub_view.h"

namespace mforms {
  namespace stub {

    class TextEntryWrapper : public ViewWrapper {
      TextEntryWrapper(::mforms::TextEntry *self, TextEntryType type);
      static bool create(::mforms::TextEntry *self, TextEntryType type);
      static void set_text(::mforms::TextEntry *self, const std::string &text);
      static void set_max_length(::mforms::TextEntry *self, int len);
      static std::string get_text(::mforms::TextEntry *self);
      static void set_read_only(::mforms::TextEntry *self, bool flag);
      static void set_placeholder_text(TextEntry *self, const std::string &text);
      static void set_placeholder_color(TextEntry *self, const std::string &color);
      static void set_bordered(TextEntry *self, bool flag);

      void activated(mforms::TextEntry *self);

    public:
      static void init();
    };
  };
};

#endif
