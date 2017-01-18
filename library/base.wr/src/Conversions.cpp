/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "Conversions.h"
#include "ManagedNotifications.h"

using namespace System;
using namespace System::Drawing;

using namespace MySQL;
using namespace MySQL::Workbench;

//--------------------------------------------------------------------------------------------------

base::Color Conversions::NativeToColor(Color color) {
  return base::Color(color.R / 255.0, color.G / 255.0, color.B / 255.0, color.A / 255.0);
}

//--------------------------------------------------------------------------------------------------

Color Conversions::ColorToNative(base::Color color) {
  return Color::FromArgb((int)(color.alpha * 255), (int)(color.red * 255), (int)(color.green * 255),
                         (int)(color.blue * 255));
}

//--------------------------------------------------------------------------------------------------

Color Conversions::GetApplicationColor(ApplicationColor color, bool foreground) {
  base::Color baseColor = base::Color::get_application_color((base::ApplicationColor)color, foreground);
  if (!baseColor.is_valid())
    return Color::Black;
  return Color::FromArgb(int(baseColor.red * 255), int(baseColor.green * 255), int(baseColor.blue * 255));
}

//--------------------------------------------------------------------------------------------------

bool Conversions::UseWin8Drawing() {
  switch (base::Color::get_active_scheme()) {
    case base::ColorSchemeStandardWin8:
    case base::ColorSchemeStandardWin8Alternate:
      return true;

    default:
      return false;
  }
}

//--------------------------------------------------------------------------------------------------

bool Conversions::InHighContrastMode() {
  return base::Color::is_high_contrast_scheme();
}

//--------------------------------------------------------------------------------------------------

void Conversions::SetColorScheme(ColorScheme newScheme) {
  base::Color::set_active_scheme((base::ColorScheme)newScheme);
  ManagedNotificationCenter::Send("GNColorsChanged", IntPtr::Zero);
}

//--------------------------------------------------------------------------------------------------
