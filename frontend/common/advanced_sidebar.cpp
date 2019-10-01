/*
 * Copyright (c) 2011, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <cmath>

#include "grts/structs.db.mgmt.h"

#include "base/string_utilities.h"
#include "base/geometry.h"
#include "base/drawing.h"
#include "base/log.h"
#include "base/notifications.h"

#include "mforms/imagebox.h"
#include "mforms/button.h"
#include "mforms/utilities.h"

using namespace base;

#include "advanced_sidebar.h"

#include <math.h>

using namespace std;

using namespace wb;
using namespace mforms;

#ifdef _MSC_VER
#define SIDEBAR_FONT "Segoe UI"
#define SIDEBAR_TITLE_FONT_SIZE 11
#define SIDEBAR_ENTRY_FONT_SIZE 11
#else
#ifdef __APPLE__
#define SIDEBAR_FONT "Lucida Grande"
#define SIDEBAR_TITLE_FONT_SIZE 11
#define SIDEBAR_ENTRY_FONT_SIZE 11
#else
#define SIDEBAR_FONT "Tahoma"
#define SIDEBAR_TITLE_FONT_SIZE 11
#define SIDEBAR_ENTRY_FONT_SIZE 11
#endif
#endif

#define SECTION_ENTRY_HEIGHT 20      // Height of a single section entry.
#define SECTION_ENTRY_SPACING 0      // Vertical distance between two section entries.
#define SECTION_ENTRY_INDENT 13      // Horizontal distance from the left section border to the entry icon.
#define SECTION_ENTRY_ICON_SPACING 6 // Horizontal distance between entry icon and text.

//----------------- SidebarSection -----------------------------------------------------------------

SidebarEntry::SidebarEntry(SidebarSection *owner, const string& name, const std::string& accessibilityName,
                           const string& title, const string& icon, TaskEntryType type,
                           boost::signals2::signal<void (const std::string &)> *callback) {
  _owner = owner;
  _name = name;
  _accessibilityName = accessibilityName;
  _title = title;
  _callback = callback;
  if (!icon.empty())
    _icon = Utilities::load_icon(icon, true);
  else
    _icon = NULL;
  _type = type;
  _enabled = true;
}

//--------------------------------------------------------------------------------------------------

SidebarEntry::~SidebarEntry() {
  if (_icon != NULL)
    cairo_surface_destroy(_icon);
}

//--------------------------------------------------------------------------------------------------

void SidebarEntry::set_title(const std::string& title) {
  _title = title;
  _accessibilityName = title;
}

//--------------------------------------------------------------------------------------------------

void SidebarEntry::set_icon(const std::string& icon) {
  if (_icon)
    cairo_surface_destroy(_icon);
  _icon = Utilities::load_icon(icon, true);
}

//--------------------------------------------------------------------------------------------------

void SidebarEntry::set_enabled(bool flag) {
  _enabled = flag;
}

//--------------------------------------------------------------------------------------------------

void SidebarEntry::paint(cairo_t* cr, base::Rect bounds, bool hot, bool active, const Color& selection_color) {
  _bounds = bounds;

  // Fill background if the item is active.
  if (active) {
    cairo_set_source_rgb(cr, selection_color.red, selection_color.green, selection_color.blue);
    cairo_rectangle(cr, 2, bounds.top(), bounds.left() + bounds.width() - 4, bounds.height());
    cairo_fill(cr);
  }

  cairo_move_to(cr, bounds.left(), bounds.top());

  double offset;
  if (_icon != NULL) {
    base::Size size = mforms::Utilities::getImageSize(_icon);
    offset = (bounds.height() - size.height) / 2;
    mforms::Utilities::paint_icon(cr, _icon, bounds.left(), bounds.top() + offset);
    cairo_rel_move_to(cr, size.width + SECTION_ENTRY_ICON_SPACING, 0);
  }

  cairo_select_font_face(cr, SIDEBAR_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, SIDEBAR_ENTRY_FONT_SIZE);

  Color textColor = active ? _owner->_owner->_activeTextColor : _owner->_owner->_inactiveTextColor;
  cairo_set_source_rgba(cr, textColor.red, textColor.green, textColor.blue, _enabled ? textColor.alpha : 0.5 * textColor.alpha);

  cairo_rel_move_to(cr, 0, (bounds.height() + SIDEBAR_ENTRY_FONT_SIZE) / 2 - 2);
  cairo_show_text(cr, _title.c_str());

  if (hot) {
    // Use the color we set for the text.
    cairo_set_line_width(cr, 1);

    cairo_text_extents_t extents;
    cairo_text_extents(cr, _title.c_str(), &extents);

    double width = ceil(extents.width);
    cairo_rel_move_to(cr, -width, 2);
    cairo_rel_line_to(cr, width, 0);
    cairo_stroke(cr);
  }
}

//--------------------------------------------------------------------------------------------------

bool SidebarEntry::contains(double x, double y) {
  return false;
}

//----------------- SidebarSection -----------------------------------------------------------------

SidebarSection::Button::Button(std::string const& name, std::string const& icon_name, std::string const& alt_icon_name)
  : icon(nullptr), alt_icon(nullptr), hot(false), down(false), state(false) {
  _name = name;
  iconName = icon_name;
  altIconName = alt_icon_name;

  move(0, 0);

  if (!icon_name.empty())
    icon = Utilities::load_icon(icon_name, true);
  if (!alt_icon_name.empty())
    alt_icon = Utilities::load_icon(alt_icon_name, true);

  if (icon != nullptr)
    size = Utilities::getImageSize(icon);
  else if (alt_icon != nullptr)
    size = Utilities::getImageSize(alt_icon);

  bounds_width = (int)size.width + 5;
  bounds_height = (int)size.height + 5;
}

//--------------------------------------------------------------------------------------------------

SidebarSection::Button::~Button() {
  if (icon)
    cairo_surface_destroy(icon);
  if (alt_icon)
    cairo_surface_destroy(alt_icon);
}

//--------------------------------------------------------------------------------------------------

void SidebarSection::Button::draw(cairo_t* cr) {
  if (Utilities::icon_needs_reload(icon)) {
    if (icon)
      cairo_surface_destroy(icon);
    if (alt_icon)
      cairo_surface_destroy(alt_icon);
    icon = Utilities::load_icon(iconName, true);
    alt_icon = Utilities::load_icon(altIconName, true);

    if (icon != nullptr)
      size = Utilities::getImageSize(icon);
    else if (alt_icon != nullptr)
      size = Utilities::getImageSize(alt_icon);

    bounds_width = (int)size.width + 5;
    bounds_height = (int)size.height + 5;
  }

  cairo_surface_t* image = (state && alt_icon != NULL) ? alt_icon : icon;

  if (image) {
    double image_left = x;
    double image_top = y;
    if (hot || down) {
      // Draw a bezel shape under the icon.
      double radius = 3;
      cairo_new_sub_path(cr);
      cairo_arc(cr, image_left + bounds_width - radius, image_top + radius, radius, -M_PI / 2, 0);
      cairo_arc(cr, image_left + bounds_width - radius, image_top + bounds_height - radius, radius, 0, M_PI / 2);
      cairo_arc(cr, image_left + radius, image_top + bounds_height - radius, radius, M_PI / 2, M_PI);
      cairo_arc(cr, image_left + radius, image_top + radius, radius, M_PI, 3 * M_PI / 2);
      cairo_close_path(cr);

      cairo_set_line_width(cr, 1);
      if (down) {
        cairo_set_source_rgba(cr, 0, 0, 0, 0.1);
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, 0, 0, 0, 0.15);
      } else {
        cairo_set_source_rgba(cr, 0, 0, 0, 0.05);
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, 0, 0, 0, 0.1);
      }
      cairo_stroke(cr);
    }

    image_left += floor((bounds_width - size.width) / 2);
    image_top += floor((bounds_height - size.height) / 2);
    Utilities::paint_icon(cr, image, image_left, image_top);
  }
}

//--------------------------------------------------------------------------------------------------

bool SidebarSection::Button::check_hit(ssize_t x, ssize_t y) {
  return (x >= this->x && x < this->x + bounds_width && y >= this->y && y < this->y + bounds_height);
}

//--------------------------------------------------------------------------------------------------

void SidebarSection::Button::move(int x, int y) {
  this->x = x;
  this->y = y;
}

//--------------------------------------------------------------------------------------------------

SidebarSection::SidebarSection(SimpleSidebar* owner, const std::string& title, mforms::TaskSectionFlags flags)
  : DrawBox() {
  _owner = owner;
  _title = title;
  _selected_entry = NULL;
  _hot_entry = NULL;

  _layout_width = 0;
  _layout_height = 0;
  _expanded = true;
  _expandable = (flags & mforms::TaskSectionCollapsible) != 0;
  _expand_text_visible = false;
  _expand_text_width = 0;
  _expand_text_active = false;

  _refresh_button = NULL;
  _config_button = NULL;

  _layout_surface = NULL;
  _layout_context = NULL;
  _last_width = 0;

  if (flags & mforms::TaskSectionRefreshable) {
#ifdef __APPLE__
    _refresh_button = new Button("Refresh", "wb-sidebar-refresh.png", "busy_sidebar_mac.png");
#else
    _refresh_button = new Button("Refresh", "refresh_sidebar.png", "busy_sidebar.png");
#endif
    _enabled_buttons.push_back(_refresh_button);
  } else {
#ifdef __APPLE__
    _refresh_button = new Button("Busy", "", "busy_sidebar_mac.png");
#else
    _refresh_button = new Button("Busy", "", "busy_sidebar.png");
#endif
  }

  if (flags & mforms::TaskSectionShowConfigButton) {
    _config_button = new Button("Launch Connections Editor", "wb_perform_config.png", "");
    _enabled_buttons.push_back(_config_button);
  }
}

//--------------------------------------------------------------------------------------------------

SidebarSection::~SidebarSection() {
  clear();
  delete _refresh_button;
  delete _config_button;

  if (_layout_surface != NULL)
    cairo_surface_destroy(_layout_surface);

  if (_layout_context != NULL)
    cairo_destroy(_layout_context);
}

//--------------------------------------------------------------------------------------------------

void SidebarSection::set_selected(SidebarEntry* entry) {
  if (entry)
    _owner->clear_selection();

  if (_selected_entry != entry) {
    _selected_entry = entry;
    set_needs_repaint();
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Creates a cairo context on a small image surface, to be used for layouting.
 */
void SidebarSection::create_context_for_layout() {
  if (_layout_surface == NULL)
    _layout_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, get_width(), get_height());
  if (_layout_context == NULL)
    _layout_context = cairo_create(_layout_surface);
}

//--------------------------------------------------------------------------------------------------

#define SECTION_TOP_SPACING 4    // Vertical distance from the top border to the heading.
#define SECTION_SIDE_SPACING 6   // Horizontal spacing between border and content.
#define SECTION_BOTTOM_SPACING 7 // Vertical distance from the content to the bottom.
#define SECTION_HEADER_SPACING 6 // Vertical distance between the heading bottom line and the content top.
#define SECTION_HEADER_HEIGHT 12 // Height of the section heading.
#define SECTION_ICON_SPACING 4   // Horizontal distance between buttons and/or text.

void SidebarSection::layout(cairo_t* cr) {
  set_layout_dirty(false);

  _layout_height = SECTION_TOP_SPACING + SECTION_HEADER_HEIGHT;

  if (_expanded) {
    if (_entries.size() > 0)
      _layout_height += SECTION_HEADER_SPACING;
    _layout_width = SECTION_SIDE_SPACING;

    if (_entries.size() > 0)
      _layout_height += _entries.size() * SECTION_ENTRY_HEIGHT + (_entries.size() - 1) * SECTION_ENTRY_SPACING;
  }

  _layout_height += SECTION_BOTTOM_SPACING;

  // Precompute size of the hide/show text for hit tests.
  std::string expand_text = _expanded ? _("Hide") : _("Show");
  cairo_text_extents_t extents;

  cairo_select_font_face(cr, SIDEBAR_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, SIDEBAR_TITLE_FONT_SIZE);
  cairo_text_extents(cr, expand_text.c_str(), &extents);
  _expand_text_width = (int)ceil(extents.x_advance);

  double width = get_width();

  Rect bounds(SECTION_SIDE_SPACING, SECTION_TOP_SPACING + SIDEBAR_TITLE_FONT_SIZE, width - SECTION_SIDE_SPACING,
              SECTION_HEADER_HEIGHT);

  if (_config_button != NULL) {
    cairo_select_font_face(cr, SIDEBAR_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_text_extents(cr, _title.c_str(), &extents);
    int title_width = (int)ceil(extents.x_advance);

    _config_button->move((int)bounds.left() + title_width + SECTION_ICON_SPACING, SECTION_TOP_SPACING / 2);
    _config_button->bounds_width = SECTION_HEADER_HEIGHT + SECTION_TOP_SPACING;
    _config_button->bounds_height = SECTION_HEADER_HEIGHT + SECTION_TOP_SPACING;
  }

  if (_refresh_button) {
    _refresh_button->move((int)bounds.size.width - _refresh_button->bounds_width - SECTION_SIDE_SPACING,
                          SECTION_TOP_SPACING);
    _refresh_button->bounds_width = SECTION_HEADER_HEIGHT;
    _refresh_button->bounds_height = SECTION_HEADER_HEIGHT;
  }
}

//--------------------------------------------------------------------------------------------------

SidebarEntry* SidebarSection::entry_from_point(double x, double y) {
  if (x < 0 || y < SECTION_TOP_SPACING + SECTION_HEADER_HEIGHT + SECTION_HEADER_SPACING || x > get_width() ||
      y > get_height() || _entries.size() == 0)
    return NULL;

  y -= SECTION_TOP_SPACING + SECTION_HEADER_HEIGHT + SECTION_HEADER_SPACING;

  int index = (int)y / (SECTION_ENTRY_HEIGHT + SECTION_ENTRY_SPACING);
  if (index < (int)_entries.size())
    return _entries[index];
  else
    return NULL;
}

//--------------------------------------------------------------------------------------------------

/**
 * Find an entry with the given title and return its index or -1 if there is none.
 */
int SidebarSection::find_entry(const std::string& name) {
  for (size_t i = 0; i < _entries.size(); i++) {
    if (_entries[i]->name() == name)
      return (int)i;
  }

  return -1;
}

//--------------------------------------------------------------------------------------------------

void SidebarSection::toggle_expand() {
  _expanded = !_expanded;
  set_layout_dirty(true);
  set_needs_repaint();
  relayout();

  _expanded_changed(this);
}

//--------------------------------------------------------------------------------------------------

int SidebarSection::add_entry(const std::string& name, const std::string& accessibilityName, const std::string& title,
                              const std::string& icon, TaskEntryType type) {
  int result = find_entry(name);
  if (result > -1)
    return result;

  SidebarEntry* entry = new SidebarEntry(this, name, accessibilityName, title, icon, type, _owner->on_section_command());
  _entries.push_back(entry);
  set_layout_dirty(true);

  return (int)_entries.size() - 1;
}

//--------------------------------------------------------------------------------------------------

void SidebarSection::set_entry_text(int index, const std::string& title) {
  if (index >= 0 && index < (int)_entries.size())
    _entries[index]->set_title(title);
}

//--------------------------------------------------------------------------------------------------

void SidebarSection::set_entry_icon(int index, const std::string& icon) {
  if (index >= 0 && index < (int)_entries.size())
    _entries[index]->set_icon(icon);
}

//--------------------------------------------------------------------------------------------------

void SidebarSection::set_entry_enabled(int index, bool flag) {
  if (index >= 0 && index < (int)_entries.size())
    _entries[index]->set_enabled(flag);
}

//--------------------------------------------------------------------------------------------------

void SidebarSection::mark_busy(bool busy) {
  if (_refresh_button)
    _refresh_button->state = busy;
  set_needs_repaint();
}

//--------------------------------------------------------------------------------------------------

void SidebarSection::remove_entry(const std::string& entry) {
  int index = find_entry(entry);
  if (index < 0)
    return;

  delete _entries[index];
  _entries.erase(_entries.begin() + index);

  set_layout_dirty(true);
}

//--------------------------------------------------------------------------------------------------

void SidebarSection::clear() {
  for (size_t i = 0; i < _entries.size(); i++)
    delete _entries[i];
  _entries.clear();

  set_layout_dirty(true);
}

//--------------------------------------------------------------------------------------------------

bool SidebarSection::select(const std::string& name) {
  const int index = find_entry(name);
  if (index >= 0 && index < (int)_entries.size()) {
    set_selected(_entries[index]);
    return false;
  } else {
    set_selected(NULL);
    return false;
  }
}

//------------------------------------------------------------------------------------------------

size_t SidebarSection::getAccessibilityChildCount() {
  return (int)(_entries.size() + _enabled_buttons.size());
}

//------------------------------------------------------------------------------------------------

Accessible* SidebarSection::getAccessibilityChild(size_t index) {
  base::Accessible* accessible = NULL;

  if ((size_t)index < _enabled_buttons.size())
    accessible = _enabled_buttons[index];
  else
    accessible = _entries[index - _enabled_buttons.size()];

  return accessible;
}

//------------------------------------------------------------------------------------------------

base::Accessible* SidebarSection::accessibilityHitTest(ssize_t x, ssize_t y) {
  base::Accessible* accessible = NULL;

  if (_config_button && _config_button->check_hit(x, y))
    accessible = _config_button;
  else if (_refresh_button && _refresh_button->check_hit(x, y))
    accessible = _refresh_button;
  else
    accessible = entry_from_point(static_cast<double>(x), static_cast<double>(y));

  return accessible;
}

//--------------------------------------------------------------------------------------------------

void draw_header_text(cairo_t* cr, Rect& bounds, const std::string& text, Color color) {
  cairo_set_source_rgba(cr, color.red, color.green, color.blue, color.alpha);
  cairo_move_to(cr, bounds.left(), bounds.top());
  cairo_show_text(cr, text.c_str());
  cairo_stroke(cr);
}

//----------------------------------------------------------------------------------------------------------------------

void SidebarSection::repaint(cairo_t* cr, int areax, int areay, int areaw, int areah) {
  double width = get_width();
  if (_last_width != width) {
    _last_width = width;
    layout(cr);
  }

  Rect bounds(SECTION_SIDE_SPACING, SECTION_TOP_SPACING + SIDEBAR_TITLE_FONT_SIZE, width - SECTION_SIDE_SPACING,
              SECTION_HEADER_HEIGHT);

  cairo_select_font_face(cr, SIDEBAR_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(cr, SIDEBAR_TITLE_FONT_SIZE);
  draw_header_text(cr, bounds, _title, _owner->_inactiveTextColor);

  if (_config_button)
    _config_button->draw(cr);

  if (_refresh_button)
    _refresh_button->draw(cr);

  if (_expand_text_visible) {
    cairo_select_font_face(cr, SIDEBAR_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, SIDEBAR_TITLE_FONT_SIZE);

    std::string expand_text = _expanded ? _("Hide") : _("Show");
    Rect text_bounds = bounds;
    text_bounds.pos.x = width - _expand_text_width - SECTION_SIDE_SPACING;
    text_bounds.size.width = _expand_text_width;
    if (_expand_text_active)
      draw_header_text(cr, text_bounds, expand_text, _owner->_activeTextColor);
    else
      draw_header_text(cr, text_bounds, expand_text, _owner->_inactiveTextColor);
  }

  if (_expanded) {
    bounds.pos.x += SECTION_ENTRY_INDENT;
    bounds.size.width -= SECTION_ENTRY_INDENT;
    bounds.pos.y += SECTION_HEADER_SPACING;
    bounds.size.height = SECTION_ENTRY_HEIGHT;
    const Color& selection_color = _owner->selection_color();
    for (std::vector<SidebarEntry*>::const_iterator iterator = _entries.begin(); iterator != _entries.end();
         iterator++) {
      (*iterator)->paint(cr, bounds, *iterator == _hot_entry, (*iterator == _selected_entry), selection_color);
      bounds.pos.y += SECTION_ENTRY_HEIGHT + SECTION_ENTRY_SPACING;
    }
  }
}

//--------------------------------------------------------------------------------------------------

bool SidebarSection::mouse_leave() {
  if (DrawBox::mouse_leave())
    return true;

  if (_hot_entry != NULL || _expand_text_visible || _expand_text_active || (_config_button && _config_button->hot) ||
      (_refresh_button && _refresh_button->hot)) {
    _hot_entry = NULL;
    _expand_text_visible = false;
    _expand_text_active = false;

    if (_config_button) {
      _config_button->down = false;
      _config_button->hot = false;
    }

    if (_refresh_button) {
      _refresh_button->down = false;
      _refresh_button->hot = false;
    }

    set_needs_repaint();
    return true;
  }

  return false;
}

//--------------------------------------------------------------------------------------------------

bool SidebarSection::mouse_move(mforms::MouseButton button, int x, int y) {
  if (DrawBox::mouse_move(button, x, y))
    return true;

  bool need_refresh = false;

  if (y < SECTION_TOP_SPACING + SECTION_HEADER_HEIGHT) {
    // Header area.
    if (_expandable && !_expand_text_visible) {
      need_refresh = true;
      _expand_text_visible = true;
    }
    if (_hot_entry != NULL) {
      need_refresh = true;
      _hot_entry = NULL;
    }

    // Check buttons.
    if (_config_button) {
      bool isHot = _config_button->check_hit(x, y);
      if (isHot != _config_button->hot) {
        _config_button->hot = isHot;
        need_refresh = true;
      }
    }

    if (_refresh_button) {
      bool isHot = _refresh_button->check_hit(x, y);
      if (isHot != _refresh_button->hot) {
        _refresh_button->hot = isHot;
        need_refresh = true;
      }
    }
  } else {
    if (_expand_text_visible || (_config_button && _config_button->hot) || (_refresh_button && _refresh_button->hot)) {
      _expand_text_visible = false;
      if (_config_button)
        _config_button->hot = false;
      if (_refresh_button)
        _refresh_button->hot = false;
      need_refresh = true;
    }

    SidebarEntry* entry = entry_from_point(x, y);
    if (entry != _hot_entry) {
      if (_hot_entry != NULL || (entry != NULL && (entry->type() == mforms::TaskEntryLink ||
                                                   entry->type() == mforms::TaskEntryAlwaysActiveLink)))
        need_refresh = true;

      if (entry != NULL &&
          (entry->type() == mforms::TaskEntryLink || entry->type() == mforms::TaskEntryAlwaysActiveLink))
        _hot_entry = entry; // Only links can appear as hot entries.
      else
        _hot_entry = NULL;
    }
  }

  if (need_refresh)
    set_needs_repaint();

  return need_refresh;
}

//--------------------------------------------------------------------------------------------------

bool SidebarSection::mouse_down(mforms::MouseButton button, int x, int y) {
  if (DrawBox::mouse_down(button, x, y))
    return true;

  bool result = false;
  if (button == MouseButtonLeft) {
    if (_config_button && _config_button->hot) {
      _config_button->down = true;
      set_needs_repaint();
      result = true;
    } else if (_refresh_button && _refresh_button->hot) {
      _refresh_button->down = true;
      set_needs_repaint();
      result = true;
    } else if (_expand_text_visible) {
      _expand_text_active = true;
      set_needs_repaint();
      result = true;
    } else {
      SidebarEntry* entry = entry_from_point(x, y);
      if (entry && entry->enabled() && entry->type() == mforms::TaskEntrySelectableItem) {
        set_selected(entry);
        result = true;
      }
    }
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

bool SidebarSection::mouse_click(mforms::MouseButton button, int x, int y) {
  if (DrawBox::mouse_click(button, x, y))
    return true;

  bool handled = false;

  switch (button) {
    case MouseButtonLeft: {
      if (_expand_text_active) {
        handled = true;
        toggle_expand();
        _expand_text_active = false;
        set_needs_repaint();
      } else if (_config_button != NULL && _config_button->down) {
        handled = true;
        (*_owner->on_section_command())("configure");
      } else if (_refresh_button != NULL && _refresh_button->down && _refresh_button->icon) {
        handled = true;
        AdvancedSidebar* aSidebar;
        if ((aSidebar = dynamic_cast<AdvancedSidebar*>(_owner)) != NULL)
          aSidebar->tool_action_clicked("refresh");
      }

      if (!handled) {
        SidebarEntry* entry = entry_from_point(x, y);
        if (entry && (entry->enabled() || entry->type() == mforms::TaskEntryAlwaysActiveLink) &&
            ((entry == _hot_entry) || (entry == _selected_entry)))
          entry->execute();
      }
    } break;

    case MouseButtonRight:
      /*
       if (_context_menu != NULL && _selected_link != NULL)
       _context_menu->popup_at(this, x + 5, y + 5);
       */
      break;

    default:
      break;
  }

  return handled;
}

//--------------------------------------------------------------------------------------------------

bool SidebarSection::mouse_up(mforms::MouseButton button, int x, int y) {
  if (DrawBox::mouse_up(button, x, y))
    return true;

  bool result = false;

  switch (button) {
    case MouseButtonLeft:
      if (_config_button && _config_button->down) {
        _config_button->down = false;
        result = true;
      }

      if (_refresh_button && _refresh_button->down) {
        _refresh_button->down = false;
        result = true;
      }

      break;

    default:
      break;
  }

  if (result)
    set_needs_repaint();

  return result;
}

//--------------------------------------------------------------------------------------------------

base::Size SidebarSection::getLayoutSize(base::Size proposedSize) {
  if (is_layout_dirty()) {
    create_context_for_layout();
    layout(_layout_context);
  }

  return base::Size((double)_layout_width, (double)_layout_height);
}

//--------------------------------------------------------------------------------------------------

void SidebarSection::clear_selection() {
  const bool had_selection = _selected_entry != 0;

  set_selected(0);

  if (had_selection)
    set_needs_repaint();
}

//----------------- SimpleSidebar ------------------------------------------------------------------

// implicitly initialize the adv. sidebar
bool SimpleSidebar::__init = init_factory_method();

bool SimpleSidebar::init_factory_method() {
  register_factory("Simple", &create_instance);
  return true;
}

//----------------------------------------------------------------------------------------------------------------------

SimpleSidebar::SimpleSidebar() {
  NotificationCenter::get()->add_observer(this, "GNColorsChanged");
  NotificationCenter::get()->add_observer(this, "GNApplicationActivated");
  NotificationCenter::get()->add_observer(this, "GNApplicationDeactivated");
  updateColors();
}

//----------------------------------------------------------------------------------------------------------------------

SimpleSidebar::~SimpleSidebar() {
  NotificationCenter::get()->remove_observer(this);

  for (size_t i = 0; i < _sections.size(); i++)
    delete _sections[i];
}

//----------------------------------------------------------------------------------------------------------------------

void SimpleSidebar::updateColors() {
  std::string backColor;
  switch (Color::get_active_scheme()) {
    case base::ColorSchemeHighContrast:
    case base::ColorSchemeStandardWin7:
    case base::ColorSchemeStandardWin8:
    case base::ColorSchemeStandardWin8Alternate:
      backColor = base::Color::getApplicationColorAsString(AppColorPanelContentArea, false);
      break;
    default:
      Color systemColor = Color::getSystemColor(base::SystemColor::WindowBackgroundColor);
      backColor = systemColor.to_html();
      break;
  }

  double previousAlpha = _activeTextColor.alpha > 0 ? _activeTextColor.alpha : 1;
  _activeTextColor = Color::getSystemColor(base::SystemColor::SelectedTextColor);
  _activeTextColor.alpha = previousAlpha;
  _inactiveTextColor = Color::getSystemColor(base::SystemColor::LabelColor);
  _inactiveTextColor.alpha = previousAlpha;
  set_back_color(backColor);
  _selection_color = Color::getSystemColor(base::HighlightColor);
}

//----------------------------------------------------------------------------------------------------------------------

mforms::TaskSidebar* SimpleSidebar::create_instance() {
  return new SimpleSidebar();
}

//--------------------------------------------------------------------------------------------------

void SimpleSidebar::handle_notification(const std::string& name, void* sender, NotificationInfo& info) {
  if (name == "GNColorsChanged")
    updateColors();
  else if (name == "GNApplicationActivated") {
    _activeTextColor.alpha = 1;
    _inactiveTextColor.alpha = 1;
  } else if (name == "GNApplicationDeactivated") {
    _activeTextColor.alpha = 0.5;
    _inactiveTextColor.alpha = 0.5;
  }

  set_needs_repaint();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Find a section with the given name and return its index or -1 if there is none.
 */
int SimpleSidebar::find_section(const std::string& name) {
  for (size_t i = 0; i < _sections.size(); i++) {
    if (_sections[i]->getInternalName() == name)
      return (int)i;
  }

  return -1;
}

//--------------------------------------------------------------------------------------------------

int SimpleSidebar::add_section(const std::string& name, const std::string& accessbilityName, const string& title,
  mforms::TaskSectionFlags flags) {

  int result = find_section(title);
  if (result > -1)
    return result;

  SidebarSection* box = new SidebarSection(this, title, flags);
  box->set_name(accessbilityName);
  box->setInternalName(name);
  _sections.push_back(box);
  add(box, false, true);

  return (int)_sections.size() - 1;
}

//--------------------------------------------------------------------------------------------------

void SimpleSidebar::remove_section(const std::string& section_name) {
  int index = find_section(section_name);
  if (index < 0)
    return;

  remove(_sections[index]);
  delete _sections[index];
  _sections.erase(_sections.begin() + index);
}

//--------------------------------------------------------------------------------------------------

int SimpleSidebar::add_section_entry(const std::string& section_name, const std::string& name,
  const std::string& accessibilityName, const std::string& title, const std::string& icon, TaskEntryType type) {
  int index = find_section(section_name);
  if (index < 0)
    return -1;

  return _sections[index]->add_entry(name, accessibilityName, title, icon, type);
}

//--------------------------------------------------------------------------------------------------

void SimpleSidebar::set_section_entry_text(const std::string& section_entry, const std::string& title) {
  for (std::vector<SidebarSection*>::const_iterator section = _sections.begin(); section != _sections.end();
       ++section) {
    int entry_index = (*section)->find_entry(section_entry);
    if (entry_index >= 0 && entry_index < (*section)->entry_count()) {
      (*section)->set_entry_text(entry_index, title);
      break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

void SimpleSidebar::set_section_entry_icon(const std::string& section_entry, const std::string& icon) {
  for (std::vector<SidebarSection*>::const_iterator section = _sections.begin(); section != _sections.end();
       ++section) {
    int entry_index = (*section)->find_entry(section_entry);
    if (entry_index >= 0 && entry_index < (*section)->entry_count()) {
      (*section)->set_entry_icon(entry_index, icon);
      break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

void SimpleSidebar::set_section_entry_enabled(const std::string& section_entry, bool enabled) {
  for (std::vector<SidebarSection*>::const_iterator section = _sections.begin(); section != _sections.end();
       ++section) {
    int entry_index = (*section)->find_entry(section_entry);
    if (entry_index >= 0 && entry_index < (*section)->entry_count()) {
      (*section)->set_entry_enabled(entry_index, enabled);
      break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

void SimpleSidebar::mark_section_busy(const std::string& section, bool busy) {
  int index = find_section(section);
  if (index < 0)
    return;

  _sections[index]->mark_busy(busy);
}

//--------------------------------------------------------------------------------------------------

void SimpleSidebar::remove_section_entry(const std::string& entry_name) {
  for (std::vector<SidebarSection*>::const_iterator section = _sections.begin(); section != _sections.end();
       ++section) {
    int entry_index = (*section)->find_entry(entry_name);
    if (entry_index >= 0 && entry_index < (*section)->entry_count()) {
      (*section)->remove_entry(entry_name);
      break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

void SimpleSidebar::set_collapse_states(const std::string& data) {
  std::vector<std::string> collapsed_sections = base::split(data, ",");
  for (std::vector<std::string>::const_iterator iter = collapsed_sections.begin(); iter != collapsed_sections.end();
       ++iter) {
    int section;
    int state;
    const char* ptr = strrchr(iter->c_str(), '=');
    if (ptr) {
      section = find_section(iter->substr(0, ptr - iter->c_str()));
      if (section < 0)
        continue;

      state = base::atoi<int>(ptr + 1, 0);
    } else
      continue;

    if ((state != 0) == _sections[section]->expanded())
      _sections[section]->toggle_expand();
  }
}

//--------------------------------------------------------------------------------------------------

std::string SimpleSidebar::get_collapse_states() {
  std::string states;
  for (int i = 0; i < (int)_sections.size(); i++) {
    if (i > 0)
      states.append(",");
    states.append(base::strfmt("%s=%i", _sections[i]->getInternalName().c_str(), !_sections[i]->expanded()));
  }
  return states;
}

//--------------------------------------------------------------------------------------------------

void SimpleSidebar::clear_sections() {
  for (size_t i = 0; i < _sections.size(); i++)
    delete _sections[i];
  _sections.clear();

  relayout();
}

//--------------------------------------------------------------------------------------------------

void SimpleSidebar::clear_section(const std::string& section) {
  int index = find_section(section);
  if (index > -1) {
    delete _sections[index];
    _sections.erase(_sections.begin() + index);

    relayout();
  }
}

//--------------------------------------------------------------------------------------------------

void SimpleSidebar::set_selection_color(const std::string& color) {
  _selection_color = Color::parse(color);
  set_needs_repaint();
}

//--------------------------------------------------------------------------------------------------

void SimpleSidebar::set_selection_color(const base::SystemColor color) {
  set_selection_color(Color::getSystemColor(color).to_html());
}

//--------------------------------------------------------------------------------------------------

int SimpleSidebar::select_entry(const std::string& entry_name) {
  int was_selected = 0;

  for (std::vector<SidebarSection*>::const_iterator section = _sections.begin(); section != _sections.end();
       ++section) {
    int entry_index = (*section)->find_entry(entry_name);
    if (entry_index >= 0 && entry_index < (*section)->entry_count()) {
      if ((*section)->select(entry_name))
        was_selected = 1;
      break;
    }
  }

  return was_selected;
}

//--------------------------------------------------------------------------------------------------

std::string SimpleSidebar::selected_entry() {
  for (std::vector<SidebarSection*>::const_iterator section = _sections.begin(); section != _sections.end();
       ++section) {
    SidebarEntry* entry = (*section)->selected();
    if (entry)
      return entry->name();
  }
  return "";
}

//--------------------------------------------------------------------------------------------------

void SimpleSidebar::clear_selection() {
  for (size_t i = 0; i < _sections.size(); i++)
    _sections[i]->clear_selection();
}

//----------------- AdvancedSidebar ----------------------------------------------------------------

// implicitly initialize the adv. sidebar
bool AdvancedSidebar::__init = init_factory_method();

bool AdvancedSidebar::init_factory_method() {
  //  log_debug3("Initializing AdvancedSidebar factory method\n");
  register_factory("SchemaTree", &create_instance);
  return true;
}

//--------------------------------------------------------------------------------------------------

AdvancedSidebar::AdvancedSidebar()
  : _new_schema_tree(TreeNoColumns | TreeNoBorder | TreeSidebar | TreeNoHeader | TreeCanBeDragSource | TreeColumnsAutoResize | TreeTranslucent),
    _filtered_schema_tree(TreeNoColumns | TreeNoBorder | TreeSidebar | TreeNoHeader | TreeCanBeDragSource | TreeColumnsAutoResize | TreeTranslucent),
    _schema_search_box(true),
    _schema_search_text(mforms::SmallSearchEntry),
    _remote_search_enabled(false),
    _schema_search_warning(_("Showing loaded schemas only")),
    _schema_model(0),
    _base_model(0),
    _is_model_owner(false),
    _schema_box(false) {
  _remote_search.set_text(_("Search on Server..."));

  _schema_search_warning.set_style(mforms::SmallHelpTextStyle);
  _schema_search_warning.set_text_align(mforms::MiddleCenter);

  setup_schema_tree();
}

//--------------------------------------------------------------------------------------------------

AdvancedSidebar::~AdvancedSidebar() {
  if (_is_model_owner)
    delete _schema_model;
}

//--------------------------------------------------------------------------------------------------

void AdvancedSidebar::mark_section_busy(const std::string& section, bool busy) {
  if (section.empty()) {
    //    _schema_tree_heading->mark_busy(busy);
    return;
  }
  SimpleSidebar::mark_section_busy(section, busy);
}

//--------------------------------------------------------------------------------------------------

/**
 * Factory method for this control to be used by mforms to create the instance.
 */
mforms::TaskSidebar* AdvancedSidebar::create_instance() {
  return new AdvancedSidebar();
}

//--------------------------------------------------------------------------------------------------

/**
 * Do all the necessary setup for the schema tree (colors, columns and other visual stuff).
 */
void AdvancedSidebar::setup_schema_tree() {
  std::string background_color;
  switch (Color::get_active_scheme()) {
    case base::ColorSchemeHighContrast: // Don't touch the tree back color. We use the one provided by the OS.
      break;

    case base::ColorSchemeStandardWin7:
    case base::ColorSchemeStandardWin8:
    case base::ColorSchemeStandardWin8Alternate:
      background_color = base::Color::getApplicationColorAsString(AppColorPanelContentArea, false);
      break;

    default: {
      Color systemColor = Color::getSystemColor(base::SystemColor::WindowBackgroundColor);
      background_color = systemColor.to_html();
      break;
    }
  }

  _new_schema_tree.set_name("Schema Tree");
  _new_schema_tree.setInternalName("SchemaTree");
  _new_schema_tree.add_column(mforms::IconStringColumnType, _("Schema"), 100, false, true);
  _new_schema_tree.set_selection_mode(mforms::TreeSelectMultiple);
#ifndef __APPLE__
  _new_schema_tree.set_back_color(background_color);
#endif
  scoped_connect(_new_schema_tree.signal_changed(), std::bind(&AdvancedSidebar::on_tree_node_selected, this));

  _filtered_schema_tree.add_column(mforms::IconStringColumnType, _("Schema"), 100, false, true);
  _filtered_schema_tree.set_selection_mode(mforms::TreeSelectMultiple);
#ifndef __APPLE__
  _filtered_schema_tree.set_back_color(background_color);
#endif
  scoped_connect(_filtered_schema_tree.signal_changed(), std::bind(&AdvancedSidebar::on_tree_node_selected, this));

  _new_schema_tree.set_context_menu(&_tree_context_menu);
  _new_schema_tree.end_columns();

  _filtered_schema_tree.set_context_menu(&_tree_context_menu);
  _filtered_schema_tree.end_columns();

  scoped_connect(_tree_context_menu.signal_will_show(),
                 std::bind(&AdvancedSidebar::on_show_menu, this, std::placeholders::_1));

  _schema_search_box.set_back_color(background_color);
  _schema_search_box.set_name("Schema Search Box");
  _schema_search_box.setInternalName("schema-search-box");
  _schema_search_box.set_spacing(5);
#ifdef _MSC_VER
  _schema_search_box.set_padding(4, 0, 6, 5);
  _schema_search_box.set_size(-1, 17); // Height excluding the padding, which adds extra pixels.
#else
  _schema_search_box.set_padding(8, 1, 8, 5);
#endif

#ifdef _MSC_VER
  mforms::ImageBox* image = mforms::manage(new mforms::ImageBox, false);
  image->set_image("search_sidebar.png");
  image->set_image_align(mforms::MiddleCenter);
  _schema_search_box.add(image, false, false);
#endif

  _schema_search_text.set_placeholder_text("Filter objects");
  _schema_search_text.set_tooltip(
    "You can use wildcards to search for objects in multiple schemas.\n"
    "* - a substitue for zero or more characters\n"
    "? - a substitue for single character\n"
    "Search is possible only through already loaded schemas.");
  _schema_search_text.set_name("Schema Filter Entry");
  _schema_search_box.add(&_schema_search_text, true, true);
  scoped_connect(_schema_search_text.signal_changed(),
                 std::bind(&AdvancedSidebar::on_search_text_changed_prepare, this));
  scoped_connect(_remote_search.signal_clicked(), std::bind(&AdvancedSidebar::on_remote_search_clicked, this));

  // Add the tree itself and its section caption to the container.
  _schema_box.set_back_color(background_color);
  _schema_box.add(&_schema_search_box, false, true);
  _schema_box.add(&_new_schema_tree, true, true);
  _schema_box.show(false);
  add_end(&_schema_box, true, true);
}

//--------------------------------------------------------------------------------------------------

void AdvancedSidebar::updateColors() {
  SimpleSidebar::updateColors();

  std::string background_color;
  switch (Color::get_active_scheme()) {
    case base::ColorSchemeHighContrast:
      break;

    case base::ColorSchemeStandardWin7:
    case base::ColorSchemeStandardWin8:
    case base::ColorSchemeStandardWin8Alternate:
      background_color = base::Color::getApplicationColorAsString(AppColorPanelContentArea, false);
      break;

    default:
      Color systemColor = Color::getSystemColor(base::SystemColor::WindowBackgroundColor);
      background_color = systemColor.to_html();
      break;
  }

#ifndef __APPLE__
  _new_schema_tree.set_back_color(background_color);
  _filtered_schema_tree.set_back_color(background_color);
  _schema_box.set_back_color(background_color);
#endif
  _schema_search_box.set_back_color(background_color);
}

//--------------------------------------------------------------------------------------------------

void AdvancedSidebar::on_search_text_changed_prepare() {
  if (_filterTimer)
    bec::GRTManager::get()->cancel_timer(_filterTimer);

  _filterTimer = bec::GRTManager::get()->run_every(std::bind(&AdvancedSidebar::on_search_text_changed, this), 1.0);
}

bool AdvancedSidebar::on_search_text_changed() {
  bec::GRTManager::get()->cancel_timer(_filterTimer);
  _filterTimer = NULL;

  std::string filter = _schema_search_text.get_string_value();

  // Updates the current schema model to filtered/base
  // based on the content of filter.
  if (filter.length() > 0) {
    _filtered_schema_model->set_filter(_schema_search_text.get_string_value());
    _filtered_schema_model->filter_data();

    if (_schema_model == _base_model) {
      //_base_model->filter_data(LiveSchemaTree::Any, _schema_search_text.get_string_value(), *_filtered_schema_model);
      _schema_box.remove(&_new_schema_tree);
      set_schema_model(_filtered_schema_model);
      _schema_box.add(&_filtered_schema_tree, true, true);

      if (_remote_search_enabled)
        _schema_box.add(&_remote_search, false, true);
      else
        _schema_box.add(&_schema_search_warning, false, true);

      _base_model->enable_events(false);
      _filtered_schema_model->enable_events(true);
    }
  } else {
    _schema_box.remove(&_new_schema_tree);
    _schema_box.remove(&_filtered_schema_tree);

    if (_remote_search_enabled)
      _schema_box.remove(&_remote_search);
    else
      _schema_box.remove(&_schema_search_warning);

    set_schema_model(_base_model);
    _schema_box.add(&_new_schema_tree, true, true);
    _base_model->enable_events(true);
    _filtered_schema_model->enable_events(false);
  }

  // Raises a signal indicating the filter has changed
  _search_box_changed_signal(filter);

  return false;
}

void AdvancedSidebar::on_tree_node_selected() {
  _tree_node_selected();
}

//--------------------------------------------------------------------------------------------------

void AdvancedSidebar::on_remote_search_clicked() {
  std::vector<std::string> filter = base::split(_schema_search_text.get_string_value(), ".", 2);
  std::string schema_filter = filter[0];
  std::string object_filter = "";
  if (filter.size() == 2)
    object_filter = filter[1];

  _filtered_schema_model->load_data_for_filter(schema_filter, object_filter);
}

//--------------------------------------------------------------------------------------------------

void AdvancedSidebar::add_items_from_list(mforms::MenuBase& menu, const bec::MenuItemList& items) {
  for (bec::MenuItemList::const_iterator item = items.begin(); item != items.end(); ++item) {
    if (item->type == bec::MenuAction) {
      mforms::MenuItem* mitem =
        menu.add_item_with_title(item->caption, std::bind(&AdvancedSidebar::handle_menu_command, this, item->internalName), "", "");
      mitem->set_name(item->accessibilityName);
      mitem->setInternalName(item->internalName);
      mitem->set_enabled(item->enabled);
    } else if (item->type == bec::MenuSeparator) {
      mforms::MenuItem* mitem = mforms::manage(new mforms::MenuItem("", mforms::SeparatorMenuItem));
      mitem->set_name(item->accessibilityName);
      mitem->setInternalName(item->internalName);
      menu.add_item(mitem);
    } else if (item->type == bec::MenuCascade) {
      mforms::MenuItem* submenu = mforms::manage(new mforms::MenuItem(item->caption));
      submenu->set_name(item->accessibilityName);
      submenu->setInternalName(item->internalName);
      add_items_from_list(*submenu, item->subitems);
      menu.add_submenu(submenu);
      submenu->set_enabled(item->enabled);
    }
  }
}

//--------------------------------------------------------------------------------------------------

void AdvancedSidebar::on_show_menu(mforms::MenuItem* parent_item) {
  if (!parent_item) {
    _tree_context_menu.remove_all();

    std::list<mforms::TreeNodeRef> nodes =
      _schema_model == _base_model ? _new_schema_tree.get_selection() : _filtered_schema_tree.get_selection();

    bec::MenuItemList items = _schema_model->get_popup_items_for_nodes(nodes);
    add_items_from_list(_tree_context_menu, items);
  }
}

//--------------------------------------------------------------------------------------------------

void AdvancedSidebar::handle_menu_command(const std::string& command) {
  std::list<mforms::TreeNodeRef> nodes =
    _schema_model == _base_model ? _new_schema_tree.get_selection() : _filtered_schema_tree.get_selection();

  _schema_model->activate_popup_item_for_nodes(command, nodes);
}

//--------------------------------------------------------------------------------------------------

void AdvancedSidebar::tool_action_clicked(const std::string& action) {
  std::list<mforms::TreeNodeRef> nodes;
  _schema_model->activate_popup_item_for_nodes(action, nodes);
}

//--------------------------------------------------------------------------------------------------

void AdvancedSidebar::set_schema_model(LiveSchemaTree* model) {
  // Sets the given model as the base model if none assigned already
  if (!_base_model) {
    _base_model = model;
    _base_model->set_model_view(&_new_schema_tree);
    _base_model->enable_events(true);
  }

  if (_is_model_owner) {
    delete _schema_model;
    _is_model_owner = false;
  }
  _schema_model = model;
  _schema_box.show(_schema_model != NULL);
}

void AdvancedSidebar::set_filtered_schema_model(wb::LiveSchemaTree* model) {
  _filtered_schema_model = model;
  _filtered_schema_model->set_model_view(&_filtered_schema_tree);
}

//--------------------------------------------------------------------------------------------------

/**
 * Expands the schema node with the given index. A schema node is a top level node.
 */
void AdvancedSidebar::expand_schema(int schema_index) {
  // TODO: _new_schema_tree
  //_schema_tree.set_expanded(schema_index, true);
}
