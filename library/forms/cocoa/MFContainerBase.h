/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

#import "MFView.h"
#include "mforms/view.h"
#include "mforms/label.h"

@interface MFContainerBase : NSView {
  float mLeftPadding;
  float mTopPadding;
  float mBottomPadding;
  float mRightPadding;
  mforms::View *mOwner;

  NSTrackingArea *mTrackingArea;
}

- (void)setPaddingLeft:(float)lpad right:(float)rpad top:(float)tpad bottom:(float)bpad;
- (BOOL)setFreezeRelayout:(BOOL)flag;
- (void)setBackgroundColor:(NSColor *)color;
- (void)setBackgroundImage:(NSString *)path withAlignment:(mforms::Alignment)align;

@end
