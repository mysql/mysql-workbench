/* 
 * Copyright (c) 2011, 2014, Oracle and/or its affiliates. All rights reserved.
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
namespace
{
template <typename T>
T cast(void *ptr)
{
  return dynamic_cast<T>((Gtk::Widget*)ptr);
}
}

static Glib::RefPtr<Gtk::AccelGroup> accel_group;

//------------------------------------------------------------------------------
static void process_click(Gtk::MenuItem* mi, mforms::MenuItem* item)
{
  const int ignore_signal = (long)mi->get_data("ignore_signal");
  if (!ignore_signal && mi)
    item->callback();
}

//------------------------------------------------------------------------------
void mforms::gtk::set_accel_group(Glib::RefPtr<Gtk::AccelGroup> ag)
{
  accel_group = ag;
}

//------------------------------------------------------------------------------
Gtk::MenuBar* mforms::widget_for_menubar(mforms::MenuBar* self)
{
  Gtk::MenuBar* mb = self->get_data<Gtk::MenuBar>();
  return mb;
}

//------------------------------------------------------------------------------
bool mforms::gtk::MenuItemImpl::create_menu_bar(mforms::MenuBar *item)
{
  Gtk::MenuBar* mb = cast<Gtk::MenuBar*>(item->get_data_ptr());
  if (mb)
    delete mb;
  mb = Gtk::manage(new Gtk::MenuBar());
  mb->show();
  item->set_data(Gtk::manage(mb));
  return mb;
}

static void free_menu(Gtk::Menu *data)
{
  if (data)
    delete data;
}

//------------------------------------------------------------------------------
bool mforms::gtk::MenuItemImpl::create_context_menu(mforms::ContextMenu *menu)
{
  Gtk::Menu *mb = NULL;
  mb = (Gtk::Menu*)menu->get_data_ptr();
  if (mb == NULL)
  {
    Gtk::Menu *mb = new Gtk::Menu();
    menu->set_data(mb, (void(*)(void*))free_menu);
    mb->signal_map_event().connect_notify(sigc::hide(sigc::mem_fun(*menu, &mforms::ContextMenu::will_show)));
  }
  return mb;
}

//------------------------------------------------------------------------------
bool mforms::gtk::MenuItemImpl::create_menu_item(mforms::MenuItem *item, const std::string &label, const mforms::MenuItemType type)
{
  Gtk::MenuItem* mi = cast<Gtk::MenuItem*>(item->get_data_ptr());

  if (mi)
  {
    item->set_data(0);
    delete mi;
  }

  if (type == mforms::SeparatorMenuItem)  
    item->set_data(Gtk::manage(new Gtk::SeparatorMenuItem()));
  else
  {
    if (type == mforms::CheckedMenuItem)
    {
      Gtk::CheckMenuItem* ci = Gtk::manage(new Gtk::CheckMenuItem(label));
      item->set_data(ci);
    }
    else
      item->set_data(Gtk::manage(new Gtk::MenuItem(label)));
  }

  mi = cast<Gtk::MenuItem*>(item->get_data_ptr());
  if (mi)
  {
    mi->show();
    if (type != mforms::SeparatorMenuItem)
    {
      mi->set_use_underline(true);
      mi->signal_activate().connect(sigc::bind(sigc::ptr_fun(process_click), mi, item));
    }
  }

  return mi;
}

//------------------------------------------------------------------------------
void mforms::gtk::MenuItemImpl::set_title(mforms::MenuItem *item, const std::string& label)
{
  Gtk::MenuItem* mi = cast<Gtk::MenuItem*>(item->get_data_ptr());
  if (mi)
    mi->set_label(label);
}

//------------------------------------------------------------------------------
std::string mforms::gtk::MenuItemImpl::get_title(mforms::MenuItem *item)
{
  std::string ret;
  Gtk::MenuItem* mi = cast<Gtk::MenuItem*>(item->get_data_ptr());
  if (mi)
    ret = mi->get_label();
  return ret;
}

//------------------------------------------------------------------------------
void mforms::gtk::MenuItemImpl::set_shortcut(mforms::MenuItem *item, const std::string& item_shortcut)
{
  if (!item_shortcut.empty())
  {
    Gdk::ModifierType   accel_mods = (Gdk::ModifierType)0;
    guint                accel_key = 0;

    // convert the accelerator format from Control+X to <control>x which is recognized by gtk
    std::vector<std::string> parts(base::split(item_shortcut, "+"));

    if (parts.size() > 0)
    {
      std::string shortcut = parts.back();
      parts.pop_back();

      if (shortcut == "Space")
        shortcut = "space";
      else if (shortcut == "PageUp")
        shortcut = "Page_Up";
      else if (shortcut == "PageDown")
        shortcut = "Page_Down";
      else if (shortcut == "Slash")
        shortcut = "slash";
      else if (shortcut == "Minus")
        shortcut = "minus";
      else if (shortcut == "Plus")
        shortcut = "plus";

      while (parts.size() > 0)
      {
        std::string mod = parts.back();
        parts.pop_back();
        std::transform(mod.begin(), mod.end(), mod.begin(), (int(*)(int))std::tolower);
        if ("modifier" == mod)
          mod = "control";
        shortcut = "<"+mod+">"+shortcut;
      }

      if (!shortcut.empty())
        Gtk::AccelGroup::parse(shortcut, accel_key, accel_mods);
    }

    Gtk::MenuItem* mi = cast<Gtk::MenuItem*>(item->get_data_ptr());
    if (accel_key != 0 && mi)
    {
      if (accel_group)
        mi->add_accelerator("activate", accel_group, accel_key, accel_mods, Gtk::ACCEL_VISIBLE);
      else
        log_error("AccelGroup was not set for menubar\n");
    }
  }
}

//------------------------------------------------------------------------------
void mforms::gtk::MenuItemImpl::set_enabled(mforms::MenuBase *item, bool is_on)
{
  Gtk::Widget* mb = item->get_data<Gtk::Widget>();
  if (mb)
    mb->set_sensitive(is_on);
}

//------------------------------------------------------------------------------
bool mforms::gtk::MenuItemImpl::get_enabled(mforms::MenuBase *item)
{
  bool ret = false;
  Gtk::Widget* mb = item->get_data<Gtk::Widget>();
  if (mb)
    ret = mb->get_sensitive();
  return ret;
}

//------------------------------------------------------------------------------
void mforms::gtk::MenuItemImpl::set_checked(mforms::MenuItem *item, bool on)
{
  Gtk::CheckMenuItem* mi = cast<Gtk::CheckMenuItem*>(item->get_data_ptr());
  if (mi)
  {
    mi->set_data("ignore_signal", (void*)1);
    mi->set_active(on);
    mi->set_data("ignore_signal", 0);
  }
  else
    log_error("Passed MenuItem '%s' does not have CheckMenuItem at %p\n", mforms::gtk::MenuItemImpl::get_title(item).c_str(), item->get_data_ptr());
}

//------------------------------------------------------------------------------
bool mforms::gtk::MenuItemImpl::get_checked(mforms::MenuItem *item)
{
  bool ret = false;
  Gtk::CheckMenuItem* mi = cast<Gtk::CheckMenuItem*>(item->get_data_ptr());
  if (mi)
  {
    ret = mi->get_active();
  }
  else
    log_error("Passed MenuItem '%s' does not have CheckMenuItem at %p\n", mforms::gtk::MenuItemImpl::get_title(item).c_str(), item->get_data_ptr());
  return ret;
}

//------------------------------------------------------------------------------
static void menu_will_show(mforms::MenuBase *item)
{
  mforms::MenuBar *mbar;
  if ((mbar = dynamic_cast<mforms::MenuBar*>(item->get_top_menu())))
    mbar->will_show_submenu_from(dynamic_cast<mforms::MenuItem*>(item));
}

void mforms::gtk::MenuItemImpl::insert_item(mforms::MenuBase *menub, int index, mforms::MenuItem *item)
{
  Gtk::MenuShell* menu_shell = cast<Gtk::MenuShell*>(menub->get_data_ptr());
  Gtk::MenuItem* item_to_insert = cast<Gtk::MenuItem*>(item->get_data_ptr());

  if (!menu_shell)
  {
    Gtk::MenuItem* mi = cast<Gtk::MenuItem*>(menub->get_data_ptr());
    if (mi)
    {
      Gtk::Menu* menu = 0;
      if (mi->has_submenu()) // item already has submenu, add to it
        menu = mi->get_submenu();
      else
      { // no submenu yet in item, create one
        menu = Gtk::manage(new Gtk::Menu());
        mi->signal_activate().connect(sigc::bind(sigc::ptr_fun(menu_will_show), menub));
        mi->set_submenu(*menu);
        menu->show();
      }
      menu_shell = menu;
    }
    else
      log_error("Passed MenuBase %p does not contain neither Gtk::MenuBar nor Gtk::MenuItem\n", menub);
  }

  if (menu_shell && item_to_insert)
    menu_shell->insert(*item_to_insert, index);
  else
    log_error("Internal error in MenuBase::insert_item()\n");
}

//------------------------------------------------------------------------------
void mforms::gtk::MenuItemImpl::remove_item(mforms::MenuBase *menu, mforms::MenuItem *item)
{
  Gtk::MenuShell* menu_shell = cast<Gtk::MenuShell*>(menu->get_data_ptr());
  if (!menu_shell)
  {
    Gtk::MenuItem* mi = cast<Gtk::MenuItem*>(menu->get_data_ptr());
    if (mi)
    {
      if (mi->has_submenu())
        menu_shell = mi->get_submenu();
      else
        log_error("Requesting to remove MenuItem from Menu with no sub menu\n");
    }
    else
      log_error("Passed MenuBase %p does not contain neither Gtk::MenuBar nor Gtk::MenuItem\n", menu);
  }

  Gtk::MenuItem* item_to_remove = item ? cast<Gtk::MenuItem*>(item->get_data_ptr()) : 0;
  if (menu_shell)
  {
    if (item_to_remove)
      menu_shell->remove(*item_to_remove); // May not work needs to be tested
    else
    {
      typedef Glib::ListHandle<Gtk::Widget*>    WList;
      WList list = menu_shell->get_children();
      for (base::const_range<WList> it(list); it; ++it)
        menu_shell->remove(*(*it));
    }
  }
}


void mforms::gtk::MenuItemImpl::popup_menu(mforms::ContextMenu *menu, int x, int y)
{
  Gtk::Menu* mb = cast<Gtk::Menu*>(menu->get_data_ptr());

  mb->popup(3, gtk_get_current_event_time()); // 3 is normally right mouse button, according to doc
}

//------------------------------------------------------------------------------
void mforms::gtk::lf_menubar_init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_menu_item_impl.create_menu_bar  = mforms::gtk::MenuItemImpl::create_menu_bar;
  f->_menu_item_impl.create_context_menu= mforms::gtk::MenuItemImpl::create_context_menu;
  f->_menu_item_impl.create_menu_item = mforms::gtk::MenuItemImpl::create_menu_item;
  f->_menu_item_impl.set_title        = mforms::gtk::MenuItemImpl::set_title;
  f->_menu_item_impl.get_title        = mforms::gtk::MenuItemImpl::get_title;
  f->_menu_item_impl.set_shortcut     = mforms::gtk::MenuItemImpl::set_shortcut;
  f->_menu_item_impl.set_enabled      = mforms::gtk::MenuItemImpl::set_enabled;
  f->_menu_item_impl.get_enabled      = mforms::gtk::MenuItemImpl::get_enabled;
  f->_menu_item_impl.set_checked      = mforms::gtk::MenuItemImpl::set_checked;
  f->_menu_item_impl.get_checked      = mforms::gtk::MenuItemImpl::get_checked;

  f->_menu_item_impl.insert_item      = mforms::gtk::MenuItemImpl::insert_item;
  f->_menu_item_impl.remove_item      = mforms::gtk::MenuItemImpl::remove_item;

  f->_menu_item_impl.popup_menu       = mforms::gtk::MenuItemImpl::popup_menu;
}

