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

#import "WBPaletteTabItem.h"
#import "CGColorUtilities.h"

@implementation WBPaletteTabItem

- (void) updateAppearance;
{
	[super updateAppearance];
	
	if ( (mState == NSOnState) && mEnabled )
		mTitleLayer.opacity = 1.0;
	else
		mTitleLayer.opacity = 0.7;
}	

- (CGFloat) preferredWidth;
{
	CGFloat preferredWidth = 0;
	
	NSFont* font = [NSFont boldSystemFontOfSize: 9];
	NSDictionary* attributes = @{NSFontAttributeName: font};
	CGFloat labelWidth = ceil([mLabel sizeWithAttributes:attributes].width);
	preferredWidth = 5 + labelWidth + 5;
	
	return preferredWidth;
}



- (instancetype) initWithIdentifier: (id) identifier
                    label: (NSString*) label
                direction: (WBTabDirection) tabDirection
                placement: (WBTabPlacement) tabPlacement
                     size: (WBTabSize) tabSize
                  hasIcon: (BOOL) hasIcon
                 canClose: (BOOL) canClose;
{
	self = [super initWithIdentifier: identifier
                             label: label
                         direction: tabDirection
                         placement: tabPlacement
                              size: tabSize
                           hasIcon: hasIcon
                          canClose: canClose];
	
	if (self != nil) {
		CGColorRef colorActiveSelected = WB_CGColorCreateCalibratedRGB(0.909, 0.909, 0.909, 1);
		CGColorRef colorActiveNotSelected = WB_CGColorCreateCalibratedRGB(0.85, 0.85, 0.85, 1);
		CGColorRef colorNotActiveSelected = WB_CGColorCreateCalibratedRGB(0.909, 0.909, 0.909, 1);
		CGColorRef colorNotActiveNotSelected = WB_CGColorCreateCalibratedRGB(0.85, 0.85, 0.85, 1);
		[self setColorActiveSelected: colorActiveSelected
          colorActiveNotSelected: colorActiveNotSelected
          colorNotActiveSelected: colorNotActiveSelected
		   colorNotActiveNotSelected: colorNotActiveNotSelected];
		CGColorRelease(colorActiveSelected);
		CGColorRelease(colorActiveNotSelected);
		CGColorRelease(colorNotActiveSelected);
		CGColorRelease(colorNotActiveNotSelected);
		
		// Border.
		self.cornerRadius = 4;
		self.borderWidth = 1;
		CGColorRef c = WB_CGColorCreateCalibratedRGB(0.3, 0.3, 0.3, 0.7);
		self.borderColor = c;
		CGColorRelease(c);
	}
	
	return self;
}

+ (WBTabItem*) tabItemWithIdentifier: (id) identifier
                               label: (NSString*) label;
{
  return [[WBPaletteTabItem alloc] initWithIdentifier: identifier
                                                label: label
                                            direction: WBTabDirectionUp
                                            placement: WBTabPlacementTop
                                                 size: WBTabSizeSmall
                                              hasIcon: NO
                                             canClose: NO];
}



@end


