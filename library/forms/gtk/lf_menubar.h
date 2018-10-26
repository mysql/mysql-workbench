/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MFORMS_LF_MENUBAR_H_
#define _MFORMS_LF_MENUBAR_H_

#include <mforms/menubar.h>
#include <glibmm/refptr.h>

namespace Gtk {
  class MenuBar;
  class AccelGroup;
}

namespace mforms {
  Gtk::MenuBar *widget_for_menubar(MenuBar *self);
  void on_add_menubar_to_window(MenuBar *menu, Gtk::Window *window);

  namespace gtk {
    void lf_menubar_init();

    struct MenuItemImpl {
      static bool create_menu_bar(MenuBar *item);
      static bool create_context_menu(ContextMenu *item);
      static bool create_menu_item(MenuItem *item, const std::string &, const MenuItemType type);
      static bool copy_menu_item(MenuItem *item, const MenuItem *other);
      static void set_title(MenuItem *item, const std::string &);
      static std::string get_title(MenuItem *item);
      static void set_shortcut(MenuItem *item, const std::string &);
      static void set_enabled(MenuBase *item, bool);
      static bool get_enabled(MenuBase *item);
      static void set_checked(MenuItem *item, bool);
      static bool get_checked(MenuItem *item);

      static void insert_item(MenuBase *menu, int index, MenuItem *item);
      static void remove_item(MenuBase *menu, MenuItem *item); // NULL item to remove all
      static void popup_menu(mforms::ContextMenu *menu, View *owner, base::Point location);
      
      static void set_name(MenuItem *item, const std::string &);
      static std::string get_name(MenuItem *item);
    };

  }; // namespace gtk
} // namespace mforms

#endif
