/* 
 * Copyright (c) 2009, 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "wb_backend_public_interface.h"
#include <ctime>

#include "base/notifications.h"
#include "base/data_types.h"
#include "base/any.h"
#include "mforms/appview.h"
#include "mforms/drawbox.h"
#include "mforms/tabview.h"

#include "home_screen_helpers.h"
#include "home_screen_documents.h"

class SidebarSection;

namespace mforms
{
  class Menu;
};

namespace wb
{
  class ConnectionsSection;
  class XConnectionsSection;
  class XConnectionEntry;

  typedef void (*home_screen_action_callback)(HomeScreenAction action, const base::any &object, void* user_data);

  class CommandUI;

  //--------------------------------------------------------------------------------------------------

  /**
   * This class implements the main (home) screen in MySQL Workbench, containing
   * sections for connections, plugins and documents.
   */
  class MYSQLWBBACKEND_PUBLIC_FUNC HomeScreen : public mforms::AppView, public base::Observer
  {
  private:
    SidebarSection *_sidebarSection;

    std::string _pending_script; // The path to a script that should be opened next time a connection is opened.
    home_screen_action_callback _callback;
    void* _user_data;
    mforms::TabView _tabView;

    void update_colors();

    std::vector<HomeScreenSection*> _sections;
  public:
    HomeScreen();
    virtual ~HomeScreen();
    
    void addSection(HomeScreenSection *section);
    void addSectionEntry(const std::string& icon_name, HomeScreenSection* section, std::function<void()> callback, bool canSelect);

    std::function<void (base::any, std::string)> handle_context_menu;
    
    void set_callback(home_screen_action_callback callback, void* user_data);
    void trigger_callback(HomeScreenAction action, const base::any &object);
    void openConnection(const dataTypes::XProject &project);

    void cancelOperation();
    void clearSidebar();

    void set_menu(mforms::Menu *menu, HomeScreenMenuType type);

    void on_resize();
    void setup_done();

    virtual void handle_notification(const std::string &name, void *sender, base::NotificationInfo &info);
  };
}

#endif // _HOME_SCREEN_H_
