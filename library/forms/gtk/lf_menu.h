#ifndef __LF_MENU_H__
#define __LF_MENU_H__

#include "mforms/base.h"
#include "mforms/menu.h"
#include <gtkmm/menu.h>
#include "lf_base.h"

namespace mforms {
  namespace gtk {

    class MenuImpl : public ObjectImpl {
      Gtk::Menu _menu;

      Gtk::MenuItem *item_at(const int index);

      static bool create(Menu *self);

      static void remove_item(Menu *self, int i);
      static int add_item(Menu *self, const std::string &caption, const std::string &action);
      static int add_separator(Menu *self);
      static int add_submenu(Menu *self, const std::string &caption, Menu *submenu);
      static void clear(Menu *self);

      static void set_item_enabled(Menu *self, int i, bool flag);

      static void popup_at(Menu *self, Object *control, int x, int y);

    public:
      MenuImpl(Menu *self);

      static void init();
    }; // class MenuImpl

  } // namespace gtk
} // namespace mforms

#endif
