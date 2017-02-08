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
#ifndef _LF_SCROLL_PANEL_H_
#define _LF_SCROLL_PANEL_H_

#include "mforms/scrollpanel.h"

#include "lfi_bin.h"

namespace mforms {
  namespace gtk {

    class ScrollPanelImpl : public ViewImpl, public BinImpl {
      Gtk::ScrolledWindow *_swin;
      bool _vertical, _horizontal;
      bool _autohide;

      virtual Gtk::Widget *get_outer() const {
        return _swin;
      }
      virtual Gtk::Widget *get_inner() const {
        return _swin;
      }

    protected:
      ScrollPanelImpl(::mforms::ScrollPanel *self, mforms::ScrollPanelFlags flags);
      ~ScrollPanelImpl();

      static bool create(::mforms::ScrollPanel *self, mforms::ScrollPanelFlags flags);
      static void add(::mforms::ScrollPanel *self, ::mforms::View *child);
      static void remove(::mforms::ScrollPanel *self);
      static void set_visible_scrollers(::mforms::ScrollPanel *self, bool vertical, bool horizontal);
      static void set_autohide_scrollers(::mforms::ScrollPanel *self, bool flag);
      static void scroll_to_view(mforms::ScrollPanel *, mforms::View *);
      static base::Rect get_content_rect(mforms::ScrollPanel *);
      static void scroll_to(mforms::ScrollPanel *self, int x, int y);
      virtual void set_padding_impl(int left, int top, int right, int bottom);

    public:
      static void init();
    };
  };
};

#endif
