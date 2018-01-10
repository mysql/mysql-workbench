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

#import "WBEditorTabView.h"
#import "WBEditorTabItem.h"
#import "CGColorUtilities.h"

@implementation WBEditorTabView

- (void) drawRect: (NSRect) frame;
{
  [super drawRect: frame];
  
  NSRect r = [self bounds];
  r.origin.y += [self tabAreaHeight];
  
  double gray = (214.0 / 255.0);
  NSColor* c = [NSColor colorWithCalibratedRed: gray
                                         green: gray
                                          blue: gray
                                         alpha: 1];

  [c set];
  [NSBezierPath fillRect: r];
  
  gray = (130.0 / 255.0);
  c = [NSColor colorWithCalibratedRed: gray
                                green: gray
                                 blue: gray
                                alpha: 1];
  [c set];
  [NSBezierPath strokeLineFromPoint: r.origin
                            toPoint: NSMakePoint(r.origin.x, r.size.height - 2)];
  [NSBezierPath strokeLineFromPoint: NSMakePoint(r.origin.x, r.size.height - 2)
                            toPoint: NSMakePoint(r.origin.x + 2, r.size.height)];
  [NSBezierPath strokeLineFromPoint: NSMakePoint(r.origin.x + 2, r.size.height)
                            toPoint: NSMakePoint(r.size.width - 2, r.size.height)];
  [NSBezierPath strokeLineFromPoint: NSMakePoint(r.size.width - 2, r.size.height)
                            toPoint: NSMakePoint(r.size.width, r.size.height - 2)];
  [NSBezierPath strokeLineFromPoint: NSMakePoint(r.size.width, r.size.height - 2)
                            toPoint: NSMakePoint(r.size.width, r.origin.y)];
}



- (float) contentPadding;
{
  return 2;
}



- (CGColorRef) tabRowActiveBackgroundColorCreate;
{
//	return WB_CGColorCreateCalibratedRGB(1, 0.2, 0.2, 1);
	return WB_CGColorCreateCalibratedRGB(0.93, 0.93, 0.93, 1);
}



- (CGColorRef) tabRowInactiveBackgroundColorCreate;
{
	return nil;
}



- (CALayer*) shadowLayer;
{
  return nil;
}



- (WBTabItem*) tabItemWithIdentifier: (id) identifier
                               label: (NSString*) label;
{
	WBTabItem* item = [WBEditorTabItem tabItemWithIdentifier: identifier
                                                        label: label];
	
	return item;
}


- (void) doCustomize;
{
	mTabPlacement = WBTabPlacementBottom;
	mTabDirection = WBTabDirectionDown;
	mTabSize = WBTabSizeLarge;
	
	[super doCustomize];

	CGColorRef c = WB_CGColorCreateCalibratedRGB(0.93, 0.93, 0.93, 1);
	[mTabRowLayer setBackgroundColor: c];
	CGColorRelease(c);
}



@end


