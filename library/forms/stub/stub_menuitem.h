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
#ifndef _STUB_MENUITEM_H_
#define _STUB_MENUITEM_H_

#include "stub_base.h"

namespace mforms {
  namespace stub {

    class MenuItemWrapper : public ObjectWrapper {
    protected:
      MenuItemWrapper(::mforms::MenuItem *self) : ObjectWrapper(self) {
      }

      static bool create_menu_bar(MenuBar *item) {
        return true;
      }

      static bool create_menu_item(MenuItem *item, const std::string &, const MenuItemType type) {
        return true;
      }

      static void set_title(MenuItem *item, const std::string &) {
      }

      static std::string get_title(MenuItem *item) {
        return "";
      }

      static void set_shortcut(MenuItem *item, const std::string &) {
      }

      static void set_enabled(MenuBase *item, bool) {
      }

      static bool get_enabled(MenuBase *item) {
        return true;
      }

      static void set_checked(MenuItem *item, bool) {
      }

      static bool get_checked(MenuItem *item) {
        return true;
      }

      static void insert_item(MenuBase *menu, int index, MenuItem *item) {
      }

      static void remove_item(MenuBase *menu, MenuItem *item) {
      }

      static bool create_context_menu(mforms::ContextMenu *item) {
        return false;
      }

    public:
      static void init() {
        ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

        f->_menu_item_impl.create_menu_bar = &MenuItemWrapper::create_menu_bar;
        f->_menu_item_impl.create_menu_item = &MenuItemWrapper::create_menu_item;
        f->_menu_item_impl.get_checked = &MenuItemWrapper::get_checked;
        f->_menu_item_impl.get_enabled = &MenuItemWrapper::get_enabled;
        f->_menu_item_impl.get_title = &MenuItemWrapper::get_title;
        f->_menu_item_impl.insert_item = &MenuItemWrapper::insert_item;
        f->_menu_item_impl.remove_item = &MenuItemWrapper::remove_item;
        f->_menu_item_impl.set_checked = &MenuItemWrapper::set_checked;
        f->_menu_item_impl.set_enabled = &MenuItemWrapper::set_enabled;
        f->_menu_item_impl.set_shortcut = &MenuItemWrapper::set_shortcut;
        f->_menu_item_impl.set_title = &MenuItemWrapper::set_title;
        f->_menu_item_impl.create_context_menu = &MenuItemWrapper::create_context_menu;
      }
    };
  }
}

#endif
