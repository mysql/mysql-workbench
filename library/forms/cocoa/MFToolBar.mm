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

#import "MFToolBar.h"
#import "MFBase.h"
#include "mforms/toolbar.h"
#import "MFMForms.h"


static struct {
  const float height;
  const float xpadding;
  const float item_width;
} layoutInfo[] = {
  { 34, 12, 36 - 2 * 7 }, // Main
  { 34, 12, 25 - 2 * 7 }, // Secondary
  { 36, 12, 36 - 2 * 7 }, // ToolPicker
  { 34, 8, 20 - 2 * 1 },  // Options
  { 34, 4, 26 - 2 * 3 }   // Palette
};

using namespace mforms;

//----------------------------------------------------------------------------------------------------------------------

@interface MFColorMenuItem : NSMenuItem {
}
@end

//----------------------------------------------------------------------------------------------------------------------

@implementation MFColorMenuItem

static NSColor* colorFromHexString(const char* hexcolor) {
  int r, g, b;
  
  if (sscanf(hexcolor, "#%02x%02x%02x", &r, &g, &b) != 3)
    return nil;
  
  return [NSColor colorWithDeviceRed: r / 255.0 green: g / 255.0 blue: b / 255.0 alpha: 1.0];
}

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithColorName: (NSString*)color {
  self = [super init];
  if (self) {
    NSImage *image= [[NSImage alloc] initWithSize: NSMakeSize(24, 16)];
    [image lockFocus];
    [[NSColor lightGrayColor] set];
    NSFrameRect(NSMakeRect(1, 1, 22, 14));
    [colorFromHexString(color.UTF8String) set];
    NSRectFill(NSMakeRect(2, 2, 20, 12));
    [image unlockFocus];
    self.image = image;
    self.title = @"";
    self.representedObject = color;
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityMenuBarItemRole;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@interface MFToolBarSeparatorImpl : NSView {
  ToolBarItem *mOwner;
  BOOL mHorizontal;
}

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithItemObject:(ToolBarItem*)item NS_DESIGNATED_INITIALIZER;

@property (readonly) mforms::ToolBarItemType itemType;
@property (readonly) mforms::ToolBarItem *toolBarItem;

- (void)setHorizontal:(BOOL)flag;

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation MFToolBarSeparatorImpl

- (instancetype)initWithItemObject: (ToolBarItem*)item {
  if (item == nil)
    return nil;

  self = [super initWithFrame: NSMakeRect(0, 0, 12, 12)];
  if (self) {
    mOwner = item;
    mOwner->set_data(self);
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithFrame: (NSRect)frame {
  return [self initWithItemObject: nil];
}

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithCoder: (NSCoder *)coder {
  return [self initWithItemObject: nil];
}

//----------------------------------------------------------------------------------------------------------------------

- (ToolBarItem*)toolBarItem {
  return mOwner;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setHorizontal:(BOOL)flag {
  mHorizontal= flag;
  if (flag) {
    NSRect frame = self.frame;
    float tmp = frame.size.height;
    frame.size.height = frame.size.width;
    frame.size.width = tmp;
    self.frame = frame;
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (ToolBarItemType)itemType {
  return mOwner->get_type();
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityMenuBarItemRole;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@interface MFToolBarActionItemImpl : NSButton {
  ToolBarItem *mOwner;
  BOOL mToolPicker;
}

- (instancetype)initWithItemObject:(ToolBarItem*)item NS_DESIGNATED_INITIALIZER;

@property (readonly) mforms::ToolBarItem *toolBarItem;

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation MFToolBarActionItemImpl

- (instancetype)initWithItemObject: (ToolBarItem*)item {
  if (item == nil)
    return nil;

  self = [super initWithFrame: NSMakeRect(0, 0, layoutInfo[0].item_width, layoutInfo[0].item_width)];
  if (self) {
    mOwner = item;
    mOwner->set_data(self);
    switch (item->get_type()) {
      case ToggleItem:
        [self setButtonType: NSButtonTypeToggle];
        [self.cell setHighlightsBy: NSChangeBackgroundCellMask];
        self.bordered = NO;
        break;

      case TextActionItem:
        self.imagePosition = NSNoImage;
        self.bordered = NO;
        break;

      case SegmentedToggleItem:
        [self setButtonType: NSButtonTypeToggle];
        self.imagePosition = NSImageOnly;
        [self.cell setHighlightsBy: NSChangeBackgroundCellMask];
        self.bordered = NO;
        break;

      default: // ActionItem etc.
        self.bezelStyle = NSBezelStyleTexturedRounded;
        self.buttonType = NSButtonTypeMomentaryPushIn;
        self.bordered = NO;

        break;
    }

    self.target = self;
    self.action = @selector(perform:);
    self.title = @"";
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

-(instancetype)initWithFrame: (NSRect)frame {
  return [self initWithItemObject: nil];
}

//----------------------------------------------------------------------------------------------------------------------

-(instancetype)initWithCoder: (NSCoder *)coder {
  return [self initWithItemObject: nil];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)dealloc {
  mOwner = nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

- (ToolBarItem*)toolBarItem {
  return mOwner;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)perform: (id)sender {
  mOwner->callback();
  if (mToolPicker)
    self.bordered = self.state == NSControlStateValueOn;  
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setState: (NSInteger)value {
  super.state = value;
  if (mToolPicker)
    self.bordered = value == NSControlStateValueOn;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setImage: (NSImage *)image {
  super.image = image;
  if (self.title.length > 0)
    self.imagePosition = NSImageLeft;
  else
    self.imagePosition = NSImageOnly;

  [self sizeToFit];

  NSRect frame = self.frame;
  if (NSWidth(frame) > NSHeight(frame))
    frame.size.height = NSWidth(frame);
  else
    frame.size.width = NSHeight(frame);
  self.frame = frame;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)viewDidMoveToSuperview {
  if (mOwner == NULL)
    return;

  MFToolBarImpl *toolbar = (MFToolBarImpl*)self.superview;
  if (toolbar == nil)
    return;

  if (!self.alternateImage && mOwner->get_type() == ToggleItem) {
    mToolPicker = YES;
    [self setButtonType: NSButtonTypeOnOff];
    [self setBordered: NO];
    self.bezelStyle = NSBezelStyleTexturedSquare;
    [self.cell setBackgroundColor: toolbar.backgroundColor];

    if (toolbar.type == mforms::SecondaryToolBar) {
      NSSize tsize = toolbar.frame.size;
      [self setFrameSize: NSMakeSize(tsize.height, tsize.height)];
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setStringValue: (NSString*)value {
  self.title = value;
  if (self.image != nil)
    self.imagePosition = NSImageLeft;
  else
    self.imagePosition = NSImageOnly;

  [self sizeToFit];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString*)stringValue {
  return self.title;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityButtonRole;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@interface MFToolBarLabelItemImpl : NSTextField {
  MFToolBarImpl *mToolbar;
  ToolBarItem *mOwner;
}
- (instancetype)initWithItemObject:(ToolBarItem*)item NS_DESIGNATED_INITIALIZER;
- (void)setToolbar:(MFToolBarImpl*)toolbar;

@property (readonly) mforms::ToolBarItem *toolBarItem;

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation MFToolBarLabelItemImpl

- (instancetype)initWithItemObject: (ToolBarItem*)item {
  if (item == nil)
    return nil;

  self = [super initWithFrame: NSMakeRect(0, 0, 200, layoutInfo[0].item_width)];
  if (self) {
    mOwner = item;
    mOwner->set_data(self);
    [self setBordered: NO];
    [self setEditable: NO];
    [self setDrawsBackground: NO];
    if (item->get_type() == mforms::TitleItem) {
      self.font = [NSFont boldSystemFontOfSize: [NSFont smallSystemFontSize]];
    }
    else
      self.font = [NSFont systemFontOfSize: [NSFont smallSystemFontSize]];
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

-(instancetype)initWithFrame: (NSRect)frame {
  return [self initWithItemObject: nil];
}

//----------------------------------------------------------------------------------------------------------------------

-(instancetype)initWithCoder: (NSCoder *)coder {
  return [self initWithItemObject: nil];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setToolbar:(MFToolBarImpl *)toolbar {
  mToolbar = toolbar;
}

//----------------------------------------------------------------------------------------------------------------------

- (ToolBarItem*)toolBarItem {
  return mOwner;
}

//----------------------------------------------------------------------------------------------------------------------

- (void) setStringValue:(NSString *)aString {
  super.stringValue = aString;
  [self sizeToFit];
  [mToolbar resizeSubviewsWithOldSize: NSZeroSize];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityStaticTextRole;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@interface MFToolBarImageItemImpl : NSImageView {
  ToolBarItem *mOwner;
}
- (instancetype)initWithItemObject:(ToolBarItem*)item NS_DESIGNATED_INITIALIZER;

@property (readonly) mforms::ToolBarItem *toolBarItem;

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation MFToolBarImageItemImpl

- (instancetype)initWithItemObject: (ToolBarItem*)item {
  if (item == nil)
    return nil;

  self = [super initWithFrame: NSMakeRect(0, 0, layoutInfo[0].item_width, layoutInfo[0].item_width)];
  if (self) {
    mOwner = item;
    mOwner->set_data(self);
    self.imageFrameStyle = NSImageFrameNone;
    self.imageScaling = NSImageScaleNone;
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

-(instancetype)initWithFrame: (NSRect)frame {
  return [self initWithItemObject: nil];
}

//----------------------------------------------------------------------------------------------------------------------

-(instancetype)initWithCoder: (NSCoder *)coder {
  return [self initWithItemObject: nil];
}

//----------------------------------------------------------------------------------------------------------------------

- (ToolBarItem*)toolBarItem {
  return mOwner;
}

- (void)setImage:(NSImage*)image {
  super.image = image;
  [self setFrameSize: image.size];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityImageRole;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@interface MFToolBarSearchItemImpl : NSSearchField {
  ToolBarItem *mOwner;
}
- (instancetype)initWithItemObject:(ToolBarItem*)item NS_DESIGNATED_INITIALIZER;

@property (readonly) mforms::ToolBarItem *toolBarItem;

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation MFToolBarSearchItemImpl

- (instancetype)initWithItemObject: (ToolBarItem*)item {
  if (item == nil)
    return nil;

  self = [super initWithFrame: NSMakeRect(0, 0, 200, layoutInfo[0].item_width)];
  if (self) {
    mOwner = item;
    mOwner->set_data(self);
    [self.cell setSendsSearchStringImmediately: NO];
    self.target = self;
    self.action = @selector(perform:);
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

-(instancetype)initWithFrame: (NSRect)frame {
  return [self initWithItemObject: nil];
}

//----------------------------------------------------------------------------------------------------------------------

-(instancetype)initWithCoder: (NSCoder *)coder {
  return [self initWithItemObject: nil];
}

//----------------------------------------------------------------------------------------------------------------------

- (ToolBarItem*)toolBarItem {
  return mOwner;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)perform:(id)sender {
  mOwner->callback();
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityTextFieldRole;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@interface MFToolBarTextItemImpl : NSTextField {
  ToolBarItem *mOwner;
}
- (instancetype)initWithItemObject:(ToolBarItem*)item NS_DESIGNATED_INITIALIZER;

@property (readonly) mforms::ToolBarItem *toolBarItem;

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation MFToolBarTextItemImpl

- (instancetype)initWithItemObject: (ToolBarItem*)item {
  if (item == nil)
    return nil;

  self = [super initWithFrame: NSMakeRect(0, 0, 200, layoutInfo[0].item_width)];
  if (self) {
    mOwner = item;
    mOwner->set_data(self);
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

-(instancetype)initWithFrame: (NSRect)frame {
  return [self initWithItemObject: nil];
}

//----------------------------------------------------------------------------------------------------------------------

-(instancetype)initWithCoder: (NSCoder *)coder {
  return [self initWithItemObject: nil];
}

//----------------------------------------------------------------------------------------------------------------------

- (ToolBarItem*)toolBarItem {
  return mOwner;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityTextFieldRole;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@interface MFToolBarSelectorItemImpl : NSPopUpButton {
  ToolBarItem *mOwner;
}
- (instancetype)initWithItemObject:(ToolBarItem*)item NS_DESIGNATED_INITIALIZER;

@property (readonly) mforms::ToolBarItem *toolBarItem;

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation MFToolBarSelectorItemImpl

- (instancetype)initWithItemObject:(ToolBarItem*)item {
  self = [super initWithFrame: NSMakeRect(0, 0, 150, layoutInfo[0].item_width)];
  if (self) {
    mOwner = item;
    mOwner->set_data(self);
    self.target = self;
    self.action = @selector(perform:);
    self.cell.controlSize = NSControlSizeSmall;
    self.font = [NSFont systemFontOfSize: [NSFont smallSystemFontSize]];
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (ToolBarItem*)toolBarItem {
  return mOwner;
}

//----------------------------------------------------------------------------------------------------------------------

- (void) viewDidMoveToSuperview {
  switch (((MFToolBarImpl*)self.superview).type) {
    case OptionsToolBar:
      //[self setBordered: NO];
      break;
    default:
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)perform:(id)sender {
  mOwner->callback();
}


//----------------------------------------------------------------------------------------------------------------------

- (NSString*)stringValue {
  if (mOwner->get_type() == ColorSelectorItem)
    return self.selectedItem.representedObject;
  else
    return self.selectedItem.title;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setStringValue:(NSString*)value {
  if (mOwner->get_type() == ColorSelectorItem)
    [self selectItemAtIndex: [self indexOfItemWithRepresentedObject: value]];
  else
    [self selectItemWithTitle: value];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityPopUpButtonRole;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation MFToolBarImpl

- (instancetype)initWithObject:(ToolBar*)owner type:(ToolBarType)type {
  if (type == ToolPickerToolBar) // Vertical toolbar.
    self = [super initWithFrame: NSMakeRect(0, 0, layoutInfo[ToolPickerToolBar].height, 100)];
  else
    self = [super initWithFrame: NSMakeRect(0, 0, 100, layoutInfo[type].height)];

  if (self) {
    mOwner = owner;
    mOwner->set_data(self);
    
    [[NSNotificationCenter defaultCenter] addObserver: self
                                             selector: @selector(windowKeyChanged:)
                                                 name: NSWindowDidBecomeMainNotification
                                               object: nil];
    [[NSNotificationCenter defaultCenter] addObserver: self
                                             selector: @selector(windowKeyChanged:)
                                                 name: NSWindowDidResignMainNotification
                                               object: nil];    
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSSize)minimumSize {
  NSSize requiredSize;

  if (mOwner->get_type() == ToolPickerToolBar)
    requiredSize = NSMakeSize(layoutInfo[ToolPickerToolBar].height, 100);
  else
    requiredSize = NSMakeSize(100, layoutInfo[mOwner->get_type()].height);

  NSSize minSize = super.minimumSize;

  return { MAX(requiredSize.width, minSize.width), MAX(requiredSize.height, minSize.height) };
}


//----------------------------------------------------------------------------------------------------------------------

- (BOOL)expandsOnLayoutVertically:(BOOL)vertically {
  if (vertically)
    return NO;
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)insertItem:(id)item atIndex:(NSInteger)index {
  BOOL wasEmpty = NO;
  if (mOwner->get_type() == ToolPickerToolBar && [item respondsToSelector: @selector(setHorizontal:)])
    [item setHorizontal: YES];

  if ([item isKindOfClass: [MFToolBarSearchItemImpl class]]) {
    if (mOwner->get_type() == SecondaryToolBar)
    {
      [item cell].controlSize = NSControlSizeSmall;
      [item setFont: [NSFont systemFontOfSize: [NSFont smallSystemFontSize]]];
      [item setFrame: NSMakeRect(0, 0, 120, 19)];
    }
  }

  if ([item respondsToSelector: @selector(setToolbar:)])
    [item setToolbar: (id)self];
  
  if (self.subviews.count == 0)
    wasEmpty = YES;

  NSView *view = item;
  if (index >= (int)self.subviews.count)
    [self addSubview: view];
  else
    [self addSubview: view positioned: NSWindowBelow relativeTo: self.subviews[index]];
  if (wasEmpty) {
    if (mOwner->get_type() == MainToolBar) {
      NSRect rect = self.frame;
      rect.size.height = layoutInfo[MainToolBar].height;
      self.frame = rect;
    }
  }
    
  [self relayout];
}

//----------------------------------------------------------------------------------------------------------------------

- (void) dealloc {
  mOwner = nullptr;
  [[NSNotificationCenter defaultCenter] removeObserver: self];
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL) isFlipped {
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)removeItem: (id)item {
  [item removeFromSuperview];
  [self resizeSubviewsWithOldSize: NSZeroSize];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)resizeSubviewsWithOldSize:(NSSize)oldBoundsSize {
  NSRect rect = self.bounds;

  float spacing = 6;

  if (mOwner->get_type() == ToolPickerToolBar) {
    float x = NSMinX(rect);
    float y = NSMinY(rect) + layoutInfo[ToolPickerToolBar].xpadding;
    float w = layoutInfo[ToolPickerToolBar].height;
    
    int expanderCount = 0;
    float totalHeight = 0;
    
    for (NSView *item in self.subviews) {
      if ([item isKindOfClass: [MFToolBarSeparatorImpl class]]) {
        if ([(id)item itemType] == ExpanderItem)
          expanderCount++;
        else
          totalHeight += NSHeight(item.frame);
      } else {
        totalHeight += NSHeight(item.frame) + spacing;
      }
    }

    for (NSView *item in self.subviews) {
      NSRect r = item.frame;
      r.origin = { x, y };
      if ([item isKindOfClass: [MFToolBarSeparatorImpl class]]) {
        if ( [(id)item itemType] == ExpanderItem) {
          r.size.width = w;
          r.size.height = (NSHeight(rect) - totalHeight) / expanderCount;
          if (r.size.height < 0)
            r.size.height = 0;
        } // Otherwise this is a non-expanding separator.

        y += NSHeight(r);
      } else {
        r.origin.x += (NSWidth(rect) - NSWidth(r)) / 2;
        [item setFrame: r];

        y += NSHeight(r) + spacing;
      }
    }
  } else {
    ToolBarType type = mOwner->get_type();
    float xpadding = layoutInfo[type].xpadding;
    float x = floor(NSMinX(rect)) + xpadding;
    float y = floor(NSMinY(rect));
    float h = layoutInfo[type].height;
    
    int expanderCount = 0;
    float totalWidth = xpadding * 2;
    
    for (NSView *item in self.subviews) {
      if ([item isKindOfClass: [MFToolBarSeparatorImpl class]]) {
        if ([(id)item itemType] == ExpanderItem)
          expanderCount++;
        else
          totalWidth += NSWidth(item.frame);
      } else {
        totalWidth += NSWidth(item.frame) + spacing;
      }
    }

    for (NSView *item in self.subviews) {
      NSRect r = [item frame];
      r.origin = { x, y };
      if ([item isKindOfClass: [MFToolBarSeparatorImpl class]]) {
        if ( [(id)item itemType] == ExpanderItem) {
          r.size.height = h;
          r.size.width = (NSWidth(rect) - totalWidth) / expanderCount;
          if (r.size.width < 0)
            r.size.width = 0;
        } // Otherwise this is a non-expanding separator.

        x += NSWidth(r);
      } else {
        r.origin.y += (NSHeight(rect) - NSHeight(r)) / 2;
        [item setFrame: r];

        x += NSWidth(r) + spacing;
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)windowKeyChanged: (NSNotification*)notif {
  [self setNeedsDisplay: YES];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)drawRect: (NSRect)rect {
  BOOL isActive = self.window.mainWindow;
  BOOL isDark = NO;
  if (@available(macOS 10.14, *)) {
    isDark = self.window.effectiveAppearance.name == NSAppearanceNameDarkAqua;
  }

  rect = self.bounds;

  switch (mOwner->get_type()) {
    case ToolPickerToolBar:
    case PaletteToolBar:
    case SecondaryToolBar:
    case OptionsToolBar:
      [self.backgroundColor set];
      NSRectFill(rect);
      break;

    default: {
      NSGradient *grad;
      if (isActive) {
        if (isDark) {
          grad = [[NSGradient alloc] initWithStartingColor: [NSColor colorWithDeviceWhite: 0x29 / 255.0 alpha: 1.0]
                                               endingColor: [NSColor colorWithDeviceWhite: 0x29 / 255.0 alpha: 1.0]];
        } else {
          grad = [[NSGradient alloc] initWithStartingColor: [NSColor colorWithDeviceWhite: 0xcb / 255.0 alpha: 1.0]
                                               endingColor: [NSColor colorWithDeviceWhite: 0xc7 / 255.0 alpha: 1.0]];
        }
      } else {
        if (isDark) {
          grad = [[NSGradient alloc] initWithStartingColor: [NSColor colorWithDeviceWhite: 0x29 / 255.0 alpha: 1.0]
                                               endingColor: [NSColor colorWithDeviceWhite: 0x29 / 255.0 alpha: 1.0]];
        } else {
          grad = [[NSGradient alloc] initWithStartingColor: [NSColor colorWithDeviceWhite: 0xf6 / 255.0 alpha: 1.0]
                                               endingColor: [NSColor colorWithDeviceWhite: 0xf6 / 255.0 alpha: 1.0]];
        }
      }
      [grad drawInRect: rect angle: 90.0];

      break;
    }
  }

  switch (mOwner->get_type()) {
    case MainToolBar: {
      // No separator line.
      break;
    }

    case OptionsToolBar: {
      if (isDark)
        [[NSColor colorWithDeviceWhite: 0x44 / 255.0 alpha: 1.0] set];
      else
        [[NSColor colorWithDeviceWhite: 0xe5 / 255.0 alpha: 1.0] set];

      [NSBezierPath strokeLineFromPoint: { 0, floor(NSMaxY(rect)) - 0.5 }
                                toPoint: { NSMaxX(rect), floor(NSMaxY(rect)) - 0.5 }];
      break;
    }

    default: {
      if (isDark)
        [NSColor.blackColor set];
      else
        [[NSColor colorWithDeviceWhite: 0xa7 / 255.0 alpha: 1.0] set];

      [NSBezierPath strokeLineFromPoint: { 0, floor(NSMaxY(rect)) - 0.5 }
                                toPoint: { NSMaxX(rect), floor(NSMaxY(rect)) - 0.5 }];
      break;
    }
  }

}

//----------------------------------------------------------------------------------------------------------------------

- (NSColor*)backgroundColor {
  switch (mOwner->get_type()) {
    case OptionsToolBar:
      return NSColor.controlBackgroundColor;

    default:
      return NSColor.windowBackgroundColor;
  }

  return NSColor.windowBackgroundColor;
}

//----------------------------------------------------------------------------------------------------------------------

- (mforms::ToolBarType)type {
  return mOwner->get_type();
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)mouseDownCanMoveWindow {
  return NO;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)destroy {
  [self removeFromSuperview];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityMenuBarRole;
}

@end

//----------------------------------------------------------------------------------------------------------------------

static bool create_tool_bar(ToolBar *tb, ToolBarType type) {
  return [[MFToolBarImpl alloc] initWithObject: tb type: type] != nil;
}

//----------------------------------------------------------------------------------------------------------------------

static void insert_item(ToolBar *toolbar, int index, ToolBarItem *item) {
  [toolbar->get_data() insertItem: item->get_data() atIndex: index];
}

//----------------------------------------------------------------------------------------------------------------------

static void remove_item(ToolBar *toolbar, ToolBarItem *item) {
  [toolbar->get_data() removeItem: item->get_data()];
}

//----------------------------------------------------------------------------------------------------------------------

static bool create_tool_item(ToolBarItem *item, ToolBarItemType type) {
  switch (type) {
    case ActionItem:
    case ToggleItem:
    case TextActionItem:
    case SegmentedToggleItem:
      return [[MFToolBarActionItemImpl alloc] initWithItemObject: item] != nil;

    case SeparatorItem:
    case ExpanderItem:
      return [[MFToolBarSeparatorImpl alloc] initWithItemObject: item] != nil;

    case SearchFieldItem:
      return [[MFToolBarSearchItemImpl alloc] initWithItemObject: item] != nil;

    case FlatSelectorItem:
    case SelectorItem:
      return [[MFToolBarSelectorItemImpl alloc] initWithItemObject: item] != nil;

    case ColorSelectorItem:
      return [[MFToolBarSelectorItemImpl alloc] initWithItemObject: item] != nil;

    case LabelItem:
    case TitleItem:
      return [[MFToolBarLabelItemImpl alloc] initWithItemObject: item] != nil;

    case ImageBoxItem:
      return [[MFToolBarImageItemImpl alloc] initWithItemObject: item] != nil;

    case TextEntryItem:
      return [[MFToolBarTextItemImpl alloc] initWithItemObject: item] != nil;

    default:
      return false;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void set_item_icon(ToolBarItem *item, const std::string &image) {
  id tbitem = item->get_data();
  std::string name = base::strip_extension(base::basename(image));
  NSImage *i = [NSImage imageNamed: wrap_nsstring(name)];
  if (!i || !i.valid)
    NSLog(@"invalid icon for toolbar %s", image.c_str());
  else
    [tbitem setImage: i];
}

//----------------------------------------------------------------------------------------------------------------------

static void set_item_alt_icon(ToolBarItem *item, const std::string &image) {
  id tbitem = item->get_data();
  std::string name = base::strip_extension(base::basename(image));
  NSImage *i = [NSImage imageNamed: wrap_nsstring(name)];
  if (!i || !i.valid)
    NSLog(@"invalid icon for toolbar %s", image.c_str());
  else
    [tbitem setAlternateImage: i];
}

//----------------------------------------------------------------------------------------------------------------------

static void set_item_text(ToolBarItem *item, const std::string &text) {
  id tbitem = item->get_data();
  [tbitem setStringValue: wrap_nsstring(text)];
}

//----------------------------------------------------------------------------------------------------------------------

static std::string get_item_text(ToolBarItem *item) {
  id tbitem = item->get_data();
  return [tbitem stringValue].UTF8String ?: "";
}

//----------------------------------------------------------------------------------------------------------------------

static void set_item_name(ToolBarItem *item, const std::string &name) {
  NSView *view = item->get_data();
  view.accessibilityTitle = [NSString stringWithUTF8String: name.c_str()];
}

//----------------------------------------------------------------------------------------------------------------------

static void set_item_enabled(ToolBarItem *item, bool flag) {
  id tbitem = item->get_data();
  [tbitem setEnabled: flag];
}

//----------------------------------------------------------------------------------------------------------------------

static bool get_item_enabled(ToolBarItem *item) {
  id tbitem = item->get_data();
  return [tbitem isEnabled];
}

//----------------------------------------------------------------------------------------------------------------------

static void set_item_checked(ToolBarItem *item, bool flag) {
  id tbitem = item->get_data();
  if ([tbitem state] != (flag ? NSControlStateValueOn : NSControlStateValueOff))
    [tbitem setState: flag ? NSControlStateValueOn : NSControlStateValueOff];
}

//----------------------------------------------------------------------------------------------------------------------

static bool get_item_checked(ToolBarItem *item) {
  id tbitem = item->get_data();
  return [tbitem state] == NSControlStateValueOn;
}

//----------------------------------------------------------------------------------------------------------------------

static void set_item_tooltip(ToolBarItem *item, const std::string &text) {
  id tbitem = item->get_data();
  [tbitem setToolTip: wrap_nsstring(text)];
}

//----------------------------------------------------------------------------------------------------------------------

static void set_selector_items(ToolBarItem *item, const std::vector<std::string> &items) {
  id tbitem = item->get_data();
  if ([tbitem isKindOfClass: [MFToolBarSelectorItemImpl class]]) {
    if (item->get_type() == ColorSelectorItem) {
      NSMenu *menu= [[NSMenu alloc] initWithTitle: @""];
      [tbitem cell].controlSize = NSControlSizeSmall;
      for (std::vector<std::string>::const_iterator color= items.begin(); color != items.end(); ++color) {
        [menu addItem: [[MFColorMenuItem alloc] initWithColorName: wrap_nsstring(*color)]];
      }
      [tbitem setMenu: menu];
      [tbitem sizeToFit];
    } else {
      NSMutableArray *array = [NSMutableArray arrayWithCapacity: items.size()];
      for (std::vector<std::string>::const_iterator iter = items.begin();
           iter != items.end(); ++iter)
        [array addObject: wrap_nsstring(*iter)];
      [tbitem removeAllItems];
      [tbitem addItemsWithTitles: array];
      [tbitem sizeToFit];
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void cf_toolbar_init() {
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();
  
  f->_tool_bar_impl.create_tool_bar = create_tool_bar;
  f->_tool_bar_impl.insert_item = insert_item;
  f->_tool_bar_impl.remove_item = remove_item;
  
  f->_tool_bar_impl.create_tool_item = create_tool_item;
  f->_tool_bar_impl.set_item_icon = set_item_icon;
  f->_tool_bar_impl.set_item_alt_icon = set_item_alt_icon;
  f->_tool_bar_impl.set_item_text = set_item_text;
  f->_tool_bar_impl.get_item_text = get_item_text;
  f->_tool_bar_impl.set_item_name = set_item_name;
  f->_tool_bar_impl.set_item_enabled = set_item_enabled;
  f->_tool_bar_impl.get_item_enabled = get_item_enabled;
  f->_tool_bar_impl.set_item_checked = set_item_checked;
  f->_tool_bar_impl.get_item_checked = get_item_checked;
  f->_tool_bar_impl.set_item_tooltip = set_item_tooltip;
  f->_tool_bar_impl.set_selector_items = set_selector_items;
}

//----------------------------------------------------------------------------------------------------------------------
