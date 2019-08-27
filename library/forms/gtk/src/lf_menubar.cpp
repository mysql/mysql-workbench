/*
 * Copyright (c) 2011, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "../lf_mforms.h"
#include "../lf_menubar.h"
#include <gtkmm/menushell.h>
#include <gtkmm/menubar.h>
#include <gtkmm/menu.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/accelgroup.h>
#include <gtkmm/separatormenuitem.h>
#include <mforms.h>
#include "base/wb_iterators.h"
#include "base/string_utilities.h"
#include "base/log.h"
DEFAULT_LOG_DOMAIN("mforms.linux")

//------------------------------------------------------------------------------
namespace {
  template <typename T>
  T cast(void *ptr) {
    return dynamic_cast<T>((Gtk::Widget *)ptr);
  }
}

class MyMenuBar : public Gtk::MenuBar {
public:
  Glib::RefPtr<Gtk::AccelGroup> accel_group;
};

//------------------------------------------------------------------------------
static void process_click(Gtk::MenuItem *mi, mforms::MenuItem *item) {
  const int ignore_signal = (long)mi->get_data("ignore_signal");
  if (!ignore_signal && mi)
    item->callback();
}

//------------------------------------------------------------------------------
Gtk::MenuBar *mforms::widget_for_menubar(mforms::MenuBar *self) {
  Gtk::MenuBar *mb = dynamic_cast<Gtk::MenuBar *>(self->get_data<Gtk::Object>());
  return mb;
}

//------------------------------------------------------------------------------

Glib::RefPtr<Gtk::AccelGroup> get_accel_group(mforms::MenuBase *m) {
  MyMenuBar *mbar = NULL;
  while (m && !(mbar = dynamic_cast<MyMenuBar *>(m->get_data<Gtk::Object>())))
    m = m->get_parent();
  if (mbar)
    return mbar->accel_group;
  return Glib::RefPtr<Gtk::AccelGroup>();
}

//------------------------------------------------------------------------------
static void propagate_accel_group(mforms::MenuBase *item, Glib::RefPtr<Gtk::AccelGroup> agroup) {
  Gtk::MenuItem *mi = cast<Gtk::MenuItem *>(item->get_data_ptr());
  if (mi && mi->has_submenu())
    mi->get_submenu()->set_accel_group(agroup);

  for (int i = item->item_count() - 1; i >= 0; --i) {
    mforms::MenuItem *it = item->get_item(i);
    if (!it->get_shortcut().empty())
      mforms::gtk::MenuItemImpl::set_shortcut(it, it->get_shortcut());
    propagate_accel_group(it, agroup);
  }
}

//------------------------------------------------------------------------------
void mforms::on_add_menubar_to_window(mforms::MenuBar *menu,
                                      Gtk::Window *window) { // must be called when a menubar is attached to a window,
                                                             // so that the accelgroup can be created and attached
  MyMenuBar *mbar = cast<MyMenuBar *>(menu->get_data_ptr());

  if (!mbar->accel_group) {
    mbar->accel_group = window->get_accel_group();
    propagate_accel_group(menu, mbar->accel_group);
  }
}

//------------------------------------------------------------------------------
bool mforms::gtk::MenuItemImpl::create_menu_bar(mforms::MenuBar *item) {
  MyMenuBar *mb = cast<MyMenuBar *>(item->get_data_ptr());
  if (mb)
    delete mb;
  mb = Gtk::manage(new MyMenuBar());
  mb->show();
  auto acc = mb->get_accessible();
  if (acc) {
    acc->set_name("MenuBar");
  }
  item->set_data(mb);
  return mb;
}

static void free_menu(Gtk::Menu *data) {
  if (data)
    delete data;
}

//------------------------------------------------------------------------------
bool mforms::gtk::MenuItemImpl::create_context_menu(mforms::ContextMenu *menu) {
  Gtk::Menu *mb = NULL;
  mb = (Gtk::Menu *)menu->get_data_ptr();
  if (mb == NULL) {
    Gtk::Menu *mb = new Gtk::Menu();
    menu->set_data(mb, (void (*)(void *))free_menu);
    mb->signal_map_event().connect_notify(sigc::hide(sigc::mem_fun(*menu, &mforms::ContextMenu::will_show)));

    Glib::RefPtr<Atk::Object> acc;
    auto parent = mb->get_parent();
    if (parent != nullptr) {
      acc = parent->get_accessible();
    } else {
      acc = mb->get_accessible();
    }
    if (acc) {
      acc->set_name("Context Menu");
    }

  }
  return mb;
}

//------------------------------------------------------------------------------
bool mforms::gtk::MenuItemImpl::create_menu_item(mforms::MenuItem *item, const std::string &label,
                                                 const mforms::MenuItemType type) {
  Gtk::MenuItem *mi = cast<Gtk::MenuItem *>(item->get_data_ptr());

  if (mi) {
    item->set_data(0);
    delete mi;
  }

  if (type == mforms::SeparatorMenuItem)
    item->set_data(Gtk::manage(new Gtk::SeparatorMenuItem()));
  else {
    if (type == mforms::CheckedMenuItem) {
      Gtk::CheckMenuItem *ci = Gtk::manage(new Gtk::CheckMenuItem(label));
      item->set_data(ci);
    } else
      item->set_data(Gtk::manage(new Gtk::MenuItem(label)));
  }

  mi = cast<Gtk::MenuItem *>(item->get_data_ptr());
  if (mi) {
    mi->show();
    if (type != mforms::SeparatorMenuItem) {
      mi->set_use_underline(true);
      mi->signal_activate().connect(sigc::bind(sigc::ptr_fun(process_click), mi, item));
    }
  }

  return mi;
}

//------------------------------------------------------------------------------
void mforms::gtk::MenuItemImpl::set_title(mforms::MenuItem *item, const std::string &label) {
  Gtk::MenuItem *mi = cast<Gtk::MenuItem *>(item->get_data_ptr());
  if (mi)
    mi->set_label(label);
}

//------------------------------------------------------------------------------
std::string mforms::gtk::MenuItemImpl::get_title(mforms::MenuItem *item) {
  std::string ret;
  Gtk::MenuItem *mi = cast<Gtk::MenuItem *>(item->get_data_ptr());
  if (mi)
    ret = mi->get_label();
  return ret;
}

//------------------------------------------------------------------------------
void mforms::gtk::MenuItemImpl::set_name(mforms::MenuItem *item, const std::string &name) {
  Gtk::MenuItem *mi = cast<Gtk::MenuItem *>(item->get_data_ptr());

  if (mi)
    mi->get_accessible()->set_name(name);
}

//------------------------------------------------------------------------------
std::string mforms::gtk::MenuItemImpl::get_name(mforms::MenuItem *item) {
  std::string ret;
  Gtk::MenuItem *mi = cast<Gtk::MenuItem *>(item->get_data_ptr());

  if (mi)
    ret = mi->get_accessible()->get_name();
  return ret;
}

static void add_shortcuts(Glib::RefPtr<Gtk::AccelGroup> accel_group, Gtk::MenuItem *menu_item,
                          const std::vector<std::string> &modifiers, const std::vector<std::string> &shortcuts) {
  std::string modifier;

  for (std::vector<std::string>::const_iterator iter = modifiers.begin(); iter != modifiers.end(); ++iter) {
    std::string current_modifier = *iter;
    std::transform(current_modifier.begin(), current_modifier.end(), current_modifier.begin(),
                   (int (*)(int))std::tolower);

    if (current_modifier == "modifier")
      current_modifier = "control";

    modifier += "<" + current_modifier + ">";
  }

  for (std::vector<std::string>::const_iterator iter = shortcuts.begin(); iter != shortcuts.end(); ++iter) {
    std::string shortcut = modifier + *iter;
    Gdk::ModifierType accel_mods = (Gdk::ModifierType)0;
    guint accel_key = 0;

    Gtk::AccelGroup::parse(shortcut, accel_key, accel_mods);

    if (accel_key != 0)
      menu_item->add_accelerator("activate", accel_group, accel_key, accel_mods, Gtk::ACCEL_VISIBLE);
    else
      logError("Accelerator key not found for %s.\n", shortcut.c_str());
  }
}

//------------------------------------------------------------------------------
void mforms::gtk::MenuItemImpl::set_shortcut(mforms::MenuItem *item, const std::string &item_shortcut) {
  if (item_shortcut.empty()) {
    logWarning("Shortcut is empty\n");
    return;
  }

  Gtk::MenuItem *menu_item = cast<Gtk::MenuItem *>(item->get_data_ptr());

  if (menu_item == NULL) {
    logError("Menu item was not defined (%s)\n", item_shortcut.c_str());
    return;
  }

  // convert the accelerator format from Control+X to <control>x which is recognized by gtk
  std::vector<std::string> parts(base::split(item_shortcut, "+"));

  std::vector<std::string> keys;
  std::string key = parts.back();
  parts.pop_back();

  if (key == "Space")
    keys.push_back("space");
  else if (key == "PageUp") {
    keys.push_back("Page_Up");
    keys.push_back("KP_Page_Up");
  } else if (key == "PageDown") {
    keys.push_back("Page_Down");
    keys.push_back("KP_Page_Down");
  } else if (key == "Slash") {
    keys.push_back("slash");
    keys.push_back("KP_Divide");
  } else if (key == "Minus") {
    keys.push_back("minus");
    keys.push_back("KP_Subtract");
  } else if (key == "Plus") {
    keys.push_back("plus");
    keys.push_back("KP_Add");
  } else if (key == "Asterisk") {
    keys.push_back("multiply");
    keys.push_back("KP_Multiply");
  } else if (key == "Period") {
    keys.push_back("period");
    keys.push_back("KP_Decimal");
  } else if (key == "Return") {
    keys.push_back("Return");
    keys.push_back("KP_Enter");
  } else if (key == "Home") {
    keys.push_back("Home");
    keys.push_back("KP_Home");
  } else if (key == "End") {
    keys.push_back("End");
    keys.push_back("KP_End");
  } else if (key == "Insert") {
    keys.push_back("Insert");
    keys.push_back("KP_Insert");
  } else if (key == "Delete") {
    keys.push_back("Delete");
    keys.push_back("KP_Delete");
  } else if (key == "Up") {
    keys.push_back("Up");
    keys.push_back("KP_Up");
  } else if (key == "Down") {
    keys.push_back("Down");
    keys.push_back("KP_Down");
  } else if (key == "Left") {
    keys.push_back("Left");
    keys.push_back("KP_Left");
  } else if (key == "Right") {
    keys.push_back("Right");
    keys.push_back("KP_Right");
  } else if (key == "0") {
    keys.push_back("0");
    keys.push_back("KP_0");
  } else if (key == "1") {
    keys.push_back("1");
    keys.push_back("KP_1");
  } else if (key == "2") {
    keys.push_back("2");
    keys.push_back("KP_2");
  } else if (key == "3") {
    keys.push_back("3");
    keys.push_back("KP_3");
  } else if (key == "4") {
    keys.push_back("4");
    keys.push_back("KP_4");
  } else if (key == "5") {
    keys.push_back("5");
    keys.push_back("KP_5");
  } else if (key == "6") {
    keys.push_back("6");
    keys.push_back("KP_6");
  } else if (key == "7") {
    keys.push_back("7");
    keys.push_back("KP_7");
  } else if (key == "8") {
    keys.push_back("8");
    keys.push_back("KP_8");
  } else if (key == "9") {
    keys.push_back("9");
    keys.push_back("KP_9");
  } else
    keys.push_back(key);

  // if the item is not in a menu yet, the shortcut adding will be deferred
  if (item->get_parent() && get_accel_group(item->get_parent()))
    add_shortcuts(get_accel_group(item->get_parent()), menu_item, parts, keys);
}

//------------------------------------------------------------------------------
void mforms::gtk::MenuItemImpl::set_enabled(mforms::MenuBase *item, bool is_on) {
  Gtk::Widget *mb = item->get_data<Gtk::Widget>();
  if (mb)
    mb->set_sensitive(is_on);
}

//------------------------------------------------------------------------------
bool mforms::gtk::MenuItemImpl::get_enabled(mforms::MenuBase *item) {
  bool ret = false;
  Gtk::Widget *mb = item->get_data<Gtk::Widget>();
  if (mb)
    ret = mb->get_sensitive();
  return ret;
}

//------------------------------------------------------------------------------
void mforms::gtk::MenuItemImpl::set_checked(mforms::MenuItem *item, bool on) {
  Gtk::CheckMenuItem *mi = cast<Gtk::CheckMenuItem *>(item->get_data_ptr());
  if (mi) {
    mi->set_data("ignore_signal", (void *)1);
    mi->set_active(on);
    mi->set_data("ignore_signal", 0);
  } else
    logError("Passed MenuItem '%s' does not have CheckMenuItem at %p\n",
             mforms::gtk::MenuItemImpl::get_title(item).c_str(), item->get_data_ptr());
}

//------------------------------------------------------------------------------
bool mforms::gtk::MenuItemImpl::get_checked(mforms::MenuItem *item) {
  bool ret = false;
  Gtk::CheckMenuItem *mi = cast<Gtk::CheckMenuItem *>(item->get_data_ptr());
  if (mi) {
    ret = mi->get_active();
  } else
    logError("Passed MenuItem '%s' does not have CheckMenuItem at %p\n",
             mforms::gtk::MenuItemImpl::get_title(item).c_str(), item->get_data_ptr());
  return ret;
}

//------------------------------------------------------------------------------
static void menu_will_show(mforms::MenuBase *item) {
  mforms::MenuBar *mbar;
  if ((mbar = dynamic_cast<mforms::MenuBar *>(item->get_top_menu())))
    mbar->will_show_submenu_from(dynamic_cast<mforms::MenuItem *>(item));
}

void mforms::gtk::MenuItemImpl::insert_item(mforms::MenuBase *menub, int index, mforms::MenuItem *item) {
  Gtk::MenuShell *menu_shell = cast<Gtk::MenuShell *>(menub->get_data_ptr());
  Gtk::MenuItem *item_to_insert = cast<Gtk::MenuItem *>(item->get_data_ptr());

  if (!menu_shell) // menub is not a menubar
  {
    Gtk::MenuItem *mi = cast<Gtk::MenuItem *>(menub->get_data_ptr());
    if (mi) {
      Gtk::Menu *menu = 0;
      if (mi->has_submenu()) // item already has submenu, add to it
        menu = mi->get_submenu();
      else { // no submenu yet in item, create one
        menu = Gtk::manage(new Gtk::Menu());
        mi->signal_activate().connect(sigc::bind(sigc::ptr_fun(menu_will_show), menub));
        mi->set_submenu(*menu);
        menu->show();
      }
      menu_shell = menu;
    } else
      logError("Passed MenuBase %p does not contain neither Gtk::MenuBar nor Gtk::MenuItem\n", menub);
  } else {
    if (menub->get_parent() && get_accel_group(menub))
      propagate_accel_group(menub, get_accel_group(menub));
  }
  if (menu_shell && item_to_insert) {
    menu_shell->insert(*item_to_insert, index);
    item_to_insert->show();
  } else {
    logError("Internal error in MenuBase::insert_item()\n");
  }
}

//------------------------------------------------------------------------------
void mforms::gtk::MenuItemImpl::remove_item(mforms::MenuBase *menu, mforms::MenuItem *item) {
  Gtk::MenuShell *menu_shell = cast<Gtk::MenuShell *>(menu->get_data_ptr());
  if (!menu_shell) {
    Gtk::MenuItem *mi = cast<Gtk::MenuItem *>(menu->get_data_ptr());
    if (mi) {
      if (mi->has_submenu())
        menu_shell = mi->get_submenu();
      else
        logError("Requesting to remove MenuItem from Menu with no sub menu\n");
    } else
      logError("Passed MenuBase %p does not contain neither Gtk::MenuBar nor Gtk::MenuItem\n", menu);
  }

  Gtk::MenuItem *item_to_remove = item ? cast<Gtk::MenuItem *>(item->get_data_ptr()) : 0;
  if (menu_shell) {
    if (item_to_remove) {
      menu_shell->remove(*item_to_remove); // May not work needs to be tested
      item->release();
    } else {
      typedef Glib::ListHandle<Gtk::Widget *> WList;
      WList list = menu_shell->get_children();
      for (base::const_range<WList> it(list); it; ++it)
        delete *it;
    }
  }
}

void mforms::gtk::MenuItemImpl::popup_menu(mforms::ContextMenu *menu, View *owner, base::Point location) {
  Gtk::Menu *mb = cast<Gtk::Menu *>(menu->get_data_ptr());
  //
  mb->popup(3, gtk_get_current_event_time()); // 3 is normally right mouse button, according to doc
}
//------------------------------------------------------------------------------
void mforms::gtk::lf_menubar_init() {
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_menu_item_impl.create_menu_bar = mforms::gtk::MenuItemImpl::create_menu_bar;
  f->_menu_item_impl.create_context_menu = mforms::gtk::MenuItemImpl::create_context_menu;
  f->_menu_item_impl.create_menu_item = mforms::gtk::MenuItemImpl::create_menu_item;
  f->_menu_item_impl.set_title = mforms::gtk::MenuItemImpl::set_title;
  f->_menu_item_impl.get_title = mforms::gtk::MenuItemImpl::get_title;
  f->_menu_item_impl.set_name = mforms::gtk::MenuItemImpl::set_name;
  f->_menu_item_impl.set_shortcut = mforms::gtk::MenuItemImpl::set_shortcut;
  f->_menu_item_impl.set_enabled = mforms::gtk::MenuItemImpl::set_enabled;
  f->_menu_item_impl.get_enabled = mforms::gtk::MenuItemImpl::get_enabled;
  f->_menu_item_impl.set_checked = mforms::gtk::MenuItemImpl::set_checked;
  f->_menu_item_impl.get_checked = mforms::gtk::MenuItemImpl::get_checked;

  f->_menu_item_impl.insert_item = mforms::gtk::MenuItemImpl::insert_item;
  f->_menu_item_impl.remove_item = mforms::gtk::MenuItemImpl::remove_item;
  f->_menu_item_impl.popup_at = mforms::gtk::MenuItemImpl::popup_menu;
}
