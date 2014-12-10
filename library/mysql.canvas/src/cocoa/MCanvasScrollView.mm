/* 
 * Copyright (c) 2005, 2014, Oracle and/or its affiliates. All rights reserved.
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

#import "MCanvasScrollView.h"
#import "MCanvasViewer.h"
#include "mdc.h"


@implementation MCanvasScrollView

- (id)initWithFrame:(NSRect)frame
{
  self= [super initWithFrame:frame];
  if (self)
  {
    CGFloat scrollerWidth = [NSScroller scrollerWidthForControlSize: NSRegularControlSize
                                                      scrollerStyle: NSScrollerStyleOverlay];

    _hScroller= [[NSScroller alloc] initWithFrame:NSMakeRect(0, 0, NSWidth(frame) - scrollerWidth, scrollerWidth)];
    _vScroller= [[NSScroller alloc] initWithFrame:NSMakeRect(NSMaxX(frame) - scrollerWidth, scrollerWidth,
                                                             scrollerWidth, NSHeight(frame))];
    [self addSubview:_hScroller];
    [self addSubview:_vScroller];
    
    [_hScroller setAction:@selector(scrolled:)];
    [_hScroller setTarget:self];
    [_vScroller setAction:@selector(scrolled:)];
    [_vScroller setTarget:self];
    
    [_hScroller setEnabled:YES];
    [_vScroller setEnabled:YES];
  }
  return self;
}


- (void)dealloc
{
  [_hScroller release];
  [_vScroller release];
  [super dealloc];
}


- (NSView*)documentView
{
  return _contentView;
}


- (void)tile
{
  NSRect frame= [self bounds];
  NSRect contentFrame;

  CGFloat scrollerWidth = [NSScroller scrollerWidthForControlSize: NSRegularControlSize
                                                    scrollerStyle: NSScrollerStyleOverlay];
  contentFrame= NSMakeRect(0, scrollerWidth, NSWidth(frame) - scrollerWidth, NSHeight(frame) - scrollerWidth);
  [_contentView setFrame:contentFrame];
  if (_hAccessoryView)
  {
    NSRect accRect= [_hAccessoryView frame];
    [_hAccessoryView setFrame:NSMakeRect(0, 0, NSWidth(accRect), scrollerWidth)];
    [_hScroller setFrame:NSMakeRect(NSWidth(accRect), 0, NSWidth(frame) - NSWidth(accRect) - scrollerWidth, scrollerWidth)];
  }
  else
    [_hScroller setFrame:NSMakeRect(0, 1, NSWidth(frame) - scrollerWidth, scrollerWidth)];

  [_vScroller setFrame:NSMakeRect(NSWidth(frame) - scrollerWidth, scrollerWidth,
                                  scrollerWidth, NSHeight(frame) - scrollerWidth)];
  
  [self reflectContentRect];
}


- (void)scrolled:(id)sender
{
  float line;
  float page;
  NSRect visible= [_contentView documentVisibleRect];
  NSRect total= [_contentView documentRect];
 
  if (sender == _hScroller)
  {
    line = (NSWidth(visible)/20) / NSWidth(total);
    page = NSWidth(visible) / NSWidth(total);
  }
  else
  {
    line = (NSHeight(visible)/20) / NSHeight(total);
    page = NSHeight(visible) / NSHeight(total);    
  }
  switch ([sender hitPart])
  {
    case NSScrollerDecrementPage:
      [sender setFloatValue:MAX([sender floatValue] - page, 0.0)];
      break;
    case NSScrollerIncrementPage:
      [sender setFloatValue:MIN([sender floatValue] + page, 1.0)];
      break;      
    case NSScrollerDecrementLine:
      [sender setFloatValue:MAX([sender floatValue] - line, 0.0)];
      break;
    case NSScrollerIncrementLine:
      [sender setFloatValue:MIN([sender floatValue] + line, 1.0)];
      break;

    default: // Silent compiler.
      break;
  }

  [_contentView scrollToPoint:NSMakePoint([_hScroller floatValue] * (NSWidth(total) - NSWidth(visible)), 
                                          [_vScroller floatValue] * (NSHeight(total) - NSHeight(visible)))];
}


- (void)reflectContentRect
{
  NSRect total= [_contentView documentRect];
  NSRect visible= [_contentView documentVisibleRect];
   
  if (NSWidth(visible) >= NSWidth(total))
  {
    [_hScroller setFloatValue:0.0];
    [_hScroller setKnobProportion:1.0];
  }
  else
  {
    [_hScroller setFloatValue:visible.origin.x/(NSWidth(total)-NSWidth(visible))];
    [_hScroller setKnobProportion:NSWidth(visible)/NSWidth(total)];
  }
  
  if (NSHeight(visible) >= NSHeight(total))
  {
    [_vScroller setFloatValue:0.0];
    [_vScroller setKnobProportion:1.0];
  }
  else
  {
    [_vScroller setFloatValue:visible.origin.y/(NSHeight(total)-NSHeight(visible))];
    [_vScroller setKnobProportion:NSHeight(visible)/NSHeight(total)];
  }
}


static void canvas_view_viewport_changed(MCanvasScrollView *self)
{
  [self reflectContentRect];
}


- (void)setContentCanvas:(MCanvasViewer*)canvas
{
  _contentView= canvas;

  [canvas canvas]->signal_viewport_changed()->connect(boost::bind(canvas_view_viewport_changed, self));
  
  [self addSubview:canvas];
  [self tile];
}


- (NSSize)contentSize
{
  NSSize size= [self frame].size;
  
  CGFloat scrollerWidth = [NSScroller scrollerWidthForControlSize: NSRegularControlSize
                                                    scrollerStyle: NSScrollerStyleOverlay];
  size.width -= scrollerWidth;
  size.height -= scrollerWidth;
  
  return size;
}

- (void)resizeSubviewsWithOldSize:(NSSize)oldBoundsSize
{
  [self tile];
}


- (void)setHAccessory:(NSView*)view
{
  [self addSubview:view];
  _hAccessoryView= view;
  [self tile];
}


- (NSScroller*)verticalScroller
{
  return _vScroller;
}

- (NSScroller*)horizontalScroller
{
  return _hScroller;
}


- (void)scrollWheel:(NSEvent *)theEvent
{
  // setting to /20 makes horizontal scroller (which is like a button) in my mouse 
  // scroll nicely, but it doesn't work well with trackpads
  //[_hScroller setFloatValue:[_hScroller floatValue] - [theEvent deltaX]/20];
  [_hScroller setFloatValue:[_hScroller floatValue] - [theEvent deltaX]/200];
  [self scrolled:_hScroller];
  [_vScroller setFloatValue:[_vScroller floatValue] - [theEvent deltaY]/200];
  [self scrolled:_vScroller];
}


- (void)drawRect:(NSRect)frame
{
  [[NSColor whiteColor] set];
  NSRectFill(frame);
}

@end
