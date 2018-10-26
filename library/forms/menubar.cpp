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

#include "mforms/mforms.h"

using namespace mforms;

MenuBase::MenuBase() : _parent(0) {
  _impl = &mforms::ControlFactory::get_instance()->_menu_item_impl;
}

MenuBase::~MenuBase() {
  for (std::vector<MenuItem *>::iterator iter = _items.begin(); iter != _items.end(); ++iter) {
    (*iter)->release();
  }
  _items.clear();
}

MenuItem *MenuBase::add_item_with_title(const std::string &title, std::function<void()> action,
                                        const std::string &name, const std::string &internalName) {
  MenuItem *item = manage(new MenuItem(title));
  item->signal_clicked()->connect(action);
  add_item(item);
  item->set_name(name);
  item->setInternalName(internalName);
  return item;
}

MenuItem *MenuBase::add_check_item_with_title(const std::string &title, std::function<void()> action,
                                              const std::string &name, const std::string &internalName) {
  MenuItem *item = manage(new MenuItem(title, CheckedMenuItem));
  item->signal_clicked()->connect(action);
  add_item(item);
  item->set_name(name);
  item->setInternalName(internalName);
  return item;
}

MenuItem *MenuBase::add_separator() {
  MenuItem *item = manage(new MenuItem("", SeparatorMenuItem));
  add_item(item);
  return item;
}

void MenuBase::add_item(MenuItem *item) {
  insert_item(-1, item);
}

void MenuBase::insert_item(int index, MenuItem *item) {
  if (index < 0 || index > (int)_items.size())
    index = (int)_items.size();

  item->_parent = this;

  _impl->insert_item(this, index, item);
  _items.insert(_items.begin() + index, item);

  // item->retain();
}

void MenuBase::remove_all() {
  _impl->remove_item(this, NULL); // null means remove all
  std::vector<MenuItem *>::iterator iter;
  for (iter = _items.begin(); iter != _items.end(); ++iter) {
    (*iter)->_parent = 0;
    (*iter)->release();
  }
  _items.clear();
}

void MenuBase::remove_item(MenuItem *item) {
  std::vector<MenuItem *>::iterator iter = std::find(_items.begin(), _items.end(), item);
  if (iter != _items.end()) {
    (*iter)->_parent = 0;
    _impl->remove_item(this, item);
    item->release();
    _items.erase(iter);
  }
}

void MenuBase::set_enabled(bool flag) {
  _impl->set_enabled(this, flag);
}

bool MenuBase::get_enabled() {
  return _impl->get_enabled(this);
}

MenuItem *MenuBase::find_item(const std::string &name) {
  for (std::vector<MenuItem *>::const_iterator iter = _items.begin(); iter != _items.end(); ++iter) {
    if ((*iter)->getInternalName() == name)
      return *iter;
    MenuItem *item;
    if ((item = (*iter)->find_item(name)))
      return item;
  }
  return 0;
}

MenuItem *MenuBase::get_item(int i) {
  if (i < 0 || i >= (int)_items.size())
    return NULL;
  return _items[i];
}

int MenuBase::get_item_index(MenuItem *item) {
  std::vector<MenuItem *>::const_iterator it = std::find(_items.begin(), _items.end(), item);
  if (it == _items.end())
    return -1;
  return (int)(it - _items.begin());
}

int MenuBase::item_count() {
  return (int)_items.size();
}

void MenuBase::validate() {
  for (std::vector<MenuItem *>::const_iterator iter = _items.begin(); iter != _items.end(); ++iter)
    (*iter)->validate();
}

MenuBase *MenuBase::get_top_menu() {
  if (dynamic_cast<MenuBar *>(this) != NULL)
    return dynamic_cast<MenuBar *>(this);

  if (dynamic_cast<ContextMenu *>(this) != NULL)
    return dynamic_cast<ContextMenu *>(this);

  MenuBase *p = _parent;
  while (p && p->get_parent())
    p = p->get_parent();

  return p;
}

MenuItem::MenuItem(const std::string &title, const MenuItemType type) : MenuBase(), _type(type) {
  _impl->create_menu_item(this, title, type); // ! Warning there will be no checked menu with this!
}

void MenuItem::set_title(const std::string &title) {
  _impl->set_title(this, title);
}

std::string MenuItem::get_title() {
  return _impl->get_title(this);
}


void MenuItem::set_name(const std::string &name) {
  _impl->set_name(this, name);
}

void MenuItem::set_shortcut(const std::string &shortcut) {
  _shortcut = shortcut;
  _impl->set_shortcut(this, shortcut);
}

void MenuItem::set_checked(bool flag) {
  _impl->set_checked(this, flag);
}

bool MenuItem::get_checked() {
  return _impl->get_checked(this);
}

void MenuItem::callback() {
#if defined(_MSC_VER) || defined(__APPLE__)
  // toggle the state of checkbox items, so that the behaviour works the same as in linux
  if (_type == CheckedMenuItem)
    set_checked(!get_checked());
#endif
  _clicked_signal();
}

void MenuItem::validate() {
  bool result = true;
  for (validator_function val : _validators) {
    if (!val())
      result = false;
  }

  set_enabled(result);

  if (!_items.empty())
    MenuBase::validate();
}

void MenuItem::add_validator(const validator_function &slot) {
  _validators.push_back(slot);
}

void MenuItem::clear_validators() {
  _validators.clear();
}

MenuBar::MenuBar() : MenuBase() {
  _impl->create_menu_bar(this);
}

void MenuBar::will_show_submenu_from(MenuItem *item) {
  _signal_will_show(item);
}

void MenuBar::set_item_enabled(const std::string &item_name, bool flag) {
  MenuItem *item = find_item(item_name);
  if (item)
    item->set_enabled(flag);
}

void MenuBar::set_item_checked(const std::string &item_name, bool flag) {
  MenuItem *item = find_item(item_name);
  if (item)
    item->set_checked(flag);
}

ContextMenu::ContextMenu() : MenuBase() {
  _impl->create_context_menu(this);
}

void ContextMenu::set_item_enabled(const std::string &item_name, bool flag) {
  MenuItem *item = find_item(item_name);
  if (item)
    item->set_enabled(flag);
}

void ContextMenu::set_item_checked(const std::string &item_name, bool flag) {
  MenuItem *item = find_item(item_name);
  if (item)
    item->set_checked(flag);
}

void ContextMenu::will_show() {
  will_show_submenu_from(0);
}

void ContextMenu::will_show_submenu_from(MenuItem *item) {
  _signal_will_show(item);
}

void ContextMenu::popup_at(View *owner, base::Point location) {
  _impl->popup_at(this, owner, location);
}
