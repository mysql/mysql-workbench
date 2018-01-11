/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mdc_canvas_view_macosx.h"

@protocol CanvasViewerDelegate

- (BOOL)canvasMouseDown:(mdc::MouseButton)button location:(NSPoint)pos state:(mdc::EventState)state;
- (BOOL)canvasMouseDoubleClick:(mdc::MouseButton)button location:(NSPoint)pos state:(mdc::EventState)state;
- (BOOL)canvasMouseUp:(mdc::MouseButton)button location:(NSPoint)pos state:(mdc::EventState)state;
- (BOOL)canvasMouseMoved:(NSPoint)pos state:(mdc::EventState)state;

- (BOOL)canvasKeyDown:(mdc::KeyInfo)key state:(mdc::EventState)state;
- (BOOL)canvasKeyUp:(mdc::KeyInfo)key state:(mdc::EventState)state;

- (NSDragOperation)canvasDraggingEntered:(id<NSDraggingInfo>)sender;
- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender;
- (BOOL)canvasPerformDragOperation:(id<NSDraggingInfo>)sender;

@end

@interface MCanvasViewer : NSView {
  mdc::QuartzCanvasView *_view;
  NSCursor *_cursor;
  int _buttonState;
  NSTrackingArea *_trackingArea;

  BOOL _firstResponder;
}

@property(readonly) mdc::CanvasView *canvas;
@property(weak) id<CanvasViewerDelegate> delegate;
@property(readonly) NSRect documentRect;
@property(readonly) NSRect documentVisibleRect;

- (void)setupQuartz;
- (void)setCursor:(NSCursor *)cursor;
- (void)scrollToPoint:(NSPoint)offset;

@end
