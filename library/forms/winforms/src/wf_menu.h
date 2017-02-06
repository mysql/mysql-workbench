/*
 * Copyright (c) 2010, 2017, Oracle and/or its affiliates. All rights reserved.
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
    class MenuWrapper : public ObjectWrapper {
    protected:
      MenuWrapper(mforms::Menu* backend);

      static bool create(mforms::Menu* backend);
      static void remove_item(mforms::Menu* backend, int i);
      static int add_item(mforms::Menu* backend, const std::string& caption, const std::string& action);
      static int add_separator(mforms::Menu* backend);
      static int add_submenu(mforms::Menu* backend, const std::string& caption, mforms::Menu* submenu);
      static void set_item_enabled(mforms::Menu* backend, int i, bool flag);
      static void popup_at(mforms::Menu* backend, mforms::Object* control, int x, int y);
      static void clear(mforms::Menu* backend);

    public:
      static void init();
    };
  };
};
