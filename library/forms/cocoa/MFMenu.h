/*
 * Copyright (c) 2010, 2017, Oracle and/or its affiliates. All rights reserved.
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
