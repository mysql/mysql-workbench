/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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

#import "MTogglePane.h"

@interface MTogglePane () {
  NSTextField *_label;
  NSButton *_toggleButton;

  NSView *_content;

  NSMutableArray *_buttons;

  BOOL _initializing;
  BOOL _relayouting;
}
@end

//----------------------------------------------------------------------------------------------------------------------

#define HEADER_HEIGHT 26

@implementation MTogglePane

- (instancetype)initWithFrame: (NSRect)frame includeHeader: (BOOL)hasHeader {
  self = [super initWithFrame:frame];
  if (self != nil) {
    _initializing= YES;
    if (hasHeader) {
      _toggleButton= [[NSButton alloc] initWithFrame:NSMakeRect(5, 5, 13, 13)];
      _toggleButton.bezelStyle = NSBezelStyleDisclosure;
      [_toggleButton setButtonType: NSButtonTypeOnOff];
      _toggleButton.title = @"";
      _toggleButton.action = @selector(toggle:);
      _toggleButton.target = self;
      _toggleButton.state = NSControlStateValueOn; // expanded by default
      [self addSubview:_toggleButton];
    
      _label= [[NSTextField alloc] initWithFrame:NSMakeRect(20, 3, 20, 20)];
      [_label setBordered:NO];
      [_label setEditable:NO];
      _label.font = [NSFont boldSystemFontOfSize:12];
      [_label setDrawsBackground:NO];
      [self addSubview:_label];
    }
    _buttons= [NSMutableArray array];
    
    _initializing = NO;
    _relayouting = NO;
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithFrame: (NSRect)frame {
  return [self initWithFrame: frame includeHeader: YES];
}

//----------------------------------------------------------------------------------------------------------------------

-(instancetype)initWithCoder: (NSCoder *)coder {
  return [self initWithFrame: NSMakeRect(0, 0, 100, 100) includeHeader: YES];
}

//----------------------------------------------------------------------------------------------------------------------

- (void) dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver: self];
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)isFlipped {
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (IBAction)toggle: (id)sender {
  [self relayout];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setExpanded:(BOOL)flag {
  _toggleButton.state = flag ? NSControlStateValueOn : NSControlStateValueOff;
  [self relayout];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setLabel: (NSString*)label {
  _label.stringValue = label;
  [_label sizeToFit];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)contentFrameDidChange: (NSNotification*)notif {
  [self relayout];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)didAddSubview: (NSView *)subview {
  // for working with IB
  if (!_initializing) {
    _content= subview;
    [_content setPostsFrameChangedNotifications: YES];
    [[NSNotificationCenter defaultCenter] addObserver: self
                                             selector: @selector(contentFrameDidChange:)
                                                 name: NSViewFrameDidChangeNotification
                                               object: _content];
    subview.frameOrigin = NSMakePoint(0, HEADER_HEIGHT);
    subview.autoresizingMask = NSViewWidthSizable | NSViewMaxYMargin;
    [self relayout];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setContentView: (NSView*)view {
  if (_content) {
    [_content removeFromSuperview];
    [[NSNotificationCenter defaultCenter] removeObserver: self name: nil object: _content];
  }

  [self addSubview:view];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSView*)contentView {
  return _content;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)resizeSubviewsWithOldSize: (NSSize)oldBoundsSize {
  [self relayout];
  [super resizeSubviewsWithOldSize: oldBoundsSize];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)relayout {
  NSRect contentRect = _content.frame;
  NSRect newContentRect;
  NSRect buttonRect;
  NSRect rect = self.frame;

  if (_relayouting)
    return;
  _relayouting = YES;
  
  if (!_toggleButton || _toggleButton.state == NSControlStateValueOn) {
    rect.size.height = HEADER_HEIGHT + NSHeight(contentRect);

    [_content setHidden: NO];
  } else {
    rect.size.height = HEADER_HEIGHT;
    [_content setHidden: YES];
  }
  
  buttonRect.origin.x = rect.size.width - _buttons.count * HEADER_HEIGHT;
  buttonRect.origin.y = 0;
  buttonRect.size.width = HEADER_HEIGHT;
  buttonRect.size.height = HEADER_HEIGHT;

  for (NSButton *btn in _buttons) {
    btn.frame = buttonRect;
    buttonRect.origin.x += HEADER_HEIGHT;
  }
  
  newContentRect = NSMakeRect(0, HEADER_HEIGHT, NSWidth(rect), NSHeight(contentRect));
  
  if (!NSEqualRects(newContentRect, contentRect))
    _content.frame = newContentRect;
  
  if (!NSEqualRects(self.frame, rect))
    self.frame = rect;
  
  _relayouting = NO;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSButton*)addButton: (NSImage*)icon withAction: (SEL)selector target: (id)target {
  NSButton *button = [[NSButton alloc] initWithFrame: NSMakeRect(0, 0, HEADER_HEIGHT, HEADER_HEIGHT)];

  _initializing = YES;
  
  [_buttons addObject: button];
  
  [button setBordered: NO];
  button.image = icon;
  button.imagePosition = NSImageOnly;
  button.action = selector;
  button.target = target;
  button.enabled = (selector != nil) && (target != nil);
  
  [self addSubview:button];
  
  [self relayout];
  
  _initializing = NO;
  
  return button;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)drawRect: (NSRect)dirtyRect {
  BOOL isDark = NO;
  if (@available(macOS 10.14, *)) {
    isDark = self.window.effectiveAppearance.name == NSAppearanceNameDarkAqua;
  }

  if (isDark)
    [[NSColor colorWithDeviceRed: 0x3b / 255.0 green: 0x3b / 255.0 blue: 0x3b / 255.0 alpha: 1] set];
  else
    [[NSColor colorWithDeviceWhite: 0xec / 255.0 alpha: 1] set];
  dirtyRect.size.height = HEADER_HEIGHT;
  NSRectFill(dirtyRect);

  if (isDark)
    [NSColor.blackColor set];
  else
    [[NSColor colorWithDeviceWhite: 0xc0 / 255.0 alpha: 1] set];
  [NSBezierPath strokeLineFromPoint: { NSMinX(dirtyRect), NSMinY(dirtyRect) + 0.5 }
                            toPoint: { NSMaxX(dirtyRect), NSMinY(dirtyRect) + 0.5 }];

  // Draw also a bottom line if this entry is expanded or the last one in the child list.
  if ((!_toggleButton || _toggleButton.state == NSControlStateValueOn) || (self == self.superview.subviews.lastObject)) {
    [NSBezierPath strokeLineFromPoint: { NSMinX(dirtyRect), NSMaxY(dirtyRect) - 0.5 }
                              toPoint: { NSMaxX(dirtyRect), NSMaxY(dirtyRect) - 0.5 }];
  }
}

@end

//----------------------------------------------------------------------------------------------------------------------
