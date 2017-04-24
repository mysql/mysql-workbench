/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _HOME_SCREEN_H_
#define _HOME_SCREEN_H_

#include <ctime>
#include "base/notifications.h"
#include "base/data_types.h"
#include "base/any.h"
#include "mforms/appview.h"
#include "mforms/drawbox.h"
#include "mforms/tabview.h"

#include "home_screen_helpers.h"


namespace mforms {
  class Menu;
  class ConnectionsSection;
  class XProjectsSection;
  class XProjectEntry;

  class CommandUI;
  class SidebarSection;
  class HomeScreen;

  //----------------- ShortcutSection ----------------------------------------------------------------
  struct SidebarEntry : mforms::Accessible {
    SidebarSection *owner;
    std::function<void()> callback;
    bool canSelect;

    cairo_surface_t *icon;
    std::string title;       // Shorted title, depending on available space.
    base::Rect title_bounds; // Relative bounds of the title text.
    base::Rect acc_bounds;   // Bounds to be used for accessibility
    base::Color indicatorColor; // Color of the indicator triangle

    SidebarEntry();
    // ------ Accesibility Methods -----
    virtual std::string get_acc_name();
    virtual Accessible::Role get_acc_role();
    virtual base::Rect get_acc_bounds();
    virtual std::string get_acc_default_action();
    virtual void do_default_action();
  };


  class SidebarSection : public mforms::DrawBox {
  private:
    HomeScreen *_owner;

    std::vector<std::pair<SidebarEntry *, HomeScreenSection *>> _entries;

    SidebarEntry *_hotEntry;
    SidebarEntry *_activeEntry; // For the context menu.

    std::function<bool(int, int)> _accessible_click_handler;

  public:
    const int SIDEBAR_LEFT_PADDING = 18;
    const int SIDEBAR_TOP_PADDING = 18; // The vertical offset of the first shortcut entry.
    const int SIDEBAR_RIGHT_PADDING = 25;
    const int SIDEBAR_ROW_HEIGHT = 50;
    const int SIDEBAR_SPACING = 18; // Vertical space between entries.

    //--------------------------------------------------------------------------------------------------

    SidebarSection(HomeScreen *owner);
    virtual ~SidebarSection();
    void drawTriangle(cairo_t *cr, int x1, int y1, int x2, int y2, base::Color &color, float alpha);
    void repaint(cairo_t *cr, int areax, int areay, int areaw, int areah);
    int shortcutFromPoint(int x, int y);
    void addEntry(const std::string &title, const std::string &icon_name, HomeScreenSection *section, std::function<void()> callback,
                  bool canSelect);
    HomeScreenSection *getActive();
    void setActive(HomeScreenSection *section);
    void layout(cairo_t *cr);
    virtual bool mouse_click(mforms::MouseButton button, int x, int y);
    bool mouse_leave();

    virtual bool mouse_move(mforms::MouseButton button, int x, int y);
    virtual int get_acc_child_count();
    virtual Accessible *get_acc_child(int index);
    virtual Accessible::Role get_acc_role();
    virtual mforms::Accessible *hit_test(int x, int y);
  };

  //--------------------------------------------------------------------------------------------------

  /**
   * This class implements the main (home) screen in MySQL Workbench, containing
   * sections for connections, plugins and documents.
   */
  class MFORMS_EXPORT HomeScreen : public mforms::AppView, public base::Observer {
  private:
    SidebarSection *_sidebarSection;

    std::string _pending_script; // The path to a script that should be opened next time a connection is opened.
    mforms::TabView _tabView;
    bool _singleSection;

    void update_colors();

    std::vector<HomeScreenSection *> _sections;

  public:
    HomeScreen(bool singleSection = false);
    virtual ~HomeScreen();

    void addSection(HomeScreenSection *section);
    void addSectionEntry(const std::string &title, const std::string &icon_name, std::function<void()> callback,
                         bool canSelect);

    std::function<void(base::any, std::string)> handleContextMenu;
    std::function<void(HomeScreenAction action, const base::any &object)> onHomeScreenAction;

    void trigger_callback(HomeScreenAction action, const base::any &object);

    void cancelOperation();

    void set_menu(mforms::Menu *menu, HomeScreenMenuType type);

    void on_resize();
    void setup_done();
    void showSection(size_t index);

    virtual void handle_notification(const std::string &name, void *sender, base::NotificationInfo &info);
  };
}

#endif // _HOME_SCREEN_H_
