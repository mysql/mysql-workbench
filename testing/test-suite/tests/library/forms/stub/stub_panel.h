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

#ifndef _STUB_PANEL_H_
#define _STUB_PANEL_H_

#include "stub_container.h"

namespace mforms {
  namespace stub {

    class PanelWrapper : public ContainerWrapper {
    protected:
      PanelWrapper(::mforms::Panel *self, ::mforms::PanelType type) : ContainerWrapper(self) {
      }

      static bool create(::mforms::Panel *self, ::mforms::PanelType type) {
        return true;
      }

      static void set_title(::mforms::Panel *self, const std::string &title) {
      }

      static void set_active(::mforms::Panel *self, bool flag) {
      }

      static bool get_active(::mforms::Panel *self) {
        return false;
      }

      static void set_back_color(::mforms::Panel *self, const std::string &color) {
      }

      static void add(::mforms::Panel *self, ::mforms::View *child) {
      }

      static void remove(::mforms::Panel *self, ::mforms::View *child) {
      }

    public:
      static void init() {
        ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

        f->_panel_impl.create = &PanelWrapper::create;
        f->_panel_impl.set_title = &PanelWrapper::set_title;
        f->_panel_impl.set_back_color = &PanelWrapper::set_back_color;

        f->_panel_impl.add = &PanelWrapper::add;
        f->_panel_impl.remove = &PanelWrapper::remove;

        f->_panel_impl.set_active = &PanelWrapper::set_active;
        f->_panel_impl.get_active = &PanelWrapper::get_active;
      }
    };
  };
};

#endif
