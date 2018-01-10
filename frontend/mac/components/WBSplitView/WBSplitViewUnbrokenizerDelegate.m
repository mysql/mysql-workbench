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

#import "WBSplitViewUnbrokenizerDelegate.h"

@implementation WBSplitViewUnbrokenizerDelegate

- (instancetype)init
{
  self= [super init];
  if (self)
  {
    mTopCollapseLimit= 60;
    mBottomCollapseLimit= 60;
  }
  return self;
}

- (void)preventCollapseOfView:(NSView*)view
{
  if (!mUncollapsableViews)
    mUncollapsableViews = [[NSMutableSet alloc] initWithCapacity: 1];
  [mUncollapsableViews addObject: view];
}


- (CGFloat) splitView: (NSSplitView*) sender
constrainMinCoordinate: (CGFloat) proposedMin
          ofSubviewAt: (NSInteger) offset;
{
  CGFloat constrained = proposedMin;
  
  if (offset == 0) {
    constrained = mTopCollapsedMinHeight;
  }

  return constrained;
}


- (CGFloat) splitView: (NSSplitView*) sender
constrainMaxCoordinate: (CGFloat) proposedMax
          ofSubviewAt: (NSInteger) offset;
{
  CGFloat constrained = proposedMax;
  
  if (offset == 0) {
    constrained = sender.frame.size.height - mBottomCollapsedMinHeight;
  }
  
  return constrained;
}


- (BOOL)splitView:(NSSplitView *)splitView canCollapseSubview:(NSView *)subview
{
  return ![mUncollapsableViews containsObject: subview];
}


- (NSView*)topViewOfSplitView: (NSSplitView*)view 
{
  return view.subviews[0];
}


- (NSView*)bottomViewOfSplitView: (NSSplitView*)view 
{
  return view.subviews[1];
}


- (void) expandTopOfSplitView: (NSSplitView*)view 
                     toHeight: (CGFloat) height;
{
  [view setPosition: height
   ofDividerAtIndex: 0];
  [view adjustSubviews];
  
  mTopCollapsed = NO;
  
  NSView *topView= [self topViewOfSplitView: view];

  NSRect r = topView.frame;
  r.origin.y = 9;
  r.size.height -= 36;
  topView.frame = r;
  topView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
}



- (void) expandBottomOfSplitView: (NSSplitView*)view 
                          height: (CGFloat) height;
{
  [view setPosition: height
   ofDividerAtIndex: 0];
  [view adjustSubviews];
  
  mBottomCollapsed = NO;
  
  NSView *bottomView= [self bottomViewOfSplitView: view];

  NSRect r = bottomView.frame;
  r.origin.y = 0;
  r.size.height = bottomView.bounds.size.height - 25;
  bottomView.frame = r;
  bottomView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
  [bottomView setHidden: NO];
}



- (void)collapseBottomOfSplitView: (NSSplitView*)split
{
  [split setPosition: NSHeight(split.frame) - mBottomCollapsedMinHeight 
    ofDividerAtIndex: 0];  
}
  
  
- (void)collapseTopOfSplitView: (NSSplitView*)split
{
  [split setPosition: 0 ofDividerAtIndex: 0];
}


- (CGFloat) splitView: (NSSplitView*) sender
constrainSplitPosition: (CGFloat) proposedPosition
          ofSubviewAt: (NSInteger) offset;
{
  CGFloat allowedPos = proposedPosition;
  CGFloat maxDividerCoordinate = sender.frame.size.height - mBottomExpandedMinHeight;
  CGFloat resultsAreaPivot = sender.frame.size.height - mBottomCollapseLimit;

  if (proposedPosition < mTopExpandedMinHeight) {
    if (proposedPosition > mTopCollapseLimit) {
      allowedPos = mTopExpandedMinHeight;
      if (mTopCollapsed) {
        [self expandTopOfSplitView: sender
                            toHeight: allowedPos];
      }
    }
    else {
      // User dragged divider up above the pivot point, so we close the query area.
      allowedPos = 0;
      if (!mTopCollapsed) {
        [self topViewOfSplitView: sender].autoresizingMask = NSViewWidthSizable | NSViewMinYMargin;
        mTopCollapsed = YES;
      }
    }
  }
  
  else if (proposedPosition > maxDividerCoordinate) {
    // Divider dragged below the limit for results area.
    if (proposedPosition < resultsAreaPivot) {
      // Divider is above the pivot point.
      allowedPos = maxDividerCoordinate;
      if (mBottomCollapsed) {
        [self expandBottomOfSplitView: sender
                               height: allowedPos];
      }
    }
    else {
      // User dragged divider below the pivot point, so we close the results area.
      allowedPos = sender.frame.size.height;
      if (!mBottomCollapsed) {
        NSView *view= [self bottomViewOfSplitView: sender];
        view.autoresizingMask = NSViewWidthSizable | NSViewMinYMargin;
        mBottomCollapsed = YES;          
      }
    }
  }
  
  return allowedPos;
}


- (NSRect)splitView:(NSSplitView *)splitView
      effectiveRect:(NSRect)proposedEffectiveRect 
       forDrawnRect:(NSRect)drawnRect
   ofDividerAtIndex:(NSInteger)dividerIndex
{
  // if the divider is too thin, increase effective rect by 2px to make it less impossible to drag
  if (splitView.vertical)
  {
    if (proposedEffectiveRect.size.width < 2)
    {
      proposedEffectiveRect.origin.x -= 1;
      proposedEffectiveRect.size.width += 2;
    }
  }
  else
  {
    if (proposedEffectiveRect.size.height < 2)
    {
      proposedEffectiveRect.origin.y -= 1;
      proposedEffectiveRect.size.height += 2;
    }
  }
  return proposedEffectiveRect;
}


- (void)setTopExpandedMinHeight: (CGFloat)h
{
  mTopExpandedMinHeight= h;
}


- (void)setTopCollapsedMinHeight: (CGFloat)h
{
  mTopCollapsedMinHeight= h;
}


- (void)setBottomExpandedMinHeight: (CGFloat)h
{
  mBottomExpandedMinHeight= h;
}


- (void)setBottomCollapsedMinHeight: (CGFloat)h
{
  mBottomCollapsedMinHeight= h;
}


- (void)setTopCollapseLimit: (CGFloat)h
{
  mTopCollapseLimit= h;
}

- (void)setBottomCollapseLimit: (CGFloat)h
{
  mBottomCollapseLimit= h;
}


- (BOOL)splitView:(NSSplitView *)splitView shouldAdjustSizeOfSubview:(NSView *)subview
{
  if ([delegate respondsToSelector: @selector(splitView:shouldAdjustSizeOfSubview:)])
    return [delegate splitView: splitView shouldAdjustSizeOfSubview: subview];
  return YES;
}


- (void)splitViewDidResizeSubviews:(NSNotification *)notification
{
  if ([delegate respondsToSelector: @selector(splitViewDidResizeSubviews:)])
    [delegate splitViewDidResizeSubviews: notification];
}

@end
