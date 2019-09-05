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

#ifndef _STUB_MENU_H_
#define _STUB_MENU_H_

#include "stub_base.h"

namespace mforms {
  namespace stub {
    class MenuWrapper : public ObjectWrapper {
    public:
      MenuWrapper(Menu* self);

      static bool create(Menu* self);
      static void remove_item(Menu* self, int i);
      static int add_item(Menu* self, const std::string& caption, const std::string& action);
      static int add_separator(Menu* self);
      static int add_submenu(Menu* self, const std::string& caption, Menu* submenu);
      static void set_item_enabled(Menu* self, int i, bool flag);
      static void popup_at(Menu* self, Object* control, int x, int y);
      static void clear(Menu* self);

    public:
      static void init();
    };
  };
};

#endif
