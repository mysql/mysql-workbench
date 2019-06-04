/*
 * Copyright (c) 2005, 2019, Oracle and/or its affiliates. All rights reserved.
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

#import "MCanvasScrollView.h"
#import "MCanvasViewer.h"
#include "mdc.h"

@implementation MCanvasScrollView

- (instancetype)initWithFrame: (NSRect)frame {
  self = [super initWithFrame: frame];
  if (self)  {
    CGFloat scrollerWidth = [NSScroller scrollerWidthForControlSize: NSControlSizeRegular
                                                      scrollerStyle: NSScrollerStyleOverlay];

    _hScroller= [[NSScroller alloc] initWithFrame:NSMakeRect(0, 0, NSWidth(frame) - scrollerWidth, scrollerWidth)];
    _vScroller= [[NSScroller alloc] initWithFrame:NSMakeRect(NSMaxX(frame) - scrollerWidth, scrollerWidth,
                                                             scrollerWidth, NSHeight(frame))];
    [self addSubview:_hScroller];
    [self addSubview:_vScroller];
    
    _hScroller.action = @selector(scrolled:);
    _hScroller.target = self;
    _vScroller.action = @selector(scrolled:);
    _vScroller.target = self;
    
    [_hScroller setEnabled:YES];
    [_vScroller setEnabled:YES];
  }
  return self;
}

- (NSView*)documentView {
  return _contentView;
}

- (void)tile {
  NSRect frame = self.bounds;
  NSRect contentFrame;

  CGFloat scrollerWidth = [NSScroller scrollerWidthForControlSize: NSControlSizeRegular
                                                    scrollerStyle: NSScrollerStyleOverlay];
  contentFrame = NSMakeRect(0, scrollerWidth, NSWidth(frame) - scrollerWidth, NSHeight(frame) - scrollerWidth);
  _contentView.frame = contentFrame;
  if (_hAccessoryView) {
    NSRect accRect = _hAccessoryView.frame;
    _hAccessoryView.frame = NSMakeRect(0, 0, NSWidth(accRect), scrollerWidth);
    _hScroller.frame = NSMakeRect(NSWidth(accRect), 0, NSWidth(frame) - NSWidth(accRect) - scrollerWidth, scrollerWidth);
  } else {
    _hScroller.frame = NSMakeRect(0, 1, NSWidth(frame) - scrollerWidth, scrollerWidth);
  }

  _vScroller.frame = NSMakeRect(NSWidth(frame) - scrollerWidth, scrollerWidth,
                                scrollerWidth, NSHeight(frame) - scrollerWidth);
  
  [self reflectContentRect];
}

- (void)scrolled: (id)sender {
  float line;
  float page;
  NSRect visible = _contentView.documentVisibleRect;
  NSRect total = _contentView.documentRect;
 
  if (sender == _hScroller) {
    line = NSWidth(visible) / 20 / NSWidth(total);
    page = NSWidth(visible) / NSWidth(total);
  } else {
    line = (NSHeight(visible)/20) / NSHeight(total);
    page = NSHeight(visible) / NSHeight(total);    
  }

  switch ([sender hitPart])  {
    case NSScrollerDecrementPage:
      [sender setFloatValue:MAX([sender floatValue] - page, 0.0)];
      break;
    case NSScrollerIncrementPage:
      [sender setFloatValue:MIN([sender floatValue] + page, 1.0)];
      break;      

    default:
      break;
  }

  [_contentView scrollToPoint:NSMakePoint(_hScroller.floatValue * (NSWidth(total) - NSWidth(visible)), 
                                          _vScroller.floatValue * (NSHeight(total) - NSHeight(visible)))];
}

- (void)reflectContentRect {
  NSRect total = _contentView.documentRect;
  NSRect visible = _contentView.documentVisibleRect;
   
  if (NSWidth(visible) >= NSWidth(total)) {
    _hScroller.floatValue = 0.0;
    _hScroller.knobProportion = 1.0;
  } else {
    _hScroller.floatValue = visible.origin.x/(NSWidth(total)-NSWidth(visible));
    _hScroller.knobProportion = NSWidth(visible)/NSWidth(total);
  }
  
  if (NSHeight(visible) >= NSHeight(total)) {
    _vScroller.floatValue = 0.0;
    _vScroller.knobProportion = 1.0;
  } else {
    _vScroller.floatValue = visible.origin.y/(NSHeight(total)-NSHeight(visible));
    _vScroller.knobProportion = NSHeight(visible)/NSHeight(total);
  }
}

static void canvas_view_viewport_changed(void *self) {
  [(__bridge id)self reflectContentRect];
}

- (void)setContentCanvas:(MCanvasViewer*)canvas {
  _contentView = canvas;

  canvas.canvas->signal_viewport_changed()->connect(std::bind(canvas_view_viewport_changed, (__bridge void *)self));
  
  [self addSubview: canvas];
  [self tile];
}

- (NSSize)contentSize {
  NSSize size = self.frame.size;
  
  CGFloat scrollerWidth = [NSScroller scrollerWidthForControlSize: NSControlSizeRegular
                                                    scrollerStyle: NSScrollerStyleOverlay];
  size.width -= scrollerWidth;
  size.height -= scrollerWidth;
  
  return size;
}

- (void)resizeSubviewsWithOldSize:(NSSize)oldBoundsSize {
  [self tile];
}

- (void)setHAccessory:(NSView*)view {
  [self addSubview: view];
  _hAccessoryView = view;
  [self tile];
}

- (NSScroller*)verticalScroller {
  return _vScroller;
}

- (NSScroller*)horizontalScroller {
  return _hScroller;
}

- (void)scrollWheel: (NSEvent *)theEvent {
  // setting to /20 makes horizontal scroller (which is like a button) in my mouse 
  // scroll nicely, but it doesn't work well with trackpads
  //[_hScroller setFloatValue:[_hScroller floatValue] - [theEvent deltaX]/20];
  _hScroller.floatValue = _hScroller.floatValue - theEvent.deltaX / 200;
  [self scrolled: _hScroller];
  _vScroller.floatValue = _vScroller.floatValue - theEvent.deltaY / 200;
  [self scrolled: _vScroller];
}

- (void)drawRect:(NSRect)frame {
  [NSColor.textBackgroundColor set];
  NSRectFill(frame);
}

@end
