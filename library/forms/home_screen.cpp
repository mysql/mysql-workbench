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

#include "mforms/home_screen.h"
#include "mforms/home_screen_connections.h"

#include "base/any.h"

using namespace mforms;

//----------------- ShortcutSection ----------------------------------------------------------------

struct SidebarEntry : mforms::Accessible {
  std::function<void()> callback;
  bool canSelect;

  cairo_surface_t *icon;
  std::string title;       // Shorted title, depending on available space.
  base::Rect title_bounds; // Relative bounds of the title text.
  base::Rect acc_bounds;   // Bounds to be used for accessibility
  base::Color indicatorColor; // Color of the indicator triangle

  // ------ Accesibility Methods -----
  virtual std::string get_acc_name() {
    return title;
  }
  virtual Accessible::Role get_acc_role() {
    return Accessible::ListItem;
  }
  virtual base::Rect get_acc_bounds() {
    return acc_bounds;
  }
  virtual std::string get_acc_default_action() {
    return "Open Item";
  }
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

  SidebarSection(HomeScreen *owner) {
    _owner = owner;
    _hotEntry = nullptr;
    _activeEntry = nullptr;

    _accessible_click_handler = std::bind(&SidebarSection::mouse_click, this, mforms::MouseButtonLeft,
                                          std::placeholders::_1, std::placeholders::_2);
  }

  //--------------------------------------------------------------------------------------------------

  ~SidebarSection() {
    for (auto &it : _entries) {
      deleteSurface(it.first->icon);
      delete it.first;
    }
    _entries.clear();
  }

  //--------------------------------------------------------------------------------------------------


  void drawTriangle(cairo_t *cr, int x1, int y1, int x2, int y2, base::Color &color, float alpha) {
    cairo_set_source_rgba(cr, color.red, color.green, color.blue, alpha);
    cairo_move_to(cr, x2, y1 + abs(y2 - y1) / 3);
    cairo_line_to(cr, x1 + abs(x2 - x1) * 0.6, y1 + abs(y2 - y1) / 2);
    cairo_line_to(cr, x2, y2 - abs(y2 - y1) / 3);
    cairo_fill(cr);
  }

  //------------------------------------------------------------------------------------------------

  void repaint(cairo_t *cr, int areax, int areay, int areaw, int areah) {
    layout(cr);

    int height = get_height();

    cairo_select_font_face(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT, CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT_SIZE);

    cairo_set_source_rgb(cr, 0, 0, 0);

    // Shortcuts block.
    int yoffset = SIDEBAR_TOP_PADDING;
    if (_entries.size() > 0 && yoffset < height) {
      cairo_select_font_face(cr, mforms::HomeScreenSettings::HOME_NORMAL_FONT, CAIRO_FONT_SLANT_NORMAL,
                             CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_SUBTITLE_FONT_SIZE);

      for (auto &iterator : _entries) {
        float alpha = iterator.first == _activeEntry ? 1 : 0.5f;
        if ((yoffset + SIDEBAR_ROW_HEIGHT) > height)
          alpha = 0.25f;

        iterator.first->acc_bounds.pos.x = SIDEBAR_LEFT_PADDING;
        iterator.first->acc_bounds.pos.y = yoffset;
        iterator.first->acc_bounds.size.width = get_width() - (SIDEBAR_LEFT_PADDING + SIDEBAR_RIGHT_PADDING);
        iterator.first->acc_bounds.size.height = SIDEBAR_ROW_HEIGHT;

        mforms::Utilities::paint_icon(cr, iterator.first->icon, SIDEBAR_LEFT_PADDING, yoffset, alpha);

        if (!iterator.first->title.empty())
          cairo_set_source_rgba(cr, 0, 0, 0, alpha);


        if (iterator.first == _activeEntry)  //we need to draw an indicator
          drawTriangle(cr, get_width() - SIDEBAR_RIGHT_PADDING, yoffset, get_width(), yoffset + SIDEBAR_ROW_HEIGHT,
                       iterator.first->indicatorColor, alpha);

        yoffset += SIDEBAR_ROW_HEIGHT + SIDEBAR_SPACING;
        if (yoffset >= height)
          break;
      }
    }
  }

  //--------------------------------------------------------------------------------------------------

  int shortcutFromPoint(int x, int y) {
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
  void addEntry(const std::string &icon_name, HomeScreenSection *section, std::function<void()> callback,
                bool canSelect) {
    SidebarEntry *entry = new SidebarEntry;

    entry->callback = callback;
    entry->canSelect = canSelect;
    if (section)
      entry->indicatorColor = section->getIndicatorColor();
    else
      entry->indicatorColor = base::Color("#ffffff"); // Use white as default indicator color

    // See if we can load the icon. If not use the placeholder.
    entry->icon = mforms::Utilities::load_icon(icon_name, true);
    if (entry->icon == NULL)
      throw std::runtime_error("Icon not found: " + icon_name);

    _entries.push_back({entry, section});
    if (_activeEntry == nullptr && entry->canSelect && section != nullptr) {
      _activeEntry = entry;
      // If this is first entry, we need to show it.
      section->get_parent()->show(true);
    }

    set_layout_dirty(true);
  }

  //--------------------------------------------------------------------------------------------------

  HomeScreenSection *getActive() {
    if (_activeEntry != nullptr) {
      for (auto &it : _entries) {
        if (it.first == _activeEntry)
          return it.second;
      }
    }

    return nullptr;
  }

  //--------------------------------------------------------------------------------------------------

  void setActive(HomeScreenSection *section) {
    SidebarEntry *entryForSection = nullptr;
    for (auto &it : _entries) {
      if (it.second == section) {
        if (it.first == _activeEntry)
          return; // Section already active. Nothing to do.

        entryForSection = it.first;
      }
    }

    if (_activeEntry != nullptr) {
      for (auto &it : _entries) {
        if (it.first == _activeEntry)
          it.second->get_parent()->show(false);
      }
    }

    _activeEntry = entryForSection;
    section->get_parent()->show(true);
    set_needs_repaint();
  }

  //--------------------------------------------------------------------------------------------------

  void layout(cairo_t *cr) {
    if (is_layout_dirty()) {
      set_layout_dirty(false);

      double icon_xoffset = SIDEBAR_LEFT_PADDING;
      double text_xoffset = icon_xoffset + 60;

      double yoffset = SIDEBAR_TOP_PADDING;

      double text_width = get_width() - text_xoffset - SIDEBAR_RIGHT_PADDING;

      cairo_select_font_face(cr, mforms::HomeScreenSettings::HOME_NORMAL_FONT, CAIRO_FONT_SLANT_NORMAL,
                             CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_SUBTITLE_FONT_SIZE);

      cairo_font_extents_t font_extents;
      cairo_font_extents(cr, &font_extents);
      double text_height = ceil(font_extents.height);

      // Compute bounding box for each shortcut entry.
      for (auto iterator : _entries) {
        int icon_height = imageHeight(iterator.first->icon);

        std::string title = iterator.first->title;
        if (!title.empty()) {
          iterator.first->title_bounds.pos.x = text_xoffset;

          // Text position is the lower-left corner.
          iterator.first->title_bounds.pos.y = icon_height / 4 + text_height / 2;
          iterator.first->title_bounds.size.height = text_height;

          cairo_text_extents_t extents;
          iterator.first->title = mforms::Utilities::shorten_string(cr, title, text_width);
          cairo_text_extents(cr, iterator.first->title.c_str(), &extents);
          iterator.first->title_bounds.size.width = extents.width;
        }

        yoffset += SIDEBAR_ROW_HEIGHT + SIDEBAR_SPACING;
      }
    }
  }

  //--------------------------------------------------------------------------------------------------

  virtual bool mouse_double_click(mforms::MouseButton button, int x, int y) {
    return mouse_click(button, x, y); // Handle both the same way. Important especially for fast scrolling.
  }

  //--------------------------------------------------------------------------------------------------

  virtual bool mouse_click(mforms::MouseButton button, int x, int y) {
    switch (button) {
      case mforms::MouseButtonLeft: {
        if (_hotEntry != nullptr && _hotEntry->canSelect) {
          _activeEntry = _hotEntry;
          set_needs_repaint();
        }

        if (_hotEntry != nullptr && _hotEntry->callback)
          _hotEntry->callback();
      } break;

      default:
        break;
    }
    return false;
  }

  //--------------------------------------------------------------------------------------------------

  bool mouse_leave() {
    if (_hotEntry != nullptr) {
      _hotEntry = nullptr;
      set_needs_repaint();
      return true;
    }
    return false;
  }

  //--------------------------------------------------------------------------------------------------

  virtual bool mouse_move(mforms::MouseButton button, int x, int y) {
    SidebarEntry *shortcut = nullptr;
    int row = shortcutFromPoint(x, y);
    if (row > -1)
      shortcut = _entries[row].first;
    if (shortcut != _hotEntry) {
      _hotEntry = shortcut;
      set_needs_repaint();
      return true;
    }
    return false;
  }

  //------------------------------------------------------------------------------------------------

  virtual int get_acc_child_count() {
    return (int)_entries.size();
  }

  //------------------------------------------------------------------------------------------------

  virtual Accessible *get_acc_child(int index) {
    mforms::Accessible *accessible = NULL;

    if (index < (int)_entries.size())
      accessible = _entries[index].first;

    return accessible;
  }

  //------------------------------------------------------------------------------------------------

  virtual Accessible::Role get_acc_role() {
    return Accessible::List;
  }

  virtual mforms::Accessible *hit_test(int x, int y) {
    mforms::Accessible *accessible = NULL;

    int row = shortcutFromPoint(x, y);
    if (row != -1)
      accessible = _entries[row].first;

    return accessible;
  }
};

//----------------- HomeScreen ---------------------------------------------------------------------

HomeScreen::HomeScreen(bool singleSection) : AppView(true, "home", true), _singleSection(singleSection) {
  if (!_singleSection) {
    _sidebarSection = new SidebarSection(this);
    _sidebarSection->set_name("Home Shortcuts Section");
    _sidebarSection->set_size(85, -1);
    add(_sidebarSection, false, true);
  } else
    _sidebarSection = nullptr;

  update_colors();

  Box::scoped_connect(signal_resized(), std::bind(&HomeScreen::on_resize, this));
  base::NotificationCenter::get()->add_observer(this, "GNColorsChanged");
}

//--------------------------------------------------------------------------------------------------

HomeScreen::~HomeScreen() {
  base::NotificationCenter::get()->remove_observer(this);
  clear_subviews(); // Remove our sections or the View d-tor will try to release them.

  delete _sidebarSection;
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::update_colors() {
  set_back_color("#ffffff");
  if (_sidebarSection != nullptr)
#ifdef __APPLE__
    _sidebarSection->set_back_color("#323232");
#else
    _sidebarSection->set_back_color("#464646");
#endif


}

//--------------------------------------------------------------------------------------------------

void HomeScreen::addSection(HomeScreenSection *section) {
  if (section == nullptr)
    throw std::runtime_error("Empty HomeScreenSection given");

  if (_singleSection && !_sections.empty())
    throw std::runtime_error("HomeScreen is in singleSection mode. Only one section allowed.");

  _sections.push_back(section);

  if (_sidebarSection != nullptr) {
    mforms::ScrollPanel *scroll = mforms::manage(new mforms::ScrollPanel(mforms::ScrollPanelNoFlags));
    scroll->add(section->getContainer());
    add(scroll, true, true);
    scroll->show(false);

    bool isCallbackOnly = section->callback ? true : false;
    _sidebarSection->addEntry(section->getIcon(), section,
                              [this, isCallbackOnly, section]() {
                                if (isCallbackOnly)
                                  section->callback();
                                else {
                                  for (auto &it : _sections) {
                                    if (it != section)
                                      it->getContainer()->get_parent()->show(false);
                                    else
                                      it->getContainer()->get_parent()->show(true);
                                  }
                                }

                              },
                              !isCallbackOnly);
  } else {
    add(section->getContainer(), true, true);
    section->getContainer()->show(true);
  }
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::addSectionEntry(const std::string &icon_name, HomeScreenSection *section,
                                 std::function<void()> callback, bool canSelect) {
  if (_sidebarSection != nullptr)
    _sidebarSection->addEntry(icon_name, section, callback, canSelect);
  else
    throw std::runtime_error("HomeScreen is in single section mode");
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::trigger_callback(HomeScreenAction action, const base::any &object) {
  onHomeScreenAction(action, object);
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::cancelOperation() {
  for (auto &it : _sections)
    it->cancelOperation();
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::set_menu(mforms::Menu *menu, HomeScreenMenuType type) {
  switch (type) {
    case HomeMenuConnection:
    case HomeMenuConnectionGroup:
    case HomeMenuConnectionGeneric: {
      for (auto &it : _sections)
        it->setContextMenu(menu, type);
      break;
    }
    case HomeMenuDocumentModelAction: {
      for (auto &it : _sections)
        it->setContextMenuAction(menu, type);
      break;
    }

    case HomeMenuDocumentModel:
      for (auto &it : _sections)
        it->setContextMenu(menu, type);
      break;

    case HomeMenuDocumentSQLAction:
      for (auto &it : _sections)
        it->setContextMenuAction(menu, type);
      break;

    case HomeMenuDocumentSQL:
      for (auto &it : _sections)
        it->setContextMenu(menu, type);
      break;
  }
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::on_resize() {
  // Resize changes the layout so if there is pending script loading the popup is likely misplaced.
  cancelOperation();
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::setup_done() {
  if (_sidebarSection != nullptr) {
    if (_sidebarSection->getActive()) {
      _sidebarSection->getActive()->setFocus();
    }
  } else {
    if (!_sections.empty())
      _sections.back()->setFocus();
  }
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::showSection(size_t index) {
  if (index < _sections.size() && _sidebarSection != nullptr) {
    _sidebarSection->setActive(_sections[index]);
    _sidebarSection->getActive()->setFocus();
  }
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::handle_notification(const std::string &name, void *sender, base::NotificationInfo &info) {
  if (name == "GNColorsChanged") {
    update_colors();
  }
}

//--------------------------------------------------------------------------------------------------
