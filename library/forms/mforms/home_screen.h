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

class SidebarSection;

namespace mforms {
  class Menu;
  class ConnectionsSection;
  class XProjectsSection;
  class XProjectEntry;

  class CommandUI;

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
    void addSectionEntry(const std::string &icon_name, HomeScreenSection *section, std::function<void()> callback,
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
