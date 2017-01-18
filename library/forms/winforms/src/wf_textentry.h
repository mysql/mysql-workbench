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

namespace MySQL {
  namespace Forms {

  public
    class TextEntryWrapper : public ViewWrapper {
    protected:
      TextEntryWrapper(mforms::TextEntry *text);

      static bool create(mforms::TextEntry *backend, mforms::TextEntryType type);
      static void set_text(mforms::TextEntry *backend, const std::string &text);
      static void set_placeholder_text(mforms::TextEntry *backend, const std::string &text);
      static void set_placeholder_color(mforms::TextEntry *backend, const std::string &color);
      static std::string get_text(mforms::TextEntry *backend);
      static void set_max_length(mforms::TextEntry *backend, int length);
      static void set_read_only(mforms::TextEntry *backend, bool flag);
      static void set_bordered(mforms::TextEntry *backend, bool flag);

      static void cut(mforms::TextEntry *self);
      static void copy(mforms::TextEntry *self);
      static void paste(mforms::TextEntry *self);
      static void select(mforms::TextEntry *self, const base::Range &range);
      static base::Range get_selection(mforms::TextEntry *self);

      virtual void set_front_color(String ^ color);

    public:
      static void init();
    };
  };
};
