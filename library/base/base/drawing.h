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

#pragma once

/**
 * Definitions used for cross-platform drawing.
 */

#ifndef SWIG
#include "common.h"
#endif

#include <map>

#ifdef _MSC_VER
  #define DEFAULT_FONT_FAMILY "Tahoma"
  #define DEFAULT_FONT_SIZE 11

  #define DEFAULT_SMALL_FONT "Modern"
  #define DEFAULT_MONOSPACE_FONT_FAMILY "Consolas"
  #define DEFAULT_MONOSPACE_FONT_FAMILY_ALT "Lucida Console"

  #define DETAILS_FONT_FAMILIY "Arial"
#elif defined(__APPLE__)
  #define DEFAULT_FONT_FAMILY "Helvetica"
  #define DEFAULT_FONT_SIZE 11
  #define DEFAULT_SMALL_FONT "Helvetica"
  #define DEFAULT_MONOSPACE_FONT_FAMILY "Menlo"

  #define DETAILS_FONT_FAMILIY "Lucida Grande"
#else
  #define DEFAULT_FONT_FAMILY "Helvetica"
  #define DEFAULT_FONT_SIZE 11
  #define DEFAULT_SMALL_FONT "Sans"
  #define DEFAULT_MONOSPACE_FONT_FAMILY "Bitstream Vera Sans Mono"

  #define DETAILS_FONT_FAMILIY "Helvetica"
#endif

namespace base {

  enum ColorScheme {
    ColorSchemeStandard,
    ColorSchemeStandardWin7,
    ColorSchemeStandardWin8,
    ColorSchemeStandardWin8Alternate,
    ColorSchemeHighContrast,
    ColorSchemeCustom = 128,
  };

  enum ApplicationColor {
    AppColorMainTab = 0,
    AppColorMainBackground,
    AppColorPanelHeader,
    AppColorPanelHeaderFocused,
    AppColorPanelToolbar,
    AppColorPanelContentArea,
    AppColorTabUnselected,     // For both, top and bottom style.
                               // Back color also used for uncovered tab area.
    AppColorBottomTabSelected, // No focused/unfocused style.
    AppColorTopTabSelectedFocused,
    AppColorTopTabSelectedUnfocused,
    AppColorStatusbar,
  };

  // The platform specific type of color to return (label, window, control etc.). Types not supported directly by a
  // platform should return a supported type that is closest.
  enum SystemColor {
    ControlShadowColor,            // Dark border for controls
    ControlDarkShadowColor,        // Darker border for controls
    ControlColor,                  // Control face and old window background color
    ControlHighlightColor,         // Light border for controls
    ControlLightHighlightColor,    // Lighter border for controls
    ControlTextColor,              // Text on controls
    ControlBackgroundColor,        // Background of large controls (browser and the like)
    SelectedControlColor,          // Control face for selected controls
    SecondarySelectedControlColor, // Color for selected controls when control is not active (that is, not focused)
    SelectedControlTextColor,      // Text on selected controls
    DisabledControlTextColor,      // Text on disabled controls

    TextColor,                     // Document text
    TextBackgroundColor,           // Document text background
    LabelColor,                    // Foreground color for static text and related elements
    SecondaryLabelColor,           // Foreground color for secondary static text and related elements
    TertiaryLabelColor,            // Foreground color for disabled static text and related elements
    QuaternaryLabelColor,          // Foreground color for large secondary or disabled static text, separators, large glyphs/icons, etc
    SelectedTextColor,             // Selected document text
    SelectedTextBackgroundColor,   // Selected document text background
    GridColor,                     // Grids in controls

    WindowBackgroundColor,         // Background fill for window contents
    WindowFrameColor,              // Window frames
    WindowFrameTextColor,          // Text on window frames
    SecondaryBackgroundColor,      // Sidebars and similar.

    SelectedMenuItemColor,     // Highlight color for menus
    SelectedMenuItemTextColor, // Highlight color for menu text

    HighlightColor, // Highlight color for UI elements (this is abstract and defines the color all highlights tend
                    // toward)

    HeaderColor,     // Background color for headers in treeviews/grids.
    HeaderTextColor, // Text color for headers in treeview/grids.

    AlternateSelectedControlColor,     // Similar to SelectedControlColor for use in lists and treeviews.
    AlternateSelectedControlTextColor, // Similar to SelectedControlTextColor.
  };

  struct HSVColor;

  class BASELIBRARY_PUBLIC_FUNC Color {
  public:
    double red, green, blue, alpha;

    Color();
    Color(double ar, double ag, double ab, double aa = 1.0);
    Color(const HSVColor &hsv);
    Color(const std::string &color);

    bool operator != (const Color &other);
    std::string to_html() const;
    long toRGB() const;
    long toBGR() const;
    bool is_valid() const;
    Color invert() const;
    double brightness() const;
    Color brighten(float fraction) const;
    Color darken(float fraction) const;

    static Color parse(const std::string &color);

    static inline Color black() {
      return Color(0, 0, 0);
    }
    static inline Color white() {
      return Color(1, 1, 1);
    }
    static inline Color invalid() {
      return Color(-1, -1, -1);
    }

    static Color getApplicationColor(ApplicationColor color, bool foreground);
    static std::string getApplicationColorAsString(ApplicationColor color, bool foreground);

    static Color getSystemColor(SystemColor colorType);

    static void set_active_scheme(ColorScheme scheme);
    static ColorScheme get_active_scheme();
    static bool is_high_contrast_scheme();
    static void prepareForTesting();

    // Persistence support. Also called when colors were changed in preferences.
    static void load_custom_colors(const std::map<std::string, std::string> &colors);
    static void save_custom_colors(std::map<std::string, std::string> &colors);
  };

  struct BASELIBRARY_PUBLIC_FUNC HSVColor {
    int h;          // 0 ~ 360
    double s, v, a; // 0 ~ 1.0

    HSVColor() : h(0), s(0), v(0), a(1){};
    HSVColor(int ah, double as, double av, double aa = 1.0) : h(ah), s(as), v(av), a(aa){};
    HSVColor(const Color &rgb);
  };

  class BASELIBRARY_PUBLIC_FUNC OSConstants {
  public:
    static std::string defaultFontName();

    static float systemFontSize();
    static float smallSystemFontSize();
    static float labelFontSize();
  };

} // namespace base
