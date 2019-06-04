/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

/*!
 @class
 WBTabView

 @abstract
 WBTabView implements a customized look for tab views, while trying to be semantically compatible with NSTabView.
 WBTabView is the base class of several customized classes each implementing a different look.

 @discussion
 An WBTabView inherits from NSTabView. The method -doCustomize does the following:

 1. Adds a tabless NSTabView as subview inside self, and moves any existing tab view item to it.
 2. Creates a view mTabRowView in which it draws its customized tabs.

 The custom tabs are build with a hierarchy of core animation layers.
 */

#import "WBTabView.h"
#import "ResponderLayer.h"
#import "WBTabItem.h"
#import "CGColorUtilities.h"

// Defined in MTabSwitcher.mm.
extern NSDictionary<NSString *, NSColor *> *activeColorsDark;
extern NSDictionary<NSString *, NSColor *> *inactiveColorsDark;
extern NSDictionary<NSString *, NSColor *> *activeColorsLight;
extern NSDictionary<NSString *, NSColor *> *inactiveColorsLight;

extern NSDictionary<NSString *, NSGradient *> *activeGradientsDark;
extern NSDictionary<NSString *, NSGradient *> *inactiveGradientsDark;
extern NSDictionary<NSString *, NSGradient *> *activeGradientsLight;
extern NSDictionary<NSString *, NSGradient *> *inactiveGradientsLight;

#import <Quartz/Quartz.h>

// Import the runtime stuff that enable swizzling.
#import <objc/objc-class.h>

#define TAB_ITEM_SPACING (3)
#define TAB_ITEM_SMALL_SPACING (0)

//----------------------------------------------------------------------------------------------------------------------

// We want to call the tabView's delegate to notify it that the NSTabViewItem has changed its label.
// Since a normal NSTabViewItem in Cocoa does not provide any callack to its delegate, we need to do that ourselves.
// We do it by:
// 1. Implementing a "better" version of the method -setLabel: with the name -alternateSetLabel:
// 2. Later, we exchange the new and rhe original methods. See method_exchangeImplementations().
@interface NSTabViewItem (WBTabView_swizzling_additions)

- (void)alternateSetLabel: (NSString*)newLabel;

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation NSTabViewItem (WBTabView_swizzling_additions)

- (void)alternateSetLabel: (NSString*)newLabel {
  // Calling the same method self does not lead to an infinite recursion, because the
  // method name -alternateSetLabel: now "points" to the original -setLabel:
  [self alternateSetLabel: newLabel];

  if ([self.tabView.superview respondsToSelector: @selector(updateLabelForTabViewItem:)])
    // Then call the tabview's delegate to notify.
    [(WBTabView*)self.tabView.superview updateLabelForTabViewItem: self.identifier];
}

@end

//----------------------------------------------------------------------------------------------------------------------

@interface WBRightClickThroughView : NSView
@end

//----------------------------------------------------------------------------------------------------------------------

@implementation WBRightClickThroughView

- (void)rightMouseDown: (NSEvent*)event {
  [self.superview rightMouseDown: event];
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation WBTabView

- (NSDictionary<NSString *, NSColor *> *)currentColors {
  BOOL isActive = self.window.mainWindow;
  BOOL isDark = NO;
  if (@available(macOS 10.14, *)) {
    isDark = self.window.effectiveAppearance.name == NSAppearanceNameDarkAqua;
  }

  return isActive
    ? (isDark ? activeColorsDark : activeColorsLight)
    : (isDark ? inactiveColorsDark : inactiveColorsLight);
}

//----------------------------------------------------------------------------------------------------------------------

- (void)drawRect: (NSRect)rect {
  [[NSColor colorWithDeviceWhite: 232 / 255.0 alpha:1.0] set];
  NSRectFill(rect);
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)isFlipped {
  return NO;
}

//----------------------------------------------------------------------------------------------------------------------

- (CGFloat)tabAreaHeight {
  return 26.0;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSSize)contentSize {
  NSSize size = NSZeroSize;

  if (mTabView != nil) {
    size = mTabView.contentRect.size;
  }

  return size;
}

//----------------------------------------------------------------------------------------------------------------------

- (WBTabItem*)tabItemWithIdentifier: (id)identifier
                         label: (NSString*)label {
  WBTabItem* item = [WBTabItem tabItemWithOwner: self
                                     identifier: identifier
                                          label: label
                                      direction: mTabDirection
                                      placement: mTabPlacement
                                           size: mTabSize
                                        hasIcon: (mTabSize == WBTabSizeLarge)
                                       canClose: (mTabSize == WBTabSizeLarge)];

  return item;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)updateTabArrowPositions: (BOOL)show {
  CGFloat rowWidth = self.frame.size.width;

  if (show) {
    CGRect r = mLeftArrow.frame;
    if (mEnablAnimations && (r.origin.x < 0)) {
      [NSAnimationContext currentContext].duration = .15;
    }
    r.origin.x = 0;
    mLeftArrow.frame = r;

    r = mTabMenu.frame;
    r.origin.x = rowWidth - mTabMenu.frame.size.width;
    mTabMenu.frame = r;

    r = mRightArrow.frame;
    r.origin.x = rowWidth - mRightArrow.frame.size.width - mTabMenu.frame.size.width;
    mRightArrow.frame = r;
  } else {
    // Hide the arrows.
    CGRect r = mLeftArrow.frame;
    if (mEnablAnimations && (r.origin.x == 0)) {
      [NSAnimationContext currentContext].duration = .15;
    }
    r.origin.x = -r.size.width;
    mLeftArrow.frame = r;

    r = mTabMenu.frame;
    r.origin.x = rowWidth;
    mTabMenu.frame = r;

    r = mRightArrow.frame;
    r.origin.x = rowWidth;
    mRightArrow.frame = r;
  }

  [mLeftArrow setEnabled: show && (mLastSelectedTabIndex > 0)];
  [mRightArrow setEnabled: show && (mLastSelectedTabIndex < (NSInteger)mTabItems.count - 1)];
}

//----------------------------------------------------------------------------------------------------------------------

// Called after setup
- (void)setEnableAnimations {
  mEnablAnimations = YES;
}

//----------------------------------------------------------------------------------------------------------------------

#pragma mark Private API called from public API

- (void)layoutTabItemsDisregardingTabItem: (WBTabItem*)tabItemToDisregard {
  NSInteger oldOffset = mTabScrollOffset;

  // Compute the vertical position of the tabs.
  CGFloat tabHeightHalf = ([mTabItems.lastObject frame].size.height / 2);
  CGFloat horizon; // The horizon is a line in the vertical middle of the tab, that cuts the tab in half.
  if (mTabPlacement == WBTabPlacementTop) {
    // Tabs along the upper edge of tab view.
    if (mTabDirection == WBTabDirectionUp) {
      // Tabs sticking up from the tab view.
      horizon = 0;
    } else {
      // Tabs hanging down from the ceiling pointing towards the tabview,
      // like those hanging from the window's toolbar.
      horizon = mTabRowLayer.frame.size.height;
    }
  } else {
    // Tabs along the lower edge of tab view.
    if (mTabDirection == WBTabDirectionUp) {
      horizon = 0;
      NSAssert(NO, @"Strange tab configuration.");
    } else {
      horizon = mTabRowLayer.frame.size.height;
    }
  }
  CGFloat tabBottom = horizon - tabHeightHalf;

  // Iterate over all tabs, compute the horizontal postition for each tab.
  CGFloat rowWidth = self.frame.size.width;
  NSMutableArray* leftEdges = [NSMutableArray array];
  CGFloat totalWidth = TAB_ITEM_SPACING;
  for (WBTabItem* item in mTabItems) {
    [leftEdges addObject: @((float)totalWidth)];
    totalWidth += item.frame.size.width + (mTabSize == WBTabSizeLarge ? TAB_ITEM_SPACING : TAB_ITEM_SMALL_SPACING);
  }

  CGFloat leftEdge;
  CGFloat rightEdge;
  BOOL overflow = (totalWidth > rowWidth);
  if (overflow) {
    // There are too many tabs to fit on the tab row.
    leftEdge = CGRectGetMaxX(mLeftArrow.frame);
    rightEdge = mRightArrow.frame.origin.x;
  } else {
    // All tabs to fit on the tab row.
    mTabScrollOffset = 0;
    leftEdge = 0;
    rightEdge = rowWidth;
  }

  leftEdge += TAB_ITEM_SPACING;
  rightEdge -= TAB_ITEM_SPACING;

  // Apply the scroll offset to tab left edges.
  NSMutableArray* offsetLeftEdges = [NSMutableArray array];

  NSUInteger i, c = leftEdges.count;
  for (i = 0; i < c; i++) {
    float tabLeftX = [leftEdges[i] floatValue];
    tabLeftX += mTabScrollOffset;
    [offsetLeftEdges addObject: @(tabLeftX)];
  }

  // Adjust the scroll offset if there is still unused space to the right of the tabs.
  CGFloat lastTabRightEdge = [offsetLeftEdges.lastObject floatValue] + [mTabItems.lastObject frame].size.width;
  if (lastTabRightEdge < rightEdge) {
    mTabScrollOffset += (rightEdge - lastTabRightEdge);
    mTabScrollOffset = MIN(mTabScrollOffset, 0);
    offsetLeftEdges = [NSMutableArray array];

    for (NSUInteger i = 0; i < leftEdges.count; ++i) {
      float tabLeftX = [leftEdges[i] floatValue];
      tabLeftX += mTabScrollOffset;
      [offsetLeftEdges addObject: @(tabLeftX)];
    }
  }

  // Adjust the scroll offset if the selected tab is not fully visible.
  if ((mLastSelectedTabIndex != NSNotFound) && (mLastSelectedTabIndex < (NSInteger)mTabItems.count)) {
    WBTabItem* item = mTabItems[mLastSelectedTabIndex];
    float tabLeftX = [offsetLeftEdges[mLastSelectedTabIndex] floatValue];
    float tabRightX = tabLeftX + item.frame.size.width;
    if (tabLeftX < leftEdge) {
      mTabScrollOffset -= (tabLeftX - leftEdge);
    } else if (tabRightX > rightEdge) {
      mTabScrollOffset -= (tabRightX - rightEdge);
    }
  }

  // Apply the new scroll offset to tab left edges.
  offsetLeftEdges = [NSMutableArray array];
  for (NSUInteger i = 0; i < leftEdges.count; ++i) {
    float tabLeftX = [leftEdges[i] floatValue];
    tabLeftX += mTabScrollOffset;
    [offsetLeftEdges addObject: @(tabLeftX)];
  }

  if (mEnablAnimations && (oldOffset != mTabScrollOffset)) {
    [NSAnimationContext currentContext].duration = .15;
  } else {
    [NSAnimationContext currentContext].duration = 0;
  }

  // Show or hide arrows.
  [self updateTabArrowPositions:overflow];

  // Position the actual tab layers.
  for (NSUInteger i = 0; i < mTabItems.count; ++i) {
    WBTabItem* item = mTabItems[i];
    CGRect itemFrame = item.frame;
    if (item != tabItemToDisregard) {
      CGRect r = CGRectMake([offsetLeftEdges[i] floatValue], tabBottom, itemFrame.size.width, itemFrame.size.height);
      item.frame = r;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

// Delayed call after TabView is resized.
- (void)layoutTabItemsQuickly {
  [NSAnimationContext currentContext].duration = 0.0;
  [self layoutTabItemsDisregardingTabItem: nil];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)insertTabItemWithIdentifier: (id)identifier label: (NSString*)label atIndex: (NSUInteger)index {
  WBTabItem* item = [self tabItemWithIdentifier: identifier label: label];
  if (mTabPlacement == WBTabPlacementTop) {
    item.autoresizingMask = (kCALayerMaxXMargin | kCALayerMinYMargin);
  } else {
    item.autoresizingMask = (kCALayerMaxXMargin | kCALayerMaxYMargin);
  }

  item.delegate = self;

  if (mTabItems == nil) {
    mTabItems = [NSMutableArray new];
  }
  if (index == NSUIntegerMax) {
    [mTabItems addObject: item];
    [mTabRowLayer addSublayer: item];
  } else {
    [mTabItems insertObject: item atIndex: index];
    [mTabRowLayer insertSublayer: item atIndex: (unsigned)index];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)removeTabViewWithIdentifier: (id)identifier {
  for (WBTabItem* item in mTabItems) {
    if ([item.identifier isEqual: identifier]) {
      [NSAnimationContext currentContext].duration = 0.0;

      NSInteger index = [mTabItems indexOfObject: item];
      [item removeFromSuperlayer];
      [mTabItems removeObject: item];

      if (mSelectedTab == item) {
        mSelectedTab = nil;
        WBTabItem* newTab = nil;
        if (index == (NSInteger)mTabItems.count) {
          newTab = mTabItems.lastObject;
        } else {
          newTab = mTabItems[index];
        }

        [self selectTabViewItemWithIdentifier: newTab.identifier];
      }

      for (NSTabViewItem* tabViewItem in mTabView.tabViewItems) {
        if (tabViewItem.identifier == identifier) {
          [mTabView removeTabViewItem: tabViewItem];
        }
      }

      break;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

#pragma mark Overriding public NSTabView API

- (void)addTabViewItem: (NSTabViewItem*)tabViewItem {
  if (mTabView != nil) {
    [mTabView addTabViewItem: tabViewItem];

    id identifier = tabViewItem.identifier;
    NSString* label = tabViewItem.label;
    [self insertTabItemWithIdentifier: identifier label: label atIndex: NSUIntegerMax];
    [self layoutTabItemsDisregardingTabItem: nil];

    if (mDoneCustomizing) {
      if ([self.delegate respondsToSelector: @selector(tabViewDidChangeNumberOfTabViewItems:)]) {
        [self.delegate tabViewDidChangeNumberOfTabViewItems: self];
      }
    }

    [self selectLastTabViewItem: self];
  } else {
    [super addTabViewItem: tabViewItem];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)removeTabViewItem: (NSTabViewItem*)tabViewItem {
  if (mTabView != nil) {
    id identifier = tabViewItem.identifier;
    [self removeTabViewWithIdentifier: identifier];

    [NSAnimationContext currentContext].duration = (mEnablAnimations ? 0.15 : 0.0);
    [self layoutTabItemsDisregardingTabItem: nil];

    if (mDoneCustomizing) {
      if ([self.delegate respondsToSelector: @selector(tabViewDidChangeNumberOfTabViewItems:)]) {
        [self.delegate tabViewDidChangeNumberOfTabViewItems: self];
      }
    }
  } else if (mDoneCustomizing) {
    [super removeTabViewItem: tabViewItem];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (NSArray*)tabViewItems {
  if (mTabView != nil)
    return mTabView.tabViewItems;
  else
    return super.tabViewItems;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSTabViewItem*)tabViewItemAtIndex: (NSInteger)index {
  if (mTabView != nil)
    return [mTabView tabViewItemAtIndex: index];
  else
    return [super tabViewItemAtIndex: index];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSInteger)indexOfTabViewItemWithIdentifier: (id)identifier {
  if (mTabView != nil)
    return [mTabView indexOfTabViewItemWithIdentifier: identifier];
  else
    return [super indexOfTabViewItemWithIdentifier: identifier];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSInteger)indexOfTabViewItem: (NSTabViewItem*)tabViewItem {
  return [self indexOfTabViewItemWithIdentifier: tabViewItem.identifier];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)selectTabViewItemWithIdentifier: (id)identifier {
  if (mTabView != nil) {
    NSTabViewItem* tabViewItem = nil;
    for (NSTabViewItem* item in mTabView.tabViewItems) {
      if ([item.identifier isEqual: identifier]) {
        tabViewItem = item;
        break;
      }
    }

    if (tabViewItem != nil) {
      for (WBTabItem* item in mTabItems) {
        if ([item.identifier isEqual: identifier]) {
          BOOL shouldSelect = YES;
          if (mDoneCustomizing) {
            if ([self.delegate respondsToSelector: @selector(tabView: shouldSelectTabViewItem:)]) {
              shouldSelect = [self.delegate tabView: self shouldSelectTabViewItem: tabViewItem];
            }
          }
          if (shouldSelect) {
            if (mDoneCustomizing) {
              if ([self.delegate respondsToSelector: @selector(tabView:willSelectTabViewItem:)]) {
                [self.delegate tabView: self willSelectTabViewItem: tabViewItem];
              }
            }

            id oldTab = mSelectedTab;
            id newTab = item;
            mSelectedTab = newTab;
            [oldTab setState: NSControlStateValueOff];
            [newTab setState: NSControlStateValueOn];
            mLastSelectedTabIndex = [mTabItems indexOfObject: newTab];
            [self layoutTabItemsDisregardingTabItem: nil];

            [mTabView selectTabViewItemWithIdentifier: identifier];

            if (mDoneCustomizing) {
              if ([self.delegate respondsToSelector: @selector(tabView:didSelectTabViewItem:)]) {
                [self.delegate tabView: self didSelectTabViewItem: tabViewItem];
              }
            }
          }
          break;
        }
      }
    }
  } else {
    [super selectTabViewItemWithIdentifier: identifier];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)selectFirstTabViewItem: (id)sender {
  if ((mTabView != nil) && (mTabItems.count > 0)) {
    id identifier = [mTabItems[0] identifier];
    [self selectTabViewItemWithIdentifier: identifier];
  } else {
    [super selectFirstTabViewItem: sender];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)selectLastTabViewItem: (id)sender {
  if (mTabView != nil) {
    id identifier = [mTabItems.lastObject identifier];
    [self selectTabViewItemWithIdentifier: identifier];
  } else {
    [super selectLastTabViewItem: sender];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (NSTabViewItem*)selectedTabViewItem {
  if (mTabView != nil)
    return mTabView.selectedTabViewItem;
  else
    return super.selectedTabViewItem;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSInteger)numberOfTabViewItems {
  if (mTabView != nil)
    return mTabView.numberOfTabViewItems;
  else
    return super.numberOfTabViewItems;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSRect)contentRect {
  if (mTabView != nil)
    return mTabView.contentRect;
  else
    return super.contentRect;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)selectNextTabViewItem: (id)sender {
  NSTabViewItem* item = self.selectedTabViewItem;
  NSInteger index = [self indexOfTabViewItem: item];
  index++;
  if (index < (NSInteger)mTabItems.count) {
    id identifier = [mTabItems[index] identifier];
    [self selectTabViewItemWithIdentifier: identifier];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)selectPreviousTabViewItem: (id)sender {
  NSTabViewItem* item = self.selectedTabViewItem;
  NSInteger index = [self indexOfTabViewItem: item];
  index--;
  if (index >= 0) {
    id identifier = [mTabItems[index] identifier];
    [self selectTabViewItemWithIdentifier: identifier];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)insertTabViewItem: (NSTabViewItem*)tabViewItem atIndex: (NSInteger)index {
  if (mTabView != nil) {
    [mTabView insertTabViewItem: tabViewItem atIndex: index];

    id identifier = tabViewItem.identifier;
    NSString* label = tabViewItem.label;
    [self insertTabItemWithIdentifier: identifier label: label atIndex: index];
    [self layoutTabItemsDisregardingTabItem: nil];

    if (mDoneCustomizing) {
      if ([self.delegate respondsToSelector: @selector(tabViewDidChangeNumberOfTabViewItems:)]) {
        [self.delegate tabViewDidChangeNumberOfTabViewItems: self];
      }
    }

    [self selectLastTabViewItem: self];
  } else {
    [super insertTabViewItem: tabViewItem atIndex: index];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setFrame: (NSRect)frame {
  super.frame = frame;

  [self performSelector: @selector(layoutTabItemsQuickly) withObject: nil afterDelay: 0];
}

//----------------------------------------------------------------------------------------------------------------------

#pragma mark -
#pragma mark TabItemDelegateProtocol methods

- (void)selectTab: (WBTabItem*)sender {
  [NSAnimationContext currentContext].duration = 0.0;
  [self selectTabViewItemWithIdentifier: sender.identifier];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)closeTab: (WBTabItem*)sender {
  if (mDoneCustomizing) {
    if ([self.delegate respondsToSelector: @selector(tabView:willCloseTabViewItem:)]) {
      for (NSTabViewItem* item in mTabView.tabViewItems) {
        if ([item.identifier isEqual: sender.identifier]) {
          if (![(id<WBTabViewDelegateProtocol>)self.delegate tabView: self willCloseTabViewItem: item]) {
            return;
          }
        }
      }
    }
  }

  for (NSTabViewItem* item in mTabView.tabViewItems) {
    if (item.identifier == sender.identifier) {
      [self removeTabViewItem: item];
      break;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (CGRect)tabItem: (WBTabItem*)sender draggedToFrame: (CGRect)r {
  if (r.origin.x < TAB_ITEM_SPACING) {
    r.origin.x = TAB_ITEM_SPACING;
  }

  NSMutableSet* tabsToDisregard = [NSMutableSet set];
  [tabsToDisregard addObject: sender];
  BOOL didMoveOne;
  do {
    didMoveOne = NO;
    NSUInteger ix = [mTabItems indexOfObject: sender];
    NSUInteger i, c = mTabItems.count;
    for (i = 0; i < c; i++) {
      WBTabItem* peer = mTabItems[i];
      if (![tabsToDisregard containsObject:peer]) {
        CGRect peerFrame = peer.frame;
        CGFloat peerMid = CGRectGetMinX(peerFrame) + (peerFrame.size.width / 2);
        if (i > ix) {
          if (CGRectGetMaxX(r) > peerMid) {
            [mTabItems removeObject:peer];
            [mTabItems insertObject:peer atIndex: ix];
            didMoveOne = YES;
            [tabsToDisregard addObject:peer];
            break;
          }
        } else {
          if (CGRectGetMinX(r) < peerMid) {
            [mTabItems removeObject:peer];
            [mTabItems insertObject:peer atIndex: ix];
            didMoveOne = YES;
            [tabsToDisregard addObject:peer];
            break;
          }
        }
      }
    }
  } while (didMoveOne);

  [NSAnimationContext currentContext].duration = (mEnablAnimations ? 0.1 : 0.0);
  [self layoutTabItemsDisregardingTabItem: sender];

  return r;
}

//----------------------------------------------------------------------------------------------------------------------

#pragma mark User interaction

- (void)mouseDown: (NSEvent*)event {
  NSAssert((mMouseDownLayer == nil), @"mMouseDownLayer was not nil.");

  if ((event.modifierFlags & NSEventModifierFlagControl) != 0) {
    [self rightMouseDown: event];
  } else {
    NSPoint loc = [mTabRowView convertPoint: event.locationInWindow fromView: nil];
    CGPoint cgp = NSPointToCGPoint(loc);
    mMouseDownLayer = [mTabRowLayer mouseDownAtPoint: cgp];

    if (event.clickCount > 1) {
      // This was a double click.
      SEL selector = NSSelectorFromString(@"tabViewItemDidReceiveDoubleClick:");
      if ([self.delegate respondsToSelector: selector]) {
        #pragma clang diagnostic ignored "-Warc-performSelector-leaks"
        [self.delegate performSelector: selector withObject: self.selectedTabViewItem];
      }
    }

    if (mMouseDownLayer == nil) {
      [self.nextResponder mouseDown: event];
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)mouseDragged: (NSEvent*)event {
  if (mMouseDownLayer != nil) {
    NSPoint loc = [mTabRowView convertPoint: event.locationInWindow fromView: nil];
    CGPoint cgp = NSPointToCGPoint(loc);
    cgp = [mTabRowLayer convertPoint: cgp toLayer:mMouseDownLayer];
    [mMouseDownLayer mouseDraggedToPoint: cgp];
  } else {
    [self.nextResponder mouseDragged: event];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)mouseUp: (NSEvent*)event {
  if (mMouseDownLayer != nil) {
    [mMouseDownLayer mouseUp];
    mMouseDownLayer = nil;

    [self layoutTabItemsDisregardingTabItem: nil];
  } else {
    [self.nextResponder mouseUp: event];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (NSMenu*)menuForEvent: (NSEvent*)theEvent {
  NSMenu* menu = nil;

  SEL selector = NSSelectorFromString(@"tabView:menuForIdentifier:");
  if ([self.delegate respondsToSelector: selector]) {
    NSPoint loc = [mTabRowView convertPoint: theEvent.locationInWindow fromView: nil];
    CGPoint cgp = NSPointToCGPoint(loc);
    ResponderLayer* item = [mTabRowLayer responderLayerAtPoint: cgp];
    if ([item isKindOfClass:[WBTabItem class]]) {
      menu = [self.delegate performSelector: selector
                                 withObject: self
                                 withObject: ((WBTabItem*)item).identifier];
    }
  }

  return menu;
}

//----------------------------------------------------------------------------------------------------------------------

// Called when user clicks arrows in tab row to switch tabs.
- (void)tabViewArrowAction: (id)sender {
  NSInteger selectedIndex = mLastSelectedTabIndex;
  if (sender == mLeftArrow) {
    selectedIndex--;
  } else if (sender == mRightArrow) {
    selectedIndex++;
  }

  selectedIndex = MAX(selectedIndex, 0);
  selectedIndex = MIN(selectedIndex, (NSInteger)mTabItems.count - 1);
  id selectedTabIdentifier = [mTabItems[selectedIndex] identifier];
  [NSAnimationContext currentContext].duration = 0.0;
  [self selectTabViewItemWithIdentifier: selectedTabIdentifier];
}

//----------------------------------------------------------------------------------------------------------------------

// Called when user picks an item in the tab context menu.
- (void)selectTabViewMenu: (id)sender {
  id identifier = [sender representedObject];
  [self selectTabViewItemWithIdentifier: identifier];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)tabViewMenuAction: (id)sender {
  NSMenu* menu = [[NSMenu alloc] initWithTitle: @"Tabs"];
  for (NSTabViewItem* item in self.tabViewItems) {
    NSMenuItem* menuItem = [menu addItemWithTitle: item.label action: @selector(selectTabViewMenu:) keyEquivalent: @""];
    menuItem.target = self;
    menuItem.representedObject = item.identifier;
    menuItem.state = ([mSelectedTab.identifier isEqual: item.identifier] ? NSControlStateValueOn : NSControlStateValueOff);
  }

  NSEvent* theEvent = NSApp.currentEvent;
  NSPoint location = NSPointFromCGPoint(mTabMenu.frame.origin);
  location = [mTabRowView convertPoint: location toView: nil];
  NSEvent* newEvent = [NSEvent mouseEventWithType: theEvent.type
                                         location: location
                                    modifierFlags: theEvent.modifierFlags
                                        timestamp: theEvent.timestamp
                                     windowNumber: theEvent.windowNumber
                                          context: nil
                                      eventNumber: theEvent.eventNumber
                                       clickCount: theEvent.clickCount
                                         pressure: theEvent.pressure];
  [NSMenu popUpContextMenu:menu withEvent: newEvent forView: self];

  // We want every mouse-down event to be balanced by a mouse-up event.
  // Selecting from the pop-up menu will not cause a mouse-up event, so we
  // insert a -mouseUp: into the event stream.
  [self performSelector: @selector(mouseUp:) withObject: nil afterDelay:0];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)tabViewDragged: (id)sender atPoint: (CGPoint)point {
  if ([self.delegate conformsToProtocol: @protocol(WBTabViewDelegateProtocol)])
    [(id<WBTabViewDelegateProtocol>)self.delegate tabView: self draggedHandleAtOffset: NSPointFromCGPoint(point)];
}

//----------------------------------------------------------------------------------------------------------------------

#pragma mark -

- (void)handleWindowDidBecomeMain: (id)aNotification {
  mEnabled = YES;
  [self updateColors];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)handleWindowDidResignMain: (id)aNotification {
  mEnabled = NO;
  [self updateColors];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)viewWillMoveToWindow: (nullable NSWindow *)newWindow {
  if (self.window != nil)
    [self.window removeObserver: self forKeyPath: @"effectiveAppearance"];
  if (newWindow != nil) {
    [newWindow addObserver: self forKeyPath: @"effectiveAppearance" options: 0 context: nil];
    [self performSelector: @selector(updateColors) withObject: nil afterDelay: 0];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)observeValueForKeyPath: (NSString *)keyPath
                      ofObject: (id)object
                        change: (NSDictionary *)change
                       context: (void *)context {
  if ([keyPath isEqualToString: @"effectiveAppearance"]) {
    [self updateColors];
    return;
  }
  [super observeValueForKeyPath: keyPath ofObject: object change: change context: context];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)updateColors {
  [NSAnimationContext currentContext].duration = 0;

  for (WBTabItem *item in mTabItems) {
    [item updateAppearance];
  }

  NSColor *tabRowBackgroundColor = self.currentColors[@"tabViewBackground"];
  CGColorRef cgColor = tabRowBackgroundColor.CGColor;
  mTabRowLayer.backgroundColor = cgColor;
  mLeftArrow.backgroundColor = cgColor;
  mRightArrow.backgroundColor = cgColor;
  mTabMenu.backgroundColor = cgColor;

  BOOL isDark = NO;
  if (@available(macOS 10.14, *)) {
    isDark = self.window.effectiveAppearance.name == NSAppearanceNameDarkAqua;
  }

  NSBundle *bundle = [NSBundle bundleForClass: WBTabView.class];
  tabMenuImage.contents = [bundle imageForResource: (isDark ? @"TabMenuIconDark" : @"TabMenuIconLight")];
  tabMenuImage.contentsScale = self.window.backingScaleFactor;

  leftArrowImage.contents = [bundle imageForResource: (isDark ? @"LeftArrowIconDark" : @"LeftArrowIconLight")];
  leftArrowImage.contentsScale = self.window.backingScaleFactor;

  rightArrowImage.contents = [bundle imageForResource: (isDark ? @"RightArrowIconDark" : @"RightArrowIconLight")];
  rightArrowImage.contentsScale = self.window.backingScaleFactor;
}

//----------------------------------------------------------------------------------------------------------------------

#pragma mark Setup

- (void)updateLabelForTabViewItem: (id)identifier {
  for (NSTabViewItem* item in self.tabViewItems) {
    if (item.identifier == identifier) {
      NSString* label = item.label;
      for (WBTabItem* customTab in mTabItems) {
        if ([customTab.identifier isEqual: identifier]) {
          [customTab setLabel: label];
        }
      }
      break;
    }
  }
  [self layoutTabItemsDisregardingTabItem: nil];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setIcon: (NSImage*)icon forTabViewItem: (id)identifier {
  for (NSTabViewItem* item in self.tabViewItems) {
    if ([item.identifier isEqual: identifier]) {
      for (WBTabItem* customTab in mTabItems) {
        if ([customTab.identifier isEqual: identifier]) {
          [customTab setIconImage: icon];
        }
      }
      break;
    }
  }
  [self layoutTabItemsDisregardingTabItem: nil];
}

//----------------------------------------------------------------------------------------------------------------------

- (float)contentPadding {
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)makeInitialSwap {
  self.accessibilityTitle = @"Modeling Tab Control";
  self.tabViewType = NSNoTabsNoBorder;

  // Create the NSTabView to hold the actual views.
  {
    NSRect newFrame = self.frame;
    newFrame.origin.x += 7;
    newFrame.size.width -= 14;
    newFrame.size.height -= 16;
    if (mTabPlacement == WBTabPlacementTop) {
      newFrame.origin.y += 10;
    } else {
      newFrame.origin.y += 6;
    }
    self.frame = newFrame;
  }

  // Set up the NSTabView that will contain and display the actual content.
  NSRect contentFrame = self.bounds;
  contentFrame.size.height -= self.tabAreaHeight;
  if (mTabPlacement == WBTabPlacementBottom) {
    contentFrame.origin.y = self.tabAreaHeight;
  }

  float contentPadding = [self contentPadding];
  contentFrame.origin.x += contentPadding;
  contentFrame.size.width -= 2 * contentPadding;
  contentFrame.size.height -= contentPadding;

  NSTabView* theContainerTabView = [[NSTabView alloc] initWithFrame: contentFrame];
  theContainerTabView.tabViewType = NSNoTabsNoBorder;
  theContainerTabView.autoresizingMask = (NSViewWidthSizable | NSViewHeightSizable);

  // Set up the view that will contain the custom tabs.
  NSRect tabRowFrame = self.bounds;
  tabRowFrame.size.height = self.tabAreaHeight;
  if (mTabPlacement == WBTabPlacementTop) {
    tabRowFrame.origin.y = contentFrame.size.height;
  }

  mTabRowView = [[WBRightClickThroughView alloc] initWithFrame: tabRowFrame];
  if (mTabPlacement == WBTabPlacementTop) {
    mTabRowView.autoresizingMask = (NSViewWidthSizable | NSViewMinYMargin);
  } else {
    mTabRowView.autoresizingMask = (NSViewWidthSizable | NSViewMaxYMargin);
  }

  [self setAutoresizesSubviews:YES];

  NSArray* tabViewItems = self.tabViewItems;
  for (NSTabViewItem* item in tabViewItems) {
    [theContainerTabView addTabViewItem: item];
  }

  mTabView = theContainerTabView;

  [self addSubview: mTabRowView];
  mTabRowLayer = [ResponderLayer layer];

  mTabRowLayer.zPosition = -5;

  [mTabRowView setWantsLayer: YES];
  mTabRowView.layer = mTabRowLayer;

  [self addSubview: mTabView];
}

//----------------------------------------------------------------------------------------------------------------------

- (WBTabMenuLayer*)tabMenuLayer {
  WBTabMenuLayer* baseLayer = [WBTabMenuLayer layer];

  // Make layer to hold the arrow icon.
  tabMenuImage = [CALayer layer];
  tabMenuImage.contentsScale = self.window.backingScaleFactor;
  NSBundle* bundle = [NSBundle bundleForClass: WBTabView.class];
  NSImage *image = [bundle imageForResource: @"TabMenuIconLight"];
  CGRect r = CGRectZero;
  r.size = NSSizeToCGSize(image.size);
  r.size.height = mTabRowView.frame.size.height;
  tabMenuImage.frame = r;
  tabMenuImage.contents = image;

  // Set up the base layer and add the icon layer to it.
  r.origin.x = -r.size.width;
  r.size.height -= 1;
  baseLayer.frame = r;
  [baseLayer addSublayer: tabMenuImage];

  baseLayer.autoresizingMask = kCALayerMinXMargin;
  baseLayer.zPosition = -1.01;

  return baseLayer;
}

//----------------------------------------------------------------------------------------------------------------------

- (WBTabArrow*)leftArrowLayer {
  WBTabArrow* baseLayer = [WBTabArrow layer];

  // Make layer to hold the arrow icon.
  leftArrowImage = [CALayer layer];
  leftArrowImage.contentsScale = self.window.backingScaleFactor;
  NSBundle* bundle = [NSBundle bundleForClass:WBTabView.class];
  NSImage *image = [bundle imageForResource: @"LeftArrowIconLight"];
  CGRect r = CGRectZero;
  r.size = NSSizeToCGSize(image.size);
  r.size.height = mTabRowView.frame.size.height;
  leftArrowImage.frame = r;
  leftArrowImage.contents = image;

  // Set up the base layer and add the icon layer to it.
  r.origin.x = -r.size.width;
  r.size.height -= 1;
  baseLayer.frame = r;
  [baseLayer addSublayer: leftArrowImage];

  baseLayer.zPosition = -1.01;

  return baseLayer;
}

//----------------------------------------------------------------------------------------------------------------------

- (WBTabArrow*)rightArrowLayer {
  WBTabArrow* baseLayer = [WBTabArrow layer];

  // Make layer to hold the arrow icon.
  rightArrowImage = [CALayer layer];
  rightArrowImage.contentsScale = self.window.backingScaleFactor;
  NSBundle* bundle = [NSBundle bundleForClass:WBTabView.class];
  NSImage *image = [bundle imageForResource: @"RightArrowIconLight"];
  CGRect r = CGRectZero;
  r.size = NSSizeToCGSize(image.size);
  r.size.height = mTabRowView.frame.size.height;
  rightArrowImage.frame = r;
  rightArrowImage.contents = image;

  // Set up the base layer and add the icon layer to it.
  r.origin.x = 10000;
  r.size.height -= 1;
  baseLayer.frame = r;
  [baseLayer addSublayer: rightArrowImage];

  baseLayer.autoresizingMask = kCALayerMinXMargin;
  baseLayer.zPosition = -1.01;

  return baseLayer;
}

//----------------------------------------------------------------------------------------------------------------------

- (CALayer*)lineLayer {
  // Create a line between non-selected tabs and the tabviewcontants.
  CALayer* lineLayer = [CALayer layer];

  lineLayer.borderWidth = 1;
  CGColorRef c = WB_CGColorCreateCalibratedRGB(0.3, 0.3, 0.3, 0.7);
  lineLayer.borderColor = c;
  CGColorRelease(c);

  CGRect r = CGRectZero;
  r.origin.x = 0;
  if (mTabDirection == WBTabDirectionUp) {
    r.origin.y = 0;
    lineLayer.autoresizingMask = (kCALayerWidthSizable | kCALayerMaxYMargin);
  } else {
    r.origin.y = mTabRowLayer.frame.size.height - 1;
    lineLayer.autoresizingMask = (kCALayerWidthSizable | kCALayerMinYMargin);
  }

  r.size.width = mTabRowLayer.frame.size.width;
  r.size.height = 1;
  lineLayer.frame = r;

  lineLayer.zPosition = -2;
  [mTabRowLayer addSublayer: lineLayer];

  return lineLayer;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)createTabRow {
  // Create left and right arrows, and tab menu icon.
  NSColor *tabRowBackgroundColor = self.currentColors[@"tabViewBackground"];
  CGColorRef cgColor = tabRowBackgroundColor.CGColor;

  mLeftArrow = [self leftArrowLayer];
  mLeftArrow.backgroundColor = cgColor;
  [mTabRowLayer addSublayer:mLeftArrow];
  mLeftArrow.delegate = self;

  mRightArrow = [self rightArrowLayer];
  mRightArrow.backgroundColor = cgColor;
  [mTabRowLayer addSublayer:mRightArrow];
  mRightArrow.delegate = self;

  mTabMenu = [self tabMenuLayer];
  mTabMenu.backgroundColor = cgColor;
  [mTabRowLayer addSublayer:mTabMenu];
  mTabMenu.delegate = self;

  // Populate with tabs.
  NSArray* tabViewItems = mTabView.tabViewItems;
  for (NSTabViewItem* item in tabViewItems) {
    id identifier = item.identifier;
    NSString* label = item.label;
    [self insertTabItemWithIdentifier: identifier label: label atIndex: NSUIntegerMax];
  }

  [self layoutTabItemsDisregardingTabItem: nil];

}

//----------------------------------------------------------------------------------------------------------------------

- (void)doCustomize {
  if (!mDoneCustomizing) {
    if (mTabPlacement == WBTabPlacementUndefined) {
      NSLog(@"* Tab placement undefined. Using WBTabPlacementTop.");
      mTabPlacement = WBTabPlacementTop;
    }

    if (mTabDirection == WBTabDirectionUndefined) {
      NSLog(@"* Tab direction undefined. Using WBTabDirectionUp.");
      mTabDirection = WBTabDirectionUp;
    }

    if (mTabSize == WBTabSizeUndefined) {
      NSLog(@"* Tab size undefined. Using WBTabSizeLarge.");
      mTabSize = WBTabSizeLarge;
    }

    mLastSelectedTabIndex = NSNotFound;

    // Create the new tab view layer hierarchy.
    [self makeInitialSwap];

    // Create our own custom tabs.
    [self createTabRow];

    // Set up notifications.
    NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
    [nc addObserver: self
           selector: @selector(handleWindowDidBecomeMain:)
               name: NSWindowDidBecomeMainNotification
             object: self.window];
    [nc addObserver: self
           selector: @selector(handleWindowDidResignMain:)
               name: NSWindowDidResignMainNotification
             object: self.window];

    [self layoutTabItemsDisregardingTabItem: nil];
    [self performSelector: @selector(layoutTabItemsDisregardingTabItem:) withObject: nil afterDelay: 0];

    mDoneCustomizing = YES;

    // Code can add tabs programatically right after loading the nib, so we
    // delay the call to enable animations until next event loop.
    [self performSelector: @selector(setEnableAnimations) withObject: nil afterDelay: 0];
  }
}

//----------------------------------------------------------------------------------------------------------------------

#pragma mark Create and Destroy

+ (void)load {
  // Substitute -setLabel: method for NSTabVIewItem so that it notifies us so we
  // can update the corresponding custom tab.
  Method originalMethod = class_getInstanceMethod([NSTabViewItem class], @selector(setLabel:));
  Method alternateMethod = class_getInstanceMethod([NSTabViewItem class], @selector(alternateSetLabel:));
  method_exchangeImplementations(originalMethod, alternateMethod);
}

//----------------------------------------------------------------------------------------------------------------------

- (void)awakeFromNib {
  mEnabled = YES;
  [self doCustomize];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)dealloc {
  NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];
  [nc removeObserver: self];

  [NSObject cancelPreviousPerformRequestsWithTarget: self];
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)mouseDownCanMoveWindow {
  return NO;
}

@end

//----------------------------------------------------------------------------------------------------------------------
