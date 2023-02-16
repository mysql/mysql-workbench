/*
 * Copyright (c) 2005, 2018, Oracle and/or its affiliates. All rights reserved.
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

#import "MQIndicatorCell.h"

//----------------------------------------------------------------------------------------------------------------------

@implementation MQIndicatorCell

- (instancetype)initWithCoder: (NSCoder*)coder {
  self = [super initWithCoder: coder];
  if (self != nil) {
    _attribs = [NSMutableDictionary dictionaryWithObjectsAndKeys:[NSFont systemFontOfSize:11], NSFontAttributeName,
                [NSColor blackColor], NSForegroundColorAttributeName,
                nil];
    
    _arrow = @"\xe2\x96\xb6";
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (id)copyWithZone: (NSZone*)zone {
  MQIndicatorCell *copy = (MQIndicatorCell*)[super copyWithZone: zone];
  copy->_attribs = _attribs;
  copy->_arrow= _arrow;
  return copy;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)editWithFrame: (NSRect)aRect
               inView: (NSView *)controlView
               editor: (NSText *)textObj
             delegate: (id)anObject
                event:( NSEvent *)theEvent {
}

//----------------------------------------------------------------------------------------------------------------------

- (void)selectWithFrame: (NSRect)aRect
                 inView: (NSView *)controlView
                 editor: (NSText *)textObj
               delegate: (id)anObject
                  start: (NSInteger)selStart
                 length: (NSInteger)selLength {
}

//----------------------------------------------------------------------------------------------------------------------

- (void)drawWithFrame: (NSRect)cellFrame inView: (NSView *)controlView {
  [NSColor.textBackgroundColor set];
  --cellFrame.origin.y;
  ++cellFrame.size.height;
  cellFrame.size.width += 2;
  NSRectFill(cellFrame);
  
  if (_selected) {
    if (controlView.window.firstResponder == controlView)
      _attribs[NSForegroundColorAttributeName] = NSColor.textColor;
    else
      _attribs[NSForegroundColorAttributeName] = [NSColor lightGrayColor];

    NSPoint position = NSMakePoint(NSMinX(cellFrame) + (NSWidth(cellFrame) - 12) / 2, NSMinY(cellFrame) + (NSHeight(cellFrame) - 12) / 2);
    if (_placeholder)
      [@"*" drawAtPoint: position withAttributes: _attribs];
    else
      [_arrow drawAtPoint: position withAttributes: _attribs];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setSelected: (BOOL)flag {
  _selected= flag;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setPlaceholder: (BOOL)flag {
  _placeholder= flag;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setIsNull:(BOOL)flag {
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setIsBlob:(BOOL)flag {
}

@end

//----------------------------------------------------------------------------------------------------------------------
