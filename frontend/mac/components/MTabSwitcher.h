/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
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

typedef NS_ENUM(NSInteger, MTabSwitcherStyle) {
  MSectionTabSwitcher,
  MEditorTabSwitcher,
  MEditorBottomTabSwitcher,
  MEditorBottomTabSwitcherPinnable,
  MMainTabSwitcher
};

@interface NSObject (MTabSwitcherDelegateExtras)
- (BOOL)tabView:(NSTabView *_Nonnull)tabView willCloseTabViewItem:(NSTabViewItem *_Nonnull)item;
- (BOOL)tabView:(NSTabView *_Nonnull)tabView itemHasCloseButton:(NSTabViewItem *_Nonnull)item;
- (BOOL)tabView:(NSTabView *_Nonnull)tabView itemIsPinned:(NSTabViewItem *_Nonnull)item;
- (void)tabView:(NSTabView *_Nonnull)tabView itemPinClicked:(NSTabViewItem *_Nonnull)item;
- (NSImage *_Nonnull)tabView:(NSTabView *_Nonnull)tabView iconForItem:(NSTabViewItem *_Nonnull)tabViewItem;
- (void)tabView:(NSTabView *_Nonnull)tabView
  willDisplayMenu:(NSMenu *_Nonnull)menu
   forTabViewItem:(NSTabViewItem *_Nonnull)item;
- (void)tabView:(NSTabView *_Nonnull)tabView
  didReorderTabViewItem:(NSTabViewItem *_Nonnull)item
                toIndex:(NSInteger)index;
- (NSString *_Nonnull)tabView:(NSTabView *_Nonnull)tabView toolTipForItem:(NSTabViewItem *_Nonnull)item;
@end

@interface MTabSwitcher : NSView<NSTabViewDelegate>
@property (nullable, weak) IBOutlet NSTabView *mTabView;
@property (nullable, weak) IBOutlet id delegate;
@property float minTabWidth;
@property(nullable, readonly) NSTabViewItem *clickedItem;
@property(nonatomic) MTabSwitcherStyle tabStyle;
@property BOOL allowTabReordering;

- (void)setTabView: (NSTabView *_Nullable)tabView;

- (void)setBusyTab:(NSTabViewItem *_Nullable)tab;
- (void)closeTabViewItem:(NSTabViewItem *_Nullable)item;
- (void)tile;

// Methods needed for accessibility.
- (NSRect)tabItemRect: (NSTabViewItem *_Nullable)aItem;
- (void)makeTabVisibleAndSelect: (id _Nonnull)sender;
- (BOOL)hasCloseButton: (NSTabViewItem * _Nonnull)item;
- (BOOL)allowClosingItem: (NSTabViewItem * _Nonnull)item;
- (NSMenu *_Nullable)prepareMenuForItem: (NSTabViewItem * _Nonnull)item;
- (NSMenu *_Nullable)prepareMenuForTabs;

@end
