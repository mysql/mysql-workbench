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

#ifndef _STUB_SCROLL_PANEL_H_
#define _STUB_SCROLL_PANEL_H_

#include "stub_container.h"

namespace mforms {
  namespace stub {

    class ScrollPanelWrapper : public ContainerWrapper {
    protected:
      ScrollPanelWrapper(::mforms::ScrollPanel *self, bool bordered) : ContainerWrapper(self) {
      }

      static bool create(::mforms::ScrollPanel *self, ::mforms::ScrollPanelFlags flags) {
        return true;
      }

      static void add(::mforms::ScrollPanel *self, ::mforms::View *child) {
      }

      static void remove(::mforms::ScrollPanel *self) {
      }

      static void set_visible_scrollers(::mforms::ScrollPanel *self, bool vertical, bool horizontal) {
      }

      static void set_autohide_scrollers(::mforms::ScrollPanel *self, bool flag) {
      }

      static void scroll_to_view(ScrollPanel *, View *) {
      }

    public:
      static void init() {
        ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

        f->_spanel_impl.create = &ScrollPanelWrapper::create;

        f->_spanel_impl.add = &ScrollPanelWrapper::add;
        f->_spanel_impl.remove = &ScrollPanelWrapper::remove;
        f->_spanel_impl.set_visible_scrollers = &ScrollPanelWrapper::set_visible_scrollers;
        f->_spanel_impl.set_autohide_scrollers = &ScrollPanelWrapper::set_autohide_scrollers;
        f->_spanel_impl.scroll_to_view = &ScrollPanelWrapper::scroll_to_view;
      }
    };
  };
};

#endif
