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

#include <gtkmm/menushell.h>

#include "../lf_mforms.h"
#include "../lf_menu.h"
#include "base/log.h"
#include "mforms.h"

DEFAULT_LOG_DOMAIN("Menu")

//------------------------------------------------------------------------------
mforms::gtk::MenuImpl::MenuImpl(mforms::Menu *self) : mforms::gtk::ObjectImpl(self) {

  auto parent = _menu.get_parent();
  if (parent) {
    Glib::RefPtr<Atk::Object> acc = parent->get_accessible();
    if (acc)
      acc->set_name("Context Menu");
  } else {
    logWarning("Unable to set context menu a11y name.\n");
  }

}

//------------------------------------------------------------------------------
Gtk::MenuItem *mforms::gtk::MenuImpl::item_at(const int i) {
  Gtk::MenuItem *item = 0;
  std::vector<Gtk::Widget *> items = _menu.get_children();
  if ((int)items.size() > i)
    item = dynamic_cast<Gtk::MenuItem *>(items[i]);

  return item;
}

//------------------------------------------------------------------------------
bool mforms::gtk::MenuImpl::create(Menu *self) {
  return new mforms::gtk::MenuImpl(self);
}

//------------------------------------------------------------------------------
void mforms::gtk::MenuImpl::remove_item(Menu *self, int i) {
  MenuImpl *menu = self->get_data<MenuImpl>();
  if (menu) {
    menu->_menu.remove(*menu->_menu.get_children()[i]);
  }
}

//------------------------------------------------------------------------------
int mforms::gtk::MenuImpl::add_item(Menu *self, const std::string &caption, const std::string &action) {
  int index = -1;
  MenuImpl *menu = self->get_data<MenuImpl>();
  if (menu) {
    Gtk::MenuItem *item = Gtk::manage(new Gtk::MenuItem(caption, true));
    menu->_menu.append(*item);
    item->show();
    index = menu->_menu.get_children().size() - 1;
    item->signal_activate().connect(sigc::bind(sigc::mem_fun(self, &mforms::Menu::handle_action), action));
  }
  return index;
}

//------------------------------------------------------------------------------
int mforms::gtk::MenuImpl::add_separator(Menu *self) {
  int index = -1;
  MenuImpl *menu = self->get_data<MenuImpl>();
  if (menu) {
    Gtk::SeparatorMenuItem *sep = Gtk::manage(new Gtk::SeparatorMenuItem());
    menu->_menu.append(*sep);
    sep->show();
    index = menu->_menu.get_children().size() - 1;
  }
  return index;
}

//------------------------------------------------------------------------------
int mforms::gtk::MenuImpl::add_submenu(Menu *self, const std::string &caption, Menu *submenu) {
  int index = -1;
  MenuImpl *menu = self->get_data<MenuImpl>();
  MenuImpl *sub_menu = submenu->get_data<MenuImpl>();
  if (menu) {
    Gtk::MenuItem *item = Gtk::manage(new Gtk::MenuItem(caption, true));
    item->set_submenu(sub_menu->_menu);
    menu->_menu.append(*item);
    item->show();
    index = menu->_menu.get_children().size() - 1;
  }
  return index;
}

//------------------------------------------------------------------------------
void mforms::gtk::MenuImpl::set_item_enabled(Menu *self, int i, bool flag) {
  MenuImpl *menu = self->get_data<MenuImpl>();
  if (menu) {
    Gtk::MenuItem *item = menu->item_at(i);
    if (item)
      item->set_sensitive(flag);
  }
}

//------------------------------------------------------------------------------
void mforms::gtk::MenuImpl::popup_at(Menu *self, Object *control, int x, int y) {
  MenuImpl *menu = self->get_data<MenuImpl>();

  if (menu)
    menu->_menu.popup(3, gtk_get_current_event_time()); // 3 is normally right mouse button, according to doc
}

//------------------------------------------------------------------------------
void mforms::gtk::MenuImpl::clear(Menu *self) {
  MenuImpl *menu = self->get_data<MenuImpl>();

  if (menu) {
    Gtk::Menu &shell = menu->_menu;

    const std::vector<Gtk::Widget *> children = shell.get_children();
    const int size = children.size();

    if (size > 0) {
      for (int i = 0; i < size; ++i)
        shell.remove(*(children[i]));
    }
  }
}

//------------------------------------------------------------------------------
void mforms::gtk::MenuImpl::init() {
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_menu_impl.create = &mforms::gtk::MenuImpl::create;
  f->_menu_impl.remove_item = &mforms::gtk::MenuImpl::remove_item;
  f->_menu_impl.add_item = &mforms::gtk::MenuImpl::add_item;
  f->_menu_impl.add_separator = &mforms::gtk::MenuImpl::add_separator;
  f->_menu_impl.add_submenu = &mforms::gtk::MenuImpl::add_submenu;
  f->_menu_impl.set_item_enabled = &mforms::gtk::MenuImpl::set_item_enabled;
  f->_menu_impl.popup_at = &mforms::gtk::MenuImpl::popup_at;
  f->_menu_impl.clear = &mforms::gtk::MenuImpl::clear;
}
