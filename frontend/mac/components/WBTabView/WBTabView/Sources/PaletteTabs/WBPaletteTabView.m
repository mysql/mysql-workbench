/*
 * Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
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

#import "WBPaletteTabView.h"
#import "WBPaletteTabItem.h"

@implementation WBPaletteTabView

- (CALayer*) shadowLayer;
{
  return nil;
}



- (CALayer*) plaqueLayer;
{
  // Create a gradient plaque in the tab row area, behind the tabs.
  CALayer* plaqueLayer = [CALayer layer];
  
  CGRect r = [mTabRowLayer bounds];
  r.size.height = [self tabAreaHeight] - 1;
  [plaqueLayer setFrame: r];
  [plaqueLayer setAutoresizingMask: (kCALayerWidthSizable | kCALayerMaxYMargin)];
  [plaqueLayer setZPosition: -4];
  
  NSBundle* b = [NSBundle bundleForClass: [self class]];
  NSString* path = [b pathForResource: @"TabRowGradient"
                               ofType: @"png"];
  mPlaqueImage = [[NSImage alloc] initWithContentsOfFile: path];
  NSImageRep* rep = [mPlaqueImage representations][0];
  CGImageRef img = [(id)rep CGImage];
  [plaqueLayer setContents: (id)img];
  
  return plaqueLayer;
}



- (WBTabItem*) tabItemWithIdentifier: (id) identifier
                               label: (NSString*) label;
{
	WBTabItem* item = [WBPaletteTabItem tabItemWithIdentifier: identifier
                                                      label: label];
	
	return item;
}



- (void) doCustomize;
{
	mTabPlacement = WBTabPlacementTop;
	mTabDirection = WBTabDirectionUp;
	mTabSize = WBTabSizeSmall;
	
	[super doCustomize];
  
  NSRect r = [mTabView frame];
  r = NSInsetRect(r, 3, 3);
  [mTabView setFrame: r];
}



- (void) dealloc
{
  [mPlaqueImage release];
  
  [super dealloc];
}

@end


