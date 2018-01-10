/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <map>
#include "boost/signals2.hpp"

#include "base/trackable.h"
#include "mforms/base.h"
#include "base/ui_form.h"

namespace mforms {
  class Menu;
}

namespace mforms {

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct MFORMS_EXPORT MenuImplPtrs {
    bool (*create)(Menu *self);

    void (*remove_item)(Menu *self, int i);
    int (*add_item)(Menu *self, const std::string &caption, const std::string &action);
    int (*add_separator)(Menu *self);
    int (*add_submenu)(Menu *self, const std::string &caption, Menu *submenu);
    void (*clear)(Menu *self);

    void (*set_item_enabled)(Menu *self, int i, bool flag);

    void (*popup_at)(Menu *self, Object *control, int x, int y);
  };
#endif
#endif

  class MFORMS_EXPORT Menu : public Object, public base::trackable {
  private:
    MenuImplPtrs *_menu_impl;
    std::function<void(const std::string &)> _action_handler;
    boost::signals2::signal<void()> _on_will_show;
    boost::signals2::signal<void(const std::string &)> _on_action;
    std::map<const std::string, int> _item_map;

  public:
    Menu();

    bool empty() const;
    void clear();
    void remove_item(int i);
    int add_item(const std::string &caption, const std::string &action);
    int add_separator();
    int add_submenu(const std::string &caption, Menu *submenu);
    void add_items_from_list(const bec::MenuItemList &list);

    void set_item_enabled(int i, bool flag);
    void set_item_enabled(const std::string &action, bool flag);
#ifndef SWIG
    void set_handler(const std::function<void(const std::string &)> &action_handler);
#endif
    void popup_at(Object *control, int x, int y);
    void popup();

    void handle_action(const std::string &action);

    int get_item_index(const std::string &action);
#ifndef SWIG
    boost::signals2::signal<void()> *signal_will_show() {
      return &_on_will_show;
    }
    boost::signals2::signal<void(const std::string &)> *signal_on_action() {
      return &_on_action;
    }
#endif
  };
};
