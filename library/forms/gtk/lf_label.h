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

#ifndef _LF_LABEL_H_
#define _LF_LABEL_H_

#include "mforms/label.h"

#include "lf_view.h"

namespace mforms {
  namespace gtk {

    class LabelImpl : public ViewImpl {
      Gtk::Label *_label;
      mforms::LabelStyle _style;
      bool _font_set;
      Pango::FontDescription _font;

    protected:
      virtual Gtk::Widget *get_outer() const {
        return _label;
      }

      LabelImpl(::mforms::Label *self);
      static bool create(::mforms::Label *self);
      static void set_style(::mforms::Label *self, ::mforms::LabelStyle style);
      static void set_text(::mforms::Label *self, const std::string &text);
      static void set_color(::mforms::Label *self, const std::string &text);
      static void set_wrap_text(::mforms::Label *self, bool flag);
      static void set_text_align(::mforms::Label *self, ::mforms::Alignment align);

      void realized();

    public:
      static void init();
    };
  };
};

#endif
