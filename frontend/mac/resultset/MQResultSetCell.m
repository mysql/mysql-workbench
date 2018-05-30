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

#import "MQResultSetCell.h"

@implementation MQResultSetCell

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)init {
  self= [super init];
  if (self)   {
    _blobIcon = [NSImage imageNamed: @"field_overlay_blob"];
    _nullIcon = [NSImage imageNamed: @"field_overlay_null"];
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (id)copyWithZone:(NSZone*)zone {
  MQResultSetCell *copy = (MQResultSetCell*)[super copyWithZone:zone];
  copy->_blobIcon = _blobIcon;
  copy->_nullIcon = _nullIcon;
  return copy;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)editWithFrame: (NSRect)aRect
               inView: (NSView *)controlView
               editor: (NSText *)textObj
             delegate: (id)anObject
                event: (NSEvent *)theEvent {
  if (!_blob) {
    self.textColor = [NSColor blackColor];
    aRect.size.height-= 3.0;
    aRect.size.width-= 3.0;
    [super editWithFrame:aRect inView:controlView editor:textObj delegate:anObject event:theEvent];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setPlaceholder:(BOOL)flag {
  _placeholder= flag;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setIsBlob:(BOOL)flag {
  _blob= flag;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setIsNull:(BOOL)flag {
  _null= flag;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)drawWithFrame:(NSRect)cellFrame inView:(NSView *)controlView {
  NSRect imageFrame = cellFrame;
  imageFrame.origin.x += 4;

  [super drawWithFrame: cellFrame inView: controlView];

  if (!_placeholder) {
    if (_blob && !_null) {
      if (self.objectValue) {
        imageFrame.size = _blobIcon.size;
        imageFrame.origin.y += floor((NSHeight(cellFrame) - _blobIcon.size.height) / 2);
        [_blobIcon drawInRect: imageFrame
                     fromRect: NSZeroRect
                    operation: NSCompositingOperationSourceOver
                     fraction: 1
               respectFlipped: YES
                        hints: nil];
      }
    }

    if (_null) {
      imageFrame.size = _nullIcon.size;
      imageFrame.origin.y += floor((NSHeight(cellFrame) - _nullIcon.size.height) / 2);
      [_nullIcon drawInRect: imageFrame
                fromRect: NSZeroRect
               operation: NSCompositingOperationSourceOver
                fraction: 1
          respectFlipped: YES
                   hints: nil];
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

@end
