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

#import "WBGrayTabItem.h"
#import "CGColorUtilities.h"

//----------------------------------------------------------------------------------------------------------------------

@implementation WBGrayTabItem

- (CGFloat)preferredWidth {
  CGFloat preferredWidth = 0;
  
  NSFont* font = [NSFont boldSystemFontOfSize: 11.5];
  NSDictionary* attributes = @{NSFontAttributeName: font};
  CGFloat labelWidth = ceil([mLabel sizeWithAttributes:attributes].width);
  preferredWidth = 25 + mDocumentIconImage.size.width + labelWidth + mCloseButtonImage.size.width;
  
  return preferredWidth;
}

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithOwner: (WBTabView *)owner
                   identifier: (id) identifier
                        label: (NSString*) label
                    direction: (WBTabDirection) tabDirection
                    placement: (WBTabPlacement) tabPlacement
                         size: (WBTabSize) tabSize
                      hasIcon: (BOOL) hasIcon
                     canClose: (BOOL) canClose {
  self = [super initWithOwner: (WBTabView *)owner
                   identifier: identifier
                        label: label
                    direction: tabDirection
                    placement: tabPlacement
                         size: tabSize
                      hasIcon: hasIcon
                     canClose: canClose];
	
	if (self != nil) {
		// Border.
		self.borderWidth = 1;
		CGColorRef c = WB_CGColorCreateCalibratedRGB(0.3, 0.3, 0.3, 0.7);
		self.borderColor = c;
		CGColorRelease(c);
	}
	
	return self;
}

//----------------------------------------------------------------------------------------------------------------------

+ (WBTabItem*)tabItemWithOwner: (WBTabView *)owner
                    identifier: (id)identifier
                         label: (NSString*)label {
  return [[self alloc] initWithOwner: (WBTabView *)owner
                          identifier: identifier
                               label: label
                           direction: WBTabDirectionUp
                           placement: WBTabPlacementTop
                                size: WBTabSizeLarge
                             hasIcon: YES
                            canClose: YES];
}

@end

//----------------------------------------------------------------------------------------------------------------------
