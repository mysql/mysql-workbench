//
//  MContainerView.m
//  MySQLWorkbench
//
//  Created by Alfredo Kojima on 12/May/09.
//  Copyright 2009 Sun Microsystems Inc. All rights reserved.
//

#import "MContainerView.h"


@implementation MContainerView

- (void)setMinContentSize:(NSSize)size
{
  mMinSize= size;
}


- (void)setPadding:(NSSize)padding
{
  mPadding= padding;
}


- (BOOL)isFlipped
{
  return YES;
}


// resize subview keeping a minimum size
- (void)resizeSubviewsWithOldSize:(NSSize)oldBoundsSize
{
  id item= [[self subviews] lastObject];
  NSSize size= [self frame].size;
  NSRect rect;

  if (size.width < mMinSize.width + 2*mPadding.width)
    size.width= mMinSize.width + 2*mPadding.width;
  if (size.height < mMinSize.height + 2*mPadding.height)
    size.height= mMinSize.height + 2*mPadding.height;
  
  rect.origin.x= mPadding.width;
  rect.origin.y= mPadding.height;
  
  rect.size= size;
  rect.size.width-= mPadding.width*2;
  rect.size.height-= mPadding.height*2;
  
  [item setFrame: rect];
}

@end
