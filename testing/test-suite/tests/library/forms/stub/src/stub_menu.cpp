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

#include "../stub_menu.h"

namespace mforms {
  namespace stub {

    //------------------------------------------------------------------------------
    MenuWrapper::MenuWrapper(Menu* self) : ObjectWrapper(self) {
    }

    //------------------------------------------------------------------------------
    bool MenuWrapper::create(Menu* self) {
      return true;
    }

    //------------------------------------------------------------------------------
    void MenuWrapper::remove_item(Menu* self, int i) {
    }

    //------------------------------------------------------------------------------
    int MenuWrapper::add_item(Menu* self, const std::string& caption, const std::string& action) {
      return 0;
    }

    //------------------------------------------------------------------------------
    int MenuWrapper::add_separator(Menu* self) {
      return 0;
    }

    //------------------------------------------------------------------------------
    int MenuWrapper::add_submenu(Menu* self, const std::string& caption, Menu* submenu) {
      return 0;
    }

    //------------------------------------------------------------------------------
    void MenuWrapper::set_item_enabled(Menu* self, int i, bool flag) {
    }

    //------------------------------------------------------------------------------
    void MenuWrapper::popup_at(Menu* self, Object* control, int x, int y) {
    }

    void MenuWrapper::clear(Menu* self) {
    }

    //------------------------------------------------------------------------------
    void MenuWrapper::init() {
      ::mforms::ControlFactory* f = ::mforms::ControlFactory::get_instance();

      f->_menu_impl.create = &MenuWrapper::create;
      f->_menu_impl.remove_item = &MenuWrapper::remove_item;
      f->_menu_impl.add_item = &MenuWrapper::add_item;
      f->_menu_impl.add_separator = &MenuWrapper::add_separator;
      f->_menu_impl.add_submenu = &MenuWrapper::add_submenu;
      f->_menu_impl.set_item_enabled = &MenuWrapper::set_item_enabled;
      f->_menu_impl.popup_at = &MenuWrapper::popup_at;
      f->_menu_impl.clear = &MenuWrapper::clear;
    }
  }
}