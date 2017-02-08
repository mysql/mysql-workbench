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
