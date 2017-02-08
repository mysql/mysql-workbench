/* 
 * Copyright (c) 2005, 2016, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */


#import "MQIndicatorCell.h"

@implementation MQIndicatorCell

- (instancetype)initWithCoder:(NSCoder*)coder
{
  self = [super initWithCoder:coder];
  if (self != nil)
  {
    _attribs = [NSMutableDictionary dictionaryWithObjectsAndKeys:[NSFont systemFontOfSize:11], NSFontAttributeName,
                [NSColor blackColor], NSForegroundColorAttributeName,
                nil];
    
    _arrow = @"\xe2\x96\xb6";
  }
  return self;
}




- (id)copyWithZone:(NSZone*)zone 
{
  MQIndicatorCell *copy = (MQIndicatorCell*)[super copyWithZone: zone];
  copy->_attribs = _attribs;
  copy->_arrow= _arrow;
  return copy;
}


- (void)editWithFrame:(NSRect)aRect inView:(NSView *)controlView editor:(NSText *)textObj delegate:(id)anObject event:(NSEvent *)theEvent
{
}

- (void)selectWithFrame:(NSRect)aRect inView:(NSView *)controlView editor:(NSText *)textObj delegate:(id)anObject start:(NSInteger)selStart length:(NSInteger)selLength
{
}

- (void)drawWithFrame:(NSRect)cellFrame inView:(NSView *)controlView 
{
  [[NSColor whiteColor] set];
  cellFrame.origin.y -= 1;
  cellFrame.size.height += 1;
  cellFrame.size.width += 2;
  NSRectFill(cellFrame);
  
  if (_selected)
  {
    if (controlView.window.firstResponder == controlView)
      _attribs[NSForegroundColorAttributeName] = [NSColor blackColor];
    else
      _attribs[NSForegroundColorAttributeName] = [NSColor lightGrayColor];
    
    if (_placeholder)
      [@"*" drawAtPoint:NSMakePoint(cellFrame.origin.x+cellFrame.size.width-14,cellFrame.origin.y+2)
           withAttributes:_attribs];
    else
      [_arrow drawAtPoint:NSMakePoint(cellFrame.origin.x+cellFrame.size.width-14,cellFrame.origin.y+2)
           withAttributes:_attribs];
  }
}

- (void)setSelected:(BOOL)flag
{
  _selected= flag;
}

- (void)setPlaceholder:(BOOL)flag
{
  _placeholder= flag;
}

@end
