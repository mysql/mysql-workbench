/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

#import "MStatusBar.h"

//----------------------------------------------------------------------------------------------------------------------

@implementation MStatusBar

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self != nil) {
      [[NSNotificationCenter defaultCenter] addObserver: self
                                               selector: @selector(windowBecameKey:)
                                                   name: NSWindowDidBecomeKeyNotification
                                                 object: nil];
      [[NSNotificationCenter defaultCenter] addObserver: self
                                               selector: @selector(windowResignedKey:)
                                                   name: NSWindowDidResignKeyNotification
                                                 object: nil];
    }
    return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (void) dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver: self];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)windowBecameKey:(NSNotification*)notif {
  if (notif.object == self.window)
    [self setNeedsDisplay: YES];
}


//----------------------------------------------------------------------------------------------------------------------

- (void)windowResignedKey:(NSNotification*)notif {
  if (notif.object == self.window)
    [self setNeedsDisplay: YES];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)drawRect: (NSRect)rect {
  BOOL dark = false;
  if (@available(macOS 10.14, *)) {
    dark = self.window.effectiveAppearance.name == NSAppearanceNameDarkAqua;
  }

  NSRect b = self.bounds;
  NSGradient *gradient;

  if (self.window.keyWindow) {
    if (dark) {
      gradient = [[NSGradient alloc] initWithColorsAndLocations:
                  [NSColor colorWithDeviceRed: 0x40 / 255.0 green: 0x40 / 255.0 blue: 0x40 / 255.0 alpha: 1.0],
                  (CGFloat)0.0,
                  [NSColor colorWithDeviceRed: 0x32 / 255.0 green: 0x32 / 255.0 blue: 0x32 / 255.0 alpha: 1.0],
                  (CGFloat)1.0,
                  nil];
    } else {
      gradient = [[NSGradient alloc] initWithColorsAndLocations:
                  [NSColor colorWithDeviceRed: 0xdd / 255.0 green: 0xdd / 255.0 blue: 0xdd / 255.0 alpha: 1.0],
                  (CGFloat)0.0,
                  [NSColor colorWithDeviceRed: 0xc7 / 255.0 green: 0xc7 / 255.0 blue: 0xc7 / 255.0 alpha: 1.0],
                  (CGFloat)1.0,
                  nil];
    }
    [gradient drawInRect: b angle: 270];

    if (dark) {
      [[NSColor blackColor] set];
    } else {
      [[NSColor colorWithDeviceRed: 0xc1 / 255.0 green: 0xc1 / 255.0 blue: 0xc1 / 255.0 alpha: 1.0] set];
    }
    [NSBezierPath strokeLineFromPoint: NSMakePoint(0, NSHeight(b))
                              toPoint: NSMakePoint(NSWidth(b), NSHeight(b))];
  } else {
    if (dark) {
      [[NSColor colorWithDeviceRed: 0x29 / 255.0 green: 0x2d / 255.0 blue: 0x2b / 255.0 alpha: 1.0] set];
      NSRectFill(b);
      [[NSColor blackColor] set];
    } else {
      [[NSColor colorWithDeviceRed: 0xf6 / 255.0 green: 0xf6 / 255.0 blue: 0xf6 / 255.0 alpha: 1.0] set];
      NSRectFill(b);
      [[NSColor colorWithDeviceRed: 0xd1 / 255.0 green: 0xd1 / 255.0 blue: 0xd1 / 255.0 alpha: 1.0] set];
    }
    [NSBezierPath strokeLineFromPoint: NSMakePoint(0, NSHeight(b))
                              toPoint: NSMakePoint(NSWidth(b), NSHeight(b))];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)mouseDownCanMoveWindow
{
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityGroupRole;
}

- (NSString*)accessibilityLabel {
  return @"Status Bar";
}

@end
