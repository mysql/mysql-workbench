/*
 * Copyright (c) 2010, 2019, Oracle and/or its affiliates. All rights reserved.
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

#import "MFPopup.h"
#import "MFMForms.h"

#include <cairo/cairo-quartz.h>

#pragma mark - PopupContentView

@interface PopupContentView : NSView {
  mforms::Popup *mOwner;
}
@end

//----------------------------------------------------------------------------------------------------------------------

@implementation PopupContentView

- (instancetype)initWithOwner: (mforms::Popup*)popup {
  self = [self initWithFrame: NSMakeRect(0, 0, 10, 10)];
  if (self) {
    mOwner = popup;
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)drawRect: (NSRect)rect {
  if (mOwner == nil)
    return;
  
  NSRect frame = self.frame;
  
  [[NSColor clearColor] set];
  NSRectFillUsingOperation(frame, NSCompositingOperationSourceOver);

  CGContextRef cgref = (CGContextRef)[NSGraphicsContext currentContext].CGContext;
  cairo_surface_t *surface = cairo_quartz_surface_create_for_cg_context(cgref, NSWidth(frame), NSHeight(frame));
  
  cairo_t *cr = cairo_create(surface);
  try {
    mOwner->repaint(cr, rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
  }
  catch(...) {
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    throw;
  }
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)isFlipped {
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)isOpaque {
  return NO;
}

//----------------------------------------------------------------------------------------------------------------------

@end

#pragma mark - MFPopupImpl

//----------------------------------------------------------------------------------------------------------------------

@implementation MFPopupImpl

@synthesize popupStyle = mStyle;

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithObject: (mforms::Popup*)popup style: (mforms::PopupStyle)aStyle
{
  NSUInteger mask = NSWindowStyleMaskBorderless;
  if (aStyle == mforms::PopupBezel)
    mask |= NSWindowStyleMaskHUDWindow;
  self = [super initWithContentRect: NSMakeRect(0, 0, 800, 300)
                          styleMask: mask
                            backing: NSBackingStoreBuffered
                              defer: NO];
  if (self) {
    mResult = 0;
    mDone = NO;
    mOwner = popup;
    mOwner->set_data(self);
    self.popupStyle = aStyle;
    
    [self setAcceptsMouseMovedEvents: YES];
    [self setMovableByWindowBackground: NO];
    [self setOpaque: NO];

    NSView* content = [[PopupContentView alloc] initWithOwner: popup];
    self.contentView = content;

    switch (aStyle) {
      case mforms::PopupBezel: {
        [self setHidesOnDeactivate: YES];
        [self setHasShadow : YES];
        NSRect windowFrame = self.frame;
        windowFrame.origin.x = 26;
        windowFrame.origin.y = 14;
        windowFrame.size.width -= 52;
        windowFrame.size.height -= 28;
        content.frame = windowFrame;
        break;
      }
        
      default:
        [self setHidesOnDeactivate: NO];
        [self setHasShadow : NO];
        content.frame = self.frame;
        break;
    }
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)dealloc {
  [NSObject cancelPreviousPerformRequestsWithTarget: self];
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)isFlipped {
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)hidePopup {
  if (mOwner == nil) // Set to nil if we were called already (orderOut triggeres the resignKey event).
    return;

  mforms::Popup *owner = mOwner;
  mOwner = nil;
  switch (self.popupStyle) {
    case mforms::PopupBezel:
      [NSAnimationContext currentContext].duration = 0.25;
      [self animator].alphaValue = 0;
      [self performSelector: @selector(orderPopupOut) withObject: nil afterDelay: 0.5
                    inModes: @[NSModalPanelRunLoopMode, NSDefaultRunLoopMode]];
      break;

    default:
      [self orderOut: nil];
      break;
  }

  owner->closed();
}

//----------------------------------------------------------------------------------------------------------------------

- (void)orderPopupOut {
  [self orderOut: nil];
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Helper to avoid code duplication.
 */
- (void)doMouseUp: (NSEvent*)event {
  if (mOwner == nil)
    return;
  
  NSPoint p = [self.contentView convertPoint: event.locationInWindow fromView: nil];
  mforms::MouseButton mouseButton;
  switch (event.buttonNumber) {
    case NSEventTypeRightMouseDown:
      mouseButton = mforms::MouseButtonRight;
      break;

    case NSEventTypeOtherMouseDown:
      mouseButton = mforms::MouseButtonOther;
      break;

    default:
      mouseButton = mforms::MouseButtonLeft;
  }
  
  switch (event.clickCount) {
    case 1:
      mOwner->mouse_up(mouseButton, p.x, p.y);
      if (!mDone && mOwner != nil)
        mOwner->mouse_click(mouseButton, p.x, p.y);
      break;
    case 2:
      mOwner->mouse_double_click(mouseButton, p.x, p.y);
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Helper to avoid code duplication.
 */
- (void)doMouseDown: (NSEvent*)event {
  if (mOwner == nil)
    return;
  
  NSPoint p = [self.contentView convertPoint: event.locationInWindow fromView: nil];
  mforms::MouseButton mouseButton;
  switch (event.buttonNumber) {
    case NSEventTypeRightMouseDown:
      mouseButton = mforms::MouseButtonRight;
      break;

    case NSEventTypeOtherMouseDown:
      mouseButton = mforms::MouseButtonOther;
      break;

    default:
      mouseButton = mforms::MouseButtonLeft;
  }
  
  mOwner->mouse_down(mouseButton, p.x, p.y);
}

//----------------------------------------------------------------------------------------------------------------------

- (void)mouseDown: (NSEvent*)event {
  [self doMouseDown: event];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)mouseUp: (NSEvent*)event {
  [self doMouseUp: event];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)otherMouseDown: (NSEvent*)event {
  [self doMouseDown: event];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)otherMouseUp: (NSEvent*)event {
  [self doMouseUp: event];
}

//----------------------------------------------------------------------------------------------------------------------

- (void) rightMouseDown: (NSEvent*) event {
  [self doMouseDown: event];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)rightMouseUp: (NSEvent*)event {
  [self doMouseUp: event];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)mouseMoved: (NSEvent *)event {
  if (mOwner == nil)
    return;
  
  NSPoint p = [self.contentView convertPoint: event.locationInWindow fromView: nil];
  mOwner->mouse_move(mforms::MouseButtonNone, p.x, p.y);
}
	
//----------------------------------------------------------------------------------------------------------------------

- (void)mouseEntered: (NSEvent *)event {
  if (mOwner != nil)
    mOwner->mouse_enter();
}

//----------------------------------------------------------------------------------------------------------------------

- (void)mouseExited: (NSEvent *)event {
  if (mOwner != nil)
    mOwner->mouse_leave();
}

//----------------------------------------------------------------------------------------------------------------------

- (void)keyDown: (NSEvent *)event {
  unsigned short code = event.keyCode;
  if (code == 53) // Esc
  {
    mResult = 0;
    mDone = YES;
    [self hidePopup];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)acceptsFirstResponder {
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)canBecomeKeyWindow {
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSInteger)runModalAtPosition: (NSPoint)position {
  // By design the window's upper right corner is to set at the given position.
  NSRect frame = self.frame;
  position.x -= frame.size.width;
  [self setFrameTopLeftPoint: position];
  
  [self makeKeyAndOrderFront: nil];
  
  [NSAnimationContext currentContext].duration = 0.25;
  [self animator].alphaValue = 1;

  mDone = NO;

  // Local loop to simulate a modal window.
  while (!mDone) {
    NSEvent *theEvent = [self nextEventMatchingMask:
                         NSEventMaskMouseMoved
                         | NSEventMaskMouseEntered
                         | NSEventMaskMouseExited
                         | NSEventMaskLeftMouseDown
                         | NSEventMaskLeftMouseUp
                         | NSEventMaskRightMouseDown
                         | NSEventMaskRightMouseUp
                         | NSEventMaskOtherMouseDown
                         | NSEventMaskOtherMouseUp
                         | NSEventMaskKeyDown
                         | NSEventMaskKeyUp
                         ];
    if (theEvent.type == NSEventTypeLeftMouseUp &&
        ![self.contentView mouse: theEvent.locationInWindow inRect:self.contentView.bounds]) {
      [self hidePopup];
      mDone = YES;
    } else {
      [self sendEvent: theEvent];
    }
  }
  
  return mResult;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)showAtPosition: (NSPoint)position {
  [self setFrameTopLeftPoint: position];
  [self makeKeyAndOrderFront: nil];

  mDone = NO;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)resignKeyWindow {
  [super resignKeyWindow];
  [self performSelector: @selector(hidePopup)
             withObject: nil
             afterDelay: 0
                inModes: @[NSModalPanelRunLoopMode, NSDefaultRunLoopMode]];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityWindowRole;
}

//----------------------------------------------------------------------------------------------------------------------

static bool popup_create(mforms::Popup *self, mforms::PopupStyle style) {
  return [[MFPopupImpl alloc] initWithObject: self style: style] != nil;
}

//----------------------------------------------------------------------------------------------------------------------

static void popup_destroy(mforms::Popup *self) {
  self->set_data(nil); // Should release the panel.
}

//----------------------------------------------------------------------------------------------------------------------

static void popup_set_needs_repaint(mforms::Popup *self) {
  [[self->get_data() contentView] setNeedsDisplay: YES];
}

//----------------------------------------------------------------------------------------------------------------------

static void popup_set_size(mforms::Popup *self, int w, int h) {
  NSRect frame = [self->get_data() frame];
  frame.size.width = w;
  frame.size.height = h;
  [self->get_data() setFrame: frame display: YES];
}

//----------------------------------------------------------------------------------------------------------------------

static int popup_show(mforms::Popup *self, int x, int y) {
  MFPopupImpl *popup = (MFPopupImpl*)self->get_data();
  switch (popup.popupStyle) {
    case mforms::PopupBezel:
      return (int)[popup runModalAtPosition: NSMakePoint(x, y)];
      break;

    default:
      [popup showAtPosition: NSMakePoint(x, y)];
      return 0;
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static base::Rect popup_get_content_rect(mforms::Popup *self) {
  NSRect frame = [self->get_data() contentView].frame;
  return base::Rect(frame.origin.x, frame.origin.y, frame.size.width, frame.size.height);
}

//----------------------------------------------------------------------------------------------------------------------

static void popup_set_modal_result(mforms::Popup *self, int result) {
  MFPopupImpl *popup = (MFPopupImpl*)self->get_data();
  popup->mResult = result;
  popup->mDone = YES;
  [popup hidePopup];
}

//----------------------------------------------------------------------------------------------------------------------

void cf_popup_init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();
  
  f->_popup_impl.create = &popup_create;
  f->_popup_impl.destroy = &popup_destroy;
  f->_popup_impl.set_needs_repaint = &popup_set_needs_repaint;
  f->_popup_impl.set_size = &popup_set_size;
  f->_popup_impl.show = &popup_show;
  f->_popup_impl.get_content_rect = &popup_get_content_rect;
  f->_popup_impl.set_modal_result = &popup_set_modal_result;
}

//----------------------------------------------------------------------------------------------------------------------

@end

