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

#ifndef _STUB_TABVIEW_H_
#define _STUB_TABVIEW_H_

#include "stub_view.h"

namespace mforms {
  namespace stub {

    class TabViewWrapper : public ViewWrapper {
    protected:
      TabViewWrapper(::mforms::TabView *self, bool tabless) : ViewWrapper(self) {
      }

      static bool create(::mforms::TabView *self, ::mforms::TabViewType) {
        return true;
      }

      static void set_active_tab(::mforms::TabView *self, int index) {
      }

      static int get_active_tab(::mforms::TabView *self) {
        return -1;
      }

      static int add_page(::mforms::TabView *self, ::mforms::View *page, const std::string &caption,
                          bool hasCloseButton) {
        return 0;
      }

      static void set_tab_title(TabView *, int, const std::string &) {
      }

      static void remove_page(TabView *, View *) {
      }

      static void setAuxView(TabView *, View *) {

      }

      static void setAllowsReordering(TabView *, bool) {

      }

    public:
      static void init() {
        ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

        f->_tabview_impl.create = &TabViewWrapper::create;
        f->_tabview_impl.get_active_tab = &TabViewWrapper::get_active_tab;
        f->_tabview_impl.set_active_tab = &TabViewWrapper::set_active_tab;
        f->_tabview_impl.add_page = &TabViewWrapper::add_page;
        f->_tabview_impl.set_tab_title = &TabViewWrapper::set_tab_title;
        f->_tabview_impl.remove_page = &TabViewWrapper::remove_page;
        f->_tabview_impl.set_aux_view = &TabViewWrapper::setAuxView;
        f->_tabview_impl.set_allows_reordering = &TabViewWrapper::setAllowsReordering;
      }
    };
  };
};

#endif
