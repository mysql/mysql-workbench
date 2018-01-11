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

#import "MContainerView.h"


@implementation MContainerView

- (void)setMinContentSize:(NSSize)size
{
  mMinSize= size;
}


- (void)setPadding:(NSSize)padding
{
  mPadding= padding;
}


- (BOOL)isFlipped
{
  return YES;
}


// resize subview keeping a minimum size
- (void)resizeSubviewsWithOldSize:(NSSize)oldBoundsSize
{
  id item= self.subviews.lastObject;
  NSSize size= self.frame.size;
  NSRect rect;

  if (size.width < mMinSize.width + 2*mPadding.width)
    size.width= mMinSize.width + 2*mPadding.width;
  if (size.height < mMinSize.height + 2*mPadding.height)
    size.height= mMinSize.height + 2*mPadding.height;
  
  rect.origin.x= mPadding.width;
  rect.origin.y= mPadding.height;
  
  rect.size= size;
  rect.size.width-= mPadding.width*2;
  rect.size.height-= mPadding.height*2;
  
  [item setFrame: rect];
}

@end
