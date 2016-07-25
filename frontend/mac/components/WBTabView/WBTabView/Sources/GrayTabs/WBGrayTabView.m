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

#import "WBGrayTabView.h"
#import "WBGrayTabItem.h"
#import "CGColorUtilities.h"

@implementation WBGrayTabView

- (void)drawRect: (NSRect)rect;
{
  [super drawRect: rect];

  // fills area inside a tab
  [[NSColor colorWithCalibratedRed: 0.93
                             green: 0.93
                              blue: 0.93
                             alpha: 1] set];
  [NSBezierPath fillRect: rect];
}


- (NSRect)contentRect
{
  NSRect rect= super.contentRect;
  
  rect.size.height= NSHeight(self.frame) - self.tabAreaHeight;
  
  return rect;
}


- (CALayer*) shadowLayer;
{
	// Create a line between non-selected tabs and the tabviewcontants.
	CALayer* lineLayer = [CALayer layer];
	
	lineLayer.borderWidth = 1;
	CGColorRef c = WB_CGColorCreateCalibratedRGB(0.68, 0.68, 0.68, 1);
	lineLayer.borderColor = c;
	CGColorRelease(c);
	
	CGRect r = CGRectZero;
	r.origin.x = 0;
	r.origin.y = self.layer.frame.size.height - self.tabAreaHeight;
	r.size.width = self.layer.frame.size.width;
	r.size.height = 1;
	lineLayer.frame = r;
	lineLayer.autoresizingMask = (kCALayerWidthSizable | kCALayerMinYMargin);
	
	lineLayer.zPosition = -2;
	[self.layer addSublayer: lineLayer];
	
	return lineLayer;
}

- (WBTabItem*) tabItemWithIdentifier: (id) identifier
							   label: (NSString*) label;
{
	WBTabItem* item = [WBGrayTabItem tabItemWithIdentifier: identifier
                                                   label: label];
	
	
	return item;
}



- (void) doCustomize;
{
  mTabPlacement = WBTabPlacementTop;
  mTabDirection = WBTabDirectionUp;
  mTabSize = WBTabSizeLarge;

  
  [super doCustomize];
  
  NSRect r = mTabView.frame;
  r.size.height -= 5;
  mTabView.frame = r;
}

@end
