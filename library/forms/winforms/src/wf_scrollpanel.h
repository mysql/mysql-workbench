/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

    ref class ScrollFillPanel;

  public
    class ScrollPanelWrapper : public ViewWrapper {
    private:
      gcroot<ScrollFillPanel ^> container; // Can differ from the actual control wrapped here.
    protected:
      ScrollPanelWrapper(mforms::ScrollPanel *backend);

      static bool create(mforms::ScrollPanel *backend, mforms::ScrollPanelFlags flags);
      static void add(mforms::ScrollPanel *backend, mforms::View *view);
      static void remove(mforms::ScrollPanel *backend);
      static void set_autohide_scrollers(mforms::ScrollPanel *backend, bool);
      static void set_visible_scrollers(mforms::ScrollPanel *backend, bool, bool);
      static void scroll_to_view(mforms::ScrollPanel *backend, mforms::View *view);
      static base::Rect get_content_rect(mforms::ScrollPanel *backend);
      static void scroll_to(mforms::ScrollPanel *backend, int x, int y);

    public:
      static void init();
    };
  };
};
