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

#import "WBMiniToolbar.h"
#import "NSColor_extras.h"

static const float DEFAULT_HEIGHT = 24;

//----------------------------------------------------------------------------------------------------------------------

@implementation WBMiniToolbar

- (instancetype)initWithFrame: (NSRect)frame {
  frame.size.height = DEFAULT_HEIGHT;
  self = [super initWithFrame: frame];
  if (self != nil) {
    mOptionInfoList = [NSMutableArray array];

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

- (void)windowKeyChanged: (NSNotification*)notification {
  [self setNeedsDisplay: YES];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSSize)minimumSize {
  return NSMakeSize(10, DEFAULT_HEIGHT);
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)expandsOnLayoutVertically: (BOOL)vertically {
  if (vertically)
    return NO;
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)resizeSubviewsWithOldSize: (NSSize)osize {
  [self tile];
  [super resizeSubviewsWithOldSize: osize];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)drawRect: (NSRect)rect {
  [NSColor.windowBackgroundColor set];
  NSRectFill(rect);
}

//----------------------------------------------------------------------------------------------------------------------

- (void)removeAllItems {
  for (id view in [self.subviews reverseObjectEnumerator])
    [view removeFromSuperview];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setDelegate: (id)delegate {
  mDelegate= delegate;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)colorPopupChanged: (id)sender {
  id info = mOptionInfoList[[sender tag]];
  
  [mDelegate miniToolbar: self
            popupChanged: info[@"name"]
                  option: info[@"option"]
                   value: [sender selectedItem].representedObject];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)popupChanged: (id)sender {
  id info= mOptionInfoList[[sender tag]];
  
  [mDelegate miniToolbar: self
            popupChanged: info[@"name"]
                  option: info[@"option"]
                   value: [sender titleOfSelectedItem]];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)tile {
  float height = NSHeight(self.frame);
  float x = 5;
  float fixedWidth = 0;
  int expanderCount = 0;

  for (id item in self.subviews) {
    if ([item class] == [NSView class] && NSHeight([item frame]) == 0)
      expanderCount++;
    else
      fixedWidth += NSWidth([item frame]);
  }
  fixedWidth += 6 * (self.subviews.count-1);

  for (id item in self.subviews) {
    NSRect frame = [item frame];
    frame.origin.x = x;
    frame.origin.y = (height - NSHeight(frame)) / 2;
    if ([item class] == [NSView class] && NSHeight([item frame]) == 0) {
      frame.size.width = (NSWidth(self.frame) - 10 - fixedWidth) / expanderCount;
    }   
    [item setFrame: frame];
    x += NSWidth(frame);
    x += 6;
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (NSButton*)addButtonWithTitle: (NSString*)title
                         target: (id)target
                         action: (SEL)action
                            tag: (int)tag {
  NSButton *button = [[NSButton alloc] initWithFrame: NSMakeRect(0, 0, 10, 10)];
  
  [button setButtonType: NSButtonTypeMomentaryLight];
  button.title = title;
  button.tag = tag;
  button.target = target;
  button.action = action;
  [button setBordered: YES];

  button.bezelStyle = NSBezelStyleRounded;
  
  [self addSubview: button];
  
  [button sizeToFit];
  [self tile];
  
  return button;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSButton*)addButtonWithIcon: (NSImage*)icon
                        target: (id)target
                        action: (SEL)action
                           tag: (int)tag {
  NSButton *button = [[NSButton alloc] initWithFrame: NSMakeRect(0, 0, 18, 18)];
  
  [button setButtonType: NSButtonTypeMomentaryLight];
  [button setBordered: NO];
  button.imagePosition = NSImageOnly;
  button.image = icon;
  button.tag = tag;
  button.target = target;
  button.action = action;
  [button setBordered: NO];
  
  [self addSubview: button];
  [self tile];
  
  return button;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSSegmentedControl*)addSegmentedButtonsWithIconsAndTags: (NSArray*)iconsAndTags
                                                    target: (id)target
                                                    action: (SEL)action {
  NSSegmentedControl *seg = [[NSSegmentedControl alloc] initWithFrame: NSMakeRect(0, 0, 30 * (iconsAndTags.count / 2), 24)];
  seg.segmentCount = iconsAndTags.count / 2;
  seg.segmentStyle = NSSegmentStyleTexturedSquare;
  [seg.cell setTrackingMode: NSSegmentSwitchTrackingSelectAny];
  for (NSUInteger i = 0; i < iconsAndTags.count /2; i++) {
    [seg setImage: iconsAndTags[i * 2] forSegment: i];
    [seg.cell setTag: [iconsAndTags[i * 2 + 1] intValue] forSegment: i];
  }

  [self addSubview: seg];
  [self tile];
  
  return seg;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSTextField*)addLabelWithTitle: (NSString*)title {
  NSTextField *label = [[NSTextField alloc] initWithFrame: NSMakeRect(0, 0, 10, 10)];
  
  label.stringValue = title;
  [label setEditable: NO];
  [label setDrawsBackground: NO];
  [label setBordered: NO];
  label.font = [NSFont systemFontOfSize: [NSFont labelFontSize]];
  
  [self addSubview: label];
  [label sizeToFit];
  [self tile];
  
  return label;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)addExpandingSpace {
  NSView *view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 0, 0)];
  [self addSubview: view];
  [view setHidden: YES];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)addSeparator {
  NSBox *box= [[NSBox alloc] initWithFrame: NSMakeRect(0, 2, 1, 18)];
  box.titlePosition = NSNoTitle;
  box.boxType = NSBoxSeparator;
  [self addSubview: box];

  [self tile];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSPopUpButton*)addSelectionPopUpWithItems: (NSArray*)items
                                      target: (id)target
                                      action: (SEL)action
                                defaultValue: (NSString*)defaultValue {
  NSPopUpButton *popup= [[NSPopUpButton alloc] initWithFrame: NSMakeRect(0, 0, 10, 10)];
  for (NSString *title in items) {
    if (title.length == 0)
      [popup.menu addItem: [NSMenuItem separatorItem]];
    else
      [popup addItemWithTitle: title];
  }

  popup.target = target;
  popup.action = action;
  popup.cell.controlSize = NSControlSizeSmall;
  popup.font = [NSFont systemFontOfSize: [NSFont smallSystemFontSize]];
  [popup sizeToFit];
  [popup setBordered: NO];
  [self addSubview: popup];
  
  if (!defaultValue)
    [popup selectItemAtIndex: 0];
  else
    [popup selectItemWithTitle: defaultValue];
  
  [self tile];
  
  return popup;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSPopUpButton*)addSelectionPopUpWithItems: (NSArray*)items
                                        name: (NSString*)name
                                      option: (NSString*)option
                                defaultValue: (NSString*)defaultValue {
  NSPopUpButton *popup = [self addSelectionPopUpWithItems: items
                                                   target: self
                                                   action: @selector(popupChanged:)
                                             defaultValue: defaultValue];
  
  popup.tag = mOptionInfoList.count;

  [mOptionInfoList addObject: @{@"name": name,  @"option": option}];
  
  return popup;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)addSelectionPopUpWithColors: (NSArray*)colors
                               name: (NSString*)name
                             option: (NSString*)option
                       defaultValue: (NSString*)defaultValue {
  NSPopUpButton *popup = [[NSPopUpButton alloc] initWithFrame: NSMakeRect(0, 0, 10, 10)];
  NSMenu *menu = [[NSMenu alloc] initWithTitle: @""];
  NSMenuItem *selected = nil;
  popup.cell.controlSize = NSControlSizeSmall;
  for (NSColor *color in colors) {
    NSMenuItem *item = [[NSMenuItem alloc] init];
    NSImage *image = [[NSImage alloc] initWithSize: NSMakeSize(24, 16)];
    [image lockFocus];
    [[NSColor lightGrayColor] set];
    NSFrameRect(NSMakeRect(1, 1, 22, 14));
    [color set];
    NSRectFill(NSMakeRect(2, 2, 20, 12));
    [image unlockFocus];
    item.image = image;
    item.title = @"";
    item.representedObject = color.hexString;
    [menu addItem: item];
    
    if ([defaultValue isEqual: item.representedObject])
      selected= item;
  }
  popup.menu = menu;
  
  if (selected)
    [popup selectItem: selected];
  
  [popup setBordered: NO];
  popup.target = self;
  popup.action = @selector(colorPopupChanged:);
  [popup sizeToFit];

  { // fix the extra unneeded padding we get for some reason
    NSRect frame= popup.frame;
    frame.size.width -= 24;
    popup.frame = frame;
  }
  [self addSubview: popup];
  
  popup.tag = mOptionInfoList.count;
  [mOptionInfoList addObject: @{@"name": name, @"option": option}];

  [self tile];
}

@end

//----------------------------------------------------------------------------------------------------------------------
