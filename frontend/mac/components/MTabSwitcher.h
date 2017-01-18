/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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

typedef NS_ENUM(NSInteger, MTabSwitcherStyle) {
  MSectionTabSwitcher,
  MPaletteTabSwitcher, // Similar to SectionTabSwitcher, but with slight changes (darker and centered)
  MPaletteTabSwitcherSmallText,
  MEditorTabSwitcher,
  MEditorTabSwitcherX,
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

@property(nullable, weak) IBOutlet id delegate;
@property float minTabWidth;
@property(nullable, readonly) NSTabViewItem *clickedItem;
@property(nonatomic) MTabSwitcherStyle tabStyle;
@property BOOL allowTabReordering;

- (void)setTabView:
  (NSTabView *_Nullable)tabView; // TODO the associated tabview is bound via an outlet. Why do we need a setter then?

- (void)setBusyTab:(NSTabViewItem *_Nullable)tab;
- (void)closeTabViewItem:(NSTabViewItem *_Nullable)item;
- (void)tile;
- (void)makeUnselected;

@end
