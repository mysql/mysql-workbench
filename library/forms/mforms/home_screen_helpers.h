/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include <cairo/cairo.h>
#include "base/accessibility.h"

#include "mforms/drawbox.h"
#include "mforms/menu.h"
#include "base/any.h"

namespace mforms {
  typedef std::map<std::string, base::any> anyMap;

  // Use this variation to get a base::any. Use type cast directly only if you know the key exists in the map and
  // the item (base::any) does not contain a nullptr value
  base::any getAnyMapValue(const anyMap& map, const std::string& key, base::any defaultValue = base::any());

  // Use this variation to cast a base::any safely without throwing and exception
  template <typename T>
  T getAnyMapValueAs(const anyMap& map, const std::string& key, base::any defaultValue = base::any()) {
    anyMap::const_iterator iter = map.find(key);

    if (iter == map.end())
      return defaultValue.isNull() ? T() : defaultValue.as<T>();

    return iter->second;
  }

  /**
   * Value to tell observers which action was triggered on the home screen.
   */
  enum HomeScreenAction {
    ActionNone,

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
    ActionNewModelFromScript,

    ActionOpenDocs,
    ActionOpenBlog,
    ActionOpenForum,

    CloseWelcomeMessage,
    
    RescanLocalServers
  };

  enum HomeScreenMenuType {
    HomeMenuConnection,
    HomeMenuConnectionGroup,
    HomeMenuConnectionGeneric,

    HomeMenuDocumentModelAction,
    HomeMenuDocumentModel,
    HomeMenuDocumentSQLAction,
    HomeMenuDocumentSQL

  };

  class MFORMS_EXPORT HomeScreenDropInfo {
  public:
    HomeScreenDropInfo() : valueIsConnectionId(false), to(0) {
    }
    bool valueIsConnectionId;
    std::string value;
    std::size_t to;
    std::string group;
  };

  class MFORMS_EXPORT HomeScreenDropFilesInfo {
  public:
    std::string connectionId;
    std::vector<std::string> files;
  };

  class MFORMS_EXPORT HomeScreenSettings {
  public:
    static const char* HOME_TITLE_FONT;
    static const char* HOME_NORMAL_FONT;
    static const char* HOME_DETAILS_FONT;
#ifdef __APPLE__
    static const char* HOME_INFO_FONT; // Info font is only used on Mac.
#endif

    static const int HOME_TITLE_FONT_SIZE = 20;
    static const int HOME_SUBTITLE_FONT_SIZE = 16;

    static const int HOME_TILES_TITLE_FONT_SIZE = 16;
    static const int HOME_SMALL_INFO_FONT_SIZE = 12;
    static const int HOME_DETAILS_FONT_SIZE = 12;

    static const char* TILE_DRAG_FORMAT;
  };

  class MFORMS_EXPORT HomeAccessibleButton : public base::Accessible {
  public:
    std::string title;
    std::string description;
    base::Rect bounds;
    std::function<void (void)> defaultHandler;

    // ------ Accesibility Customized Methods -----

    virtual std::string getAccessibilityTitle() override;
    virtual std::string getAccessibilityDescription() override;
    virtual Accessible::Role getAccessibilityRole() override;
    virtual base::Rect getAccessibilityBounds() override;

    virtual void accessibilityDoDefaultAction() override;
    virtual std::string getAccessibilityDefaultAction() override {
      return "click";
    }
  };

  class MFORMS_EXPORT HomeScreenSection : public mforms::DrawBox {
  protected:
    std::string _iconName;

  public:
    HomeScreenSection(const std::string &icon) : _iconName(icon) {
    }
    
    virtual ~HomeScreenSection() {
    }

    std::string getIcon() {
      return _iconName;
    }

    virtual mforms::View* getContainer() {
      return this;
    }

    virtual View *get_parent() const {
      return _parent;
    }

    virtual const char* getTitle() = 0;
    virtual void cancelOperation() = 0;
    virtual void setFocus() = 0;
    virtual bool canHandle(HomeScreenMenuType type) = 0;
    virtual void setContextMenu(mforms::Menu* menu, HomeScreenMenuType type) = 0;
    virtual void setContextMenuAction(mforms::Menu* menu, HomeScreenMenuType type) = 0;

    virtual void updateColors() = 0;
    virtual void updateIcons() = 0;

    std::function<void()> callback;
  };

  // The following helpers are just temporary. They will be replaced by a cairo context class.
  inline void deleteSurface(cairo_surface_t* surface) {
    if (surface != nullptr)
      cairo_surface_destroy(surface);
  }
  int imageWidth(cairo_surface_t* image);
  int imageHeight(cairo_surface_t* image);
  void textWithDecoration(cairo_t* cr, double x, double y, const char* text, bool hot, double width);

} /* namespace wb */
