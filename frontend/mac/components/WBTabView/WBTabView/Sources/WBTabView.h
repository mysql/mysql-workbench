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
