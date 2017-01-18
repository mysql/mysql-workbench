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

#import "WBTabItem.h"

@class WBRightClickThroughView;

// Temporary solution until we are completely on XCode 8 or higher.
#if MAC_OS_X_VERSION_MAX_ALLOWED > 101104
@interface WBTabView : NSTabView<WBTabItemDelegateProtocol, CALayerDelegate>
#else
@interface WBTabView : NSTabView<WBTabItemDelegateProtocol>
#endif

{
  NSTabView* mTabView;
  ResponderLayer* mTabRowLayer;
  WBRightClickThroughView* mTabRowView;
  NSMutableArray* mTabItems;
  WBTabItem* mSelectedTab;
  ResponderLayer* mMouseDownLayer;
  BOOL mEnabled;
  NSString* mLabel;
  WBTabSize mTabSize;
  WBTabDirection mTabDirection;
  WBTabPlacement mTabPlacement;

  WBTabArrow* mLeftArrow;
  NSImage* mLeftArrowIconImage;
  WBTabArrow* mRightArrow;
  NSImage* mRightArrowIconImage;
  WBTabMenuLayer* mTabMenu;
  NSImage* mTabMenuIconImage;
  WBTabDraggerLayer* mDragger;
  NSImage* mDraggerIconImage;

  CGColorRef mColorActiveSelected;
  CGColorRef mColorActiveNotSelected;
  CGColorRef mColorNotActiveSelected;
  CGColorRef mColorNotActiveNotSelected;

  BOOL mDoneCustomizing;
  BOOL mEnablAnimations;
  //	BOOL mAllowsTabReordering;
  NSInteger mFirstVisibleTabIndex;
  NSInteger mLastVisibleTabIndex;
  NSInteger mLastSelectedTabIndex;
  NSInteger mTabScrollOffset;
}

@property(readonly) CGFloat tabAreaHeight;
//- (void) setAllowsTabReordering: (BOOL) yn;
@property(readonly) NSSize contentSize;
@property(readonly, strong) CALayer* shadowLayer;
- (void)doCustomize;
- (void)updateLabelForTabViewItem:(id)identifier;

- (void)createDragger;

- (void)setIcon:(NSImage*)icon forTabViewItem:(id)identifier;
- (void)setColorActiveSelected:(CGColorRef)colorActiveSelected
        colorActiveNotSelected:(CGColorRef)colorActiveNotSelected
        colorNotActiveSelected:(CGColorRef)colorNotActiveSelected
     colorNotActiveNotSelected:(CGColorRef)colorNotActiveNotSelected;

@end

@protocol WBTabViewDelegateProtocol

- (BOOL)tabView:(NSTabView*)tabView willCloseTabViewItem:(NSTabViewItem*)tabViewItem;

- (void)tabView:(NSTabView*)tabView draggedHandleAtOffset:(NSPoint)offset;

- (void)tabViewDraggerClicked:(NSTabView*)tabView;

@end
