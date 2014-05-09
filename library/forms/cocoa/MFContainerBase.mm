/* 
 * Copyright (c) 2009, 2013, Oracle and/or its affiliates. All rights reserved.
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

#import "MFContainerBase.h"
#include "mforms/app.h"

@implementation MFContainerBase

- (BOOL)mouseDownCanMoveWindow
{
  return NO;
}

//--------------------------------------------------------------------------------------------------

- (id)initWithFrame:(NSRect)frameRect
{
  self = [super initWithFrame: frameRect];
  //  if (self != nil)
  //  mDefaultBackColor = [[NSColor colorWithDeviceWhite:232/255.0 alpha:1.0] retain];
  return self;
}

//--------------------------------------------------------------------------------------------------

- (void) dealloc
{
  [NSObject cancelPreviousPerformRequestsWithTarget: self];
  [mBackImage release];
  [mDefaultBackColor release];
  [mBackColor release];
  [super dealloc];
}


//--------------------------------------------------------------------------------------------------

STANDARD_MOUSE_HANDLING(self) // Add handling for mouse events.

//--------------------------------------------------------------------------------------------------

- (void)setPaddingLeft:(float)lpad right:(float)rpad top:(float)tpad bottom:(float)bpad
{
  mLeftPadding = lpad;
  mRightPadding = rpad;
  mTopPadding = tpad;
  mBottomPadding = bpad;
}

//--------------------------------------------------------------------------------------------------

- (void)setEnabled:(BOOL)flag
{
  for (id subview in [self subviews])
  {
    if ([subview respondsToSelector: @selector(setEnabled:)])
      [subview setEnabled: flag];
  }
}

//--------------------------------------------------------------------------------------------------

- (void)subviewMinimumSizeChanged
{
  NSSize minSize= [self preferredSize];
  NSSize size= [self frame].size;
  
  // size of some subview has changed, we check if our current size is enough
  // to fit it and if not, request forward the size change notification to superview
  
  if (mFreezeRelayout > 0)
    return;

  if (minSize.width != size.width || minSize.height != size.height)
  {
    if ([self superview])
    {
      [[self superview] subviewMinimumSizeChanged];
      return;
    }
    else
      [self setFrameSize: minSize];
  }
  [self resizeSubviewsWithOldSize:size];
}

//--------------------------------------------------------------------------------------------------

- (BOOL)setFreezeRelayout:(BOOL)flag
{
  if (flag)
    mFreezeRelayout++;
  else
  {
    mFreezeRelayout--;
    if (mFreezeRelayout == 0)
    {
      // exec when idle to avoid crashes caused by unsettled structure changes
      // (like when removing subviews from a container destructor)
      [self performSelector: @selector(subviewMinimumSizeChanged)
                 withObject: nil
                 afterDelay: 0
                    inModes: [NSArray arrayWithObjects: NSModalPanelRunLoopMode, NSDefaultRunLoopMode, nil]];
      [[self window] recalculateKeyViewLoop];
      return YES;
    }
  }
  return NO;
}

//--------------------------------------------------------------------------------------------------

- (void)setBackgroundColor: (NSColor*) color
{
  [mBackColor autorelease];
  mBackColor = [color retain];
  [self setNeedsDisplay: YES];
}

//--------------------------------------------------------------------------------------------------

- (NSColor*)backgroundColor
{
  return mBackColor;
}

//--------------------------------------------------------------------------------------------------

/**
 * Fill the background of the container with the background color.
 */
- (void)drawRect: (NSRect) rect
{
  if (mBackColor != nil)
  {
    [mBackColor set];
    NSRectFill(rect);
  }
  else if (mDefaultBackColor != nil)
  {
    id parent = [self superview];
    // check if we're inside a NSBox or NSTabView, which draw their own background
    while (parent)
    {
      // TODO: move MFTabViewImpl to mforms dylib and use normal class access instead objc_getClass.
      if ([parent isKindOfClass: [NSBox class]] || ([parent isKindOfClass: objc_getClass("MFTabViewImpl")] && 
                                                    [[parent tabView] tabViewType] != NSNoTabsNoBorder))
        break;
      parent = [parent superview];
    }
    if (!parent)
    {
      [mDefaultBackColor set];
      NSRectFill(rect);
    }
  }
  
  if (mBackImage)
  {
    float x, y;
    NSSize isize = [mBackImage size];
    NSSize fsize = [self frame].size;

    if (fsize.height < isize.height)
      isize.height = fsize.height;
    if (fsize.width < isize.width)
      isize.width = fsize.width;

    switch (mBackImageAlignment)
    {
      case mforms::BottomLeft:
      case mforms::MiddleLeft:
      case mforms::TopLeft:
        x = 0;
        break;
      case mforms::BottomCenter:
      case mforms::MiddleCenter:
      case mforms::TopCenter:
        x = (fsize.width - isize.width) / 2;
        break;
      case mforms::BottomRight:
      case mforms::MiddleRight:
      case mforms::TopRight:
        x = fsize.width - isize.width;
        break;
      default:
        x = 0;
        break;
    }

    switch (mBackImageAlignment)
    {
      case mforms::BottomLeft:
      case mforms::BottomCenter:
      case mforms::BottomRight:
        if (![self isFlipped])
          y = 0;
        else
          y = fsize.height - isize.height;
        break;
      case mforms::MiddleLeft:
      case mforms::MiddleCenter:
      case mforms::MiddleRight:
          y = (fsize.height - isize.height) / 2;
        break;
      case mforms::TopLeft:
      case mforms::TopCenter:
      case mforms::TopRight:
        if ([self isFlipped])
          y = 0;
        else
          y = fsize.height - isize.height;
        break;
      default:
        y = 0;
        break;
    }

    [mBackImage drawInRect: NSMakeRect(x, y, isize.width, isize.height)
                  fromRect: NSZeroRect
                 operation: NSCompositeSourceOver
                  fraction: 1.0
            respectFlipped: YES
                     hints: nil];
  }
}

//--------------------------------------------------------------------------------------------------

- (void)setBackgroundImage: (NSString*) path withAlignment: (mforms::Alignment) align
{
  std::string full_path= mforms::App::get()->get_resource_path([path UTF8String]);
  if (!full_path.empty())
  {
    mBackImage = [[NSImage alloc] initWithContentsOfFile:wrap_nsstring(full_path)];
    mBackImageAlignment = align;
  }
  else
  {
    [mBackImage release];
    mBackImage = nil;
  }
}

//--------------------------------------------------------------------------------------------------

@end
