/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

#import "MCanvasViewer.h"
#include "mdc.h"
#include "mdc_canvas_view_macosx.h"

@implementation MCanvasViewer

@synthesize delegate;

- (instancetype)initWithFrame: (NSRect)frame {
  self = [super initWithFrame: frame];
  if (self != NULL) {
    _view = 0;
  }
  return self;
}

- (void)dealloc {
  [NSObject cancelPreviousPerformRequestsWithTarget: self];

  if (_view)
    _view->pre_destroy();
  delete _view;
}

- (BOOL)isFlipped {
  return YES;
}

- (void)setFrame:(NSRect)frame {
  super.frame = frame;
  if (_view) {
    _view->update_view_size((int)NSWidth(frame), (int)NSHeight(frame));

    if (_trackingArea)
      [self removeTrackingArea: _trackingArea];

    _trackingArea = [[NSTrackingArea alloc]
      initWithRect: NSMakeRect(0, 0, NSWidth(frame), NSHeight(frame))
           options: NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingActiveAlways |
                    NSTrackingInVisibleRect | NSTrackingEnabledDuringMouseDrag
             owner: self
          userInfo: 0];

    [self addTrackingArea: _trackingArea];
  }
}

- (void)drawRect: (NSRect)rect {
  if (_view) {
    [super drawRect: rect];

    _view->set_target_context((CGContextRef)NSGraphicsContext.currentContext.CGContext);
    _view->repaint(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);

    if (_firstResponder) {
    }
  } else {
    [super drawRect: rect];
    [[NSColor grayColor] set];
    NSRectFill(self.bounds);
  }
}

static void canvas_view_needs_repaint(int x, int y, int w, int h, void *viewer) {
  // Defer to main thread.
  [(__bridge NSView *)viewer performSelectorOnMainThread: @selector(redraw) withObject:nil waitUntilDone:false];
}

- (void)redraw {
  self.needsDisplay = YES;
}

- (void)setCursor:(NSCursor *)cursor {
  _cursor = cursor;

  [self.window resetCursorRects];
}

- (void)resetCursorRects {
  [super resetCursorRects];
  if (_cursor) {
    [self addCursorRect: self.bounds cursor: _cursor];
  }
}

- (void)setupQuartz {
  _view = new mdc::QuartzCanvasView(NSWidth(self.frame), NSHeight(self.frame));
  _view->signal_repaint()->connect(std::bind(canvas_view_needs_repaint, std::placeholders::_1, std::placeholders::_2,
                                             std::placeholders::_3, std::placeholders::_4, (__bridge void *)self));
}

- (mdc::CanvasView *)canvas {
  return _view;
}

- (void)scrollToPoint:(NSPoint)offset {
  _view->set_offset(base::Point(offset.x, offset.y));
}

- (NSRect)documentRect {
  base::Size size(_view->get_total_view_size());

  return NSMakeRect(0, 0, size.width, size.height);
}

- (NSRect)documentVisibleRect {
  base::Rect rect(_view->get_viewport());

  return NSMakeRect(rect.pos.x, rect.pos.y, rect.size.width, rect.size.height);
}

- (BOOL)acceptsFirstResponder {
  return YES;
}

- (BOOL)becomeFirstResponder {
  _firstResponder = YES;
  [self setNeedsDisplay: YES];

  return YES;
}

- (BOOL)resignFirstResponder {
  _firstResponder = NO;
  [self setNeedsDisplay: YES];

  return YES;
}

static mdc::EventState makeEventState(NSEvent *event) {
  int state = 0;

  if (event.modifierFlags & NSEventModifierFlagShift)
    state |= mdc::SShiftMask;
  if (event.modifierFlags & NSEventModifierFlagControl)
    state |= mdc::SControlMask;
  if (event.modifierFlags & NSEventModifierFlagOption)
    state |= mdc::SOptionMask;
  if (event.modifierFlags & NSEventModifierFlagCommand)
    state |= mdc::SCommandMask;

  return (mdc::EventState)state;
}

static mdc::KeyInfo makeKeyInfo(NSEvent *theEvent) {
  static struct KeyCodeMapping {
    unichar key;
    mdc::KeyCode kcode;
  } keycodes[] = {
    {033, mdc::KEscape},
    {010, mdc::KBackspace},
    {0x7f, mdc::KBackspace},
    {'\t', mdc::KTab},
    {' ', mdc::KSpace},
    {'\n', mdc::KEnter},
    {'\r', mdc::KEnter},

    {NSUpArrowFunctionKey, mdc::KUp},
    {NSDownArrowFunctionKey, mdc::KDown},
    {NSLeftArrowFunctionKey, mdc::KLeft},
    {NSRightArrowFunctionKey, mdc::KRight},
    {NSF1FunctionKey, mdc::KF1},
    {NSF2FunctionKey, mdc::KF2},
    {NSF3FunctionKey, mdc::KF3},
    {NSF4FunctionKey, mdc::KF4},
    {NSF5FunctionKey, mdc::KF5},
    {NSF6FunctionKey, mdc::KF6},
    {NSF7FunctionKey, mdc::KF7},
    {NSF8FunctionKey, mdc::KF8},
    {NSF9FunctionKey, mdc::KF9},
    {NSF10FunctionKey, mdc::KF10},
    {NSF11FunctionKey, mdc::KF11},
    {NSF12FunctionKey, mdc::KF12},
    /*  { NSF13FunctionKey, mdc::KF13 },
      { NSF14FunctionKey, mdc::KF14 },
      { NSF15FunctionKey, mdc::KF15 },
      { NSF16FunctionKey, mdc::KF16 },
      { NSF17FunctionKey, mdc::KF17 },
      { NSF18FunctionKey, mdc::KF18 },
      { NSF19FunctionKey, mdc::KF19 },
      { NSF20FunctionKey, mdc::KF20 },
      { NSF21FunctionKey, mdc::KF21 },
      { NSF22FunctionKey, mdc::KF22 },
      { NSF23FunctionKey, mdc::KF23 },
      { NSF24FunctionKey, mdc::KF24 },
      { NSF25FunctionKey, mdc::KF25 },
      { NSF26FunctionKey, mdc::KF26 },
      { NSF27FunctionKey, mdc::KF27 },
      { NSF28FunctionKey, mdc::KF28 },
      { NSF29FunctionKey, mdc::KF29 },
      { NSF30FunctionKey, mdc::KF30 },
      { NSF31FunctionKey, mdc::KF31 },
      { NSF32FunctionKey, mdc::KF32 },
      { NSF33FunctionKey, mdc::KF33 },
      { NSF34FunctionKey, mdc::KF34 },
      { NSF35FunctionKey, mdc::KF35 },*/
    {NSInsertFunctionKey, mdc::KInsert},
    {NSDeleteFunctionKey, mdc::KDelete},
    {NSHomeFunctionKey, mdc::KHome},
    //    { NSBeginFunctionKey, mdc::KBegin },
    {NSEndFunctionKey, mdc::KEnd},
    {NSPageUpFunctionKey, mdc::KPageUp},
    {NSPageDownFunctionKey, mdc::KPageDown},
    //    { NSPrintScreenFunctionKey, mdc::KPrintScreen },
    //    { NSScrollLockFunctionKey, mdc::KScrollLock },
    //    { NSPauseFunctionKey, mdc::KPause },
    //    { NSSysReqFunctionKey, mdc::KSysReq },
    //    { NSBreakFunctionKey, mdc::KBreak },
    //    { NSResetFunctionKey, mdc::KReset },
    //    { NSStopFunctionKey, mdc::KStop },
    //    { NSMenuFunctionKey, mdc::KMenu },
    //    { NSUserFunctionKey, mdc::KUser },
    //    { NSSystemFunctionKey, mdc::KSystem },
    //    { NSPrintFunctionKey, mdc::KPrint },
    //    { NSClearLineFunctionKey, mdc::KClearLine },
    //    { NSClearDisplayFunctionKey, mdc::KClearDisplay },
    //    { NSInsertLineFunctionKey, mdc::KInsertLine },
    //    { NSDeleteLineFunctionKey, mdc::KDeleteLine },
    //    { NSInsertCharFunctionKey, mdc::KInsert },
    //    { NSDeleteCharFunctionKey, mdc::KDelete },
    //    { NSPrevFunctionKey, mdc::KPageUp },
    //    { NSNextFunctionKey, mdc::KPageDown },
    //    { NSSelectFunctionKey, mdc::KSelect },
    //    { NSExecuteFunctionKey, mdc::KExecute },
    //    { NSUndoFunctionKey, mdc::KUndo },
    //    { NSRedoFunctionKey, mdc::KRedo },
    //    { NSFindFunctionKey, mdc::KFind },
    //    { NSHelpFunctionKey, mdc::KHelp },
    //    { NSModeSwitchFunctionKey, mdc::KModeSwitch },
  };

  mdc::KeyInfo k;

  k.keycode = mdc::KNone;
  k.string = "";
  for (unsigned int i = 0; i < sizeof(keycodes) / sizeof(*keycodes); i++) {
    if (keycodes[i].key == [theEvent.characters characterAtIndex: 0]) {
      k.keycode = keycodes[i].kcode;
      break;
    }
  }

  if (k.keycode == 0 && theEvent.characters.length > 0) {
    k.string = theEvent.charactersIgnoringModifiers.UTF8String;
  }

  return k;
}

- (void)rightMouseDown: (NSEvent *)theEvent {
  if (!_view)
    return;

  [self.window makeFirstResponder: self];

  NSPoint point = [self convertPoint: theEvent.locationInWindow fromView: nil];
  mdc::EventState state = makeEventState(theEvent);
  mdc::MouseButton button = mdc::ButtonRight;

  _buttonState |= mdc::SRightButtonMask;

  switch (theEvent.clickCount) {
    case 1:
      if (![self.delegate canvasMouseDown:button location:point state:(mdc::EventState)(state | _buttonState)])
        _view->handle_mouse_button(button, true, point.x, point.y, (mdc::EventState)(state | _buttonState));
      break;
    case 2:
      if (![self.delegate canvasMouseDoubleClick:button location:point state:(mdc::EventState)(state | _buttonState)])
        _view->handle_mouse_double_click(button, point.x, point.y, (mdc::EventState)(state | _buttonState));
      break;
  }
}

- (void)otherMouseDown: (NSEvent *)theEvent {
  if (!_view)
    return;
  NSPoint point = [self convertPoint:theEvent.locationInWindow fromView:nil];
  mdc::EventState state = makeEventState(theEvent);
  mdc::MouseButton button = mdc::ButtonMiddle;

  _buttonState |= mdc::SMiddleButtonMask;

  switch (theEvent.clickCount) {
    case 1:
      if (![self.delegate canvasMouseDown:button location:point state:(mdc::EventState)(state | _buttonState)])
        _view->handle_mouse_button(button, true, point.x, point.y, (mdc::EventState)(state | _buttonState));
      break;

    case 2:
      if (![self.delegate canvasMouseDoubleClick:button location:point state:(mdc::EventState)(state | _buttonState)])
        _view->handle_mouse_double_click(button, point.x, point.y, (mdc::EventState)(state | _buttonState));
      break;
  }
}

- (void)mouseDown:(NSEvent *)theEvent {
  if (!_view)
    return;
  NSPoint point = [self convertPoint:theEvent.locationInWindow fromView:nil];
  mdc::EventState state = makeEventState(theEvent);
  mdc::MouseButton button = mdc::ButtonLeft;

  // turn control-click to a right mouse event
  if ((state & mdc::SControlMask) && button == mdc::ButtonLeft)
    button = mdc::ButtonRight;

  _buttonState |= mdc::SLeftButtonMask;

  switch (theEvent.clickCount) {
    case 1:
      if (![self.delegate canvasMouseDown:button location:point state:(mdc::EventState)(state | _buttonState)])
        _view->handle_mouse_button(button, true, point.x, point.y, (mdc::EventState)(state | _buttonState));
      break;

    case 2:
      if (![self.delegate canvasMouseDoubleClick:button location:point state:(mdc::EventState)(state | _buttonState)])
        _view->handle_mouse_double_click(button, point.x, point.y, (mdc::EventState)(state | _buttonState));
      break;
  }
}

- (void)rightMouseUp: (NSEvent *)theEvent {
  if (!_view)
    return;
  NSPoint point = [self convertPoint: theEvent.locationInWindow fromView: nil];
  mdc::EventState state = makeEventState(theEvent);
  mdc::MouseButton button = mdc::ButtonRight;

  _buttonState &= ~mdc::SRightButtonMask;

  if (![self.delegate canvasMouseUp: button location: point state: (mdc::EventState)(state | _buttonState)]) {
    _view->handle_mouse_button(button, false, point.x, point.y, (mdc::EventState)(state | _buttonState));
  }
}

- (void)otherMouseUp: (NSEvent *)theEvent {
  if (!_view)
    return;
  NSPoint point = [self convertPoint: theEvent.locationInWindow fromView: nil];
  mdc::EventState state = makeEventState(theEvent);
  mdc::MouseButton button = mdc::ButtonMiddle;

  _buttonState &= ~mdc::SMiddleButtonMask;

  if (![self.delegate canvasMouseUp:button location:point state:(mdc::EventState)(state | _buttonState)]) {
    _view->handle_mouse_button(button, false, point.x, point.y, (mdc::EventState)(state | _buttonState));
  }
}

- (void)mouseUp: (NSEvent *)theEvent {
  if (!_view)
    return;
  NSPoint point = [self convertPoint: theEvent.locationInWindow fromView: nil];
  mdc::EventState state = makeEventState(theEvent);
  mdc::MouseButton button = (mdc::MouseButton)theEvent.buttonNumber;

  // turn control-click to a right mouse event
  if ((state & mdc::SControlMask) && button == mdc::ButtonLeft)
    button = mdc::ButtonRight;

  _buttonState &= ~mdc::SLeftButtonMask;

  if (![self.delegate canvasMouseUp: button location: point state: (mdc::EventState)(state | _buttonState)]) {
    _view->handle_mouse_button(button, false, point.x, point.y, (mdc::EventState)(state | _buttonState));
  }
}

- (void)mouseDragged: (NSEvent *)theEvent {
  if (!_view)
    return;
  NSPoint point = [self convertPoint: theEvent.locationInWindow fromView: nil];
  mdc::EventState state = makeEventState(theEvent);

  if (![self.delegate canvasMouseMoved: point state: (mdc::EventState)(state | _buttonState)]) {
    _view->handle_mouse_move(point.x, point.y, (mdc::EventState)(state | _buttonState));
  }
}

- (void)rightMouseDragged: (NSEvent *)theEvent {
  [self mouseDragged:theEvent];
}

- (void)otherMouseDragged: (NSEvent *)theEvent {
  [self mouseDragged:theEvent];
}

- (void)mouseMoved: (NSEvent *)theEvent {
  [self mouseDragged:theEvent];
}

- (void)mouseExited: (NSEvent *)theEvent {
  if (!_view)
    return;
  NSPoint point = [self convertPoint: theEvent.locationInWindow fromView: nil];
  mdc::EventState state = makeEventState(theEvent);

  //  if (![[self delegate] canvasMouseExited: point state: (mdc::EventState)(state | _buttonState)])
  { _view->handle_mouse_leave(point.x, point.y, (mdc::EventState)(state | _buttonState)); }
}

- (void)keyDown: (NSEvent *)theEvent {
  mdc::KeyInfo key = makeKeyInfo(theEvent);
  if (![self.delegate canvasKeyDown: key state: makeEventState(theEvent)])
    _view->handle_key(key, true, makeEventState(theEvent));
}

- (void)keyUp: (NSEvent *)theEvent {
  mdc::KeyInfo key = makeKeyInfo(theEvent);
  if (![self.delegate canvasKeyUp: key state: makeEventState(theEvent)])
    _view->handle_key(key, false, makeEventState(theEvent));
}

- (NSDragOperation)draggingEntered: (id<NSDraggingInfo>)sender {
  return [self.delegate canvasDraggingEntered: sender];
}

- (NSDragOperation)draggingUpdated: (id<NSDraggingInfo>)sender {
  return [self.delegate canvasDraggingEntered: sender];
}

- (BOOL)performDragOperation: (id<NSDraggingInfo>)sender {
  return [self.delegate canvasPerformDragOperation: sender];
}

@end
