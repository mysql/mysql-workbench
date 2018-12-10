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

#include "base/drawing.h"
#include "base/string_utilities.h"

using namespace base;

//----------------------------------------------------------------------------------------------------------------------

static bool inTesting = false;

Color Color::getSystemColor(SystemColor colorType) {
  DWORD sysColor = 0;
  switch (colorType) {
    case ControlShadowColor:
      sysColor = GetSysColor(COLOR_3DSHADOW);
      break;
    case ControlDarkShadowColor:
      sysColor = GetSysColor(COLOR_3DDKSHADOW);
      break;
    case ControlColor:
      sysColor = GetSysColor(COLOR_BTNFACE);
      break;
    case ControlHighlightColor:
      sysColor = GetSysColor(COLOR_BTNHIGHLIGHT);
      break;
    case ControlLightHighlightColor:
      sysColor = GetSysColor(COLOR_BTNHIGHLIGHT);
      break;
    case ControlTextColor:
      sysColor = GetSysColor(COLOR_BTNTEXT);
      break;
    case ControlBackgroundColor:
      sysColor = GetSysColor(COLOR_WINDOW);
      break;
    case SelectedControlColor:
      sysColor = GetSysColor(COLOR_BTNFACE);
      break;
    case SecondarySelectedControlColor:
      sysColor = GetSysColor(COLOR_BTNFACE);
      break;
    case SelectedControlTextColor:
      sysColor = GetSysColor(COLOR_BTNTEXT);
      break;
    case DisabledControlTextColor:
      sysColor = GetSysColor(COLOR_GRAYTEXT);
      break;
    case TextColor:
      sysColor = GetSysColor(COLOR_WINDOWTEXT);
      break;
    case TextBackgroundColor:
      sysColor = GetSysColor(COLOR_WINDOW);
      break;
    case LabelColor:
    case SecondaryLabelColor:
    case TertiaryLabelColor:
    case QuaternaryLabelColor:
      sysColor = GetSysColor(COLOR_WINDOWTEXT);
      break;
    case SelectedTextColor:
      sysColor = GetSysColor(COLOR_HIGHLIGHTTEXT);
      break;
    case SelectedTextBackgroundColor:
      sysColor = GetSysColor(COLOR_HIGHLIGHT);
      break;
    case GridColor:
      sysColor = GetSysColor(0x909090);
      break;

    case WindowBackgroundColor:
      sysColor = GetSysColor(COLOR_WINDOW);
      break;
    case WindowFrameColor:
      sysColor = GetSysColor(COLOR_WINDOWFRAME);
      break;
    case WindowFrameTextColor:
      sysColor = GetSysColor(COLOR_CAPTIONTEXT);
      break;
    case SecondaryBackgroundColor:
      sysColor = GetSysColor(COLOR_WINDOW);
      break;

    case SelectedMenuItemColor:
      sysColor = GetSysColor(COLOR_MENUHILIGHT);
      break;
    case SelectedMenuItemTextColor:
      sysColor = GetSysColor(COLOR_MENUTEXT);
      break;

    case HighlightColor:
      sysColor = GetSysColor(COLOR_HIGHLIGHT);
      break;

    case HeaderColor:
      sysColor = GetSysColor(COLOR_BTNFACE);
      break;
    case HeaderTextColor:
      sysColor = GetSysColor(COLOR_BTNTEXT);
      break;

    case AlternateSelectedControlColor:
      sysColor = GetSysColor(COLOR_BTNFACE);
      break;
    case AlternateSelectedControlTextColor:
      sysColor = GetSysColor(COLOR_BTNTEXT);
      break;
  }

  return Color(GetRValue(sysColor) / 255.0, GetGValue(sysColor) / 255.0, GetBValue(sysColor) / 255.0);
}

//----------------------------------------------------------------------------------------------------------------------

std::string OSConstants::defaultFontName() {
  NONCLIENTMETRICS metrics;
  metrics.cbSize = sizeof(metrics);
  SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
  return base::wstring_to_string(metrics.lfCaptionFont.lfFaceName);
}

//----------------------------------------------------------------------------------------------------------------------

float OSConstants::systemFontSize() {
  NONCLIENTMETRICS metrics;
  metrics.cbSize = sizeof(metrics);
  SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
  return (float)-metrics.lfCaptionFont.lfHeight;
}

//----------------------------------------------------------------------------------------------------------------------

float OSConstants::smallSystemFontSize() {
  NONCLIENTMETRICS metrics;
  metrics.cbSize = sizeof(metrics);
  SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
  return (float)-metrics.lfCaptionFont.lfHeight - 2;
}

//----------------------------------------------------------------------------------------------------------------------

float OSConstants::labelFontSize() {
  NONCLIENTMETRICS metrics;
  metrics.cbSize = sizeof(metrics);
  SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
  return (float)-metrics.lfCaptionFont.lfHeight;
}

//----------------------------------------------------------------------------------------------------------------------

void Color::prepareForTesting() {
  inTesting = true;
}
