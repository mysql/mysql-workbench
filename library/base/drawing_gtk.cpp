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

#include "base/drawing.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#pragma GCC diagnostic ignored "-Wparentheses"
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
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
    case base::SystemColor::TextColor: {
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
    case base::SystemColor::TextBackgroundColor: {
      Colors::const_iterator it = colors.find(type);
      if (it != colors.end())
        ret = it->second;
      else {
        // On Linux bg can be a gradient, so there's no realiable way of finding out the proper color.
        // Instead we will hardcode the values.
        ret = base::Color(1.0, 1.0, 1.0);
        colors[type] = ret;
      }
      break;
    }
    case base::SystemColor::DisabledControlTextColor: {
      Colors::const_iterator it = colors.find(type);
      if (it != colors.end())
        ret = it->second;
      else {
        Gtk::Entry e;
        auto styleCtx = e.get_style_context();
        ret = base::Color(rgba_color_to_mforms(styleCtx->get_color(Gtk::STATE_FLAG_INSENSITIVE)));
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
    case base::SystemColor::SelectedControlColor:
    case base::SystemColor::SelectedTextBackgroundColor: {
      Colors::const_iterator it = colors.find(type);
      if (it != colors.end())
        ret = it->second;
      else {
        if (it != colors.end())
          ret = it->second;
        else {
          // On Linux bg can be a gradient, so there's no realiable way of finding out the proper color.
          // Instead we will hardcode the values.
          ret = base::Color("#97c1ed");
          colors[type] = ret;
        }
      }
      break;
    }
    case base::SystemColor::SelectedTextColor: {
      Colors::const_iterator it = colors.find(type);
      if (it != colors.end())
        ret = it->second;
      else {
        Gtk::Label e;
        auto styleCtx = e.get_style_context();
        ret = base::Color(rgba_color_to_mforms(styleCtx->get_color(Gtk::STATE_FLAG_SELECTED)));
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
