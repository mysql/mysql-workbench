/*
 * Copyright (c) 2011, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "MFView.h"
#import "MTabSwitcher.h"

//----------------------------------------------------------------------------------------------------------------------

// Our fake interface for the tab buttons.
@interface TabItemButton : NSAccessibilityElement {
@public
  MTabSwitcher *switcher;
  NSTabViewItem *item;
  TabItemButton *closeButton;
  BOOL isCloseButton;
}
@end

//----------------------------------------------------------------------------------------------------------------------

@implementation TabItemButton

- (id)initWithObject: (NSTabViewItem *)anItem tabSwitcher: (MTabSwitcher *)aSwitcher {
  self = [super init];

  if (self) {
    isCloseButton = NO;
    item = anItem;
    switcher = aSwitcher;
  }

  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString *)accessibilityRole {
  if (item == nil)
    return NSAccessibilityButtonRole;
  return NSAccessibilityRadioButtonRole;
}

//----------------------------------------------------------------------------------------------------------------------

- (id)accessibilityParent {
  return switcher;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)accessibilityPerformPress {
  if (item == nil)
    return false;

  if (isCloseButton) {
    if ([switcher allowClosingItem: item])
      [switcher closeTabViewItem: item];
  } else {
    [switcher makeTabVisibleAndSelect: item];
  }
  return true;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString *)accessibilityIdentifier {
  return item.identifier;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString *)accessibilityTitle {
  return item.label;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString *)accessibilityValue {
  return (item.tabState == NSSelectedTab) ? @"1" : @"0";
}

//----------------------------------------------------------------------------------------------------------------------

- (NSRect)accessibilityFrame {
  NSRect frame = [switcher tabItemRect: item];
  frame = [switcher convertRect: frame toView: nil];
  frame = [[switcher window] convertRectToScreen: frame];

  return frame;
}

//----------------------------------------------------------------------------------------------------------------------

- (id)accessibilityCloseButton {
  if (![switcher hasCloseButton: item])
    return nil;

  if (closeButton == nil) {
    // Cannot create the button in init or we get an endless recursion there.
    closeButton = [[TabItemButton alloc] initWithObject: item tabSwitcher: switcher];
    closeButton->isCloseButton = YES;
  }

  return closeButton;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)accessibilityPerformShowMenu {
  NSMenu *menu = item != nil ? [switcher prepareMenuForItem: item] : [switcher prepareMenuForTabs];
  if (menu != nil) {
    NSRect frame = [switcher tabItemRect: item];
    return [menu popUpMenuPositioningItem: nil atLocation: NSMakePoint(NSMidX(frame), NSMidY(frame)) inView: switcher];
  }
  return false;
}

@end

//----------------------------------------------------------------------------------------------------------------------

// Helper class to store close/pin button info per tab item.
@interface ButtonInfo : NSObject {
}
@property NSRect bounds;
@property BOOL hit;
@property BOOL pressed;
@property BOOL visible;
@end;

@implementation ButtonInfo {}; @end;

@interface MTabSwitcher () {
@private
  id mSelectedItem;

  NSMutableDictionary *mLabelAttributes;
  NSMutableDictionary *extenderAttributes;

  NSTabViewItem *hotItem;
  NSTabViewItem *clickedItem;

  NSMutableDictionary<id, ButtonInfo *> *closeButtonInfo;
  NSMutableDictionary<id, ButtonInfo *> *pinButtonInfo;

  NSSize extenderSize;
  ButtonInfo *extenderButtonInfo;

  NSTrackingArea *mTrack;
  float mReservedSpace;
  float defaultMinTabWidth;
  NSPoint mTabDragPosition;
  NSPoint mClickTabOffset;
  NSInteger mFirstVisibleTabIndex;
  NSInteger mLastVisibleTabIndex;
  NSProgressIndicator *mBusyTabIndicator;
  NSTabViewItem *mBusyTab;
  NSMutableArray *mToolTipTags;

  BOOL draggingTab;
  BOOL reorderingTab;

  NSRect mPinRect;

  // Have to store our accessibility objects locally to keep them alive.
  NSMutableArray *accessibilityTabs;
  NSMutableArray *accessibilityChildren;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation MTabSwitcher

@synthesize delegate;
@synthesize minTabWidth;
@synthesize tabStyle;
@synthesize allowTabReordering;

NSDictionary<NSString *, NSImage *> *activeImagesDark;
NSDictionary<NSString *, NSImage *> *inactiveImagesDark;
NSDictionary<NSString *, NSImage *> *activeImagesLight;
NSDictionary<NSString *, NSImage *> *inactiveImagesLight;

NSDictionary<NSString *, NSColor *> *activeColorsDark;
NSDictionary<NSString *, NSColor *> *inactiveColorsDark;
NSDictionary<NSString *, NSColor *> *activeColorsLight;
NSDictionary<NSString *, NSColor *> *inactiveColorsLight;

NSDictionary<NSString *, NSGradient *> *activeGradientsDark;
NSDictionary<NSString *, NSGradient *> *inactiveGradientsDark;
NSDictionary<NSString *, NSGradient *> *activeGradientsLight;
NSDictionary<NSString *, NSGradient *> *inactiveGradientsLight;

static NSString *extender = @"≫";

//----------------------------------------------------------------------------------------------------------------------

+ (void)initialize {
  [self exposeBinding: @"tabStyle"];

  activeImagesDark = @{
    @"homeTabUnselected": [NSImage imageNamed: @"maintab_home_white"],
    @"homeTabSelected": [NSImage imageNamed: @"maintab_home_white"],
    @"tabClose": [NSImage imageNamed: @"wb_tab_close_dark"],
    @"pinned": [NSImage imageNamed: @"qe_sql-editor-resultset-tb-pinned"],
    @"unpinned": [NSImage imageNamed: @"qe_sql-editor-resultset-tb-pin"],
  };

  inactiveImagesDark = @{
    @"homeTabUnselected": [NSImage imageNamed: @"maintab_home_white"],
    @"homeTabSelected": [NSImage imageNamed: @"maintab_home_white"],
    @"tabClose": [NSImage imageNamed: @"wb_tab_close_dark"],
    @"pinned": [NSImage imageNamed: @"qe_sql-editor-resultset-tb-pinned"],
    @"unpinned": [NSImage imageNamed: @"qe_sql-editor-resultset-tb-pin"],
  };

  activeImagesLight = @{
    @"homeTabUnselected": [NSImage imageNamed: @"maintab_home_black"],
    @"homeTabSelected": [NSImage imageNamed: @"maintab_home_white"],
    @"tabClose": [NSImage imageNamed: @"wb_tab_close_light"],
    @"pinned": [NSImage imageNamed: @"qe_sql-editor-resultset-tb-pinned"],
    @"unpinned": [NSImage imageNamed: @"qe_sql-editor-resultset-tb-pin"],
  };

  inactiveImagesLight = @{
    @"homeTabUnselected": [NSImage imageNamed: @"maintab_home_black"],
    @"homeTabSelected": [NSImage imageNamed: @"maintab_home_white"],
    @"tabClose": [NSImage imageNamed: @"wb_tab_close_light"],
    @"pinned": [NSImage imageNamed: @"qe_sql-editor-resultset-tb-pinned"],
    @"unpinned": [NSImage imageNamed: @"qe_sql-editor-resultset-tb-pin"],
  };

  activeColorsDark = @{
    @"tabViewBackground": [NSColor colorWithDeviceRed: 0x27 / 255.0 green: 0x28 / 255.0 blue: 0x2c / 255.0 alpha: 1.0],
    @"tabLabelUnselected": [NSColor colorWithDeviceRed: 0x9e / 255.0 green: 0xa1 / 255.0 blue: 0xa4 / 255.0 alpha: 1.0],
    @"tabLabelSelected": [NSColor colorWithDeviceRed: 0xff / 255.0 green: 0xff / 255.0 blue: 0xff / 255.0 alpha: 1.0],
    @"tabBackgroundHot": [NSColor colorWithDeviceRed: 0x22 / 255.0 green: 0x25 / 255.0 blue: 0x28 / 255.0 alpha: 1.0],
    @"tabBackgroundSelected": [NSColor colorWithDeviceRed: 0x37 / 255.0 green: 0x3a / 255.0 blue: 0x3f / 255.0 alpha: 1.0],
    @"closeButtonHit": [NSColor colorWithDeviceWhite: 1 alpha: 0.2],
    @"closeButtonPressed": [NSColor colorWithDeviceWhite: 1 alpha: 0.3],
    @"borderTop": [NSColor colorWithDeviceRed: 0x5e / 255.0 green: 0x5c / 255.0 blue: 0x58 / 255.0 alpha: 1.0],
    @"borderBottom": [NSColor colorWithDeviceRed: 0 / 255.0 green: 0 / 255.0 blue: 0 / 255.0 alpha: 1.0],
  };

  inactiveColorsDark = @{
    @"tabViewBackground": [NSColor colorWithDeviceRed: 0x2a / 255.0 green: 0x26 / 255.0 blue: 0x25 / 255.0 alpha: 1.0],
    @"tabLabelUnselected": [NSColor colorWithDeviceRed: 0x67 / 255.0 green: 0x63 / 255.0 blue: 0x62 / 255.0 alpha: 1.0],
    @"tabLabelSelected": [NSColor colorWithDeviceRed: 0x7d / 255.0 green: 0x78 / 255.0 blue: 0x76 / 255.0 alpha: 1.0],
    @"tabBackgroundHot": [NSColor colorWithDeviceRed: 0x25 / 255.0 green: 0x24 / 255.0 blue: 0x20 / 255.0 alpha: 1.0],
    @"tabBackgroundSelected": [NSColor colorWithDeviceRed: 0x3c / 255.0 green: 0x37 / 255.0 blue: 0x35 / 255.0 alpha: 1.0],
    @"closeButtonHit": [NSColor colorWithDeviceWhite: 1 alpha: 0],
    @"closeButtonPressed": [NSColor colorWithDeviceWhite: 1 alpha: 0.25],
    @"borderTop": [NSColor colorWithDeviceRed: 0x57 / 255.0 green: 0x55 / 255.0 blue: 0x50 / 255.0 alpha: 1.0],
    @"borderBottom": [NSColor colorWithDeviceWhite: 0 alpha: 1.0],
  };

  activeColorsLight = @{
    @"tabViewBackground": [NSColor colorWithDeviceWhite: 0xbc / 255.0 alpha: 1.0],
    @"tabLabelUnselected": [NSColor colorWithDeviceWhite: 0x28 / 255.0 alpha: 1.0],
    @"tabLabelSelected": [NSColor colorWithDeviceWhite: 0x1c / 255.0 alpha: 1.0],
    @"tabBackgroundHot": [NSColor colorWithDeviceWhite: 0xa8 / 255.0 alpha: 1.0],
    @"tabBackgroundSelected": [NSColor colorWithDeviceWhite: 0xca / 255.0 alpha: 1.0],
    @"closeButtonHit": [NSColor colorWithDeviceWhite: 0x0 alpha: 0.08],
    @"closeButtonPressed": [NSColor colorWithDeviceWhite: 0x0 alpha: 0.2],
    @"borderTop": [NSColor colorWithDeviceRed: 0xaf / 255.0 green: 0xae / 255.0 blue: 0xae / 255.0 alpha: 1.0],
    @"borderBottom": [NSColor colorWithDeviceWhite: 0xa7 / 255.0 alpha: 1.0],
  };

  inactiveColorsLight = @{
    @"tabViewBackground": [NSColor colorWithDeviceWhite: 0xdd / 255.0 alpha: 1.0],
    @"tabLabelUnselected": [NSColor colorWithDeviceWhite: 0x95 / 255.0 alpha: 1.0],
    @"tabLabelSelected": [NSColor colorWithDeviceWhite: 0xbd / 255.0 alpha: 1.0],
    @"tabBackgroundHot": [NSColor colorWithDeviceWhite: 0xc6 / 255.0 alpha: 1.0],
    @"tabBackgroundSelected": [NSColor colorWithDeviceWhite: 0xf6 / 255.0 alpha: 1.0],
    @"closeButtonHit": [NSColor colorWithDeviceWhite: 0xe9 / 255.0 alpha: 1.0],
    @"closeButtonPressed": [NSColor colorWithDeviceWhite: 0xe9 / 255.0 alpha: 1.0],
    @"borderTop": [NSColor colorWithDeviceWhite: 0xd1 / 255.0 alpha: 1.0],
    @"borderBottom": [NSColor colorWithDeviceWhite: 0xd1 / 255.0 alpha: 1.0],
  };

  activeGradientsDark = @{
    @"tabViewBackground": [[NSGradient alloc] initWithColors: @[
      [NSColor colorWithDeviceWhite: 0x2b / 255.0 alpha: 1.0],
      [NSColor colorWithDeviceWhite: 0x29 / 255.0 alpha: 1.0]
    ]],
    @"tabViewBackgroundBottom": [[NSGradient alloc] initWithColors: @[
      [NSColor colorWithDeviceWhite: 0x2b / 255.0 alpha: 1.0],
      [NSColor colorWithDeviceWhite: 0x29 / 255.0 alpha: 1.0]
    ]],
    @"tabBackgroundHot": [[NSGradient alloc] initWithColors: @[
      [NSColor colorWithDeviceWhite: 0x22 / 255.0 alpha: 1.0],
      [NSColor colorWithDeviceWhite: 0x24 / 255.0 alpha: 1.0]
    ]],
    @"tabBackgroundSelected": [[NSGradient alloc] initWithColors: @[
      [NSColor colorWithDeviceRed: 0x3d / 255.0 green: 0x3e / 255.0 blue: 0x42 / 255.0 alpha: 1.0],
      [NSColor colorWithDeviceRed: 0x39 / 255.0 green: 0x3e / 255.0 blue: 0x3b / 255.0 alpha: 1.0],
    ]],
    @"tabBackgroundSelectedBottom": [[NSGradient alloc] initWithColors: @[
      [NSColor colorWithDeviceRed: 0x3d / 255.0 green: 0x3e / 255.0 blue: 0x42 / 255.0 alpha: 1.0],
      [NSColor colorWithDeviceRed: 0x39 / 255.0 green: 0x3e / 255.0 blue: 0x3b / 255.0 alpha: 1.0],
    ]],
    @"separator": [[NSGradient alloc] initWithColors: @[
      [NSColor colorWithDeviceRed: 0x67 / 255.0 green: 0x64 / 255.0 blue: 0x5f / 255.0 alpha: 1.0],
      [NSColor colorWithDeviceRed: 0x63 / 255.0 green: 0x61 / 255.0 blue: 0x5c / 255.0 alpha: 1.0],
    ]],
  };

  inactiveGradientsDark = @{
    @"tabViewBackground": [[NSGradient alloc] initWithColors: @[
      [NSColor colorWithDeviceRed: 0x23 / 255.0 green: 0x24 / 255.0 blue: 0x26 / 255.0 alpha: 1.0],
      [NSColor colorWithDeviceRed: 0x23 / 255.0 green: 0x24 / 255.0 blue: 0x26 / 255.0 alpha: 1.0],
    ]],
    @"tabViewBackgroundBottom": [[NSGradient alloc] initWithColors: @[
      [NSColor colorWithDeviceWhite: 0x23 / 255.0 alpha: 1.0],
      [NSColor colorWithDeviceWhite: 0x23 / 255.0 alpha: 1.0]
    ]],
    @"tabBackgroundHot": [[NSGradient alloc] initWithColors: @[
      [NSColor colorWithDeviceRed: 0x1f / 255.0 green: 0x20 / 255.0 blue: 0x22 / 255.0 alpha: 1.0],
      [NSColor colorWithDeviceRed: 0x1f / 255.0 green: 0x20 / 255.0 blue: 0x22 / 255.0 alpha: 1.0],
    ]],
    @"tabBackgroundSelected": [[NSGradient alloc] initWithColors: @[
      [NSColor colorWithDeviceRed: 0x32 / 255.0 green: 0x34 / 255.0 blue: 0x37 / 255.0 alpha: 1.0],
      [NSColor colorWithDeviceRed: 0x32 / 255.0 green: 0x34 / 255.0 blue: 0x37 / 255.0 alpha: 1.0],
    ]],
    @"tabBackgroundSelectedBottom": [[NSGradient alloc] initWithColors: @[
      [NSColor colorWithDeviceRed: 0x32 / 255.0 green: 0x34 / 255.0 blue: 0x37 / 255.0 alpha: 1.0],
      [NSColor colorWithDeviceRed: 0x32 / 255.0 green: 0x34 / 255.0 blue: 0x37 / 255.0 alpha: 1.0],
    ]],
    @"separator": [[NSGradient alloc] initWithColors: @[
      [NSColor colorWithDeviceRed: 0x59 / 255.0 green: 0x57 / 255.0 blue: 0x51 / 255.0 alpha: 1.0],
      [NSColor colorWithDeviceRed: 0x59 / 255.0 green: 0x57 / 255.0 blue: 0x51 / 255.0 alpha: 1.0],
    ]],
  };

  activeGradientsLight = @{
    @"tabViewBackground": [[NSGradient alloc] initWithColors: @[
      [NSColor colorWithDeviceWhite: 0xbd / 255.0 alpha: 1.0],
      [NSColor colorWithDeviceWhite: 0xb7 / 255.0 alpha: 1.0]
    ]],
    @"tabViewBackgroundBottom": [[NSGradient alloc] initWithColors: @[
      [NSColor colorWithDeviceWhite: 0xcd / 255.0 alpha: 1.0],
      [NSColor colorWithDeviceWhite: 0xc7 / 255.0 alpha: 1.0]
    ]],
    @"tabBackgroundHot": [[NSGradient alloc] initWithColors: @[
      [NSColor colorWithDeviceWhite: 0xae / 255.0 alpha: 1.0],
      [NSColor colorWithDeviceWhite: 0xa4 / 255.0 alpha: 1.0]
    ]],
    @"tabBackgroundSelected": [[NSGradient alloc] initWithColors: @[
      [NSColor colorWithDeviceWhite: 0xd1 / 255.0 alpha: 1.0],
      [NSColor colorWithDeviceWhite: 0xca / 255.0 alpha: 1.0]
    ]],
    @"tabBackgroundSelectedBottom": [[NSGradient alloc] initWithColors: @[
      [NSColor colorWithDeviceWhite: 0xff / 255.0 alpha: 1.0],
      [NSColor colorWithDeviceWhite: 0xf8 / 255.0 alpha: 1.0],
    ]],
    @"separator": [[NSGradient alloc] initWithColors: @[
      [NSColor colorWithDeviceRed: 0xad / 255.0 green: 0xae / 255.0 blue: 0xae / 255.0 alpha: 1.0],
      [NSColor colorWithDeviceRed: 0xa8 / 255.0 green: 0xa7 / 255.0 blue: 0xa7 / 255.0 alpha: 1.0],
    ]],
  };

  inactiveGradientsLight = @{
    @"tabViewBackground": [[NSGradient alloc] initWithColors: @[
      [NSColor colorWithDeviceWhite: 0xdd / 255.0 alpha: 1.0],
      [NSColor colorWithDeviceWhite: 0xdd / 255.0 alpha: 1.0]
    ]],
    @"tabViewBackgroundBottom": [[NSGradient alloc] initWithColors: @[
      [NSColor colorWithDeviceWhite: 0xcd / 255.0 alpha: 1.0],
      [NSColor colorWithDeviceWhite: 0xcd / 255.0 alpha: 1.0]
    ]],
    @"tabBackgroundHot": [[NSGradient alloc] initWithColors: @[
      [NSColor colorWithDeviceWhite: 0xc6 / 255.0 alpha: 1.0],
      [NSColor colorWithDeviceWhite: 0xbc / 255.0 alpha: 1.0]
    ]],
    @"tabBackgroundSelected": [[NSGradient alloc] initWithColors: @[
      [NSColor colorWithDeviceWhite: 0xf9 / 255.0 alpha: 1.0],
      [NSColor colorWithDeviceWhite: 0xf5 / 255.0 alpha: 1.0]
    ]],
    @"tabBackgroundSelectedBottom": [[NSGradient alloc] initWithColors: @[
      [NSColor colorWithDeviceWhite: 0xff / 255.0 alpha: 0.8],
      [NSColor colorWithDeviceWhite: 0xf8 / 255.0 alpha: 0.8],
    ]],
    @"separator": [[NSGradient alloc] initWithColors: @[
      [NSColor colorWithDeviceRed: 0xd2 / 255.0 green: 0xd2 / 255.0 blue: 0xd2 / 255.0 alpha: 1.0],
      [NSColor colorWithDeviceRed: 0xd2 / 255.0 green: 0xd2 / 255.0 blue: 0xd2 / 255.0 alpha: 1.0],
    ]],
  };
}

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithFrame: (NSRect)frameRect {
  self = [super initWithFrame: frameRect];
  if (self != nil) {
    // Make editor the default. Object editors rely on this default.
    self.tabStyle = MEditorBottomTabSwitcher;

    mToolTipTags = [NSMutableArray new];
    accessibilityTabs = [NSMutableArray new];
    accessibilityChildren = [NSMutableArray new];

    mTrack = [[NSTrackingArea alloc]
      initWithRect: self.bounds
           options: NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingActiveAlways
             owner: self
          userInfo: nil];
    [self addTrackingArea: mTrack];

    closeButtonInfo = [NSMutableDictionary new];
    pinButtonInfo = [NSMutableDictionary new];

    extenderAttributes = [[NSMutableDictionary alloc] initWithObjectsAndKeys:
                          [NSFont boldSystemFontOfSize: 20], NSFontAttributeName,
                          nil];
    extenderSize = [extender sizeWithAttributes: extenderAttributes];
    extenderButtonInfo = [ButtonInfo new];

    [[NSNotificationCenter defaultCenter] addObserver: self
                                             selector: @selector(frameDidChange:)
                                                 name: NSViewFrameDidChangeNotification
                                               object: self];

    [[NSNotificationCenter defaultCenter] addObserver: self
                                             selector: @selector(windowBecameKey:)
                                                 name: NSWindowDidBecomeKeyNotification
                                               object: nil];
    [[NSNotificationCenter defaultCenter] addObserver: self
                                             selector: @selector(windowResignedKey:)
                                                 name: NSWindowDidResignKeyNotification
                                               object: nil];
  }

  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver: self];

  for (id tag in mToolTipTags)
    [self removeToolTip: [tag intValue]];
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)isAccessibilityElement {
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (id)accessibilityHitTest: (NSPoint)point {
  NSRect rect = NSMakeRect(point.x, point.y, 0, 0);
  rect = [self.window convertRectFromScreen: rect];
  NSPoint localPoint = [self convertPoint: rect.origin fromView: nil];

  for (TabItemButton *button in accessibilityTabs) {
    NSRect bounds = [self tabItemRect: button->item];
    if (NSPointInRect(localPoint, bounds))
      return button;
  }

  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString *)accessibilityRole {
  return NSAccessibilityTabGroupRole;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSArray *)accessibilityTabs {
  // We are mirroring here what the standard tabview does. For each tab page exists a radio button as tab item.
  [accessibilityTabs removeAllObjects];
  for (NSTabViewItem *item in self.mTabView.tabViewItems) {
    TabItemButton *button = [[TabItemButton alloc] initWithObject: item tabSwitcher: self];
    [accessibilityTabs addObject: button];
  }

  if (extenderButtonInfo.visible && NSWidth(extenderButtonInfo.bounds) > 0) {
    TabItemButton *button = [[TabItemButton alloc] initWithObject: nil tabSwitcher: self];
    [accessibilityTabs addObject: button];
  }

  return accessibilityTabs;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSArray *)accessibilityChildren {
  [accessibilityChildren removeAllObjects];
  [accessibilityChildren addObjectsFromArray: accessibilityTabs];

  return accessibilityChildren;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setTabView: (NSTabView *)tabView {
  if (self.mTabView != tabView) {
    if (self.mTabView)
      self.mTabView.delegate = delegate;

    self.mTabView = tabView;
    delegate = self.mTabView.delegate;
    self.mTabView.delegate = self;
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (NSSize)sizeOfTabViewItem: (NSTabViewItem *)item {
  NSSize size = { NSWidth(self.frame), NSHeight(self.frame) };

  if (item != nil) {
    NSImage *icon = nil;

    switch (tabStyle) {
      case MSectionTabSwitcher: {
        int visibleCount = 0;
        for (NSTabViewItem *item in self.mTabView.tabViewItems) {
          if (!item.view.hidden)
          ++visibleCount;
        }
        size.width /= visibleCount;

        if (size.width < minTabWidth)
          size.width = minTabWidth;

        break;
      }

      case MEditorTabSwitcher:
        size.width = MAX(ceil([item.label sizeWithAttributes: mLabelAttributes].width) + 23 * 2, minTabWidth);
        break;

      case MEditorBottomTabSwitcher: {
        size.width = MAX(ceil([item.label sizeWithAttributes: mLabelAttributes].width) + 20, minTabWidth);
        if ([delegate respondsToSelector: @selector(tabView: iconForItem:)] &&
            (icon = [delegate tabView: self.mTabView iconForItem: item]))
          size.width += icon.size.width + 5;
        break;
      }

      case MEditorBottomTabSwitcherPinnable: {
        auto images = self.currentImages;

        size.width = MAX(ceil([item.label sizeWithAttributes: mLabelAttributes].width) + 20, minTabWidth);
        if ([delegate respondsToSelector: @selector(tabView: iconForItem:)] &&
            (icon = [delegate tabView: self.mTabView iconForItem: item]))
          size.width += icon.size.width;
        size.width += images[@"pinned"].size.width;
        break;
      }

      case MMainTabSwitcher:
        if (!item.label)
          size.width = 85; // The width of the left hand sidebar.
        else {
          auto images = self.currentImages;
          size.width = MAX(ceil([item.label sizeWithAttributes: mLabelAttributes].width) + 32 +
                           images[@"tabClose"].size.width, minTabWidth);
        }
        break;
    }
  } else {
    // return padding here
    switch (tabStyle) {
      case MSectionTabSwitcher:
        for (NSTabViewItem *item in self.mTabView.tabViewItems) {
          if (!item.view.hidden)
            size.width += [self sizeOfTabViewItem: item].width;
        }
        size.width = floor((NSWidth(self.frame) - size.width) / 2);
        if (size.width < 0)
          size.width = 1;
        break;

      case MEditorTabSwitcher:
        size.width = 10;
        break;

      case MEditorBottomTabSwitcher:
      case MEditorBottomTabSwitcherPinnable:
        size.width = 10;
        break;

      case MMainTabSwitcher:
        size.width = 8;
        break;
    }
  }
  return size;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setTabStyle: (MTabSwitcherStyle)style {
  tabStyle = style;
  [self setNeedsDisplay: YES];

  switch (style) {
    case MSectionTabSwitcher: {
      NSMutableParagraphStyle *style = [[NSMutableParagraphStyle defaultParagraphStyle] mutableCopy];
      style.alignment = NSTextAlignmentCenter;
      style.lineBreakMode = NSLineBreakByTruncatingTail;

      mLabelAttributes = [[NSMutableDictionary alloc] initWithObjectsAndKeys:
                          [NSFont boldSystemFontOfSize: NSFont.labelFontSize], NSFontAttributeName,
                          NSColor.blackColor, NSForegroundColorAttributeName,
                          style, NSParagraphStyleAttributeName,
                          nil];
      minTabWidth = 70;
      defaultMinTabWidth = minTabWidth;
      break;
    }

    case MEditorTabSwitcher:
    case MEditorBottomTabSwitcher:
    case MEditorBottomTabSwitcherPinnable: {
      NSMutableParagraphStyle *style = [[NSMutableParagraphStyle defaultParagraphStyle] mutableCopy];
      style.alignment = NSTextAlignmentCenter;
      style.lineBreakMode = NSLineBreakByTruncatingTail;

      mLabelAttributes = [[NSMutableDictionary alloc] initWithObjectsAndKeys:
                          [NSFont systemFontOfSize: [NSFont systemFontSizeForControlSize: NSControlSizeSmall]],
                          NSFontAttributeName,
                          style, NSParagraphStyleAttributeName,
                          nil];
      minTabWidth = 100;
      defaultMinTabWidth = minTabWidth;
      break;
    }

    case MMainTabSwitcher:
      mLabelAttributes = [[NSMutableDictionary alloc] initWithObjectsAndKeys:
        [NSFont systemFontOfSize: 11.5], NSFontAttributeName, NSColor.blackColor, NSForegroundColorAttributeName, nil];
      minTabWidth = 50;
      defaultMinTabWidth = minTabWidth;
      break;
  }

  [self setFrameSize: NSMakeSize(NSWidth(self.frame), [self sizeOfTabViewItem: nil].height)];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSRect)tabItemRect: (NSTabViewItem *)aItem {
  if (aItem == nil) {
    if (extenderButtonInfo.visible)
      return extenderButtonInfo.bounds;
    return NSZeroRect;
  }

  int skip = mFirstVisibleTabIndex;
  float x = 0;
  for (NSTabViewItem *item in self.mTabView.tabViewItems) {
    if (item.view.hidden)
      continue;
    if (skip > 0) {
      skip--;
      continue;
    }
    NSSize size = [self sizeOfTabViewItem: item];
    if (item == aItem)
      return NSMakeRect(x, 0, size.width, size.height);
    x += size.width;
  }
  return NSZeroRect;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSTabViewItem *)tabViewItemAtPoint: (NSPoint)pos {
  // If extender button is visible then all x positions right to it (minus the spacing) don't hit a tab.
  if (extenderButtonInfo.visible && pos.x > NSMinX(extenderButtonInfo.bounds) - 4)
    return nil;

  if (pos.y < 0 || pos.y > NSHeight(self.frame))
    return nil;

  switch (tabStyle) {
    // Section tabs always share the available space equally.
    case MSectionTabSwitcher: {
      int visibleCount = 0;
      for (NSTabViewItem *item in self.mTabView.tabViewItems) {
        if (!item.view.hidden)
          ++visibleCount;
      }
      if (visibleCount == 0)
        return nil;

      int tabWidth = NSWidth(self.frame) / visibleCount;
      return self.mTabView.tabViewItems[MIN(int(pos.x) / tabWidth, visibleCount - 1)];
    }

    default:
      break;
  }

  int skip = mFirstVisibleTabIndex;
  float x = 0;
  for (NSTabViewItem *item in self.mTabView.tabViewItems) {
    if (item.view.hidden)
      continue;
    if (skip > 0) {
      skip--;
      continue;
    }
    float w = [self sizeOfTabViewItem: item].width;
    if (pos.x >= x && pos.x < x + w) {
      return item;
    }
    x += w;
  }
  return nil;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSSize)minimumSize {
  return NSMakeSize(0, [self sizeOfTabViewItem: nil].height);
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)expandsOnLayoutVertically: (BOOL)vertically {
  if (vertically)
    return NO;
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)hasCloseButton: (NSTabViewItem *)item {
  return [delegate respondsToSelector: @selector(tabView: itemHasCloseButton:)] &&
         [delegate tabView: self.mTabView itemHasCloseButton: item];
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)allowClosingItem: (NSTabViewItem *)item {
  return ![delegate respondsToSelector: @selector(tabView: itemHasCloseButton:)] ||
         [delegate tabView: self.mTabView itemHasCloseButton: item];
}

//----------------------------------------------------------------------------------------------------------------------

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

- (NSDictionary<NSString *, NSGradient *> *)currentGradients {
  BOOL isActive = self.window.mainWindow;
  BOOL isDark = NO;
  if (@available(macOS 10.14, *)) {
    isDark = self.window.effectiveAppearance.name == NSAppearanceNameDarkAqua;
  }

  return isActive
    ? (isDark ? activeGradientsDark : activeGradientsLight)
    : (isDark ? inactiveGradientsDark : inactiveGradientsLight);
}

//----------------------------------------------------------------------------------------------------------------------

- (NSDictionary<NSString *, NSImage *> *)currentImages {
  BOOL isActive = self.window.mainWindow;
  BOOL isDark = NO;
  if (@available(macOS 10.14, *)) {
    isDark = self.window.effectiveAppearance.name == NSAppearanceNameDarkAqua;
  }

  return isActive
    ? (isDark ? activeImagesDark : activeImagesLight)
    : (isDark ? inactiveImagesDark : inactiveImagesLight);
}

//----------------------------------------------------------------------------------------------------------------------

- (float)drawMainTabStyleInFrame: (NSRect)rect forItem: (NSTabViewItem *)item {
  float x = NSMinX(rect);
  float width = [self sizeOfTabViewItem: item].width;
  if (width > NSWidth(rect))
    return 0;

  rect.size.width = width - 1;

  auto colors = self.currentColors;
  auto gradients = self.currentGradients;
  auto images = self.currentImages;

  --rect.size.height;
  if (!item.label) {
    // The home screen tab is the only one without a label.
    rect.size.width = width;
    if (item.tabState != NSBackgroundTab) {
      [[NSColor colorWithDeviceRed: 0x37 / 255.0 green: 0x39 / 255.0 blue: 0x3a / 255.0 alpha: 1.0] set];
      [NSBezierPath fillRect: rect];
    } else if (item == hotItem) {
      [gradients[@"tabBackgroundHot"] drawInRect: rect angle: 270];
    }

    NSImage *icon = (item.tabState != NSBackgroundTab) ? images[@"homeTabSelected"] : images[@"homeTabUnselected"];
    NSRect imageTargetRect = rect;
    imageTargetRect.origin.x += (rect.size.width - icon.size.width) / 2;
    imageTargetRect.size = icon.size;
    [icon drawInRect: imageTargetRect
            fromRect: NSMakeRect(0, 0, icon.size.width, icon.size.height)
           operation: NSCompositingOperationSourceOver
            fraction: 1.0];
  } else {
    --rect.size.height;
    ++rect.origin.y;
    float height = NSHeight(rect);

    if (item.tabState != NSBackgroundTab) {
      [gradients[@"tabBackgroundSelected"] drawInRect: rect angle: 270];

      [mLabelAttributes setValue: colors[@"tabLabelSelected"] forKey: NSForegroundColorAttributeName];
    } else {
      if (item == hotItem) {
        [gradients[@"tabBackgroundHot"] drawInRect: rect angle: 270];
      }

      [mLabelAttributes setValue: colors[@"tabLabelUnselected"] forKey: NSForegroundColorAttributeName];
    }

    [item.label drawInRect: NSMakeRect(x + 16.5, 1.5, width, height - 6) withAttributes: mLabelAttributes];

    if ([self hasCloseButton: item]) {
      auto closeImage = images[@"tabClose"];
      ButtonInfo *info = closeButtonInfo[item.identifier];
      NSRect closeRect = NSMakeRect(
        x + (width - closeImage.size.width - 8), floor((height - closeImage.size.height) / 2),
        closeImage.size.width, closeImage.size.height
      );
      NSRect hitRect = NSInsetRect(closeRect, -4, -4);

      if (item == hotItem || item == clickedItem) {
        if (info.hit || info.pressed) {
          if (info.pressed && info.hit) { // Draw pressed only when the mouse is actually within the button.
            [colors[@"closeButtonPressed"] set];
          } else {
            [colors[@"closeButtonHit"] set];
          }

          auto buttonPath = [NSBezierPath bezierPathWithRoundedRect: hitRect xRadius: 3 yRadius: 3];
          [buttonPath fill];
        }

        [closeImage drawInRect: closeRect
                      fromRect: NSMakeRect(0, 0, closeImage.size.width, closeImage.size.height)
                     operation: NSCompositingOperationSourceOver
                      fraction: 1.0];
      }
      info.bounds = hitRect;
    }
  }

  // Separator/frame lines.
  NSRect lineRect = rect;
  lineRect.size.width = 1;
  if (item.label) {
    if (item.tabState != NSBackgroundTab) {
      [gradients[@"separator"] drawInRect: lineRect angle: 270];
      lineRect.origin.x = NSMaxX(rect);
      [gradients[@"separator"] drawInRect: lineRect angle: 270];
    } else {
      lineRect.origin.x = NSMaxX(rect);
      [gradients[@"separator"] drawInRect: lineRect angle: 270];
    }
  } else if (item.tabState == NSBackgroundTab) {
    lineRect.origin.x = NSMaxX(rect);
    [gradients[@"separator"] drawInRect: lineRect angle: 270];
  }

  return width;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)drawSectionTabStyleInFrame: (NSRect)rect forItem: (NSTabViewItem *)item {
  NSString *label = item.label;
  NSSize labelSize = [label sizeWithAttributes: mLabelAttributes];

  auto colors = self.currentColors;
  auto gradients = self.currentGradients;
  rect.size.height -= 2;
  ++rect.origin.y;

  if (item.tabState != NSBackgroundTab) {
    [gradients[@"tabBackgroundSelected"] drawInRect: rect angle: 270];

    [mLabelAttributes setValue: colors[@"tabLabelSelected"] forKey: NSForegroundColorAttributeName];
  } else {
    if (item == hotItem) {
      [gradients[@"tabBackgroundHot"] drawInRect: rect angle: 270];
    }

    [mLabelAttributes setValue: colors[@"tabLabelUnselected"] forKey: NSForegroundColorAttributeName];
  }

  // Separator/frame lines.
  NSRect lineRect = rect;
  lineRect.size.width = 1;
  if (item.label) {
    if (item.tabState != NSBackgroundTab) {
      [gradients[@"separator"] drawInRect: lineRect angle: 270];
      lineRect.origin.x = NSMaxX(rect);
      [gradients[@"separator"] drawInRect: lineRect angle: 270];
    } else {
      lineRect.origin.x = NSMaxX(rect);
      [gradients[@"separator"] drawInRect: lineRect angle: 270];
    }
  } else if (item.tabState == NSBackgroundTab) {
    lineRect.origin.x = NSMaxX(rect);
    [gradients[@"separator"] drawInRect: lineRect angle: 270];
  }

  rect.origin.y = floor(NSMinY(rect) + 1 + (NSHeight(rect) - labelSize.height) / 2);
  rect.size.height = labelSize.height;
  [label drawInRect: rect withAttributes: mLabelAttributes];
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Editor tabs style.
 */
- (float)drawEditorTabStyleInFrame: (NSRect)rect forItem: (NSTabViewItem *)item {
  float x = NSMinX(rect);
  float width = [self sizeOfTabViewItem: item].width;
  if (width > NSWidth(rect))
    return 0;

  rect.size.width = width;

  auto colors = self.currentColors;
  auto gradients = self.currentGradients;
  auto images = self.currentImages;

  rect.size.height -= 2;
  ++rect.origin.y;

  if (item.tabState == NSSelectedTab) {
    [gradients[@"tabBackgroundSelected"] drawInRect: rect angle: 270];

    [mLabelAttributes setValue: colors[@"tabLabelSelected"] forKey: NSForegroundColorAttributeName];
  } else {
    if (item == hotItem) {
      [gradients[@"tabBackgroundHot"] drawInRect: rect angle: 270];
    }

    [mLabelAttributes setValue: colors[@"tabLabelUnselected"] forKey: NSForegroundColorAttributeName];
  }

  NSImage *icon = nil;
  if ([delegate respondsToSelector: @selector(tabView: iconForItem:)]) {
    icon = [delegate tabView: self.mTabView iconForItem: item];
    if (icon != nil) {
      [icon drawAtPoint: NSMakePoint(NSMinX(rect) + 8,
                                     NSMinY(rect) + floor((NSHeight(rect) - icon.size.height) / 2))
               fromRect: NSZeroRect
              operation: NSCompositingOperationSourceOver
               fraction: item.tabState == NSSelectedTab ? 1.0 : 0.5];
    }
  }

  // Separator/frame lines.
  NSRect lineRect = rect;
  lineRect.size.width = 1;
  if (item.label) {
    if (item.tabState != NSBackgroundTab) {
      [gradients[@"separator"] drawInRect: lineRect angle: 270];
      lineRect.origin.x = NSMaxX(rect);
      [gradients[@"separator"] drawInRect: lineRect angle: 270];
    } else {
      lineRect.origin.x = NSMaxX(rect);
      [gradients[@"separator"] drawInRect: lineRect angle: 270];
    }
  } else if (item.tabState == NSBackgroundTab) {
    lineRect.origin.x = NSMaxX(rect);
    [gradients[@"separator"] drawInRect: lineRect angle: 270];
  }

  [item.label drawInRect: NSMakeRect(NSMinX(rect), 1.5, NSWidth(rect), NSHeight(rect) - 4)
          withAttributes: mLabelAttributes];

  if ([self hasCloseButton: item]) {
    auto closeImage = images[@"tabClose"];
    ButtonInfo *info = closeButtonInfo[item.identifier];
    NSRect closeRect = NSMakeRect(
      x + (NSWidth(rect) - closeImage.size.width - 8),
      floor((NSHeight(rect) - closeImage.size.height) / 2) + 1,
      closeImage.size.width, closeImage.size.height
    );
    NSRect hitRect = NSInsetRect(closeRect, -4, -4);

    if (item == hotItem || item == clickedItem) {
      if (info.hit || info.pressed) {
        if (info.pressed && info.hit) { // Draw pressed only when the mouse is actually within the button.
          [colors[@"closeButtonPressed"] set];
        } else {
          [colors[@"closeButtonHit"] set];
        }

        auto buttonPath = [NSBezierPath bezierPathWithRoundedRect: hitRect xRadius: 3 yRadius: 3];
        [buttonPath fill];
      }

      [closeImage drawInRect: closeRect
                    fromRect: NSMakeRect(0, 0, closeImage.size.width, closeImage.size.height)
                   operation: NSCompositingOperationSourceOver
                    fraction: 1.0];
    }
    info.bounds = hitRect;
  }

  return width;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Bottom tabs style.
 */
- (float)drawEditorBottomTabStyleInFrame: (NSRect)rect forItem: (NSTabViewItem *)item {
  float width = [self sizeOfTabViewItem: item].width;
  if (width > NSWidth(rect))
    return 0;

  rect.size.width = width;
  --rect.size.height;
  ++rect.origin.y;

  auto colors = self.currentColors;
  auto images = self.currentImages;
  auto gradients = self.currentGradients;

  if (item.tabState == NSSelectedTab) {
    [gradients[@"tabBackgroundSelectedBottom"] drawInRect: rect angle: 270];

    [mLabelAttributes setValue: colors[@"tabLabelSelected"] forKey: NSForegroundColorAttributeName];
  } else {
    if (item == hotItem) {
      NSRect temp = rect;
      --temp.size.height;
      [gradients[@"tabBackgroundHot"] drawInRect: temp angle: 270];
    }

    [mLabelAttributes setValue: colors[@"tabLabelUnselected"] forKey: NSForegroundColorAttributeName];
  }

  // Separator/frame lines.
  NSRect lineRect = rect;
  lineRect.size.width = 1;
  if (item.label) {
    if (item.tabState != NSBackgroundTab) {
      [gradients[@"separator"] drawInRect: lineRect angle: 270];
      lineRect.origin.x = NSMaxX(rect);
      [gradients[@"separator"] drawInRect: lineRect angle: 270];
    } else {
      lineRect.origin.x = NSMaxX(rect);
      [gradients[@"separator"] drawInRect: lineRect angle: 270];
    }
  } else if (item.tabState == NSBackgroundTab) {
    lineRect.origin.x = NSMaxX(rect);
    [gradients[@"separator"] drawInRect: lineRect angle: 270];
  }

  NSSize labelSize = [item.label sizeWithAttributes: mLabelAttributes];
  [item.label drawInRect: {{ NSMinX(rect), NSMinY(rect) - (NSHeight(rect) - labelSize.height) / 2 },
    { NSWidth(rect), NSHeight(rect) }}
          withAttributes: mLabelAttributes];

  // All pinnable tabs also have a close button.
  if (tabStyle == MEditorBottomTabSwitcherPinnable) {
    ButtonInfo *info = pinButtonInfo[item.identifier];
    BOOL isPinned = [delegate tabView: self.mTabView itemIsPinned: item];

    NSImage *image = isPinned ? images[@"pinned"] : images[@"unpinned"];
    NSRect pinRect = NSMakeRect(
      NSMinX(rect) + 4,
      NSMinY(rect) + (NSHeight(rect) - image.size.height) / 2,
      image.size.width,
      image.size.height);
    NSRect hitRect = NSInsetRect(pinRect, -2, -2);

    if (hotItem == item || isPinned) {
      if (info.hit || info.pressed) {
        if (info.pressed && info.hit) {
          [colors[@"closeButtonPressed"] set];
        } else {
          [colors[@"closeButtonHit"] set];
        }

        auto buttonPath = [NSBezierPath bezierPathWithRoundedRect: hitRect xRadius: 3 yRadius: 3];
        [buttonPath fill];
      }

      [image drawInRect: pinRect
               fromRect: NSMakeRect(0, 0, image.size.width, image.size.height)
              operation: NSCompositingOperationSourceOver
               fraction: 1.0];
    }
    pinButtonInfo[item.identifier].bounds = hitRect;

    // Close button.
    auto closeImage = images[@"tabClose"];
    info = closeButtonInfo[item.identifier];
    NSRect closeRect = NSMakeRect(
      NSMinX(rect) + (NSWidth(rect) - closeImage.size.width - 8),
      NSMinY(rect) + floor((NSHeight(rect) - closeImage.size.height) / 2),
      closeImage.size.width,
      closeImage.size.height
    );
    hitRect = NSInsetRect(closeRect, -4, -4);

    if (item == hotItem || item == clickedItem) {
      if (info.hit || info.pressed) {
        if (info.pressed && info.hit) {
          [colors[@"closeButtonPressed"] set];
        } else {
          [colors[@"closeButtonHit"] set];
        }

        auto buttonPath = [NSBezierPath bezierPathWithRoundedRect: hitRect xRadius: 3 yRadius: 3];
        [buttonPath fill];
      }

      [closeImage drawInRect: closeRect
                    fromRect: NSMakeRect(0, 0, closeImage.size.width, closeImage.size.height)
                   operation: NSCompositingOperationSourceOver
                    fraction: 1.0];
    }
    info.bounds = hitRect;
  }

  return NSWidth(rect);
}

//----------------------------------------------------------------------------------------------------------------------

- (void)drawExtenderInRect: (NSRect)rect {
  auto colors = self.currentColors;

  NSPoint point = NSMakePoint(NSMinX(rect) + 8, floor((NSHeight(rect) - extenderSize.height) / 2) + 1);
  extenderButtonInfo.bounds = NSInsetRect(NSMakeRect(point.x, point.y - 1, extenderSize.width, extenderSize.height), -2, 3);

  if (extenderButtonInfo.hit || extenderButtonInfo.pressed) {
    if (extenderButtonInfo.pressed && extenderButtonInfo.hit) {
      [colors[@"closeButtonPressed"] set];
    } else {
      [colors[@"closeButtonHit"] set];
    }

    auto buttonPath = [NSBezierPath bezierPathWithRoundedRect: extenderButtonInfo.bounds xRadius: 3 yRadius: 3];
    [buttonPath fill];
  }

  [extenderAttributes setValue: colors[@"tabLabelUnselected"] forKey: NSForegroundColorAttributeName];

  [extender drawAtPoint: point withAttributes: extenderAttributes];

}

//----------------------------------------------------------------------------------------------------------------------

- (void)drawRect: (NSRect)rect {
  NSBezierPath.defaultLineWidth = 1;
  rect = self.bounds;

  auto colors = self.currentColors;
  auto gradients = self.currentGradients;

  extenderButtonInfo.visible = NO;

  switch (tabStyle) {
    case MSectionTabSwitcher: {
      [gradients[@"tabViewBackground"]  drawInRect: rect angle: 270];

      [colors[@"borderTop"] set];
      [NSBezierPath strokeLineFromPoint: NSMakePoint(NSMinX(rect) - 0.5, NSMaxY(rect) - 0.5)
                                toPoint: NSMakePoint(NSMaxX(rect) + 0.5, NSMaxY(rect) - 0.5)];
      [colors[@"borderBottom"] set];
      [NSBezierPath strokeLineFromPoint: NSMakePoint(NSMinX(rect), NSMinY(rect) + 0.5)
                                toPoint: NSMakePoint(NSMaxX(rect), NSMinY(rect) + 0.5)];
      NSRect tabArea = rect;
      int visibleCount = 0;
      for (NSTabViewItem *item in self.mTabView.tabViewItems) {
        if (!item.view.hidden)
          ++visibleCount;
      }
      tabArea.size.width /= visibleCount;

      for (NSTabViewItem *item in self.mTabView.tabViewItems) {
        if (!item.view.hidden) {
          [self drawSectionTabStyleInFrame: tabArea forItem: item];
          tabArea.origin.x += tabArea.size.width;
        }
      }

      break;
    }

    case MEditorTabSwitcher:
    case MEditorBottomTabSwitcher:
    case MEditorBottomTabSwitcherPinnable: {
      if (tabStyle == MEditorTabSwitcher)
        [gradients[@"tabViewBackground"] drawInRect: rect angle: 270];
      else
        [gradients[@"tabViewBackgroundBottom"] drawInRect: rect angle: 270];

      [colors[@"borderTop"] set];
      [NSBezierPath strokeLineFromPoint: NSMakePoint(NSMinX(rect), floor(NSMaxY(rect)) - 0.5)
                                toPoint: NSMakePoint(NSMaxX(rect), floor(NSMaxY(rect)) - 0.5)];
      [colors[@"borderBottom"] set];
      [NSBezierPath strokeLineFromPoint: NSMakePoint(NSMinX(rect), floor(NSMinY(rect)) + 0.5)
                                toPoint: NSMakePoint(NSMaxX(rect), floor(NSMinY(rect)) + 0.5)];

      NSTabViewItem *activeTab = nil;
      NSRect activeTabRect;
      NSRect tabArea = rect;
      tabArea.size.width -= mReservedSpace + extenderSize.width;

      NSArray *items = self.mTabView.tabViewItems;
      int i = mFirstVisibleTabIndex;
      NSTabViewItem *current = static_cast<NSInteger>(items.count) > mFirstVisibleTabIndex ? items[i] : nil;
      while (current != nil) {
        if (!current.view.hidden) {
          if ((hotItem != current || !draggingTab) && current.tabState != NSSelectedTab) {
            float w;
            if (tabStyle == MEditorTabSwitcher)
              w = [self drawEditorTabStyleInFrame: tabArea forItem: current];
            else
              w = [self drawEditorBottomTabStyleInFrame: tabArea forItem: current];

            if (w == 0) {
              extenderButtonInfo.visible = YES;

              // Tab did not fit anymore and wasn't drawn. Instead draw the extender.
              [self drawExtenderInRect: tabArea];
              break;
            }
            tabArea.origin.x += w;
            tabArea.size.width -= w;
          } else {
            // This tab is being dragged or is the active one, so we draw it later.
            float w = [self sizeOfTabViewItem: current].width;
            activeTabRect = tabArea;
            tabArea.origin.x += w;
            tabArea.size.width -= w;
            activeTab = current;
          }
        }
        current = ++i < (int)items.count ? items[i] : nil;
      }

      mLastVisibleTabIndex = --i;

      if (activeTab) {
        if (draggingTab)
          activeTabRect.origin.x = mTabDragPosition.x - mClickTabOffset.x;

        float w;
        if (tabStyle == MEditorTabSwitcher)
          w = [self drawEditorTabStyleInFrame: activeTabRect forItem: activeTab];
        else
          w = [self drawEditorBottomTabStyleInFrame: activeTabRect forItem: activeTab];

        if (w == 0 && !draggingTab) {
          extenderButtonInfo.visible = YES;

          // Can only be the case if the active tab is at the right end of the tab area.
          // It has been counted above as visible tab, so we need to correct this.
          [self drawExtenderInRect: activeTabRect];
          --mLastVisibleTabIndex;
        }
      }

      // Draw extender also if we came to last tab but there are invisible tabs at the left.
      if (mLastVisibleTabIndex == self.mTabView.numberOfTabViewItems - 1 && mFirstVisibleTabIndex > 0) {
        extenderButtonInfo.visible = YES;

        [self drawExtenderInRect: tabArea];
      }

      break;
    }

    case MMainTabSwitcher: {
      NSRect selectedTabRect = NSZeroRect;
      NSRect tabArea = rect;

      [gradients[@"tabViewBackground"] drawInRect: rect angle: 270];

      [colors[@"borderTop"] set];
      [NSBezierPath strokeLineFromPoint: NSMakePoint(NSMinX(rect), NSMaxY(rect) - 0.5)
                                toPoint: NSMakePoint(NSMaxX(rect), NSMaxY(rect) - 0.5)];
      [colors[@"borderBottom"] set];
      [NSBezierPath strokeLineFromPoint: NSMakePoint(NSMinX(rect), NSMinY(rect) + 0.5)
                                toPoint: NSMakePoint(NSMaxX(rect), NSMinY(rect) + 0.5)];

      NSArray *items = self.mTabView.tabViewItems;
      int i = mFirstVisibleTabIndex;
      NSTabViewItem *current = items.count > 0 ? items[i] : nil;
      while (current != nil) {
        float w;

        if (current.tabState == NSSelectedTab)
          selectedTabRect = tabArea;
        if (hotItem != current || !draggingTab) {
          w = [self drawMainTabStyleInFrame: tabArea forItem: current];

          if (w == 0) {
            extenderButtonInfo.visible = YES;

            // Tab did not fit anymore and wasn't drawn. Instead draw the extender.
            [self drawExtenderInRect: tabArea];
            break;
          }
          tabArea.origin.x += w;
          tabArea.size.width -= w;
        } else {
          w = [self sizeOfTabViewItem:current].width;
          tabArea.origin.x += w;
          tabArea.size.width -= w;
        }
        if (current.tabState == NSSelectedTab)
          selectedTabRect.size.width = w;

        current = ++i < (int)items.count ? items[i] : nil;
      }

      mLastVisibleTabIndex = --i;

      if (hotItem && draggingTab) {
        tabArea.origin.x = mTabDragPosition.x - mClickTabOffset.x;
        tabArea.size.width = [self sizeOfTabViewItem:hotItem].width;
        [self drawMainTabStyleInFrame: tabArea forItem: hotItem];
      }

      // Draw extender also if we came to last tab but there are invisible tabs at the left.
      if (mLastVisibleTabIndex == self.mTabView.numberOfTabViewItems - 1 && mFirstVisibleTabIndex > 0) {
        extenderButtonInfo.visible = YES;

        [self drawExtenderInRect: tabArea];
      }

      break;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)didAddSubview: (NSView *)subview {
  [self resizeSubviewsWithOldSize: NSZeroSize];
  [self setNeedsDisplay: YES];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)didRemoveSubview: (NSView *)subview {
  [self resizeSubviewsWithOldSize: NSZeroSize];
  [self setNeedsDisplay: YES];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)tile {
  [self resizeSubviewsWithOldSize: self.frame.size];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)frameDidChange: (NSNotification *)notification {
  [self removeTrackingArea: mTrack];
  mTrack = [[NSTrackingArea alloc] initWithRect: self.bounds
                                        options: NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingActiveAlways
                                          owner: self
                                       userInfo: nil];
  [self addTrackingArea: mTrack];

  if (tabStyle != MEditorTabSwitcher)
    return;

  for (id tag in mToolTipTags)
    [self removeToolTip: [tag intValue]];
  [mToolTipTags removeAllObjects];

  int skip = mFirstVisibleTabIndex;
  for (NSTabViewItem *item in self.mTabView.tabViewItems) {
    if (item.view.hidden)
      continue;
    if (skip > 0) {
      skip--;
      continue;
    }
    [mToolTipTags
      addObject: @([self addToolTipRect: [self tabItemRect: item] owner: self userData: (__bridge void *_Nullable)(item)])];
  }

  [self setNeedsDisplay: YES];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)windowBecameKey: (NSNotification *)notification {
  if (notification.object == self.window)
    [self setNeedsDisplay: YES];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)windowResignedKey: (NSNotification *)notification {
  if (notification.object == self.window)
    [self setNeedsDisplay: YES];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString *)view: (NSView *)view stringForToolTip: (NSToolTipTag)tag point: (NSPoint)point userData: (void *)userData {
  NSTabViewItem *item = [self tabViewItemAtPoint:point];
  if (item && [delegate respondsToSelector: @selector(tabView:toolTipForItem:)])
    return [delegate tabView: self.mTabView toolTipForItem: item];
  return nil;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)resizeSubviewsWithOldSize: (NSSize)oldSize {
  // layout the additional views (like embedded Apply/Cancel buttons)
  mReservedSpace = 0;
  int count = 0;
  for (NSView *item in self.subviews) {
    if (!item.hidden && item != mBusyTabIndicator) {
      count++;
      mReservedSpace += NSWidth(item.frame);
    }
  }

  if (count > 0) {
    mReservedSpace += 6 * (count - 1) + 32;

    float x = NSWidth(self.frame) - 16;
    for (NSView *item in [self.subviews reverseObjectEnumerator]) {
      if (!item.hidden && item != mBusyTabIndicator) {
        // We have special handling for the preferred size for our mforms buttons, but sometimes we
        // simply add standard buttons which don't have the same understanding of a preferred size. So we apply
        // this manually here.
        if ([item isKindOfClass: NSButton.class]) {
          [(NSButton *)item sizeToFit];
        }
        NSRect r = item.frame;
        item.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        if (![item isKindOfClass: NSButton.class]) {
          r.size = [item preferredSize: self.frame.size];
        }
        x -= NSWidth(r);
        r.origin.x = x;
        r.origin.y = (NSHeight(self.frame) - NSHeight(r)) / 2;
        item.frame = r;
        x -= 6;
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

#pragma mark Event Handlers

- (BOOL)mouseDownCanMoveWindow {
  return NO;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)mouseDown: (NSEvent *)theEvent {
  NSPoint clickPos = [self convertPoint: theEvent.locationInWindow fromView: nil];
  NSTabViewItem *item = [self tabViewItemAtPoint: clickPos];

  if (extenderButtonInfo.visible && NSPointInRect(clickPos, extenderButtonInfo.bounds)) {
    extenderButtonInfo.pressed = YES;
    [self setNeedsDisplay: YES];
    return;
  }

  if (item) {
    BOOL selectTab = YES;
    if ([self hasCloseButton: item]) {
      ButtonInfo *info = closeButtonInfo[item.identifier];
      if (hotItem != nil && item != mBusyTab && NSPointInRect(clickPos, info.bounds)) {
        info.pressed = YES;
        selectTab = NO;
      }
    }

    if (tabStyle == MEditorBottomTabSwitcherPinnable) {
      ButtonInfo *info = pinButtonInfo[item.identifier];
      if (hotItem == item && NSPointInRect(clickPos, info.bounds)) {
        info.pressed = YES;
        selectTab = NO;
      }
    }

    if (selectTab)
      [self.mTabView selectTabViewItem: item];

    NSRect tabRect = [self tabItemRect: item];
    mClickTabOffset = clickPos;
    mClickTabOffset.x -= tabRect.origin.x;

    mTabDragPosition = clickPos;
  }

  if (clickedItem != item) {
    clickedItem = item;
    [self setNeedsDisplay: YES];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)mouseUp: (NSEvent *)theEvent {
  NSPoint position = [self convertPoint: theEvent.locationInWindow fromView: nil];
  if (extenderButtonInfo.visible && NSPointInRect(position, extenderButtonInfo.bounds)) {
    NSMenu *menu = [self prepareMenuForTabs];
    [NSMenu popUpContextMenu: menu withEvent: theEvent forView: self];
    extenderButtonInfo.hit = NO;
    extenderButtonInfo.pressed = NO;
    return;
  }

  NSTabViewItem *item = [self tabViewItemAtPoint: position];
  if (!draggingTab && item != clickedItem) { // Ignore mouse up if this wasn't the item that received mouse down.
    ButtonInfo *info = closeButtonInfo[clickedItem.identifier];
    info.pressed = NO;
    info = pinButtonInfo[clickedItem.identifier];
    info.pressed = NO;
    clickedItem = nil;
    [self setNeedsDisplay: YES];

    return;
  }

  if ([self hasCloseButton: item]) {
    ButtonInfo *info = closeButtonInfo[clickedItem.identifier];
    if (info.pressed && NSPointInRect(position, info.bounds)) {
      if ([delegate respondsToSelector: @selector(tabView:willCloseTabViewItem:)] &&
          [delegate tabView: self.mTabView willCloseTabViewItem: item] && item != mBusyTab) {
        if ([self.mTabView indexOfTabViewItem: item] != NSNotFound)
          [self.mTabView removeTabViewItem: item];
      }
    }
    info.pressed = NO;
    [self setNeedsDisplay: YES];
  }

  if (tabStyle == MEditorBottomTabSwitcherPinnable) {
    ButtonInfo *info = pinButtonInfo[clickedItem.identifier];
    if (info.pressed && NSPointInRect(position, info.bounds))
      [delegate tabView: self.mTabView itemPinClicked: hotItem];

    info.pressed = NO;
    [self setNeedsDisplay: YES];
  }

  clickedItem = nil;
  draggingTab = NO;

  [self setNeedsDisplay: YES];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)makeTabVisibleAndSelect: (id)sender {
  NSTabViewItem *item = [sender isKindOfClass: [NSTabViewItem class]] ? sender : [sender representedObject];

  // Trigger a repaint if the item is already selected. In that case NSTabView will not cause a refresh.
  if (item.tabState == NSSelectedTab)
    [self setNeedsDisplay: YES];
  else
    [self.mTabView selectTabViewItem: item];

  NSInteger index = [self.mTabView indexOfTabViewItem: item];
  if (index < mFirstVisibleTabIndex) {
    mFirstVisibleTabIndex = index;
  } else if (index > mLastVisibleTabIndex) {
    // If the current tab style is section tabs then we always show all tabs (by definition).
    if (tabStyle == MSectionTabSwitcher) {
      mLastVisibleTabIndex = item.tabView.numberOfTabViewItems - 1;
    } else {
      // Compute right border of the given item.
      CGFloat right = 0;
      for (NSTabViewItem *child in self.mTabView.tabViewItems) {
        if (!(child.view).hidden)
        right += [self sizeOfTabViewItem: child].width;
        if (child == item)
        break;
      }

      // Compute offset we need to shift tabs to the left.
      NSRect tabArea = self.bounds;
      tabArea.origin.x = floor(NSMinX(self.bounds));
      tabArea.size.width -= mReservedSpace + extenderSize.width;
      CGFloat offset = right - NSMaxX(tabArea);

      // Finally convert that offset into a tab index we use as first visible tab.
      mFirstVisibleTabIndex = 0;
      right = 0;
      for (NSTabViewItem *child in self.mTabView.tabViewItems) {
        if (right >= offset)
          break;

        if (!(child.view).hidden)
          right += [self sizeOfTabViewItem: child].width;
        ++mFirstVisibleTabIndex;
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)closeTabViewItem: (NSTabViewItem *)item {
  if ([delegate respondsToSelector: @selector(tabView:willCloseTabViewItem:)] &&
      [delegate tabView: self.mTabView willCloseTabViewItem: item]) {
    [self.mTabView removeTabViewItem: item];
    [self setNeedsDisplay: YES];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (IBAction)handleMenuAction: (id)sender {
  switch ([sender tag]) {
    case 1000: // close tab
      if (hotItem && [self allowClosingItem: hotItem]) {
        [self closeTabViewItem: hotItem];
      }
      break;

    case 1001: // close other tabs
      for (NSTabViewItem *item in [self.mTabView.tabViewItems reverseObjectEnumerator]) {
        if (item != hotItem) {
          if ([self allowClosingItem: item])
            [self closeTabViewItem: item];
        }
      }
      [self setNeedsDisplay: YES];
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)rightMouseDown: (NSEvent *)theEvent {
  NSPoint clickPos = [self convertPoint: theEvent.locationInWindow fromView: nil];
  NSTabViewItem *item = [self tabViewItemAtPoint: clickPos];
  NSMenu *menu = [self prepareMenuForItem: item];
  if (menu != nil)
    [NSMenu popUpContextMenu: menu withEvent: theEvent forView: self];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSMenu *)prepareMenuForItem: (NSTabViewItem *)item {
  if (item != nil) {
    NSMenu *menu = self.menu;

    if ([self allowClosingItem: clickedItem])
      [[menu itemWithTag: 1000] setEnabled: YES];
    else
      [[menu itemWithTag: 1000] setEnabled: NO];
    if (self.mTabView.numberOfTabViewItems > 1)
      [[menu itemWithTag: 1001] setEnabled: YES];
    else
      [[menu itemWithTag: 1001] setEnabled: NO];

    if ([delegate respondsToSelector: @selector(tabView:willDisplayMenu:forTabViewItem:)])
      [delegate tabView: self.mTabView willDisplayMenu: menu forTabViewItem: item];

    return menu;
  }

  return nil;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSMenu *)prepareMenuForTabs {
  NSMenu *menu = [[NSMenu alloc] initWithTitle: @"Tabs Menu"];

  // Two loops, one for items before the first visible one and the other for items after the last visible one.
  int i = 0;
  while (i < mFirstVisibleTabIndex) {
    NSTabViewItem *tabItem = [self.mTabView tabViewItemAtIndex:i++];
    NSMenuItem *item = [menu addItemWithTitle: tabItem.label != nil ? tabItem.label : @"Home Screen"
                                       action: @selector(makeTabVisibleAndSelect:)
                                keyEquivalent: @""];
    item.target = self;
    item.representedObject = tabItem;
  }

  i = mLastVisibleTabIndex + 1;
  while (i < self.mTabView.numberOfTabViewItems) {
    NSTabViewItem *tabItem = [self.mTabView tabViewItemAtIndex:i++];
    NSMenuItem *item =
      [menu addItemWithTitle:tabItem.label action: @selector(makeTabVisibleAndSelect:) keyEquivalent: @""];
    item.target = self;
    item.representedObject = tabItem;
  }

  return menu;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSTabViewItem *)clickedItem {
  return clickedItem;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)mouseMoved: (NSEvent *)theEvent {
  if (!draggingTab) {
    NSPoint pos = [self convertPoint: theEvent.locationInWindow fromView: nil];
    NSTabViewItem *item = [self tabViewItemAtPoint: pos];

    BOOL needRefresh = hotItem != item;
    if (item != nil) {
      ButtonInfo *info = closeButtonInfo[item.identifier];
      BOOL hitButton = NSPointInRect(pos, info.bounds);
      needRefresh |= hitButton != info.hit;
      info.hit = hitButton;

      info = pinButtonInfo[item.identifier];
      hitButton = NSPointInRect(pos, info.bounds);
      needRefresh |= hitButton != info.hit;
      info.hit = hitButton;
    }

    // Same hit test for the extender button if visible.
    BOOL hitButton = NSPointInRect(pos, extenderButtonInfo.bounds);
    needRefresh |= hitButton != extenderButtonInfo.hit;
    extenderButtonInfo.hit = hitButton;

    hotItem = item;
    if (needRefresh)
      [self setNeedsDisplay: YES];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)mouseDragged: (NSEvent *)theEvent {
  NSPoint clickPos = [self convertPoint: theEvent.locationInWindow fromView: nil];

  if (extenderButtonInfo.visible) {
    BOOL hitButton = NSPointInRect(clickPos, extenderButtonInfo.bounds);
    if (hitButton != extenderButtonInfo.hit) {
      [self setNeedsDisplay: YES];
      extenderButtonInfo.hit = hitButton;
    }
    return;
  }

  if (hotItem) {
    if (!(draggingTab || fabs(clickPos.x - mTabDragPosition.x) > 3 || fabs(clickPos.y - mTabDragPosition.y) > 3))
      return;

    NSTabViewItem *item = [self tabViewItemAtPoint: NSMakePoint(clickPos.x, mClickTabOffset.y)];
    if ([self hasCloseButton: clickedItem]) {
      ButtonInfo *info = closeButtonInfo[clickedItem.identifier];
      if (info.pressed) {
        BOOL hitButton = NSPointInRect(clickPos, info.bounds);
        if (hitButton != info.hit) {
          [self setNeedsDisplay: YES];
          info.hit = hitButton;
        }
        return;
      }

      info = pinButtonInfo[clickedItem.identifier];
      if (info.pressed) {
        BOOL hitButton = NSPointInRect(clickPos, info.bounds);
        if (hitButton != info.hit) {
          [self setNeedsDisplay: YES];
          info.hit = hitButton;
        }
        return;
      }
    }

    if (!draggingTab && allowTabReordering && self.mTabView.numberOfTabViewItems > 1 && item != nullptr) {
      [mBusyTabIndicator setHidden: YES];
      draggingTab = YES;
    }
    mTabDragPosition = clickPos;

    if (item != nullptr && hotItem != item && draggingTab) { // handle reordering
      NSTabViewItem *draggedItem = hotItem;
      BOOL passedThreshold = NO;

      if ([self.mTabView indexOfTabViewItem: draggedItem] > [self.mTabView indexOfTabViewItem: item]) {
        if (clickPos.x < NSMaxX([self tabItemRect: item]) - 20)
          passedThreshold = YES;
      } else {
        if (clickPos.x > NSMinX([self tabItemRect: item]) + 20)
          passedThreshold = YES;
      }

      if (passedThreshold) {
        reorderingTab = YES;
        if (item) {
          NSInteger idx = [self.mTabView indexOfTabViewItem: item];
          [self.mTabView removeTabViewItem: draggedItem];
          [self.mTabView insertTabViewItem: draggedItem atIndex: idx];

          if ([delegate respondsToSelector: @selector(tabView:didReorderTabViewItem:toIndex:)])
            [delegate tabView: self.mTabView didReorderTabViewItem: draggedItem toIndex: idx];
        } else {
          int idx;
          if (clickPos.x < 0)
            idx = 0;
          else
            idx = (int)self.mTabView.numberOfTabViewItems - 1;
          [self.mTabView removeTabViewItem: draggedItem];
          if (idx == 0)
            [self.mTabView insertTabViewItem: draggedItem atIndex: 0];
          else
            [self.mTabView addTabViewItem: draggedItem];
          if ([delegate respondsToSelector: @selector(tabView:didReorderTabViewItem:toIndex:)])
            [delegate tabView: self.mTabView didReorderTabViewItem: draggedItem toIndex: idx];
        }
        [self.mTabView selectTabViewItem: draggedItem]; // Re-select the tab since it gets unselected when removed.
        reorderingTab = NO;
      }
    }
    [self setNeedsDisplay: YES];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)mouseExited: (NSEvent *)theEvent {
  if (NSEvent.pressedMouseButtons == 0 && !draggingTab && hotItem != nil) {
    [self setNeedsDisplay: YES];
    hotItem = nil;
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setBusyTab: (NSTabViewItem *)tab {
  mBusyTab = tab;
  if (tab) {
    NSRect rect = [self tabItemRect:tab];
    switch (tabStyle) {
      case MEditorTabSwitcher:
        if (mBusyTabIndicator)
          [mBusyTabIndicator setFrameOrigin: NSMakePoint(NSMaxX(rect) - 23, 2)];
        else {
          mBusyTabIndicator = [[NSProgressIndicator alloc] initWithFrame: NSMakeRect(NSMaxX(rect) - 23, 2, 16, 16)];
          mBusyTabIndicator.controlSize = NSControlSizeSmall;
          mBusyTabIndicator.style = NSProgressIndicatorStyleSpinning;
          [mBusyTabIndicator setIndeterminate: YES];
          [mBusyTabIndicator startAnimation: nil];
          [self addSubview: mBusyTabIndicator];
        }
        [self setNeedsDisplay: YES];
        break;
      default:
        break;
    }
  } else {
    [mBusyTabIndicator stopAnimation: nil];
    [mBusyTabIndicator removeFromSuperview];
    mBusyTabIndicator = nil;
  }
}

//----------------------------------------------------------------------------------------------------------------------

#pragma mark Delegate Methods

- (void)tabViewDidChangeNumberOfTabViewItems: (NSTabView *)aTabView {
  if ([delegate respondsToSelector: @selector(tabViewDidChangeNumberOfTabViewItems:)])
    [delegate tabViewDidChangeNumberOfTabViewItems: aTabView];

  // Remove all button info entries for tab items that no longer exist.
  for (id item in closeButtonInfo.allKeys) {
    if ([aTabView indexOfTabViewItemWithIdentifier: item] == NSNotFound) {
      [closeButtonInfo removeObjectForKey: item];
      [pinButtonInfo removeObjectForKey: item];
    }
  }

  // Now add new button info for new tab items.
  for (NSTabViewItem *item in aTabView.tabViewItems) {
    if ([self hasCloseButton: item] && closeButtonInfo[item.identifier] == nil) {
      closeButtonInfo[item.identifier] = [ButtonInfo new];
    }

    if (tabStyle == MEditorBottomTabSwitcherPinnable && pinButtonInfo[item.identifier] == nil)
      pinButtonInfo[item.identifier] = [ButtonInfo new];
  }

  if (!reorderingTab) {
    if ([self.mTabView indexOfTabViewItem: clickedItem] == NSNotFound)
      clickedItem = nil;
    if ([self.mTabView indexOfTabViewItem: hotItem] == NSNotFound)
      hotItem = nil;
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)tabView: (NSTabView *)aTabView didSelectTabViewItem: (NSTabViewItem *)tabViewItem {
  [self setNeedsDisplay: YES];
  if ([delegate respondsToSelector: @selector(tabView:didSelectTabViewItem:)])
    [delegate tabView:aTabView didSelectTabViewItem: tabViewItem];
}

@end

//----------------------------------------------------------------------------------------------------------------------
