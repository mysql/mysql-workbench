/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
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

#import "MColoredView.h"

//----------------------------------------------------------------------------------------------------------------------

@interface MColoredView () {
  NSColor *mColor;
  BOOL useDefault;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation MColoredView

-(instancetype)initWithFrame: (NSRect)frameRect {
  self = [super initWithFrame: frameRect];
  if (self != nil && mColor == nil) {
    useDefault = YES;
    mColor = NSColor.windowBackgroundColor;

    NSWindow *window = NSApplication.sharedApplication.mainWindow;
    [window addObserver: self forKeyPath: @"effectiveAppearance" options: 0 context: nil];
    [self updateColors: window];
  }

  return self;
}

//----------------------------------------------------------------------------------------------------------------------

-(instancetype)initWithCoder:(NSCoder *)decoder {
  self = [super initWithCoder: decoder];
  if (self != nil) {
    useDefault = YES;
    mColor = NSColor.windowBackgroundColor;

    NSWindow *window = NSApplication.sharedApplication.mainWindow;
    [window addObserver: self forKeyPath: @"effectiveAppearance" options: 0 context: nil];
    [self updateColors: window];
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)dealloc {
  NSWindow *window = NSApplication.sharedApplication.mainWindow;
  [window removeObserver: self forKeyPath: @"effectiveAppearance"];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)observeValueForKeyPath: (NSString *)keyPath
                      ofObject: (id)object
                        change: (NSDictionary *)change
                       context: (void *)context {
  if ([keyPath isEqualToString: @"effectiveAppearance"]) {
    [self updateColors: object];
    return;
  }
  [super observeValueForKeyPath: keyPath ofObject: object change: change context: context];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)updateColors: (NSWindow *)window {
  if (!useDefault)
    return;

  BOOL isDark = NO;
  if (@available(macOS 10.14, *)) {
    isDark = window.effectiveAppearance.name == NSAppearanceNameDarkAqua;
  }

  mColor = NSColor.windowBackgroundColor;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSColor*)backgroundColor {
  return mColor;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setBackgroundColor: (NSColor*)color {
  useDefault = NO;
  mColor = color;
}

//----------------------------------------------------------------------------------------------------------------------

- (void) drawRect:(NSRect)rect {
  if (mColor != nil) {
    [mColor set];
    NSRectFill(rect);
  }
}

@end

//----------------------------------------------------------------------------------------------------------------------
