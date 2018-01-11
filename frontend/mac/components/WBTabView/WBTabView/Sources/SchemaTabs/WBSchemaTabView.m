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

// Implements custom tabs. Big tabs at the top of the view, pointing upwards.
 
#import "WBSchemaTabView.h"
#import "WBSchemaTabItem.h"
#import "CGColorUtilities.h"

@implementation WBSchemaTabView

- (CGFloat) tabAreaHeight;
{
  return 42.0;
}

- (CGColorRef) tabRowActiveBackgroundColorCreate;
{
  return WB_CGColorCreateCalibratedRGB(0.925, 0.949, 0.973, 1);
}



- (CGColorRef) tabRowInactiveBackgroundColorCreate;
{
  return WB_CGColorCreateCalibratedRGB(0.925, 0.949, 0.973, 1);
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
  WBTabItem* item = [WBSchemaTabItem tabItemWithIdentifier: identifier
                                                     label: label];
  
  return item;
}



- (void) doCustomize;
{
  mTabPlacement = WBTabPlacementTop;
  mTabDirection = WBTabDirectionUp;
  mTabSize = WBTabSizeLarge;
  
  [super doCustomize];
}



@end


