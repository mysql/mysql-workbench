/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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
