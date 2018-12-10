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

//
//  ResponderLayer.h
//
//  Created by Jacob Engstrand on 2008-08-12.
//  This source code is public domain. No rights reserved.
//

/**

 @class
ResponderLayer

 @abstract
 WBTabView implements a customized look for tab views, while trying to be semantically compatible with NSTabView.
 WBTabView is the base class of several customized classes each implementing a different look.

 @discussion
 An WBTabView inherits from NSTabView. The method -doCustomize does the following:
 1. Adds a tabless NSTabView as subview inside self, and moves any existing tab view item to it.
 2. Creates a view mTabRowView in which it draws its customized tabs.
 */

#import <QuartzCore/QuartzCore.h>

@interface ResponderLayer : CAGradientLayer {
  CGPoint mMouseDownPoint;
}

- (ResponderLayer*)mouseDownAtPoint:(CGPoint)mouse;
- (void)mouseDraggedToPoint:(CGPoint)mouse;
- (void)mouseUp;
- (ResponderLayer*)responderLayerAtPoint:(CGPoint)mouse;

@end
