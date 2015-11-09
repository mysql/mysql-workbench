/* 
 * Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
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
#include "mforms/appview.h"
#include "mforms/drawbox.h"

#include "grts/structs.app.h"
#include "grts/structs.db.mgmt.h"


#ifdef __APPLE__
#define HOME_TITLE_FONT "Helvetica Neue Light"
#define HOME_NORMAL_FONT "Helvetica Neue Light"
#define HOME_DETAILS_FONT "Helvetica Neue Light"
// Info font is only used on Mac.
#define HOME_INFO_FONT "Baskerville"
#elif defined(_WIN32)
#define HOME_TITLE_FONT "Segoe UI"
#define HOME_NORMAL_FONT "Segoe UI"
#define HOME_NORMAL_FONT_XP "Tahoma"

#define HOME_DETAILS_FONT "Segoe UI"
#else
#define HOME_TITLE_FONT "Tahoma"
#define HOME_NORMAL_FONT "Tahoma"
#define HOME_DETAILS_FONT "Helvetica"
#endif

#define HOME_TITLE_FONT_SIZE 20
#define HOME_SUBTITLE_FONT_SIZE 16

#define HOME_TILES_TITLE_FONT_SIZE 16
#define HOME_SMALL_INFO_FONT_SIZE 12
#define HOME_DETAILS_FONT_SIZE 12

#define TILE_DRAG_FORMAT "com.mysql.workbench-drag-tile-format"


class ShortcutSection;
class DocumentsSection;

namespace mforms
{
  class Menu;
};

namespace wb
{
  class ConnectionsSection;
  
  /**
   * Value to tell observers which action was triggered on the home screen.
   */
  enum HomeScreenAction
  {
    ActionNone,

    ActionShortcut,
    ActionRemoveShortcut,

    ActionOpenConnectionFromList,
    ActionNewConnection,
    ActionManageConnections,
    ActionFilesWithConnection,
    ActionMoveConnectionToGroup,
    ActionMoveConnection,
    ActionUpdateFabricConnections,

    ActionSetupRemoteManagement,
    
    ActionEditSQLScript,

    ActionOpenEERModel,
    ActionNewEERModel,
    ActionOpenEERModelFromList,
    ActionNewModelFromDB,
    ActionNewModelFromScript,
  };

  enum HomeScreenMenuType
  {
    HomeMenuConnection,
    HomeMenuConnectionGroup,
    HomeMenuConnectionFabric,
    HomeMenuConnectionGeneric,

    HomeMenuDocumentModelAction,
    HomeMenuDocumentModel,
    HomeMenuDocumentSQLAction,
    HomeMenuDocumentSQL,
  };

  typedef void (*home_screen_action_callback)(HomeScreenAction action, const grt::ValueRef &object, void* user_data);

  class CommandUI;


  struct HomeAccessibleButton : mforms::Accessible
  {
    std::string name;
    std::string default_action;
    base::Rect bounds;
    boost::function <bool (int, int)> default_handler;

    // ------ Accesibility Customized Methods -----

    virtual std::string get_acc_name() { return name; }
    virtual std::string get_acc_default_action() { return default_action;}
    virtual Accessible::Role get_acc_role() { return Accessible::PushButton;}
    virtual base::Rect get_acc_bounds() { return bounds;}

    virtual void do_default_action()
    {
      if (default_handler)
      default_handler((int)bounds.center().x, (int)bounds.center().y);
    }
  };

  //--------------------------------------------------------------------------------------------------

  /**
   * This class implements the main (home) screen in MySQL Workbench, containing
   * sections for connections, plugins and documents.
   */
  class MYSQLWBBACKEND_PUBLIC_FUNC HomeScreen : public mforms::AppView, public base::Observer
  {
  private:
    ShortcutSection *_shortcut_section;
    ConnectionsSection *_connection_section;
    DocumentsSection *_document_section;

    std::string _pending_script; // The path to a script that should be opened next time a connection is opened.
    db_mgmt_ManagementRef _rdbms;
    home_screen_action_callback _callback;
    void* _user_data;

    void update_colors();

    std::vector<db_mgmt_ConnectionRef> _oldAuthList;
  public:
    HomeScreen(CommandUI *cmdui, db_mgmt_ManagementRef rdbms);
    ~HomeScreen();
    
    db_mgmt_ManagementRef rdbms() { return _rdbms; };
    
    boost::function<void (grt::ValueRef, std::string)> handle_context_menu;
    
    void set_callback(home_screen_action_callback callback, void* user_data);
    void trigger_callback(HomeScreenAction action, const grt::ValueRef &object);

    void cancel_script_loading();

    void clear_shortcuts();
    void add_shortcut(const grt::ValueRef &object, const std::string &icon_name);

    void clear_connections(bool clear_state = true);
    void add_connection(db_mgmt_ConnectionRef connection, const std::string &title,
      const std::string &description, const std::string &user, const std::string &schema);

    void oldAuthConnections(const std::vector<db_mgmt_ConnectionRef> &list);

    void clear_documents();
    void add_document(const grt::StringRef &path, const time_t &time, const std::string schemas,
      long file_size);

    void set_menu(mforms::Menu *menu, HomeScreenMenuType type);

    void on_resize();
    void setup_done();

    virtual void handle_notification(const std::string &name, void *sender, base::NotificationInfo &info);
  };
}

#endif // _HOME_SCREEN_H_
