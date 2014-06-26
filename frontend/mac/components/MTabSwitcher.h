/* 
 * Copyright (c) 2011, 2014, Oracle and/or its affiliates. All rights reserved.
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

typedef enum 
{
  MSectionTabSwitcher,
  MPaletteTabSwitcher, // Similar to SectionTabSwitcher, but with slight changes (darker and centered)
  MPaletteTabSwitcherSmallText,
  MEditorTabSwitcher,
  MEditorBottomTabSwitcher,
  MMainTabSwitcher
} MTabSwitcherStyle;



@interface NSObject(MTabSwitcherDelegateExtras)
- (BOOL)tabView:(NSTabView*)tabView willCloseTabViewItem:(NSTabViewItem*)item;
- (BOOL)tabView:(NSTabView*)tabView itemHasCloseButton:(NSTabViewItem*)item;
- (NSImage*)tabView:(NSTabView*)tabView iconForItem:(NSTabViewItem*)tabViewItem;
- (void)tabView:(NSTabView*)tabView willDisplayMenu:(NSMenu*)menu forTabViewItem:(NSTabViewItem*)item;
- (void)tabView:(NSTabView*)tabView didReorderTabViewItem:(NSTabViewItem*)item toIndex:(NSInteger)index;
- (NSString*)tabView:(NSTabView*)tabView toolTipForItem:(NSTabViewItem*)item;
@end


@interface MTabSwitcher : NSView <NSTabViewDelegate>
{
@private
  IBOutlet NSTabView *mTabView;
  id mSelectedItem;
  id mDelegate;
  NSMutableDictionary *mLabelAttributes;
  NSMutableDictionary *mLabelDisabledAttributes;
  NSMutableDictionary *mLabelShadowAttributes;
  NSTabViewItem *mHoverItem;
  NSTabViewItem *mClickedItem;
  MTabSwitcherStyle mStyle;
  NSMutableDictionary *mCloseButtonRects;
  NSTrackingArea *mTrack;
  float mReservedSpace;
  float mDefaultMinTabWidth;
  float mMinTabWidth;
  NSPoint mTabDragPosition;
  NSPoint mClickTabOffset;
  NSRect mExternderButtonRect;
  int mFirstVisibleTabIndex;
  int mLastVisibleTabIndex;
  NSProgressIndicator *mBusyTabIndicator;
  NSTabViewItem *mBusyTab;
  NSMutableArray *mToolTipTags;

  BOOL mAllowTabReordering;
  BOOL mInside;
  BOOL mDraggingTab;
  BOOL mUnselected;
  BOOL mCloseHighlighted;
  BOOL mClosePressed;
  BOOL mReorderingTab;
}

@property (assign) float minTabWidth;

- (void)setBusyTab: (NSTabViewItem*)tab;

- (IBAction)handleMenuAction:(id)sender;
- (void)setTabStyle:(MTabSwitcherStyle)style;
- (void)setTabView:(NSTabView*)tabView;
- (void)setAllowTabReordering:(BOOL)flag;

- (NSTabViewItem*)clickedItem;
- (void)closeTabViewItem: (NSTabViewItem*)item;

- (void)setDelegate:(id)delegate;
- (id)delegate;

- (void)tile;

- (void)makeUnselected;

@end
