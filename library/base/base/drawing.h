/* 
 * Copyright (c) 2011, 2013, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _DRAWING_H_
#define _DRAWING_H_

/**
 * Definitions used for cross-platform drawing.
 */

#ifndef SWIG
#include "common.h"
#endif

#include <map>

#ifdef _WIN32
  #define DEFAULT_FONT_FAMILY "Tahoma"
  #define DEFAULT_FONT_SIZE 11
  
  #define DEFAULT_SMALL_FONT "Modern"
  #define DEFAULT_MONOSPACE_FONT_FAMILY "Consolas"
  #define DEFAULT_MONOSPACE_FONT_FAMILY_ALT "Lucida Console"
#elif defined(__APPLE__)
  #define DEFAULT_FONT_FAMILY "Helvetica"
  #define DEFAULT_FONT_SIZE 11

  #define DEFAULT_MONOSPACE_FONT_FAMILY "AndaleMono"
#else
  #define DEFAULT_FONT_FAMILY "Helvetica"
  #define DEFAULT_FONT_SIZE 11

  #define DEFAULT_MONOSPACE_FONT_FAMILY "Bitstream Vera Sans Mono"
#endif

namespace base {

  enum ColorScheme
  {
    ColorSchemeStandard,
    ColorSchemeStandardWin7,
    ColorSchemeStandardWin8,
    ColorSchemeStandardWin8Alternate,
    ColorSchemeHighContrast,
    ColorSchemeCustom = 128,
  };

  enum ApplicationColor
  {
    AppColorMainTab = 0,
    AppColorMainBackground,
    AppColorPanelHeader,
    AppColorPanelHeaderFocused,
    AppColorPanelToolbar,
    AppColorPanelContentArea,
    AppColorTabUnselected,           // For both, top and bottom style.
                                     // Back color also used for uncovered tab area.
    AppColorBottomTabSelected,       // No focused/unfocused style.
    AppColorTopTabSelectedFocused,
    AppColorTopTabSelectedUnfocused,
    AppColorStatusbar,
  };

  struct HSVColor;

  class BASELIBRARY_PUBLIC_FUNC Color
  {
  public:
    double red, green, blue, alpha;

    Color();
    Color(double ar, double ag, double ab, double aa = 1.0);
    Color(const HSVColor &hsv);
    Color(const std::string &color);

    bool operator !=(const Color &other);
    std::string to_html() const;
    bool is_valid() const;

    static Color parse(const std::string &color);

    static inline Color Black() { return Color(0, 0, 0); }
    static inline Color White() { return Color(1, 1, 1); }
    static inline Color Invalid() { return Color(-1, -1, -1); }

    static Color get_application_color(ApplicationColor color, bool foreground);
    static std::string get_application_color_as_string(ApplicationColor color, bool foreground);
    static void set_active_scheme(ColorScheme scheme);
    static ColorScheme get_active_scheme();
    static bool is_high_contrast_scheme();

    // Persistence support. Also called when colors were changed in preferences.
    static void load_custom_colors(const std::map<std::string, std::string> &colors);
    static void save_custom_colors(std::map<std::string, std::string> &colors);
  };


  struct BASELIBRARY_PUBLIC_FUNC HSVColor
  {
    int h; // 0 ~ 360
    double s, v, a; // 0 ~ 1.0

    HSVColor() : h(0), s(0), v(0), a(1) {};
    HSVColor(int ah, double as, double av, double aa=1.0) : h(ah), s(as), v(av), a(aa) {};
    HSVColor(const Color &rgb);
  };

} // namespace base

#endif // _DRAWING_H_
