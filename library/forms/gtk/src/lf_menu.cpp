#include <gtkmm/menushell.h>

#include "../lf_mforms.h"
#include "../lf_menu.h"
#include "mforms.h"

//------------------------------------------------------------------------------
mforms::gtk::MenuImpl::MenuImpl(mforms::Menu *self) : mforms::gtk::ObjectImpl(self) {
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
