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