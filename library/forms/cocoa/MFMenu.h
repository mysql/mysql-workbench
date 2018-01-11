/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

#import <Cocoa/Cocoa.h>

#include "mforms/menu.h"

// In order to show a non-contextual popup menu we add a category that implements
// a special popup menu.
// Read http://developer.apple.com/mac/library/documentation/Cocoa/Conceptual/ (continued below)
//      MenuList/Articles/DisplayContextMenu.html#//apple_ref/doc/uid/TP40004968
// to learn why we cannot use a contextual popup.
@interface NSMenu (MForms)
+ (void)popUpMenu:(NSMenu *)menu forView:(NSView *)view atLocation:(NSPoint)point pullsDown:(BOOL)pullsDown;
@end

@interface MFMenuImpl : NSMenu {
  mforms::Menu *mOwner;
}

@end

NSMenu *nsmenuForMenu(mforms::Menu *menu);
