/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "mforms/home_screen_launchers.h"
#include "base/string_utilities.h"
#include "base/file_utilities.h"
#include "base/log.h"
#include "mforms/app.h"

#include "mforms/home_screen.h"
DEFAULT_LOG_DOMAIN("home screen launchers")

using namespace mforms;

//----------------- LauncherEntry ---------------------------------------------------------------

bool LauncherEntry::operator<(const LauncherEntry &other) const {
  return base::string_compare(other.title, title) != 0;
}

//------------------------------------------------------------------------------------------------

std::string LauncherEntry::get_acc_name() {
  return title;
}

//------------------------------------------------------------------------------------------------

std::string LauncherEntry::get_acc_description() {
  return description;
}

//------------------------------------------------------------------------------------------------

mforms::Accessible::Role LauncherEntry::get_acc_role() {
  return Accessible::ListItem;
}

//------------------------------------------------------------------------------------------------

base::Rect LauncherEntry::get_acc_bounds() {
  return bounds;
}

//------------------------------------------------------------------------------------------------

std::string LauncherEntry::get_acc_default_action() {
  return "Open";
}

//------------------------------------------------------------------------------------------------

LauncherEntry::LauncherEntry() : icon(nullptr) {
}

//------------------------------------------------------------------------------------------------

LauncherEntry::LauncherEntry(const LauncherEntry &other) {
  title = other.title;
  title_shorted = other.title_shorted;
  description = other.description;
  descriptionLines = other.descriptionLines;
  bounds = other.bounds;
  object = other.object;
  icon = other.icon;
  cairo_surface_reference(icon);
}

//------------------------------------------------------------------------------------------------

LauncherEntry &LauncherEntry::operator=(LauncherEntry &&other) {
  title = std::move(other.title);
  title_shorted = std::move(other.title_shorted);
  description = std::move(other.description);
  descriptionLines = std::move(other.descriptionLines);
  bounds = std::move(other.bounds);
  object = std::move(other.object);
  icon = other.icon;
  other.icon = nullptr;
  return *this;
}

LauncherEntry::~LauncherEntry() {
  deleteSurface(icon);
  icon = nullptr;
}

//----------------- LaunchersSection ---------------------------------------------------------------

LaunchersSection::LaunchersSection(mforms::HomeScreen *owner) : HomeScreenSection("sidebar_launchers.png") {
  _owner = owner;
  _context_menu = NULL;
  _action_menu = NULL;
  _hot_entry = -1;
  _active_entry = -1;
  _entries_per_row = 0;
}

//------------------------------------------------------------------------------------------------

LaunchersSection::~LaunchersSection() {
  if (_context_menu != NULL)
    _context_menu->release();
}

//------------------------------------------------------------------------------------------------

std::size_t LaunchersSection::entry_from_point(int x, int y) {
  int width = get_width();
  if (x < LAUNCHERS_LEFT_PADDING || x > (width - LAUNCHERS_RIGHT_PADDING) || y < LAUNCHERS_TOP_PADDING)
    return -1; // Outside the entries area.

  x -= LAUNCHERS_LEFT_PADDING;

  y -= LAUNCHERS_TOP_PADDING;
  if ((y % (LAUNCHERS_ENTRY_HEIGHT + LAUNCHERS_VERTICAL_SPACING)) > LAUNCHERS_ENTRY_HEIGHT)
    return -1; // Within the vertical spacing between two entries.

  width -= LAUNCHERS_LEFT_PADDING + LAUNCHERS_RIGHT_PADDING;
  _entries_per_row = width / LAUNCHERS_ENTRY_WIDTH;
  if (x >= _entries_per_row * LAUNCHERS_ENTRY_WIDTH)
    return -1; // After the last entry in a row.

  int height = get_height() - LAUNCHERS_TOP_PADDING;
  int column = x / LAUNCHERS_ENTRY_WIDTH;
  int row = y / (LAUNCHERS_ENTRY_HEIGHT + LAUNCHERS_VERTICAL_SPACING);

  int row_bottom = row * (LAUNCHERS_ENTRY_HEIGHT + LAUNCHERS_VERTICAL_SPACING) + LAUNCHERS_ENTRY_HEIGHT;
  if (row_bottom > height)
    return -1; // The last visible row is dimmed if not fully visible. So take it out from hit tests too.

  std::size_t count = _launchers.size();
  std::size_t index = row * _entries_per_row + column;
  if (index < count)
    return index;

  return -1;
}

//------------------------------------------------------------------------------------------------

void LaunchersSection::drawEntry(cairo_t *cr, const LauncherEntry &entry, bool hot) {
  mforms::Utilities::paint_icon(cr, entry.icon, entry.bounds.left(), entry.bounds.top());

  base::Size iconSize = mforms::Utilities::getImageSize(entry.icon);

  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_select_font_face(cr, mforms::HomeScreenSettings::HOME_NORMAL_FONT, CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_SUBTITLE_FONT_SIZE);
  int x = (int)entry.bounds.left() + (int)iconSize.width + 10;
  int y = (int)entry.bounds.top() + 18;
  cairo_text_extents_t headingExtents;
  cairo_text_extents(cr, entry.title.c_str(), &headingExtents);
  if (hot) {
    double width = 0;
    width = ceil(headingExtents.width);

    cairo_save(cr);
    cairo_set_source_rgb(cr, 1, 1, 1);
    textWithDecoration(cr, x - 1, y, entry.title.c_str(), true, width);
    textWithDecoration(cr, x + 1, y, entry.title.c_str(), true, width);
    textWithDecoration(cr, x, y - 1, entry.title.c_str(), true, width);
    textWithDecoration(cr, x, y + 1, entry.title.c_str(), true, width);
    cairo_restore(cr);

    textWithDecoration(cr, x, y, entry.title.c_str(), true, width);
  } else
    textWithDecoration(cr, x, y, entry.title_shorted.c_str(), false, 0);

  cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_SMALL_INFO_FONT_SIZE);

  if (!entry.description.empty()) {
    cairo_text_extents_t extents;
    cairo_text_extents(cr, entry.description.c_str(), &extents);

    cairo_set_source_rgb(cr, 0.41, 0.41, 0.41);

    y += (int)headingExtents.height;
    cairo_move_to(cr, x, y);

    for (auto &line : entry.descriptionLines) {
      cairo_show_text(cr, line.c_str());
      y += (int)extents.height + 2;
      cairo_move_to(cr, x, y);
    }

    cairo_stroke(cr);
  }
}

//------------------------------------------------------------------------------------------------

void LaunchersSection::layout(cairo_t *cr) {
  if (is_layout_dirty()) {
    set_layout_dirty(false);

    cairo_text_extents_t extents;

    // Keep in mind text rectangles are flipped (top is actually the base line of the text).
    double heading_left = LAUNCHERS_LEFT_PADDING;
    cairo_text_extents(cr, _("Shortcuts"), &extents);
    double text_width = ceil(extents.width);
    _launcher_heading_rect = base::Rect(heading_left, LAUNCHERS_TOP_BASELINE, text_width, ceil(extents.height));

    // Compute the shorted strings.
    cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_SUBTITLE_FONT_SIZE);

    for (std::vector<LauncherEntry>::iterator iterator = _launchers.begin(); iterator != _launchers.end(); iterator++) {
      double details_width = LAUNCHERS_ENTRY_WIDTH - 10 - imageWidth(iterator->icon);
      if (iterator->title_shorted.empty() && !iterator->title.empty())
        iterator->title_shorted = mforms::Utilities::shorten_string(cr, iterator->title, details_width);
    }
  }
}

//------------------------------------------------------------------------------------------------

void LaunchersSection::cancelOperation() {
  // pass
}

//------------------------------------------------------------------------------------------------

void LaunchersSection::setFocus() {
  // pass
}

//------------------------------------------------------------------------------------------------

bool LaunchersSection::canHandle(HomeScreenMenuType type) {
  return false;
}

//------------------------------------------------------------------------------------------------

void LaunchersSection::setContextMenu(mforms::Menu *menu, HomeScreenMenuType type) {
  // pass
}

//------------------------------------------------------------------------------------------------

void LaunchersSection::setContextMenuAction(mforms::Menu *menu, HomeScreenMenuType type) {
  // pass
}

//------------------------------------------------------------------------------------------------

void LaunchersSection::repaint(cairo_t *cr, int areax, int areay, int areaw, int areah) {
  int width = get_width();
  int height = get_height();

  cairo_set_line_width(cr, 1);
  cairo_select_font_face(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT, CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT_SIZE);

  layout(cr);

  width -= LAUNCHERS_LEFT_PADDING + LAUNCHERS_RIGHT_PADDING;
  cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT_SIZE);
  int entries_per_row = width / LAUNCHERS_ENTRY_WIDTH;

  // Heading for switching display mode. Draw heading hot only when we support more sections.
  cairo_set_source_rgb(cr, 0, 0, 0);
  textWithDecoration(cr, _launcher_heading_rect.left(), _launcher_heading_rect.top(), _("Shortcuts"),
                     false /*_hot_heading == ModelsOnly*/, _launcher_heading_rect.width());

  //  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  int row = 0;
  base::Rect bounds(0, LAUNCHERS_TOP_PADDING, LAUNCHERS_ENTRY_WIDTH, LAUNCHERS_ENTRY_HEIGHT);
  bool done = false;
  while (!done) {
    bool drawHotEntry = false;
    bounds.pos.x = LAUNCHERS_LEFT_PADDING;
    for (int column = 0; column < entries_per_row; column++) {
      std::size_t index = row * entries_per_row + column;
      if (index >= _launchers.size()) {
        done = true;
        break;
      } else {
        _launchers[index].bounds = bounds;
        if ((std::size_t)_hot_entry == index)
          drawHotEntry = true;
        else
          drawEntry(cr, _launchers[index], (std::size_t)_hot_entry == index);
      }
      bounds.pos.x += LAUNCHERS_ENTRY_WIDTH;
    }
    if (drawHotEntry)
      drawEntry(cr, _launchers[_hot_entry], true);

    row++;
    bounds.pos.y += LAUNCHERS_ENTRY_HEIGHT + LAUNCHERS_VERTICAL_SPACING;
    if (bounds.top() >= height)
      done = true;
  }
}

//------------------------------------------------------------------------------------------------

void LaunchersSection::addLauncher(const std::string &icon, const std::string &name, const std::string &description,
                                   const base::any &obj) {
  LauncherEntry entry;
  entry.title = name;
  entry.description = description;
  entry.descriptionLines = base::split(base::reflow_text(entry.description, 29, "", false, 2), "\n");

  entry.object = obj;
  if (!icon.empty())
    entry.icon = mforms::Utilities::load_icon(icon);

  if (entry.icon == nullptr) {
    logWarning("Unable to load icon: %s, using placeholder instead\n", icon.c_str());
    entry.icon = mforms::Utilities::load_icon("wb_starter_generic_52.png", true);
  }

  _launchers.push_back(entry);
  set_layout_dirty(true);
}

//------------------------------------------------------------------------------------------------

void LaunchersSection::clearLaunchers() {
  _launchers.clear();
  set_layout_dirty(true);
}

//--------------------------------------------------------------------------------------------------

bool LaunchersSection::mouse_double_click(mforms::MouseButton button, int x, int y) {
  // Similar handling like for single mouse down.
  switch (button) {
    case mforms::MouseButtonLeft: {
      if (_launcher_heading_rect.contains_flipped(x, y))
        return true;

      // Anything else.
      _active_entry = entry_from_point(x, y);
      if (_active_entry > -1) {
        _owner->cancelOperation();
        _owner->trigger_callback(HomeScreenAction::ActionShortcut, _launchers[_active_entry].object);
        return true;
      }
    } break;

    case mforms::MouseButtonRight:
      break;

    default:
      break;
  }

  return false;
}

//--------------------------------------------------------------------------------------------------

bool LaunchersSection::mouse_click(mforms::MouseButton button, int x, int y) {
  switch (button) {
    case mforms::MouseButtonLeft: {
      if (_launcher_heading_rect.contains_flipped(x, y)) {
        _owner->cancelOperation();
        return true;
      }

      // Anything else.
      _active_entry = entry_from_point(x, y);
      if (_active_entry > -1) {
        _owner->cancelOperation();
        _owner->trigger_callback(HomeScreenAction::ActionShortcut, _launchers[_active_entry].object);
        return true;
      }
    } break;

    case mforms::MouseButtonRight:
      break;

    default:
      break;
  }

  return false;
}

//--------------------------------------------------------------------------------------------------

bool LaunchersSection::mouse_leave() {
  return false;
}

//--------------------------------------------------------------------------------------------------

bool LaunchersSection::mouse_move(mforms::MouseButton button, int x, int y) {
  bool result = false;
  ssize_t entry = entry_from_point(x, y);
  if (entry != _hot_entry) {
    _hot_entry = entry;
    result = true;
  }

  if (result)
    set_needs_repaint();

  return result;
}

//------------------------------------------------------------------------------------------------

void LaunchersSection::handle_command(const std::string &command) {
  _owner->handleContextMenu(base::any(), command);
  _active_entry = -1;
}

//------------------------------------------------------------------------------------------------

int LaunchersSection::get_acc_child_count() {
  // Initial value due to the add/open/create EER Model icons
  int ret_val = 3;
  ret_val += (int)_launchers.size();

  return ret_val;
}

//------------------------------------------------------------------------------------------------

mforms::Accessible *LaunchersSection::get_acc_child(int index) {
  mforms::Accessible *accessible = NULL;
  switch (index) {
    case 0:
      break;
    case 1:
      break;
    case 2:
      break;
    default: {
      index -= 3;

      if (index < (int)_launchers.size())
        accessible = &_launchers[index];
    }
  }

  return accessible;
}

//------------------------------------------------------------------------------------------------

mforms::Accessible::Role LaunchersSection::get_acc_role() {
  return Accessible::List;
}

//------------------------------------------------------------------------------------------------

mforms::Accessible *LaunchersSection::hit_test(int x, int y) {
  mforms::Accessible *accessible = NULL;

  {
    ssize_t entry = entry_from_point(x, y);

    if (entry != -1)
      accessible = &_launchers[entry];
  }

  return accessible;
}
