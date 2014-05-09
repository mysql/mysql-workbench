/* 
 * Copyright (c) 2009, 2012, Oracle and/or its affiliates. All rights reserved.
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


#import "WBSplitView.h"

@interface NSObject(NSSplitViewDelegateExtras)
// This was introduced in 10.6
- (BOOL)splitView:(NSSplitView*)splitView shouldAdjustSizeOfSubview:(NSView*)subview;
@end


@implementation WBSplitView


- (void)setDividerThickness:(float)f
{
  mDividerThickness= f;
}


- (CGFloat) dividerThickness;
{
  return mDividerThickness;
	
  /*if ([self isVertical])
    return mDividerThickness;
  else
    return [super dividerThickness];*/
}


- (void)adjustSubviews
{
#ifndef NSAppKitVersionNumber10_5
#define NSAppKitVersionNumber10_5 949
#endif
  // 10.6 has support for splitView:shouldAdjustSizeOfSubview:, but 10.5 doesn't
  if ((int)NSAppKitVersionNumber > (int)NSAppKitVersionNumber10_5 || ![[self delegate] respondsToSelector: @selector(splitView:shouldAdjustSizeOfSubview:)])
    [super adjustSubviews];
  else
  {
    int subviewCount = 0;
    int adjustableCount = 0;
    float fixedSubviewsSize = 0.0;
    float spaceForAdjustableDistribution = 0.0;
    BOOL vertical = [self isVertical];

    for (NSView *subview in [self subviews])
    {
      subviewCount++;
      if ([[self delegate] splitView: self shouldAdjustSizeOfSubview: subview])
        adjustableCount++;
      else
        fixedSubviewsSize += vertical ? NSWidth([subview frame]) : NSHeight([subview frame]);
    }
    
    if (adjustableCount == 0)
    {
      NSLog(@"Inconsistency Error: all subviews of splitter %@ are not adjustable", self);
      [super adjustSubviews];
      return;
    }  

    NSRect rect = [self bounds];
    // will only work for 2 subviews, for more subviews the space should be divided proportionally
    if (vertical)
      spaceForAdjustableDistribution = (NSWidth(rect) - fixedSubviewsSize - (subviewCount-1)*[self dividerThickness]) / adjustableCount;
    else
      spaceForAdjustableDistribution = (NSHeight(rect) - fixedSubviewsSize - (subviewCount-1)*[self dividerThickness]) / adjustableCount;

    for (NSView *subview in [self subviews])
    {
      if ([[self delegate] splitView: self shouldAdjustSizeOfSubview: subview])
      {
        if (vertical)
          rect.size.width = spaceForAdjustableDistribution;
        else
          rect.size.height = spaceForAdjustableDistribution;
      }
      else
      {
        if (vertical)
          rect.size.width = NSWidth([subview frame]);
        else
          rect.size.height = NSHeight([subview frame]);
      }  
      [subview setFrame: rect];
      if (vertical)
        rect.origin.x += rect.size.width;
      else
        rect.origin.y += rect.size.height;
    }
  }
}


- (void) drawDividerInRect: (NSRect) aRect;
{
  NSColor* color;
  
  if (mEnabled) {
    color = [NSColor colorWithDeviceWhite: 0xaa/256.0 alpha: 1.0];
  }
  else {
    color = [NSColor colorWithCalibratedRed: 0.8
                                      green: 0.8
                                       blue: 0.8
                                      alpha: 1];
  }
  
  [color set];
  [NSBezierPath fillRect: aRect];
}

- (void)drawRect:(NSRect)rect
{
  if (mBackColor)
  {
    [mBackColor set];
    NSRectFill(rect);
  }
  else
    [super drawRect: rect];
}

- (void) handleDidBecomeMain: (id) aNotification;
{
  mEnabled = YES;
  [self setNeedsDisplay: YES];
}



- (void) handleDidResignMain: (id) aNotification;
{
  mEnabled = NO;
  [self setNeedsDisplay: YES];
}


- (void)setBackgroundColor: (NSColor*)color
{
  [mBackColor autorelease];
  mBackColor = [color retain];
}

#pragma mark Create and Destroy



- (void) awakeFromNib;
{
  mEnabled = YES;
  mDividerThickness = 1;
  
  // Set up notifications.
  NSNotificationCenter* dc = [NSNotificationCenter defaultCenter];
  [dc addObserver: self
         selector: @selector(handleDidBecomeMain:)
             name: NSWindowDidBecomeMainNotification
           object: [self window]];
  [dc addObserver: self
         selector: @selector(handleDidResignMain:)
             name: NSWindowDidResignMainNotification
           object: [self window]];
}



- (void) dealloc
{
  NSNotificationCenter* dc = [NSNotificationCenter defaultCenter];
  [dc removeObserver: self];
  
  [mBackColor release];
  [super dealloc];
}



@end


