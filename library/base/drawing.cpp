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

#ifdef __linux__
  #include <stdio.h>
  #include <strings.h>
#endif

#include <vector>
#include <map>

#include "base/drawing.h"
#include "base/string_utilities.h"
#include "base/threading.h"

using namespace base;

typedef struct {
  const char *name;
  unsigned char color[4];
} ColorName;

static ColorName colors[] = {
  {"aliceblue", {240, 248, 255, 255}},
  {"antiquewhite", {250, 235, 215, 255}},
  {"aqua", {0, 255, 255, 255}},
  {"aquamarine", {127, 255, 212, 255}},
  {"azure", {240, 255, 255, 255}},
  {"beige", {245, 245, 220, 255}},
  {"bisque", {255, 228, 196, 255}},
  {"black", {0, 0, 0, 255}},
  {"blanchedalmond", {255, 235, 205, 255}},
  {"blue", {0, 0, 255, 255}},
  {"blueviolet", {138, 43, 226, 255}},
  {"brown", {165, 42, 42, 255}},
  {"burlywood", {222, 184, 135, 255}},
  {"cadetblue", {95, 158, 160, 255}},
  {"chartreuse", {127, 255, 0, 255}},
  {"chocolate", {210, 105, 30, 255}},
  {"coral", {255, 127, 80, 255}},
  {"cornflowerblue", {100, 149, 237, 255}},
  {"cornsilk", {255, 248, 220, 255}},
  {"crimson", {220, 20, 60, 255}},
  {"cyan", {0, 255, 255, 255}},
  {"darkblue", {0, 0, 139, 255}},
  {"darkcyan", {0, 139, 139, 255}},
  {"darkgoldenrod", {184, 134, 11, 255}},
  {"darkgray", {169, 169, 169, 255}},
  {"darkgreen", {0, 100, 0, 255}},
  {"darkgrey", {169, 169, 169, 255}},
  {"darkkhaki", {189, 183, 107, 255}},
  {"darkmagenta", {139, 0, 139, 255}},
  {"darkolivegreen", {85, 107, 47, 255}},
  {"darkorange", {255, 140, 0, 255}},
  {"darkorchid", {153, 50, 204, 255}},
  {"darkred", {139, 0, 0, 255}},
  {"darksalmon", {233, 150, 122, 255}},
  {"darkseagreen", {143, 188, 143, 255}},
  {"darkslateblue", {72, 61, 139, 255}},
  {"darkslategray", {47, 79, 79, 255}},
  {"darkslategrey", {47, 79, 79, 255}},
  {"darkturquoise", {0, 206, 209, 255}},
  {"darkviolet", {148, 0, 211, 255}},
  {"deeppink", {255, 20, 147, 255}},
  {"deepskyblue", {0, 191, 255, 255}},
  {"dimgray", {105, 105, 105, 255}},
  {"dimgrey", {105, 105, 105, 255}},
  {"dodgerblue", {30, 144, 255, 255}},
  {"firebrick", {178, 34, 34, 255}},
  {"floralwhite", {255, 250, 240, 255}},
  {"forestgreen", {34, 139, 34, 255}},
  {"fuchsia", {255, 0, 255, 255}},
  {"gainsboro", {220, 220, 220, 255}},
  {"ghostwhite", {248, 248, 255, 255}},
  {"gold", {255, 215, 0, 255}},
  {"goldenrod", {218, 165, 32, 255}},
  {"gray", {128, 128, 128, 255}},
  {"grey", {128, 128, 128, 255}},
  {"green", {0, 128, 0, 255}},
  {"greenyellow", {173, 255, 47, 255}},
  {"honeydew", {240, 255, 240, 255}},
  {"hotpink", {255, 105, 180, 255}},
  {"indianred", {205, 92, 92, 255}},
  {"indigo", {75, 0, 130, 255}},
  {"ivory", {255, 255, 240, 255}},
  {"khaki", {240, 230, 140, 255}},
  {"lavender", {230, 230, 250, 255}},
  {"lavenderblush", {255, 240, 245, 255}},
  {"lawngreen", {124, 252, 0, 255}},
  {"lemonchiffon", {255, 250, 205, 255}},
  {"lightblue", {173, 216, 230, 255}},
  {"lightcoral", {240, 128, 128, 255}},
  {"lightcyan", {224, 255, 255, 255}},
  {"lightgoldenrodyellow", {250, 250, 210, 255}},
  {"lightgray", {211, 211, 211, 255}},
  {"lightgreen", {144, 238, 144, 255}},
  {"lightgrey", {211, 211, 211, 255}},
  {"lightpink", {255, 182, 193, 255}},
  {"lightsalmon", {255, 160, 122, 255}},
  {"lightseagreen", {32, 178, 170, 255}},
  {"lightskyblue", {135, 206, 250, 255}},
  {"lightslategray", {119, 136, 153, 255}},
  {"lightslategrey", {119, 136, 153, 255}},
  {"lightsteelblue", {176, 196, 222, 255}},
  {"lightyellow", {255, 255, 224, 255}},
  {"lime", {0, 255, 0, 255}},
  {"limegreen", {50, 205, 50, 255}},
  {"linen", {250, 240, 230, 255}},
  {"magenta", {255, 0, 255, 255}},
  {"maroon", {128, 0, 0, 255}},
  {"mediumaquamarine", {102, 205, 170, 255}},
  {"mediumblue", {0, 0, 205, 255}},
  {"mediumorchid", {186, 85, 211, 255}},
  {"mediumpurple", {147, 112, 219, 255}},
  {"mediumseagreen", {60, 179, 113, 255}},
  {"mediumslateblue", {123, 104, 238, 255}},
  {"mediumspringgreen", {0, 250, 154, 255}},
  {"mediumturquoise", {72, 209, 204, 255}},
  {"mediumvioletred", {199, 21, 133, 255}},
  {"midnightblue", {25, 25, 112, 255}},
  {"mintcream", {245, 255, 250, 255}},
  {"mistyrose", {255, 228, 225, 255}},
  {"moccasin", {255, 228, 181, 255}},
  {"navajowhite", {255, 222, 173, 255}},
  {"navy", {0, 0, 128, 255}},
  {"oldlace", {253, 245, 230, 255}},
  {"olive", {128, 128, 0, 255}},
  {"olivedrab", {107, 142, 35, 255}},
  {"orange", {255, 165, 0, 255}},
  {"orangered", {255, 69, 0, 255}},
  {"orchid", {218, 112, 214, 255}},
  {"palegoldenrod", {238, 232, 170, 255}},
  {"palegreen", {152, 251, 152, 255}},
  {"paleturquoise", {175, 238, 238, 255}},
  {"palevioletred", {219, 112, 147, 255}},
  {"papayawhip", {255, 239, 213, 255}},
  {"peachpuff", {255, 218, 185, 255}},
  {"peru", {205, 133, 63, 255}},
  {"pink", {255, 192, 203, 255}},
  {"plum", {221, 160, 221, 255}},
  {"powderblue", {176, 224, 230, 255}},
  {"purple", {128, 0, 128, 255}},
  {"red", {255, 0, 0, 255}},
  {"rosybrown", {188, 143, 143, 255}},
  {"royalblue", {65, 105, 225, 255}},
  {"saddlebrown", {139, 69, 19, 255}},
  {"salmon", {250, 128, 114, 255}},
  {"sandybrown", {244, 164, 96, 255}},
  {"seagreen", {46, 139, 87, 255}},
  {"seashell", {255, 245, 238, 255}},
  {"sienna", {160, 82, 45, 255}},
  {"silver", {192, 192, 192, 255}},
  {"skyblue", {135, 206, 235, 255}},
  {"slateblue", {106, 90, 205, 255}},
  {"slategray", {112, 128, 144, 255}},
  {"slategrey", {112, 128, 144, 255}},
  {"snow", {255, 250, 250, 255}},
  {"springgreen", {0, 255, 127, 255}},
  {"steelblue", {70, 130, 180, 255}},
  {"tan", {210, 180, 140, 255}},
  {"teal", {0, 128, 128, 255}},
  {"thistle", {216, 191, 216, 255}},
  {"tomato", {255, 99, 71, 255}},
  {"turquoise", {64, 224, 208, 255}},
  {"violet", {238, 130, 238, 255}},
  {"wheat", {245, 222, 179, 255}},
  {"white", {255, 255, 255, 255}},
  {"whitesmoke", {245, 245, 245, 255}},
  {"yellow", {255, 255, 0, 255}},
  {"yellowgreen", {154, 205, 50, 255}}
};

//----------------------------------------------------------------------------------------------------------------------

Color::Color() : red(-1), green(-1), blue(-1), alpha(1) {
};

//----------------------------------------------------------------------------------------------------------------------

Color::Color(double ar, double ag, double ab, double aa) : red(ar), green(ag), blue(ab), alpha(aa) {
};

//----------------------------------------------------------------------------------------------------------------------

Color::Color(const HSVColor &hsv) : alpha(hsv.a) {
  int h = hsv.h % 360;
  double s = hsv.s;
  double v = hsv.v;
  int i, f;
  double p, q, t;

  if (s == 0) {
    red = green = blue = v;
    return;
  }
  i = h / 60;
  f = h % 60;
  p = v * (1.0 - s);
  q = v * (1.0 - s * f / 60.0);
  t = v * (1.0 - s * (60.0 - f) / 60.0);

  switch (i) {
    case 0:
      red = v;
      green = t;
      blue = p;
      break;
    case 1:
      red = q;
      green = v;
      blue = p;
      break;
    case 2:
      red = p;
      green = v;
      blue = t;
      break;
    case 3:
      red = p;
      green = q;
      blue = v;
      break;
    case 4:
      red = t;
      green = p;
      blue = v;
      break;
    case 5:
      red = v;
      green = p;
      blue = q;
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------

Color::Color(const std::string &color) : alpha(1) {
  Color col = Color::parse(color);
  if (col.is_valid()) {
    red = col.red;
    green = col.green;
    blue = col.blue;
  } else {
    red = 0;
    green = 0;
    blue = 0;
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool Color::operator!=(const Color &other) {
#define COLOR_EPSILON 0.0001

  return fabs(red - other.red) > COLOR_EPSILON || fabs(green - other.green) > COLOR_EPSILON ||
         fabs(blue - other.blue) > COLOR_EPSILON || fabs(alpha - other.alpha) > COLOR_EPSILON;

#undef COLOR_EPSILON
}

//----------------------------------------------------------------------------------------------------------------------

std::string Color::to_html() const {
  if (is_valid())
    return strfmt("#%02X%02X%02X", static_cast<uint8_t>(red * 255), static_cast<uint8_t>(green * 255),
                  static_cast<uint8_t>(blue * 255));
  return "#000000";
}

//----------------------------------------------------------------------------------------------------------------------

long Color::toRGB() const {
  if (!is_valid())
    return 0;
  return ((long)(red * 255.0) << 16) + ((long)(green * 255.0) << 8) + (long)(blue * 255.0);
}

//----------------------------------------------------------------------------------------------------------------------

long Color::toBGR() const {
  if (!is_valid())
    return 0;
  return ((long)(blue * 255.0) << 16) + ((long)(green * 255.0) << 8) + (long)(red * 255.0);
}

//----------------------------------------------------------------------------------------------------------------------

bool Color::is_valid() const {
  return !(red < 0 || green < 0 || blue < 0 || alpha < 0);
}

//----------------------------------------------------------------------------------------------------------------------

Color Color::invert() const {
  if (is_valid())
    return { 1 - red, 1 - green, 1 - blue, alpha };

  return Color::black();
}

//----------------------------------------------------------------------------------------------------------------------

double Color::brightness() const {
  if (is_valid())
    return 0.2126 * red + 0.7152 * green + 0.0722 * blue; // ITU BT.709

  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

Color Color::brighten(float fraction) const {
  if (is_valid()) {
    double newRed = red + red * fraction;
    if (newRed > 1.0)
      newRed = 1.0;
    else if (newRed < 0.0)
      newRed = 0.0;
    double newGreen = green + green * fraction;
    if (newGreen > 1.0)
      newGreen = 1.0;
    else if (newGreen < 0.0)
      newGreen = 0.0;
    double newBlue = blue + blue * fraction;
    if (newBlue > 1.0)
      newBlue = 1.0;
    else if (newBlue < 0.0)
      newBlue = 0.0;
    return Color(newRed, newGreen, newBlue, alpha);
  }
  return Color();
}

//----------------------------------------------------------------------------------------------------------------------

Color Color::darken(float fraction) const {
  if (is_valid()) {
    double newRed = red - red * fraction;
    if (newRed > 1.0)
      newRed = 1.0;
    else if (newRed < 0.0)
      newRed = 0.0;
    double newGreen = green - green * fraction;
    if (newGreen > 1.0)
      newGreen = 1.0;
    else if (newGreen < 0.0)
      newGreen = 0.0;
    double newBlue = blue - blue * fraction;
    if (newBlue > 1.0)
      newBlue = 1.0;
    else if (newBlue < 0.0)
      newBlue = 0.0;

    return Color( newRed, newGreen, newBlue, alpha);
  }
  return Color();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Parse an HTML color definition into a Color structure.
 */
Color Color::parse(const std::string &color) {
  if (!color.empty()) {
    if (color[0] == '#') {
      int r, g, b;

      // Check first with 3 single values (and only 3), then with 3 double values.
      if (color.size() == 4 && sscanf(color.c_str(), "#%01x%01x%01x", &r, &g, &b) == 3)
        return Color(r * 16 / 255.0, g * 16 / 255.0, b * 16 / 255.0);
      else if (sscanf(color.c_str(), "#%02x%02x%02x", &r, &g, &b) == 3)
        return Color(r / 255.0, g / 255.0, b / 255.0);
    } else {
      for (unsigned int i = 0; i < sizeof(colors) / sizeof(ColorName); i++) {
        if (strcasecmp(colors[i].name, color.c_str()) == 0)
          return Color(colors[i].color[0] / 255.0, colors[i].color[1] / 255.0, colors[i].color[2] / 255.0,
                       colors[i].color[3] / 255.0);
      }
    }
  }
  return black();
}

//----------------------------------------------------------------------------------------------------------------------

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#define MIN3(a, b, c) MIN(MIN(a, b), c)
#define MAX3(a, b, c) MAX(MAX(a, b), c)

HSVColor::HSVColor(const Color &rgb) : a(rgb.alpha) {
  double max = MAX3(rgb.red, rgb.green, rgb.blue);
  double min = MIN3(rgb.red, rgb.green, rgb.blue);

  v = max;

  if (max == 0)
    s = 0;
  else
    s = (max - min) / max;

  if (s == 0)
    h = 0;
  else {
    int rc, gc, bc;

    rc = (int)((max - rgb.red) / (max - min));
    gc = (int)((max - rgb.green) / (max - min));
    bc = (int)((max - rgb.blue) / (max - min));

    if (rgb.red == max) {
      h = ((bc - gc) * 60);
    } else if (rgb.green == max) {
      h = 2 * 60 + ((rc - bc) * 60);
    } else {
      h = 4 * 60 + ((gc - rc) * 60);
    }
    if (h < 0)
      h += 360;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static std::shared_ptr<base::Mutex> color_mutex(new base::Mutex());

static ColorScheme active_scheme =
  ColorSchemeStandard; // Only set when loading the application or by a preferences change.
static bool high_contrast_active = false;

static std::pair<std::string, std::string> custom_colors[] = {
  std::make_pair("", ""), std::make_pair("", ""), std::make_pair("", ""), std::make_pair("", ""),
  std::make_pair("", ""), std::make_pair("", ""), std::make_pair("", ""), std::make_pair("", ""),
  std::make_pair("", ""), std::make_pair("", ""), std::make_pair("", ""),
};

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns one of the predefined application colors.
 */
std::string Color::getApplicationColorAsString(ApplicationColor color, bool foreground) {
  static const std::pair<std::string, std::string> app_colors_win7[] = {
    // Background, foreground.
    std::make_pair("#b2bed1", "#000000"), // AppColorMainTab
    std::make_pair("#283752", ""),        // AppColorMainBackground
    std::make_pair("#496184", "#ffffff"), // AppColorPanelHeader
    std::make_pair("#ffe9b6", "#000000"), // AppColorPanelHeaderFocused
    std::make_pair("#bcc7d8", "#000000"), // AppColorPanelToolbar
    std::make_pair("#ffffff", "#000000"), // AppColorPanelContentArea
    std::make_pair("#283752", "#ffffff"), // AppColorTabUnselected
    std::make_pair("#ffffff", "#000000"), // AppColorBottomTabSelected
    std::make_pair("#ffe9b6", "#000000"), // AppColorTopTabSelectedFocused
    std::make_pair("#4a6184", "#ffffff"), // AppColorTopTabSelectedUnfocused
    std::make_pair("#283752", "#ffffff"), // AppColorStatusbar
  };

  static const std::pair<std::string, std::string> app_colors_win8[] = {
    std::make_pair("#efeff2", "#282828"), std::make_pair("#efeff2", ""),        std::make_pair("#e7e7e8", "#646464"),
    std::make_pair("#0178d0", "#ffffff"), std::make_pair("#f5f5f7", "#282828"), std::make_pair("#ffffff", "#282828"),
    std::make_pair("#e7e7e8", "#282828"), std::make_pair("#ffffff", "#0c6fc2"), std::make_pair("#0178d0", "#ffffff"),
    std::make_pair("#cdd0d6", "#ffffff"), std::make_pair("#efeff2", "#404040"),
  };

  static const std::pair<std::string, std::string> app_colors_win8_alternate[] = {
    std::make_pair("#efeff2", "#282828"), std::make_pair("#679bd3", ""),        std::make_pair("#e7e7e8", "#646464"),
    std::make_pair("#0178d0", "#ffffff"), std::make_pair("#f5f5f7", "#282828"), std::make_pair("#ffffff", "#282828"),
    std::make_pair("#e7e7e8", "#282828"), std::make_pair("#ffffff", "#0c6fc2"), std::make_pair("#0178d0", "#ffffff"),
    std::make_pair("#cdd0d6", "#ffffff"), std::make_pair("#679bd3", "#404040"),
  };

  static const std::pair<std::string, std::string> app_colors_high_contrast[] = {
    // Background, foreground.
    std::make_pair("#ffffff", "#000000"), // AppColorMainTab
    std::make_pair("#808080", ""),        // AppColorMainBackground
    std::make_pair("#ffffff", "#000000"), // AppColorPanelHeader
    std::make_pair("#ffffff", "#000000"), // AppColorPanelHeaderFocused
    std::make_pair("#ffffff", "#000000"), // AppColorPanelToolbar
    std::make_pair("#ffffff", "#000000"), // AppColorPanelContentArea
    std::make_pair("#808080", "#000000"), // AppColorTabUnselected
    std::make_pair("#ffffff", "#000000"), // AppColorBottomTabSelected
    std::make_pair("#000000", "#ffffff"), // AppColorTopTabSelectedFocused
    std::make_pair("#C0C0C0", "#000000"), // AppColorTopTabSelectedUnfocused
    std::make_pair("#808080", "#000000"), // AppColorStatusbar
  };

  base::MutexLock lock(*color_mutex);

  switch (active_scheme) {
    case ColorSchemeCustom:
      if (foreground)
        return custom_colors[color].second;
      else
        return custom_colors[color].first;

    case ColorSchemeStandardWin7:
      if (foreground)
        return app_colors_win7[color].second;
      else
        return app_colors_win7[color].first;

    case ColorSchemeStandardWin8:
      if (foreground)
        return app_colors_win8[color].second;
      else
        return app_colors_win8[color].first;

    case ColorSchemeStandardWin8Alternate:
      if (foreground)
        return app_colors_win8_alternate[color].second;
      else
        return app_colors_win8_alternate[color].first;

    case ColorSchemeHighContrast:
      if (foreground)
        return app_colors_high_contrast[color].second;
      else
        return app_colors_high_contrast[color].first;

    default:
      return "";
  }
}

//----------------------------------------------------------------------------------------------------------------------

Color Color::getApplicationColor(ApplicationColor color, bool foreground) {
  return Color::parse(getApplicationColorAsString(color, foreground));
}

//----------------------------------------------------------------------------------------------------------------------

void Color::set_active_scheme(ColorScheme scheme) {
  base::MutexLock lock(*color_mutex);

  active_scheme = scheme;

  // Cache high contrast setting to avoid having to lock the mutex every time we need to query it.
  high_contrast_active = scheme == ColorSchemeHighContrast;

  // On Windows translate the default scheme to the correct one for the platform.
  if (scheme == ColorSchemeStandard) {
#if defined(_MSC_VER)
    if (IsWindows8OrGreater())
      active_scheme = ColorSchemeStandardWin8;
    else
      active_scheme = ColorSchemeStandardWin7;
#endif
  }
}

//----------------------------------------------------------------------------------------------------------------------

base::ColorScheme base::Color::get_active_scheme() {
  base::MutexLock lock(*color_mutex);

  return active_scheme;
}

//----------------------------------------------------------------------------------------------------------------------

bool Color::is_high_contrast_scheme() {
  return high_contrast_active;
}

//----------------------------------------------------------------------------------------------------------------------

void Color::load_custom_colors(const std::map<std::string, std::string> &colors) {
  static const std::map<std::string, int> app_color_map = {
    {"main-tab", AppColorMainTab},
    {"main", AppColorMainBackground},
    {"panel-header", AppColorPanelHeader},
    {"panel-header-focused", AppColorPanelHeaderFocused},
    {"panel-toolbar", AppColorPanelToolbar},
    {"panel-content", AppColorPanelContentArea},
    {"tab-unselected", AppColorTabUnselected},
    {"bottom-tab-selected", AppColorBottomTabSelected},
    {"top-tab-selected-focused", AppColorTopTabSelectedFocused},
    {"top-selected-unfocused", AppColorTopTabSelectedUnfocused},
    {"statusbar", AppColorStatusbar}
  };

  // Syntax example: "CustomColor:panel-header:fore", "#ffffff".
  for (std::map<std::string, std::string>::const_iterator iterator = colors.begin(); iterator != colors.end();
       iterator++) {
    std::string key = iterator->first;
    std::vector<std::string> parts = base::split(key, ":");
    if (parts.size() < 3 || parts[0] != "CustomColor")
      continue;

    Color color = Color::parse(parts[2]);
    if (!color.is_valid())
      continue;

    std::map<std::string, int>::const_iterator location = app_color_map.find(parts[1]);
    if (location != app_color_map.end()) {
      if (parts[2] == "fore")
        custom_colors[location->second].second = parts[2];
      else
        custom_colors[location->second].first = parts[2];
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void Color::save_custom_colors(std::map<std::string, std::string> &colors) {
  static const std::map<std::string, int> app_color_map = {
    {"main-tab", AppColorMainTab},
    {"main", AppColorMainBackground},
    {"panel-header", AppColorPanelHeader},
    {"panel-header-focused", AppColorPanelHeaderFocused},
    {"panel-toolbar", AppColorPanelToolbar},
    {"panel-content", AppColorPanelContentArea},
    {"tab-unselected", AppColorTabUnselected},
    {"bottom-tab-selected", AppColorBottomTabSelected},
    {"top-tab-selected-focused", AppColorTopTabSelectedFocused},
    {"top-selected-unfocused", AppColorTopTabSelectedUnfocused},
    {"statusbar", AppColorStatusbar}
  };

  colors.clear();
  colors["CustomColor:main-tab:back"] = custom_colors[AppColorMainTab].first;
  colors["CustomColor:main-tab:fore"] = custom_colors[AppColorMainTab].second;
  colors["CustomColor:main:back"] = custom_colors[AppColorMainBackground].first;
  colors["CustomColor:main:fore"] = custom_colors[AppColorMainBackground].second;
  colors["CustomColor:panel-header:back"] = custom_colors[AppColorPanelHeader].first;
  colors["CustomColor:panel-header:fore"] = custom_colors[AppColorPanelHeader].second;
  colors["CustomColor:panel-header-focuse:back"] = custom_colors[AppColorPanelHeaderFocused].first;
  colors["CustomColor:panel-header-focuse:fore"] = custom_colors[AppColorPanelHeaderFocused].second;
  colors["CustomColor:panel-toolbar:back"] = custom_colors[AppColorPanelToolbar].first;
  colors["CustomColor:panel-toolbar:fore"] = custom_colors[AppColorPanelToolbar].second;
  colors["CustomColor:panel-content:back"] = custom_colors[AppColorPanelContentArea].first;
  colors["CustomColor:panel-content:fore"] = custom_colors[AppColorPanelContentArea].second;
  colors["CustomColor:tab-unselected:back"] = custom_colors[AppColorTabUnselected].first;
  colors["CustomColor:tab-unselected:fore"] = custom_colors[AppColorTabUnselected].second;
  colors["CustomColor:bottom-tab-selected:back"] = custom_colors[AppColorBottomTabSelected].first;
  colors["CustomColor:bottom-tab-selected:fore"] = custom_colors[AppColorBottomTabSelected].second;
  colors["CustomColor:top-tab-selected-focused:back"] = custom_colors[AppColorTopTabSelectedFocused].first;
  colors["CustomColor:top-tab-selected-focused:fore"] = custom_colors[AppColorTopTabSelectedFocused].second;
  colors["CustomColor:top-selected-unfocused:back"] = custom_colors[AppColorTopTabSelectedUnfocused].first;
  colors["CustomColor:top-selected-unfocused:fore"] = custom_colors[AppColorTopTabSelectedUnfocused].second;
  colors["CustomColor:statusbar:back"] = custom_colors[AppColorStatusbar].first;
  colors["CustomColor:statusbar:fore"] = custom_colors[AppColorStatusbar].second;
}

//----------------------------------------------------------------------------------------------------------------------
