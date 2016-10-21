/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include <cairo/cairo.h>
#include "mforms/drawbox.h"
#include "mforms/menu.h"
#include "base/any.h"

namespace mforms
{
//--------------------------------------------------------------------------------------------------

  typedef std::map<std::string, base::any> anyMap;

  template <typename T> T getAnyMapValue(const anyMap &map, const std::string &key, base::any a = base::any())
  {
    T tmp;
    try {
      return map.at(key);
    } catch (std::out_of_range &/*err*/)
    {
      if (a.isNull())
        tmp = 0;
      else
        tmp = a.as<T>();
      return tmp;
    }
  }

  template<>
  inline std::string getAnyMapValue(const anyMap &map, const std::string &key, base::any a)
  {
      std::string tmp;
      try {
        return map.at(key);
      } catch (std::out_of_range &/*err*/)
      {
        if (a.isNull())
          return tmp;
        else
          return a.as<std::string>();
      }
  }

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

    ActionSetupRemoteManagement,

    ActionEditSQLScript,

    ActionOpenEERModel,
    ActionNewEERModel,
    ActionOpenEERModelFromList,
    ActionNewModelFromDB,
    ActionNewModelFromScript
  };

  enum HomeScreenMenuType
  {
    HomeMenuConnection,
    HomeMenuConnectionGroup,
    HomeMenuConnectionGeneric,

    HomeMenuDocumentModelAction,
    HomeMenuDocumentModel,
    HomeMenuDocumentSQLAction,
    HomeMenuDocumentSQL

  };

  class MFORMS_EXPORT HomeScreenDropInfo
  {
  public:
    HomeScreenDropInfo() : valueIsConnectionId(false), to(0) {}
    bool valueIsConnectionId;
    std::string value;
    std::size_t to;
    std::string group;
  };

  class MFORMS_EXPORT HomeScreenDropFilesInfo
  {
  public:
    std::string connectionId;
    std::vector<std::string> files;
  };

  class MFORMS_EXPORT HomeScreenSettings
  {
  public:
    #ifdef __APPLE__
    static const char* HOME_TITLE_FONT;
    static const char* HOME_NORMAL_FONT;
    static const char* HOME_DETAILS_FONT;
    // Info font is only used on Mac.
    static const char* HOME_INFO_FONT;
    #elif defined(_WIN32)
    static const char* HOME_TITLE_FONT;
    static const char* HOME_NORMAL_FONT;
    static const char* HOME_DETAILS_FONT;
    #else
    static const char* HOME_TITLE_FONT;
    static const char* HOME_NORMAL_FONT;
    static const char* HOME_DETAILS_FONT;
    #endif

    static const int HOME_TITLE_FONT_SIZE = 20;
    static const int HOME_SUBTITLE_FONT_SIZE = 16;

    static const int HOME_TILES_TITLE_FONT_SIZE = 16;
    static const int HOME_SMALL_INFO_FONT_SIZE = 12;
    static const int HOME_DETAILS_FONT_SIZE = 12;

    static const char* TILE_DRAG_FORMAT;
  };

  class MFORMS_EXPORT HomeAccessibleButton : public mforms::Accessible
  {
  public:
    std::string name;
    std::string default_action;
    base::Rect bounds;
    boost::function <bool (int, int)> default_handler;

    // ------ Accesibility Customized Methods -----

    virtual std::string get_acc_name();
    virtual std::string get_acc_default_action();
    virtual Accessible::Role get_acc_role();
    virtual base::Rect get_acc_bounds();

    virtual void do_default_action();
  };

  class MFORMS_EXPORT HomeScreenSection : public mforms::DrawBox
  {
  protected:
    std::string _iconName;
  public:
    HomeScreenSection(const std::string &icon) : _iconName(icon){}
    virtual ~HomeScreenSection() {};
    std::string getIcon() { return _iconName; }
    virtual void updateHeight() = 0;
    virtual void cancelOperation() = 0;
    virtual void setFocus() = 0;
    virtual bool canHandle(HomeScreenMenuType type) = 0;
    virtual void setContextMenu(mforms::Menu *menu, HomeScreenMenuType type) = 0;
    virtual void setContextMenuAction(mforms::Menu *menu, HomeScreenMenuType type) = 0;
    std::function<void()> callback;
  };

  // The following helpers are just temporary. They will be replaced by a cairo context class.
  inline void deleteSurface(cairo_surface_t* surface)
  {
    if (surface != nullptr)
      cairo_surface_destroy(surface);
  }
  int imageWidth(cairo_surface_t* image);
  int imageHeight(cairo_surface_t* image);
  void textWithDecoration(cairo_t* cr, double x, double y, const char* text, bool hot, double width);

} /* namespace wb */
