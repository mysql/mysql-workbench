/* 
 * Copyright (c) 2009, 2016, Oracle and/or its affiliates. All rights reserved.
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

#import "WBSplitView.h"

@interface WBSplitView()
{
  CGFloat dividerWidth;
  BOOL mEnabled;
}

@end

@implementation WBSplitView

@synthesize backgroundColor;
@synthesize dividerThickness = dividerWidth;

- (void)drawDividerInRect: (NSRect) aRect;
{
  NSColor* color;
  
  if (mEnabled) {
    color = [NSColor colorWithDeviceWhite: 0xaa/256.0 alpha: 1.0];
  }
  else {
    color = [NSColor colorWithCalibratedRed: 0.8
                                      green: 0.8
                                       blue: 0.8
                                      alpha: 1];
  }
  
  [color set];
  [NSBezierPath fillRect: aRect];
}

- (void)drawRect: (NSRect)rect
{
  if (backgroundColor != nil)
  {
    [backgroundColor set];
    NSRectFill(rect);
  }
  else
    [super drawRect: rect];
}

- (void) handleDidBecomeMain: (id) aNotification;
{
  mEnabled = YES;
  [self setNeedsDisplay: YES];
}

- (void)handleDidResignMain: (id) aNotification;
{
  mEnabled = NO;
  [self setNeedsDisplay: YES];
}

#pragma mark Create and Destroy

- (void)awakeFromNib;
{
  mEnabled = YES;
  self.dividerThickness = 1;
  
  // Set up notifications.
  NSNotificationCenter* dc = [NSNotificationCenter defaultCenter];
  [dc addObserver: self
         selector: @selector(handleDidBecomeMain:)
             name: NSWindowDidBecomeMainNotification
           object: self.window];
  [dc addObserver: self
         selector: @selector(handleDidResignMain:)
             name: NSWindowDidResignMainNotification
           object: self.window];
}

- (void)dealloc
{
  NSNotificationCenter* dc = [NSNotificationCenter defaultCenter];
  [dc removeObserver: self];
}

@end


