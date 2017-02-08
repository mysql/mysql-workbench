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

#import "WBPaletteTabView.h"
#import "WBPaletteTabItem.h"

@implementation WBPaletteTabView

- (CALayer*)shadowLayer;
{
  return nil;
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
  
  NSRect r = mTabView.frame;
  r = NSInsetRect(r, 3, 3);
  mTabView.frame = r;
}

@end


