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
