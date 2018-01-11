/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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
