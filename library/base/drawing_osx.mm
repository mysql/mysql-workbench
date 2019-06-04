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

#include "drawing.h"

using namespace base;

static bool inTesting = false;

//----------------------------------------------------------------------------------------------------------------------

std::string OSConstants::defaultFontName() {
  NSFont *font = [NSFont systemFontOfSize: 13];
  NSString *name = font.fontName;
  return name.UTF8String;
}

//----------------------------------------------------------------------------------------------------------------------

float OSConstants::systemFontSize() {
  return NSFont.systemFontSize;
}

//----------------------------------------------------------------------------------------------------------------------

float OSConstants::smallSystemFontSize() {
  return NSFont.smallSystemFontSize;
}

//----------------------------------------------------------------------------------------------------------------------

float OSConstants::labelFontSize() {
  return NSFont.labelFontSize;
}

//----------------------------------------------------------------------------------------------------------------------

Color Color::getSystemColor(SystemColor colorType) {
  // NOTE: this function must only be called from the main thread!
  NSAppearance * saved = NSAppearance.currentAppearance;
  NSAppearance.currentAppearance = NSApp.mainWindow.effectiveAppearance;

  NSColor *color;
  switch (colorType) {
    case ControlShadowColor: color = NSColor.controlShadowColor; break;
    case ControlDarkShadowColor: color = NSColor.controlDarkShadowColor; break;
    case ControlColor: color = NSColor.controlColor; break;
    case ControlHighlightColor: color = NSColor.controlHighlightColor; break;
    case ControlLightHighlightColor: color = NSColor.controlLightHighlightColor; break;
    case ControlTextColor: color = NSColor.controlTextColor; break;
    case ControlBackgroundColor: color = NSColor.controlBackgroundColor; break;
    case SelectedControlColor: color = NSColor.selectedControlColor; break;
    case SecondarySelectedControlColor: color = NSColor.secondarySelectedControlColor; break;
    case SelectedControlTextColor: color = NSColor.selectedControlTextColor; break;
    case DisabledControlTextColor: color = NSColor.disabledControlTextColor; break;

    case TextColor: color = NSColor.textColor; break;
    case LabelColor: color = NSColor.labelColor; break;
    case SecondaryLabelColor: color = NSColor.secondaryLabelColor; break;
    case TertiaryLabelColor: color = NSColor.tertiaryLabelColor; break;
    case QuaternaryLabelColor: color = NSColor.quaternaryLabelColor; break;
    case TextBackgroundColor: color = NSColor.textBackgroundColor; break;
    case SelectedTextColor: color = NSColor.selectedTextColor; break;
    case SelectedTextBackgroundColor: color = NSColor.selectedTextBackgroundColor; break;
    case GridColor: color = NSColor.gridColor; break;

    case WindowBackgroundColor: color = NSColor.windowBackgroundColor; break;
    case WindowFrameColor: color = NSColor.windowFrameColor; break;
    case WindowFrameTextColor: color = NSColor.windowFrameTextColor; break;
    case SecondaryBackgroundColor: color = NSColor.underPageBackgroundColor; break;

    case SelectedMenuItemColor: color = NSColor.selectedMenuItemColor; break;
    case SelectedMenuItemTextColor: color = NSColor.selectedMenuItemTextColor; break;

    case HighlightColor: color = NSColor.highlightColor; break;

    case HeaderColor: color = NSColor.headerColor; break;
    case HeaderTextColor: color = NSColor.headerTextColor; break;

    case AlternateSelectedControlColor: color = NSColor.alternateSelectedControlColor; break;
    case AlternateSelectedControlTextColor: color = NSColor.alternateSelectedControlTextColor; break;
      break;

    default:
      color = nil;
  }

  Color result;

  if (color != nil) {
    NSColor *rgbColor = [color colorUsingColorSpace: NSColorSpace.genericRGBColorSpace];
    if (rgbColor == nil) {
      // A pattern color probably.
      rgbColor = [color colorUsingType: NSColorTypePattern];
      NSImage *pattern = rgbColor.patternImage;
      if (pattern != nil) {
        NSBitmapImageRep *representation = (NSBitmapImageRep *)pattern.representations[0]; // Usually only has one.
        if (representation.colorSpaceName == NSCalibratedRGBColorSpace) {
          // Patterns are usually of a very small size (like 8 x 8 pixels), we simply iterate over all pixels to find an
          // average value (as if we had scaled it to one pixel).
          NSInteger count = representation.size.width * representation.size.height;
          CGFloat red = 0, green = 0, blue = 0;
          for (NSInteger x = 0; x < representation.size.width; ++x) {
            for (NSInteger y = 0; y < representation.size.height; ++y) {
              NSColor *pixel = [representation colorAtX: x y: y];
              red += pixel.redComponent;
              green += pixel.greenComponent;
              blue += pixel.blueComponent;
            }

          }
          result = Color(red / count, green / count, blue / count, 1);
        }
      }
    }

    if (!result.is_valid())
      result = Color(rgbColor.redComponent, rgbColor.greenComponent, rgbColor.blueComponent, rgbColor.alphaComponent);
  }

  NSAppearance.currentAppearance = saved;
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void Color::prepareForTesting() {
  inTesting = true;
}

//----------------------------------------------------------------------------------------------------------------------
