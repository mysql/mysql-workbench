/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mforms/home_screen_helpers.h"
#include "mforms/utilities.h"
#include "mforms/app.h"

using namespace mforms;

#ifdef __APPLE__
  const char* HomeScreenSettings::HOME_TITLE_FONT = "Helvetica Neue";
  const char* HomeScreenSettings::HOME_NORMAL_FONT = "Helvetica Neue";
  const char* HomeScreenSettings::HOME_DETAILS_FONT = "Helvetica Neue";
  const char* HomeScreenSettings::HOME_INFO_FONT = "Baskerville";
#elif defined(_MSC_VER)
  const char* HomeScreenSettings::HOME_TITLE_FONT = "Segoe UI";
  const char* HomeScreenSettings::HOME_NORMAL_FONT = "Segoe UI";
  const char* HomeScreenSettings::HOME_DETAILS_FONT = "Segoe UI";
#else
  const char* HomeScreenSettings::HOME_TITLE_FONT = "Tahoma";
  const char* HomeScreenSettings::HOME_NORMAL_FONT = "Tahoma";
  const char* HomeScreenSettings::HOME_DETAILS_FONT = "Helvetica";
#endif
const char* HomeScreenSettings::TILE_DRAG_FORMAT = "com.mysql.workbench-drag-tile-format";

//--------------------------------------------------------------------------------------------------

base::any getAnyMapValue(const anyMap& map, const std::string& key, base::any defaultValue) {
  anyMap::const_iterator iter = map.find(key);

  if (iter == map.end())
    return defaultValue;

  return iter->second;
}

//--------------------------------------------------------------------------------------------------

std::string HomeAccessibleButton::getAccessibilityTitle() {
  return title;
}

//--------------------------------------------------------------------------------------------------

std::string HomeAccessibleButton::getAccessibilityDescription() {
  return description;
}

//--------------------------------------------------------------------------------------------------

base::Accessible::Role HomeAccessibleButton::getAccessibilityRole() {
  return base::Accessible::PushButton;
}

//--------------------------------------------------------------------------------------------------

base::Rect HomeAccessibleButton::getAccessibilityBounds() {
  return bounds;
}

//--------------------------------------------------------------------------------------------------

void HomeAccessibleButton::accessibilityDoDefaultAction() {
  if (defaultHandler)
    defaultHandler();
}

//--------------------------------------------------------------------------------------------------

// The following helpers are just temporary. They will be replaced by a cairo context class.

int mforms::imageWidth(cairo_surface_t* image) {
  if (image != nullptr) {
    if (Utilities::is_hidpi_icon(image) && App::get()->backing_scale_factor() > 1.0)
      return (int)(cairo_image_surface_get_width(image) / App::get()->backing_scale_factor());
    else
      return (int)cairo_image_surface_get_width(image);
  }
  return 0;
}

//--------------------------------------------------------------------------------------------------

int mforms::imageHeight(cairo_surface_t* image) {
  if (image != nullptr) {
    if (Utilities::is_hidpi_icon(image) && App::get()->backing_scale_factor() > 1.0)
      return (int)(cairo_image_surface_get_height(image) / App::get()->backing_scale_factor());
    else
      return (int)cairo_image_surface_get_height(image);
  }
  return 0;
}

//--------------------------------------------------------------------------------------------------

/**
 * Helper to draw text with a hot decoration.
 */
void mforms::textWithDecoration(cairo_t* cr, double x, double y, const char* text, bool hot, double width) {
  cairo_move_to(cr, x, y);
  cairo_show_text(cr, text);
  cairo_stroke(cr);

  // TODO: replace this with font decoration once pango is incorporated.
  if (hot) {
    cairo_set_line_width(cr, 1);
    cairo_move_to(cr, x, (int)y + 2.5);
    cairo_line_to(cr, x + width, (int)y + 2.5);
    cairo_stroke(cr);
  }
}

//--------------------------------------------------------------------------------------------------
