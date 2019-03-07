/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _LF_TEXTENTRY_H_
#define _LF_TEXTENTRY_H_

#include "mforms/mforms.h"

#include "lf_view.h"

namespace mforms {
  namespace gtk {

    class TextEntryImpl : public ViewImpl {
      Gtk::Entry *_entry;
      Gdk::RGBA _text_color;
      Gdk::RGBA _placeholder_color;
      TextEntryType _type;
      bool _has_real_text;
      bool _changing_text;
      virtual Gtk::Widget *get_outer() const {
        return _entry;
      }

      TextEntryImpl(::mforms::TextEntry *self, TextEntryType type);
      static bool create(::mforms::TextEntry *self, TextEntryType type);
      static void set_text(::mforms::TextEntry *self, const std::string &text);
      static void set_placeholder_text(::mforms::TextEntry *self, const std::string &text);
      static void set_placeholder_color(::mforms::TextEntry *self, const std::string &color);
      static void set_max_length(::mforms::TextEntry *self, int len);
      static std::string get_text(::mforms::TextEntry *self);
      static void set_read_only(::mforms::TextEntry *self, bool flag);
      static void set_bordered(::mforms::TextEntry *self, bool flag);
      static void cut(::mforms::TextEntry *self);
      static void copy(::mforms::TextEntry *self);
      static void paste(::mforms::TextEntry *self);
      static void select(::mforms::TextEntry *self, const base::Range &range);
      static base::Range get_selection(::mforms::TextEntry *self);

      void activated(mforms::TextEntry *self);
      bool key_press(GdkEventKey *event, mforms::TextEntry *self);

      void icon_pressed(Gtk::EntryIconPosition pos, const GdkEventButton *ev);
      void set_placeholder_text(const std::string &text);
      void set_text(const std::string &text);
      void focus_in(GdkEventFocus *);
      void focus_out(GdkEventFocus *);
      void changed(mforms::TextEntry *);

    protected:
      void set_front_color(const std::string &color);
      virtual void set_back_color(const std::string &color);

    public:
      static void init();
    };
  };
};

#endif
