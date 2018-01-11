/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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


@implementation MStatusBar

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
      mGradient= [[NSGradient alloc] initWithColorsAndLocations:
                  [NSColor colorWithCalibratedRed:197/256.0 green:197/256.0 blue:196/256.0 alpha:1.0],
                  (CGFloat)0.0,
                  [NSColor colorWithCalibratedRed:151/256.0 green:150/256.0 blue:149/256.0 alpha:1.0],
                  (CGFloat)0.76,
                  [NSColor colorWithCalibratedRed:148/256.0 green:147/256.0 blue:147/256.0 alpha:1.0],
                  (CGFloat)1.0,
                  nil];
      
      [[NSNotificationCenter defaultCenter] addObserver: self
                                               selector:@selector(windowBecameKey:)
                                                   name:NSWindowDidBecomeKeyNotification
                                                 object:nil];
      [[NSNotificationCenter defaultCenter] addObserver: self
                                               selector:@selector(windowResignedKey:)
                                                   name:NSWindowDidResignKeyNotification
                                                 object:nil];
    }
    return self;
}

- (void) dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver: self];
}

- (void)windowBecameKey:(NSNotification*)notif
{
  if (notif.object == self.window)
    [self setNeedsDisplay:YES];
}


- (void)windowResignedKey:(NSNotification*)notif
{
  if (notif.object == self.window)
    [self setNeedsDisplay:YES];
}


- (void)drawRect:(NSRect)rect 
{
  if (self.window.keyWindow)
  {
    NSRect b= self.bounds;    
            
    [mGradient drawInRect:b angle: 270];

    [[NSColor colorWithCalibratedRed:224/255.0 green:224/255.0 blue:224/255.0 alpha:1.0] set];
    [NSBezierPath strokeLineFromPoint: NSMakePoint(0, NSHeight(b)-1.5) toPoint: NSMakePoint(NSWidth(b), NSHeight(b)-1.5)];

    [[NSColor colorWithDeviceWhite: 81/255.0 alpha: 1.0] set];
    [NSBezierPath strokeLineFromPoint: NSMakePoint(0, NSHeight(b)-0.5) toPoint: NSMakePoint(NSWidth(b), NSHeight(b)-0.5)];
  }
  else
  {
    [[NSColor controlColor] set];
    NSRectFill(self.bounds);
  }
}


- (BOOL)mouseDownCanMoveWindow
{
  return YES;
}


@end
