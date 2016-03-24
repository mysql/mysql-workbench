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

#include "base/string_utilities.h"
#include "base/file_utilities.h"
#include "base/threading.h"
#include "base/log.h"

#include "mforms/popup.h"
#include "mforms/menu.h"
#include "mforms/menubar.h"
#include "mforms/utilities.h"
#include "mforms/drawbox.h"
#include "mforms/textentry.h"
#include "mforms/imagebox.h"
#include "mforms/scrollpanel.h"

#include "home_screen.h"
#include "home_screen_connections.h"
#include "home_screen_x_connections.h"

#include "workbench/wb_context_names.h"
#include "base/any.h"

DEFAULT_LOG_DOMAIN("home_screen")

using namespace wb;

//----------------- ShortcutSection ----------------------------------------------------------------

struct SidebarEntry : mforms::Accessible
{
  std::function<void()> callback;
  bool canSelect;

  cairo_surface_t *icon;
  std::string title;       // Shorted title, depending on available space.
  base::Rect title_bounds; // Relative bounds of the title text.
  base::Rect acc_bounds;   // Bounds to be used for accessibility

  // ------ Accesibility Methods -----
  virtual std::string get_acc_name() { return title; }
  virtual Accessible::Role get_acc_role() { return Accessible::ListItem; }
  virtual base::Rect get_acc_bounds() { return acc_bounds; }
  virtual std::string get_acc_default_action() { return "Open Item"; }
};

class SidebarSection: public mforms::DrawBox
{
private:
  HomeScreen *_owner;
  cairo_surface_t* _defaultEntryIcon;

  std::vector<SidebarEntry*> _entries;

  SidebarEntry *_hotEntry;
  SidebarEntry *_activeEntry; // For the context menu.
  mforms::Menu _entryContextMenu;

  boost::function <bool (int, int)> _accessible_click_handler;

public:

  const int SIDEBAR_LEFT_PADDING = 18;
  const int SIDEBAR_TOP_PADDING = 18; // The vertical offset of the first shortcut entry.
  const int SIDEBAR_RIGHT_PADDING = 25;
  const int SIDEBAR_ROW_HEIGHT = 50;
  const int SIDEBAR_SPACING = 18;// Vertical space between entries.

  SidebarSection(HomeScreen *owner)
  {
    _owner = owner;
    _hotEntry = nullptr;
    _activeEntry = nullptr;
    _defaultEntryIcon = mforms::Utilities::load_icon("wb_starter_generic_52.png", true);

    _accessible_click_handler = boost::bind(&SidebarSection::mouse_click, this,
      mforms::MouseButtonLeft, _1, _2);
  }

  ~SidebarSection()
  {
    deleteSurface(_defaultEntryIcon);

    for(auto it : _entries)
      delete it;
  }

  //--------------------------------------------------------------------------------------------------

  void drawTriangle(cairo_t *cr, int x1, int y1, int x2, int y2, float alpha)
  {
    cairo_set_source_rgba(cr, 255.0, 255.0, 255.0, alpha);

    cairo_move_to(cr, x2, y1 + abs(y2 - y1) / 3);
    cairo_line_to(cr, x1 + abs(x2 - x1) * 0.6, y1 + abs(y2 - y1) / 2);
    cairo_line_to(cr, x2, y2 - abs(y2 - y1) / 3);
    cairo_fill(cr);
  }

  //------------------------------------------------------------------------------------------------

  void repaint(cairo_t *cr, int areax, int areay, int areaw, int areah)
  {
    layout(cr);

    int height = get_height();

    cairo_select_font_face(cr, wb::HomeScreenSettings::HOME_TITLE_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, wb::HomeScreenSettings::HOME_TITLE_FONT_SIZE);

    cairo_set_source_rgb(cr, 0, 0, 0);

    // Shortcuts block.
    int yoffset = SIDEBAR_TOP_PADDING;
    if (_entries.size() > 0 && yoffset < height)
    {
      cairo_select_font_face(cr, wb::HomeScreenSettings::HOME_NORMAL_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size(cr, wb::HomeScreenSettings::HOME_SUBTITLE_FONT_SIZE);

      for (auto iterator : _entries)
      {
        float alpha = (yoffset + SIDEBAR_ROW_HEIGHT) > height ? 0.25f : 1;

        iterator->acc_bounds.pos.x = SIDEBAR_LEFT_PADDING;
        iterator->acc_bounds.pos.y = yoffset;
        iterator->acc_bounds.size.width = get_width() - (SIDEBAR_LEFT_PADDING + SIDEBAR_RIGHT_PADDING);
        iterator->acc_bounds.size.height = SIDEBAR_ROW_HEIGHT;

        mforms::Utilities::paint_icon(cr, iterator->icon, SIDEBAR_LEFT_PADDING, yoffset, alpha);

        if (!iterator->title.empty())
          cairo_set_source_rgba(cr, 0, 0, 0, alpha);

        if (iterator == _activeEntry) //we need to draw an indicator
          drawTriangle(cr, get_width() - SIDEBAR_RIGHT_PADDING, yoffset, get_width(), yoffset + SIDEBAR_ROW_HEIGHT, alpha);

        yoffset += SIDEBAR_ROW_HEIGHT + SIDEBAR_SPACING;
        if (yoffset >= height)
          break;
      }
    }
  }

  //--------------------------------------------------------------------------------------------------

  int shortcutFromPoint(int x, int y)
  {
    if (x < SIDEBAR_LEFT_PADDING || y < SIDEBAR_TOP_PADDING || x > get_width() - SIDEBAR_RIGHT_PADDING)
      return -1;

    y -= SIDEBAR_TOP_PADDING;
    int point_in_row = y % (SIDEBAR_ROW_HEIGHT + SIDEBAR_SPACING);
    if (point_in_row >= SIDEBAR_ROW_HEIGHT)
      return -1; // In the spacing between entries.

    size_t row = y / (SIDEBAR_ROW_HEIGHT + SIDEBAR_SPACING);
    size_t height = get_height() - SIDEBAR_TOP_PADDING;
    size_t row_bottom = row * (SIDEBAR_ROW_HEIGHT + SIDEBAR_SPACING) + SIDEBAR_ROW_HEIGHT;
    if (row_bottom > height)
      return -1; // The last shortcut is dimmed if it goes over the bottom border.
                 // Take it out from the hit test too.

    if (row < _entries.size())
      return (int)row;

    return -1;
  }

//  //--------------------------------------------------------------------------------------------------
//
  /**
   * Adds a new sidebar entry to the internal list. The function performs some sanity checks.
   */
  void addEntry(const std::string& icon_name, std::function<void()> callback, bool canSelect)
  {

    SidebarEntry *entry = new SidebarEntry;

    entry->callback = callback;
    entry->canSelect = canSelect;


    // See if we can load the icon. If not use the placeholder.
    entry->icon = mforms::Utilities::load_icon(icon_name, true);
    if (entry->icon == NULL)
      entry->icon = _defaultEntryIcon;

    _entries.push_back(entry);
    if (_activeEntry == nullptr && entry->canSelect)
      _activeEntry = _entries.back();

    set_layout_dirty(true);
  }

  //--------------------------------------------------------------------------------------------------

  void clearEntries()
  {
    for (auto iterator : _entries)
    {
      if (iterator->icon != _defaultEntryIcon)
        deleteSurface(iterator->icon);
      delete iterator;
    }

    _hotEntry = nullptr;
    _activeEntry = nullptr;
    _entries.clear();
    set_layout_dirty(true);
  }

  //--------------------------------------------------------------------------------------------------

  void layout(cairo_t* cr)
  {
    if (is_layout_dirty())
    {
      set_layout_dirty(false);

      double icon_xoffset = SIDEBAR_LEFT_PADDING;
      double text_xoffset = icon_xoffset + 60;

      double yoffset = SIDEBAR_TOP_PADDING;

      double text_width = get_width() - text_xoffset - SIDEBAR_RIGHT_PADDING;

      cairo_select_font_face(cr, wb::HomeScreenSettings::HOME_NORMAL_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size(cr, wb::HomeScreenSettings::HOME_SUBTITLE_FONT_SIZE);

      cairo_font_extents_t font_extents;
      cairo_font_extents(cr, &font_extents);
      double text_height = ceil(font_extents.height);

      // Compute bounding box for each shortcut entry.
      for (auto iterator : _entries)
      {
        int icon_height = imageHeight(iterator->icon);


        std::string title = iterator->title;
        if (!title.empty())
        {
          iterator->title_bounds.pos.x = text_xoffset;

          // Text position is the lower-left corner.
          iterator->title_bounds.pos.y = icon_height / 4 + text_height / 2;
          iterator->title_bounds.size.height = text_height;

          cairo_text_extents_t extents;
          iterator->title = mforms::Utilities::shorten_string(cr, title, text_width);
          cairo_text_extents(cr, iterator->title.c_str(), &extents);
          iterator->title_bounds.size.width = extents.width;
        }

        yoffset += SIDEBAR_ROW_HEIGHT + SIDEBAR_SPACING;
      }
    }
  }

  //--------------------------------------------------------------------------------------------------

  virtual bool mouse_double_click(mforms::MouseButton button, int x, int y)
  {
    return mouse_click(button, x, y); // Handle both the same way. Important especially for fast scrolling.
  }

  //--------------------------------------------------------------------------------------------------

  virtual bool mouse_click(mforms::MouseButton button, int x, int y)
  {
    switch (button)
    {
      case mforms::MouseButtonLeft:
        {
          if (_hotEntry != nullptr && _hotEntry->canSelect)
          {
            _activeEntry = _hotEntry;
            set_needs_repaint();
          }

          if (_hotEntry != nullptr && _hotEntry->callback)
            _hotEntry->callback();
        }
        break;

      default:
        break;
    }
    return false;
  }

  //--------------------------------------------------------------------------------------------------

  bool mouse_leave()
  {
    if (_hotEntry != nullptr)
    {
      _hotEntry = nullptr;
      set_needs_repaint();
      return true;
    }
    return false;
  }

  //--------------------------------------------------------------------------------------------------

  virtual bool mouse_move(mforms::MouseButton button, int x, int y)
  {
    SidebarEntry *shortcut = nullptr;
    int row = shortcutFromPoint(x, y);
    if (row > -1)
      shortcut = _entries[row];
    if (shortcut != _hotEntry)
    {
      _hotEntry = shortcut;
      set_needs_repaint();
      return true;
    }
    return false;
  }

  //--------------------------------------------------------------------------------------------------

  void cancel_operation()
  {
    _owner->cancel_script_loading();
  }

  //------------------------------------------------------------------------------------------------
  
  virtual int get_acc_child_count()
  { 
    return (int)_entries.size();
  }

  //------------------------------------------------------------------------------------------------
  
  virtual Accessible* get_acc_child(int index)
  { 
    mforms::Accessible* accessible = NULL;

    if (index < (int)_entries.size())
      accessible = _entries[index];

    return accessible;
  }

  //------------------------------------------------------------------------------------------------
  
  virtual Accessible::Role get_acc_role()
  { 
    return Accessible::List;
  }

  virtual mforms::Accessible* hit_test(int x, int y)
  { 
    mforms::Accessible* accessible = NULL;

    int row = shortcutFromPoint(x, y);
    if (row != -1)
      accessible = _entries[row];

    return accessible;
  }

};

//----------------- HomeScreen ---------------------------------------------------------------------

#include "workbench/wb_command_ui.h"

HomeScreen::HomeScreen(db_mgmt_ManagementRef rdbms)
  : AppView(true, "home", true)
{
  _rdbms = rdbms;

  _callback = nullptr;
  _user_data = nullptr;
  openMigrationCallback = nullptr;

  _sidebarSection = new SidebarSection(this);
  _sidebarSection->set_name("Home Shortcuts Section");
  _sidebarSection->set_size(85, -1);
  add(_sidebarSection, false, true);

  mforms::ScrollPanel *scroll = mforms::manage(new mforms::ScrollPanel(mforms::ScrollPanelNoFlags));
  _xConnectionsSection = new wb::XConnectionsSection(this);
  _xConnectionsSection->set_name("Home X Connections Section");
  _xConnectionsSection->set_size(-1, 1); // We need initial size for OSX.
  scroll->add(_xConnectionsSection);
  add(scroll, true, true);

  scroll = mforms::manage(new mforms::ScrollPanel(mforms::ScrollPanelNoFlags));
  _connection_section = new wb::ConnectionsSection(this);
  _connection_section->set_name("Home Connections Section");
  _connection_section->set_size(-1, 1);  // We need initial size for OSX.
  scroll->add(_connection_section);
  add(scroll, true, true);
  scroll->show(false);

  scroll = mforms::manage(new mforms::ScrollPanel(mforms::ScrollPanelNoFlags));
  _document_section = new DocumentsSection(this);
  _document_section->set_name("Home Models Section");
  _document_section->set_size(-1, 1); // We need initial size for OSX.
  scroll->add(_document_section);
  add(scroll, true, true);
  scroll->show(false);

  _sidebarSection->addEntry("wb_starter_grt_shell_52.png", [this](){
    _xConnectionsSection->get_parent()->show(true);
    _xConnectionsSection->updateHeight();
    _connection_section->get_parent()->show(false);
    _document_section->get_parent()->show(false);
  }, true);

  _sidebarSection->addEntry("wb_starter_mysql_bug_reporter_52.png", [this]() {
    _xConnectionsSection->get_parent()->show(false);
    _connection_section->get_parent()->show(true);
    _connection_section->updateHeight();
    _document_section->get_parent()->show(false);
  }, true);

  _sidebarSection->addEntry("wb_starter_mysql_wb_blog_52.png", [this]() {
    _xConnectionsSection->get_parent()->show(false);
    _connection_section->get_parent()->show(false);
    _document_section->get_parent()->show(true);
    _document_section->updateHeight();
  }, true);

  _sidebarSection->addEntry("wb_starter_mysql_migration_52.png", [this]() {
    if (openMigrationCallback)
      openMigrationCallback();
  }, false);
  
//  set_menubar(mforms::manage(cmdui->create_menubar_for_context(WB_CONTEXT_HOME_GLOBAL)));
  //_toolbar = mforms::manage(cmdui->create_toolbar(""));

  update_colors();

  Box::scoped_connect(signal_resized(), boost::bind(&HomeScreen::on_resize, this));
  base::NotificationCenter::get()->add_observer(this, "GNColorsChanged");
}

//--------------------------------------------------------------------------------------------------

HomeScreen::~HomeScreen()
{
  base::NotificationCenter::get()->remove_observer(this);
  clear_subviews(); // Remove our sections or the View d-tor will try to release them.

  delete _sidebarSection;
  delete _connection_section;
  delete _document_section;
  delete _xConnectionsSection;
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::update_colors()
{
  set_back_color("#ffffff");
  _sidebarSection->set_back_color("#464646");
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::set_callback(home_screen_action_callback callback, void* user_data)
{
  _callback= callback;
  _user_data= user_data;
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::trigger_callback(HomeScreenAction action, const grt::ValueRef &object)
{
  if (action == HomeScreenAction::ActionEditSQLScript)
  {
    // Editing a script takes 2 steps. The first one happens here, as we store the request and ask the user for
    // a connection to open with that script.
    _pending_script = grt::StringRef::cast_from(object);
    _document_section->show_connection_select_message();
    return;
  }
  else
    _document_section->hide_connection_select_message();

  if (action == HomeScreenAction::ActionOpenConnectionFromList)
  {
    // The second step if we are opening an SQL script. If no SQL is selected we open the connection as usual.
    if (!_pending_script.empty()&& _callback != NULL)
    {
      grt::DictRef dict;
      dict["connection"] = object;
      dict["script"] = grt::StringRef(_pending_script);
      (*_callback)(HomeScreenAction::ActionEditSQLScript, dict, _user_data);
    }
  }

  if (_callback != NULL)
    (*_callback)(action, object, _user_data);
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::openConnection(const dataTypes::XProject &project)
{

  std::string exeName = "workbench.x";
#ifdef _WIN32
  exeName = "MySQLWorkbench.X.exe";
#endif
  logInfo("About to execute: %s\n", mforms::App::get()->get_executable_path(exeName).c_str());
  try
  {
    base::executeProcess({ mforms::App::get()->get_executable_path(exeName), "--open", project.connection.uuid });
  } catch (std::runtime_error &re)
  {
    logError("Unable to execute: %s\n", mforms::App::get()->get_executable_path(exeName).c_str());
  }

  //TODO: implement
  printf("connection uuid: %s\n", project.connection.uuid.c_str());
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::cancel_script_loading()
{
  _pending_script = "";
  _document_section->hide_connection_select_message();
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::clear_connections(bool clear_state)
{
  _connection_section->clear_connections(clear_state);
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::add_connection(db_mgmt_ConnectionRef connection, const std::string &title,
  const std::string &description, const std::string &user, const std::string &schema)
{
  _connection_section->add_connection(connection, title, description, user, schema);
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::addConnection(const dataTypes::XProject &project)
{
  if (_xConnectionsSection)
  {
    dataTypes::ProjectHolder p;
    p.name = project.name;
    p.project = project;
    p.isGroup = false;
    _xConnectionsSection->loadProjects(p);
  }
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::addConnections(const dataTypes::ProjectHolder &holder)
{
  if (_xConnectionsSection)
    _xConnectionsSection->loadProjects(holder);
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::oldAuthConnections(const std::vector<db_mgmt_ConnectionRef> &list)
{
  _oldAuthList.assign(list.begin(), list.end());
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::add_document(const grt::StringRef& path, const time_t &time,
  const std::string schemas, long file_size)
{
  _document_section->add_document(path, time, schemas, file_size);
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::clear_documents()
{
  _document_section->clear_documents();
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::set_menu(mforms::Menu *menu, HomeScreenMenuType type)
{
  switch (type)
  {
    case HomeMenuConnection:
    case HomeMenuConnectionGroup:
    case HomeMenuConnectionGeneric:
      _connection_section->set_context_menu(menu, type);
      break;

    case HomeMenuDocumentModelAction:
      _document_section->set_action_context_menu(menu, true);
      break;

    case HomeMenuDocumentModel:
      _document_section->set_context_menu(menu, true);
      break;

    case HomeMenuDocumentSQLAction:
      _document_section->set_action_context_menu(menu, false);
      break;

    case HomeMenuDocumentSQL:
      _document_section->set_context_menu(menu, false);
      break;
  }
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::on_resize()
{
  // Resize changes the layout so if there is pending script loading the popup is likely misplaced.
  if (!_pending_script.empty())
    cancel_script_loading();

}

//--------------------------------------------------------------------------------------------------

void HomeScreen::setup_done()
{
  _connection_section->focus_search_box();
  if (!_oldAuthList.empty())
  {
    std::string tmp;
    std::vector<db_mgmt_ConnectionRef>::const_iterator it;
    for (it = _oldAuthList.begin(); it != _oldAuthList.end(); ++it)
    {
      tmp.append("\n");
      tmp.append((*it)->name());
      tmp.append(" user name:");
      tmp.append((*it)->parameterValues().get_string("userName"));
    }

    int rc = mforms::Utilities::show_warning("Connections using old authentication protocol found",
              "While loading the stored connections some were found to use the old authentication protocol. "
              "This is no longer supported by MySQL Workbench and the MySQL client library. Click on the \"More Info\" button for a more detailed explanation.\n\n"
              "With this change it is essential that user accounts are converted to the new password storage or you can no longer connect with MySQL Workbench using these accounts.\n\n"
              "The following connections are affected:\n"
              +tmp,
              "Change", "Ignore", "More Info");
    if (rc == mforms::ResultOther)
    {
      mforms::Utilities::open_url("http://mysqlworkbench.org/2014/03/mysql-workbench-6-1-updating-accounts-using-the-old-pre-4-1-1-authentication-protocol/");
    }
    else if (rc == mforms::ResultOk)
    {
      std::vector<db_mgmt_ConnectionRef>::const_iterator it;
      for (it = _oldAuthList.begin(); it != _oldAuthList.end(); ++it)
      {
        if((*it).is_valid())
        {
          if ((*it)->parameterValues().has_key("useLegacyAuth"))
            (*it)->parameterValues().remove("useLegacyAuth");
        }
      }
      _oldAuthList.clear();
    }
  }
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::handle_notification(const std::string &name, void *sender, base::NotificationInfo &info)
{
  if (name == "GNColorsChanged")
  {
    update_colors();
  }
}

//--------------------------------------------------------------------------------------------------
