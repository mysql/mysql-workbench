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

#pragma once

#include "base/trackable.h"
#include <mforms/box.h>
#include <mforms/app.h>
#include <vector>

namespace mforms {
  class MenuBase;
  class MenuItem;
  class MenuBar;
  class View;
  class ContextMenu;

  enum MenuItemType { NormalMenuItem = 0, CheckedMenuItem, SeparatorMenuItem };

#ifndef SWIG
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  // This merges functionality of menus and menu items.
  struct MFORMS_EXPORT MenuItemImplPtrs {
    bool (*create_menu_bar)(MenuBar *item);
    bool (*create_context_menu)(ContextMenu *item);

    bool (*create_menu_item)(MenuItem *item, const std::string &, const MenuItemType type);
    void (*set_title)(MenuItem *item, const std::string &);
    std::string (*get_title)(MenuItem *item);
    void (*set_name)(MenuItem *item, const std::string &);
    void (*set_shortcut)(MenuItem *item, const std::string &);
    void (*set_enabled)(MenuBase *item, bool);
    bool (*get_enabled)(MenuBase *item);
    void (*set_checked)(MenuItem *item, bool);
    bool (*get_checked)(MenuItem *item);

    void (*insert_item)(MenuBase *menu, int index, MenuItem *item);
    void (*remove_item)(MenuBase *menu, MenuItem *item); // NULL item to remove all

    void (*popup_at)(ContextMenu *menu, View *owner, base::Point location);
  };
#endif
#endif

  // base abstract class for menuitem and menubase
  class MFORMS_EXPORT MenuBase : public Object, public base::trackable {
  protected:
    MenuBase();

    MenuItemImplPtrs *_impl;
    MenuBase *_parent;
    std::vector<MenuItem *> _items;

  public:
    virtual ~MenuBase();

#ifndef SWIG
    std::vector<MenuItem *> &get_subitems() {
      return _items;
    }
#endif

    MenuItem *find_item(const std::string &name);
    MenuItem *get_item(int i);
    int get_item_index(MenuItem *item);
    int item_count();

#ifndef SWIG
    MenuItem *add_item_with_title(const std::string &title, std::function<void()> action, const std::string &name, const std::string &internalName);
    MenuItem *add_check_item_with_title(const std::string &title, std::function<void()> action,
                                        const std::string &name, const std::string &internalName);
#endif
    MenuItem *add_separator();

    void add_item(MenuItem *item);
    void add_submenu(MenuItem *item) {
      add_item(item);
    }

    void insert_item(int index, MenuItem *item);
    void remove_all();
    void remove_item(MenuItem *item);
    void set_enabled(bool flag);
    bool get_enabled();

    virtual void validate();

    MenuBase *get_parent() {
      return _parent;
    }
    MenuBase *get_top_menu();
  };

  /** A menu item that can be added to the host application menus.
   */
  class MFORMS_EXPORT MenuItem : public MenuBase {
  public:
    typedef std::function<bool()> validator_function;

    /** Constructor

     @param title - passing an empty title in the constructor will make the item be a separator
     */
    MenuItem(const std::string &title, const MenuItemType type = NormalMenuItem);

    void set_title(const std::string &title);
    std::string get_title();

    void set_name(const std::string &name);

    void set_shortcut(const std::string &shortcut);
    std::string get_shortcut() {
      return _shortcut;
    }

    void set_checked(bool flag);
    bool get_checked();

#ifndef SWIG
    boost::signals2::signal<void()> *signal_clicked() {
      return &_clicked_signal;
    }
#endif
    void setInternalName(const std::string &name) {
        _internalName = name;
    }
    std::string getInternalName() {
      return _internalName;
    }

    MenuItemType get_type() {
      return _type;
    }

    void add_validator(const validator_function &slot);
    void clear_validators();

  public:
    void callback();
    virtual void validate();

  private:
    std::string _internalName;
    std::string _shortcut;
    std::vector<validator_function> _validators;
    boost::signals2::signal<void()> _clicked_signal;
    MenuItemType _type;
  };

  /** A menu that can be added to the host application.
   */
  class MFORMS_EXPORT MenuBar : public MenuBase {
  public:
    MenuBar();

#ifndef SWIG
    boost::signals2::signal<void(MenuItem *)> *signal_will_show() {
      return &_signal_will_show;
    }
#endif
    void set_item_enabled(const std::string &item_name, bool flag);
    void set_item_checked(const std::string &item_name, bool flag);

    void will_show_submenu_from(MenuItem *item);

  private:
    boost::signals2::signal<void(MenuItem *)> _signal_will_show;
  };

  /** A menu that can be attached to other controls
   */
  class MFORMS_EXPORT ContextMenu : public MenuBase {
  public:
    ContextMenu();

#ifndef SWIG
    boost::signals2::signal<void(MenuItem *)> *signal_will_show() {
      return &_signal_will_show;
    }
#endif
    void set_item_enabled(const std::string &item_name, bool flag);
    void set_item_checked(const std::string &item_name, bool flag);

    void will_show();
    void will_show_submenu_from(MenuItem *item);

    void popup_at(View *owner, base::Point location);

  private:
    boost::signals2::signal<void(MenuItem *)> _signal_will_show;
  };
};
