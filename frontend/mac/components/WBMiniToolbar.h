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

#import <Cocoa/Cocoa.h>

@class WBMiniToolbar;

@interface NSObject (WBMiniToolbarDelegate)

- (void)miniToolbar:(WBMiniToolbar *)sender
       popupChanged:(NSString *)name
             option:(NSString *)option
              value:(NSString *)value;

@end

@interface WBMiniToolbar : NSView {
  NSGradient *mGradient;
  id mDelegate;
  NSMutableArray *mOptionInfoList;
}

- (void)setGradient:(NSGradient *)gradient;
- (void)tile;

- (void)removeAllItems;

- (void)setDelegate:(id)delegate;

- (NSButton *)addButtonWithTitle:(NSString *)title target:(id)target action:(SEL)action tag:(int)tag;

- (NSButton *)addButtonWithIcon:(NSImage *)icon target:(id)target action:(SEL)action tag:(int)tag;

- (NSSegmentedControl *)addSegmentedButtonsWithIconsAndTags:(NSArray *)iconsAndTags
                                                     target:(id)target
                                                     action:(SEL)action;

- (NSTextField *)addLabelWithTitle:(NSString *)title;

- (void)addSeparator;

- (void)addExpandingSpace;

- (NSPopUpButton *)addSelectionPopUpWithItems:(NSArray *)items
                                         name:(NSString *)name
                                       option:(NSString *)option
                                 defaultValue:(NSString *)defaultValue;

- (NSPopUpButton *)addSelectionPopUpWithItems:(NSArray *)items
                                       target:(id)target
                                       action:(SEL)action
                                 defaultValue:(NSString *)defaultValue;

- (void)addSelectionPopUpWithColors:(NSArray *)colors
                               name:(NSString *)name
                             option:(NSString *)option
                       defaultValue:(NSString *)defaultValue;

@end
