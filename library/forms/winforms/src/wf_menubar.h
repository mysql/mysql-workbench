/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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

  public
    class MenuBarWrapper : public ObjectWrapper {
    protected:
      MenuBarWrapper(mforms::MenuBase *backend);

      // Only one of those can be called to create a native menu.
      static bool create_menu_bar(mforms::MenuBar *backend);
      static bool create_context_menu(mforms::ContextMenu *backend);

      static bool create_menu_item(mforms::MenuItem *item, const std::string &title, const mforms::MenuItemType type);
      static void set_title(mforms::MenuItem *item, const std::string &title);
      static std::string get_title(mforms::MenuItem *item);
      static void set_shortcut(mforms::MenuItem *item, const std::string &value);
      static void set_enabled(mforms::MenuBase *item, bool state);
      static bool get_enabled(mforms::MenuBase *item);
      static void set_checked(mforms::MenuItem *item, bool state);
      static bool get_checked(mforms::MenuItem *item);

      static void insert_item(mforms::MenuBase *menu, int index, mforms::MenuItem *item);
      static void remove_item(mforms::MenuBase *menu, mforms::MenuItem *item); // NULL item to remove all

      static void popup_at(mforms::ContextMenu *menu, mforms::View *owner, base::Point location);

    public:
      static void init();
    };
  };
};
