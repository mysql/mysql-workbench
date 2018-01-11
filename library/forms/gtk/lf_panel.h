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

#ifndef _LF_PANEL_H_
#define _LF_PANEL_H_

#include "mforms/panel.h"
#include "base/drawing.h"

#include "lfi_bin.h"

namespace mforms {
  namespace gtk {

    class PanelImpl : public ViewImpl, public BinImpl {
      Gtk::Frame *_frame;
      Gtk::EventBox *_evbox;
      Gtk::CheckButton *_title_check;
      Glib::RefPtr<Gdk::Pixbuf> _back_image;

      Gtk::RadioButtonGroup _radio_group;
      bool _radio_group_set;

      virtual Gtk::Widget *get_outer() const {
        if (_frame)
          return _frame;
        else
          return _evbox;
      }
      virtual Gtk::Widget *get_inner() const {
        return _evbox;
      }

    protected:
      PanelImpl(::mforms::Panel *self, ::mforms::PanelType type);
      static bool create(::mforms::Panel *self, ::mforms::PanelType type);
      static void set_title(::mforms::Panel *self, const std::string &title);
      static void set_active(::mforms::Panel *self, bool flag);
      static bool get_active(::mforms::Panel *self);
      static void set_back_color(::mforms::Panel *self, const std::string &color);
      static void add(::mforms::Panel *self, ::mforms::View *child);
      static void remove(::mforms::Panel *self, ::mforms::View *child);

      virtual void set_padding_impl(int left, int top, int right, int bottom);

    public:
      static void init();
      void add_to_radio_group(Gtk::RadioButton *radio); // called by radiobutton to add itself to a radio group
      ~PanelImpl();
    };
  };
};

#endif
