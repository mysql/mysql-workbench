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

#import "WBEditorTabItem.h"
#import "CGColorUtilities.h"

@implementation WBEditorTabItem

- (void) updateAppearance;
{
	[super updateAppearance];
	
	if ( (mState == NSOnState) && mEnabled ) {
		[mTitleLayer setOpacity: 1.0];
  }
	else {
		[mTitleLayer setOpacity: 0.8];
  }

  // Set the shadow.
  [self setShadowOpacity: 0];
}



- (CGFloat) preferredWidth;
{
	CGFloat preferredWidth = 0;
	
	NSFont* font = [NSFont boldSystemFontOfSize: 9];
	NSDictionary* attributes = @{NSFontAttributeName: font};
	CGFloat labelWidth = ceil([mLabel sizeWithAttributes: attributes].width);
	preferredWidth = 5 + labelWidth + 5;
  preferredWidth = MAX(preferredWidth, 86);
  
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
    double gray = (230.0 / 255.0);
    CGColorRef colorActiveSelected = WB_CGColorCreateCalibratedRGB(gray, gray, gray, 1);
		CGColorRef colorActiveNotSelected = WB_CGColorCreateCalibratedRGB(0.91, 0.91, 0.91, 1);
		CGColorRef colorNotActiveSelected = WB_CGColorCreateCalibratedRGB(gray, gray, gray, 1);
		CGColorRef colorNotActiveNotSelected = WB_CGColorCreateCalibratedRGB(0.91, 0.91, 0.91, 1);
    
    [self setColorActiveSelected: colorActiveSelected
          colorActiveNotSelected: colorActiveNotSelected
          colorNotActiveSelected: colorNotActiveSelected
		   colorNotActiveNotSelected: colorNotActiveNotSelected];
		CGColorRelease(colorActiveSelected);
		CGColorRelease(colorActiveNotSelected);
		CGColorRelease(colorNotActiveSelected);
		CGColorRelease(colorNotActiveNotSelected);
		
		// Border.
		[self setCornerRadius: 5];
		[self setBorderWidth: 1];
    gray = (130.0 / 255.0);
		CGColorRef c = WB_CGColorCreateCalibratedRGB(gray, gray, gray, 0.7);
		[self setBorderColor: c];
		CGColorRelease(c);

    c = WB_CGColorCreateCalibratedRGB(0, 0, 0, 1.0);
    [mTitleLayer setForegroundColor: c];
    CGColorRelease(c);
    NSFont* font = [NSFont systemFontOfSize: 0];
    [mTitleLayer setFont: (__bridge CFTypeRef _Nullable)(font)];

    // Center the title.
    CGRect f = [mTitleLayer frame];
    f.origin.x = 0;
    f.size.width = [self frame].size.width;
    f.origin.y -= 1;
    [mTitleLayer setFrame: f];
    [mTitleLayer setAlignmentMode: kCAAlignmentCenter];
  }
	
	return self;
}



+ (WBTabItem*) tabItemWithIdentifier: (id) identifier
                               label: (NSString*) label;
{
  return [[WBEditorTabItem alloc] initWithIdentifier: identifier
                                               label: label
                                           direction: WBTabDirectionDown
                                           placement: WBTabPlacementBottom
                                                size: WBTabSizeLarge
                                             hasIcon: NO
                                            canClose: NO];
}



@end


