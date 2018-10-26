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

#include "mforms/mforms.h"

using namespace mforms;

Menu::Menu() {
  _menu_impl = &ControlFactory::get_instance()->_menu_impl;

  _menu_impl->create(this);
}

bool Menu::empty() const {
  return _item_map.size() == 0;
}

void Menu::remove_item(int i) {
  _menu_impl->remove_item(this, i);

  std::string to_remove;

  for (std::map<const std::string, int>::iterator index = _item_map.begin(); index != _item_map.end(); index++) {
    if ((*index).second == i)
      to_remove = (*index).first;
    else if ((*index).second > i)
      (*index).second--;
  }

  if (!to_remove.empty())
    _item_map.erase(to_remove);
}

int Menu::add_item(const std::string &caption, const std::string &action) {
  int item_index = _menu_impl->add_item(this, caption, action);

  _item_map[action] = item_index;

  return item_index;
}

int Menu::add_separator() {
  return _menu_impl->add_separator(this);
}

int Menu::add_submenu(const std::string &caption, Menu *submenu) {
  // Connect our action handler to the sub menu so all commands can be triggered via the top
  // menu in the hierarchy.
  submenu->signal_on_action()->connect(std::bind(&Menu::handle_action, this, std::placeholders::_1));
  return _menu_impl->add_submenu(this, caption, submenu);
}

//--------------------------------------------------------------------------------------------------

/**
 * Adds menu entries for all the items in the given list.
 */
void Menu::add_items_from_list(const bec::MenuItemList &list) {
  for (bec::MenuItemList::const_iterator item = list.begin(); item != list.end(); ++item) {
    if (item->type == bec::MenuAction) {
      int i = add_item(item->caption, item->internalName);
      set_item_enabled(i, item->enabled);
    } else if (item->type == bec::MenuSeparator)
      add_separator();
    else if (item->type == bec::MenuCascade) {
      Menu *submenu = mforms::manage(new mforms::Menu());
      submenu->add_items_from_list(item->subitems);
      int i = add_submenu(item->caption, submenu);
      set_item_enabled(i, item->enabled);
    }
  }
}

//--------------------------------------------------------------------------------------------------

void Menu::set_item_enabled(int i, bool flag) {
  _menu_impl->set_item_enabled(this, i, flag);
}

void Menu::set_item_enabled(const std::string &action, bool flag) {
  int i = get_item_index(action);
  if (i < 0)
    throw std::invalid_argument("invalid menu action " + action);
  _menu_impl->set_item_enabled(this, i, flag);
}

void Menu::set_handler(const std::function<void(const std::string &)> &action_handler) {
  _action_handler = action_handler;
}

void Menu::popup_at(Object *control, int x, int y) {
#ifndef _MSC_VER
  _on_will_show(); // Popping up the menu will trigger the on_show event on Win.
#endif

  _menu_impl->popup_at(this, control, x, y);
}

void Menu::popup() {
#ifndef _MSC_VER
  _on_will_show(); // Popping up the menu will trigger the on_show event on Win.
#endif

  _menu_impl->popup_at(this, 0, 0, 0);
}

//--------------------------------------------------------------------------------------------------

void Menu::handle_action(const std::string &action) {
  if (_action_handler)
    _action_handler(action);
  _on_action(action);
}

//--------------------------------------------------------------------------------------------------

void mforms::Menu::clear() {
  _menu_impl->clear(this);
  _item_map.clear();
}

//--------------------------------------------------------------------------------------------------

int mforms::Menu::get_item_index(const std::string &action) {
  int item_index = -1;
  std::map<const std::string, int>::iterator location = _item_map.find(action);

  if (location != _item_map.end())
    item_index = _item_map[action];

  return item_index;
}

//--------------------------------------------------------------------------------------------------
