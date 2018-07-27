/*
 * Copyright (c) 2004, 2018, Oracle and/or its affiliates. All rights reserved.
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

#import "MTextImageCell.h"

#define IMAGE_OFFSET 2
#define IMAGE_TEXT_OFFSET 3

@implementation MTextImageCell


- (id)copyWithZone:(NSZone*)zone 
{
  MTextImageCell *copy = (MTextImageCell*)[super copyWithZone:zone];
  copy->_image = _image;
  copy.font = self.font;
  return copy;
}

- (void)setImage:(NSImage*)image 
{
  if (image != _image) 
  {
    _image = image;
  }
}

- (NSImage*)image 
{
  return _image;
}


- (void)editWithFrame:(NSRect)aRect inView:(NSView *)controlView editor:(NSText *)textObj delegate:(id)anObject event:(NSEvent *)theEvent 
{
  NSRect textFrame, imageFrame;
  NSDivideRect(aRect, &imageFrame, &textFrame, IMAGE_OFFSET + IMAGE_TEXT_OFFSET + _image.size.width, NSMinXEdge);
  [super editWithFrame: textFrame inView: controlView editor:textObj delegate:anObject event: theEvent];
}

- (void)selectWithFrame:(NSRect)aRect inView:(NSView *)controlView editor:(NSText *)textObj delegate:(id)anObject start:(NSInteger)selStart length:(NSInteger)selLength 
{
  NSRect textFrame, imageFrame;
  NSDivideRect(aRect, &imageFrame, &textFrame, IMAGE_OFFSET + IMAGE_TEXT_OFFSET + _image.size.width, NSMinXEdge);
  [super selectWithFrame: textFrame inView: controlView editor:textObj delegate:anObject start:selStart length:selLength];
}

- (void)drawWithFrame:(NSRect)cellFrame inView:(NSView *)controlView 
{
  if (_image != nil) 
  {
    NSSize imageSize;
    NSRect imageFrame;
    
    imageSize = _image.size;
    NSDivideRect(cellFrame, &imageFrame, &cellFrame, IMAGE_OFFSET + IMAGE_TEXT_OFFSET + imageSize.width, NSMinXEdge);
    if (self.drawsBackground) 
    {
      [self.backgroundColor set];
      NSRectFill(imageFrame);
    }
    imageFrame.origin.x += IMAGE_OFFSET;
    imageFrame.origin.y += ceil((cellFrame.size.height - imageSize.height) / 2);
    imageFrame.size = imageSize;
    [_image drawInRect: imageFrame
               fromRect: NSZeroRect
              operation: NSCompositingOperationSourceOver
               fraction: 1
        respectFlipped: YES
                 hints: nil];
  }
  [super drawWithFrame:cellFrame inView:controlView];
}

- (NSSize)cellSize 
{
  NSSize cellSize= super.cellSize;
  cellSize.width += (_image ? _image.size.width : 0) + IMAGE_OFFSET + IMAGE_TEXT_OFFSET;
  return cellSize;
}

@end
