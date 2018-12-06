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

#include "mforms/view.h"

#import "MFBase.h"

// A macro to ease adding support for backend based mouse/focus handling (e.g. for drag'n drop).
// No handling for right mouse button here.
#define STANDARD_MOUSE_HANDLING_NO_RIGHT_BUTTON(wrapper)        \
  -(void)mouseDown : (NSEvent *)event {                         \
    if (![self handleMouseDown:event owner:wrapper->mOwner])    \
      [super mouseDown:event];                                  \
  }                                                             \
  -(void)mouseUp : (NSEvent *)event {                           \
    if (![self handleMouseUp:event owner:wrapper->mOwner])      \
      [super mouseUp:event];                                    \
  }                                                             \
  -(void)otherMouseDown : (NSEvent *)event {                    \
    if (![self handleMouseDown:event owner:wrapper->mOwner])    \
      [super otherMouseDown:event];                             \
  }                                                             \
  -(void)otherMouseUp : (NSEvent *)event {                      \
    if (![self handleMouseUp:event owner:wrapper->mOwner])      \
      [super otherMouseUp:event];                               \
  }                                                             \
  -(void)mouseMoved : (NSEvent *)event {                        \
    if (![self handleMouseMove:event owner:wrapper->mOwner])    \
      [super mouseMoved:event];                                 \
  }                                                             \
  -(void)mouseDragged : (NSEvent *)event {                      \
    if (![self handleMouseMove:event owner:wrapper->mOwner])    \
      [super mouseDragged:event];                               \
  }                                                             \
  -(void)mouseEntered : (NSEvent *)event {                      \
    if (![self handleMouseEntered:event owner:wrapper->mOwner]) \
      [super mouseEntered:event];                               \
  }                                                             \
  -(void)mouseExited : (NSEvent *)event {                       \
    if (![self handleMouseExited:event owner:wrapper->mOwner])  \
      [super mouseExited:event];                                \
  }                                                             \
  -(BOOL)acceptsFirstResponder {                                \
    return YES;                                                 \
  }                                                             \
  -(void)updateTrackingAreas {                                  \
    [super updateTrackingAreas];                                \
    mTrackingArea = [self updateTrackingArea:mTrackingArea];    \
  }

// A macro to ease adding support for backend based mouse handling (e.g. for drag'n drop).
#define STANDARD_MOUSE_HANDLING(wrapper)                     \
  STANDARD_MOUSE_HANDLING_NO_RIGHT_BUTTON(wrapper)           \
  -(void)rightMouseDown : (NSEvent *)event {                 \
    if (![self handleMouseDown:event owner:wrapper->mOwner]) \
      [super mouseDown:event];                               \
  }                                                          \
  -(void)rightMouseUp : (NSEvent *)event {                   \
    if (![self handleMouseUp:event owner:wrapper->mOwner])   \
      [super mouseUp:event];                                 \
  }

#define STANDARD_FOCUS_HANDLING(wrapper)                     \
  -(BOOL)becomeFirstResponder {                              \
    wrapper->mOwner->focus_changed();                        \
    if (![self handleBecomeFirstResponder: wrapper->mOwner]) \
      return [super becomeFirstResponder];                   \
    return NO;                                               \
  }                                                          \
  -(BOOL)resignFirstResponder {                              \
    if (![self handleResignFirstResponder: wrapper->mOwner]) \
      return [super resignFirstResponder];                   \
    return NO;                                               \
  }

#define STANDARD_KEYBOARD_HANDLING(wrapper)                 \
  -(void)keyUp : (NSEvent *)event {                         \
    if (![self handleKeyUp: event owner:wrapper->mOwner])   \
      return [super keyUp:event];                           \
  }                                                         \
  -(void)keyDown : (NSEvent *)event {                       \
    if (![self handleKeyDown: event owner:wrapper->mOwner]) \
      return [super keyDown:event];                         \
  }

enum ViewFlags {
  ExpandFlag = 1,
  FillFlag = 2,
  PackEndFlag = 4,

  BoxFlagMask = 0xff
};

namespace mforms {
  class View;
}

@interface NSPasteboard (MySQLWorkbench)

- (void)writeNativeData:(void *)data typeAsString:(NSString *)type;
- (void)writeNativeData:(void *)data typeAsChar:(const char *)type;
- (void *)nativeDataForTypeAsString:(NSString *)type NS_RETURNS_INNER_POINTER;
- (void *)nativeDataForTypeAsChar:(const char *)type NS_RETURNS_INNER_POINTER;

@end

/** MForms control implementations must subclass their own NS counterpart and implement
 *  the size related methods like below if they need it.
 */
@interface NSView (MForms)<NSDraggingDestination, NSDraggingSource, NSPasteboardItemDataProvider>

@property NSArray *acceptableDropFormats;
@property mforms::DragOperation lastDragOperation;
@property mforms::DragOperation allowedDragOperations;
@property mforms::DropPosition lastDropPosition; // Only valid during a drag operation. Set by descendants.

@property ViewFlags viewFlags;
@property(readonly, strong) id innerView;
@property NSSize minimumSize;

- (bool)handleMouseUp:(NSEvent *)event owner:(mforms::View *)mOwner;
- (bool)handleMouseDown:(NSEvent *)event owner:(mforms::View *)mOwner;
- (bool)handleMouseMove:(NSEvent *)event owner:(mforms::View *)mOwner;
- (bool)handleMouseEntered:(NSEvent *)event owner:(mforms::View *)mOwner;
- (bool)handleMouseExited:(NSEvent *)event owner:(mforms::View *)mOwner;
- (bool)handleBecomeFirstResponder:(mforms::View *)mOwner;
- (bool)handleResignFirstResponder:(mforms::View *)mOwner;
- (bool)handleKeyUp:(NSEvent *)event owner:(mforms::View *)mOwner;
- (bool)handleKeyDown:(NSEvent *)event owner:(mforms::View *)mOwner;
- (NSTrackingArea *)updateTrackingArea:(NSTrackingArea *)currentArea;

- (NSSize)preferredSize:(NSSize)proposal;
- (void)relayout;
- (mforms::ModifierKey)modifiersFromEvent:(NSEvent *)event;

@end

NSView *nsviewForView(mforms::View *view);
