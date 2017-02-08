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

#include "base/drawing.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#include <gtkmm.h>
#pragma GCC diagnostic pop

using namespace base;

static bool inTesting = false;

std::string OSConstants::defaultFontName() {
  auto settings = Gtk::Settings::get_default();
  std::string fontName = settings->property_gtk_font_name().get_value();
  auto pangoFontDescription = pango_font_description_from_string(fontName.c_str());
  return std::string(pango_font_description_get_family(pangoFontDescription));
}

//----------------------------------------------------------------------------------------------------------------------

float OSConstants::systemFontSize() {
  auto settings = Gtk::Settings::get_default();
  std::string fontName = settings->property_gtk_font_name().get_value();
  auto pangoFontDescription = pango_font_description_from_string(fontName.c_str());
  return pango_font_description_get_size(pangoFontDescription);
}

//----------------------------------------------------------------------------------------------------------------------

float OSConstants::smallSystemFontSize() {
  return OSConstants::systemFontSize() - 2;
}

//----------------------------------------------------------------------------------------------------------------------

float OSConstants::labelFontSize() {
  return OSConstants::systemFontSize();
}

//----------------------------------------------------------------------------------------------------------------------

static base::Color rgba_color_to_mforms(const Gdk::RGBA& c) {
  return base::Color(c.get_red(), c.get_green(), c.get_blue(), c.get_alpha());
}

//----------------------------------------------------------------------------------------------------------------------

// TODO: implement all the missing SystemColors
base::Color Color::getSystemColor(base::SystemColor type) {
  typedef std::map<base::SystemColor, base::Color> Colors;
  static Colors colors;

  base::Color ret;
  if (inTesting)
    return ret;

  switch (type) {
    case base::SystemColor::HighlightColor: {
      Colors::const_iterator it = colors.find(type);
      if (it != colors.end())
        ret = it->second;
      else {
        Gtk::Entry e;
        auto styleCtx = e.get_style_context();
        base::Color new_color(rgba_color_to_mforms(styleCtx->get_color(Gtk::STATE_FLAG_SELECTED)));
        colors[type] = new_color;
        ret = new_color;
      }
      break;
    }
    case base::SystemColor::TextBackgroundColor: {
      Colors::const_iterator it = colors.find(type);
      if (it != colors.end())
        ret = it->second;
      else {
        Gtk::Entry e;
        auto styleCtx = e.get_style_context();
        ret = base::Color(rgba_color_to_mforms(styleCtx->get_background_color(Gtk::STATE_FLAG_NORMAL)));
        colors[type] = ret;
      }
      break;
    }
    case base::SystemColor::WindowBackgroundColor: {
      Colors::const_iterator it = colors.find(type);
      if (it != colors.end())
        ret = it->second;
      else {
        Gtk::Window wnd;
        auto ctx = wnd.get_style_context();
        ret = base::Color(rgba_color_to_mforms(ctx->get_background_color(Gtk::STATE_FLAG_NORMAL)));
        colors[type] = ret;
      }
      break;
    }
    default: {
      Colors::const_iterator it = colors.find(type);
      if (it != colors.end())
        ret = it->second;
      else {
        Gtk::Entry e;
        auto styleCtx = e.get_style_context();
        ret = base::Color(rgba_color_to_mforms(styleCtx->get_color(Gtk::STATE_FLAG_NORMAL)));
        colors[type] = ret;
      }
      break;
    }
  }

  return ret;
}

//----------------------------------------------------------------------------------------------------------------------

void Color::prepareForTesting() {
  inTesting = true;
}
