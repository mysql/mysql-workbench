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

#import "MFMForms.h"

#import "MFBox.h"
#import "MFLabel.h"

#import "NSColor_extras.h"

@implementation MFBoxImpl

- (id)initWithObject:(::mforms::Box*)aBox
{
  self= [super initWithFrame:NSMakeRect(10,10,10,10)];
  if (self)
  {
    mOwner= aBox;
    mOwner->set_data(self);
  }
  return self;
}


- (mforms::Object*)mformsObject
{
  return mOwner;
}


- (BOOL)isFlipped
{
  return YES;
}


- (void)setHorizontal:(BOOL)flag
{
  mHorizontal= flag;
}


- (void)setHomogeneous:(BOOL)flag
{
  mHomogeneous= flag;
}


- (void)setSpacing:(int)spacing
{
  mSpacing= spacing;
}


- (void)resizeSubviewsWithOldSizeH:(NSSize)oldBoundsSize
{
  NSSize minSize= [self minimumSize];
  NSSize size= [self frame].size;
  NSRect subframe;
  float leftover;
  int expandingSubviews= 0;
  int visibleSubviews= 0;
  BOOL packingEnd= NO;

  for (NSView *subview in [self subviews])
  {
    if (![subview isHidden])
    {
      visibleSubviews++;
      if (subview.viewFlags & ExpandFlag)
        expandingSubviews++;
    }
  }
    
  subframe.origin.x= mLeftPadding;
  subframe.origin.y= mTopPadding;
  subframe.size.height= size.height - (mTopPadding + mBottomPadding);

  if (!mHomogeneous)
  {
    // calc space that's left to be distributed among all subviews
    leftover= (size.width - minSize.width);    
  }
  else
    leftover= 0;

  // compensate for the unneeded spacing added for the 1st item
  subframe.origin.x-= mSpacing;
  for (NSView  *subview in [self subviews])
  {
    if ([subview isHidden]) continue;
    
    NSSize minSubsize;
    BOOL expand = subview.viewFlags & ExpandFlag;
    BOOL fill = subview.viewFlags & FillFlag;
    float extraSpace= 0;
    
    minSubsize= [subview preferredSize];
   
    if (!packingEnd && (subview.viewFlags & PackEndFlag))
    {
      packingEnd= YES;
      if (expandingSubviews == 0)
        subframe.origin.x+= leftover;
    }
    
    subframe.origin.x+= mSpacing;
    
    if (mHomogeneous)
    {
      float spacePerCell= (size.width - (mLeftPadding + mRightPadding + (visibleSubviews-1) * mSpacing)) / visibleSubviews;
      
      if (fill)
        subframe.size.width= spacePerCell;
      else
      {
        subframe.size.width= minSubsize.width;
        subframe.origin.x+= (spacePerCell - minSubsize.width) / 2;
        extraSpace= (spacePerCell - minSubsize.width) / 2;
      }
    }
    else
    {
      subframe.size.width= minSubsize.width;
      if (expand)
      {
        extraSpace= leftover / expandingSubviews;
        if (fill)
          subframe.size.width+= extraSpace; // use all the space given to us
        else
          subframe.origin.x+= extraSpace / 2; // just center in the space given to us
      }
    }
    subframe.origin.x= round(subframe.origin.x);
    subframe.size.width= round(subframe.size.width);

    if (NSEqualRects([subview frame], subframe))
      [subview resizeSubviewsWithOldSize: subframe.size];
    else      
    {
      NSRect f= subframe;
      
      if ([subview respondsToSelector:@selector(heightIsFixed)] &&
          [subview heightIsFixed])
      {
        f.size.height= minSubsize.height;
        f.origin.y= f.origin.y + floor((NSHeight(subframe) - NSHeight(f))/2);
      }
      [subview setFrame: f];
    }
    
    subframe.origin.x+= NSWidth(subframe);
    if (expand && !fill)
      subframe.origin.x+= extraSpace;
  }
}



- (void)resizeSubviewsWithOldSizeV:(NSSize)oldBoundsSize
{
  NSSize minSize= [self preferredSize];
  NSSize size= [self frame].size;
  NSRect subframe;
  float leftover;
  int expandingSubviews= 0;
  int visibleSubviews= 0;
  BOOL packingEnd= NO;
  
  for (NSView *subview in [self subviews])
  {
    if (![subview isHidden])
    {
      visibleSubviews++;
      if (subview.viewFlags & ExpandFlag)
        expandingSubviews++;
    }
  }
  
  subframe.origin.x= mLeftPadding;
  subframe.origin.y= mTopPadding;
  subframe.size.width= size.width - (mLeftPadding + mRightPadding);
  
  if (!mHomogeneous)
  {
    // calc space that's left to be distributed among all subviews
    // FIXME: this cannot work as both sizes are derived from the current container size
    //        not the actual content size (which might be much smaller, due to min size
    //        restrictions for the container).
    leftover= (size.height - minSize.height);
  }
  else
    leftover= 0;
  
  // compensate for the unneeded spacing added for the 1st item
  subframe.origin.y-= mSpacing;
  for (NSView *subview in [self subviews])
  {
    if ([subview isHidden]) continue;
    
    NSSize minSubsize;
    BOOL expand = subview.viewFlags & ExpandFlag;
    BOOL fill = subview.viewFlags & FillFlag;
    float extraSpace= 0;

    if ([subview isKindOfClass: [MFLabelImpl class]])
      minSubsize= [subview preferredSizeForWidth: size.width];
    else
      minSubsize= [subview preferredSize];
    
    if (!packingEnd && (subview.viewFlags & PackEndFlag))
    {
      packingEnd= YES;
      if (expandingSubviews == 0)
        subframe.origin.y+= leftover;
    }
    
    subframe.origin.y+= mSpacing;
    
    if (mHomogeneous)
    {
      float spacePerCell= (size.height - (mTopPadding + mBottomPadding + (visibleSubviews-1) * mSpacing)) / visibleSubviews;
      
      if (fill)
        subframe.size.height= spacePerCell;
      else
      {
        subframe.size.height= minSubsize.height;
        subframe.origin.y+= (spacePerCell - minSubsize.height) / 2;
        extraSpace= (spacePerCell - minSubsize.height) / 2;
      }
    }
    else
    {
      subframe.size.height= minSubsize.height;
      if (expand)
      {
        extraSpace= leftover / expandingSubviews;
        if (fill)
          subframe.size.height+= extraSpace; // use all the space given to us
        else
          subframe.origin.y+= extraSpace / 2; // just center in the space given to us
      }
    }

    subframe.origin.y= round(subframe.origin.y);
    subframe.size.height= round(subframe.size.height);
    if (NSEqualRects([subview frame], subframe))
      [subview resizeSubviewsWithOldSize: subframe.size];
    else
      [subview setFrame:subframe];
    
    subframe.origin.y+= NSHeight(subframe);
    if ((mHomogeneous || expand) && !fill)
      subframe.origin.y+= extraSpace;
  }
}



- (void)resizeSubviewsWithOldSize:(NSSize)oldBoundsSize
{
  if (!mOwner->is_destroying())
  {
    if (mHorizontal)
      [self resizeSubviewsWithOldSizeH:oldBoundsSize];
    else
      [self resizeSubviewsWithOldSizeV:oldBoundsSize];
  }
}


- (void)subviewMinimumSizeChanged
{
  if (mOwner != NULL && !mOwner->is_destroying())
    [super subviewMinimumSizeChanged];
}


- (NSSize)minimumSize
{
  if (mOwner == NULL || mOwner->is_destroying())
    return NSZeroSize;
  
  NSSize size;
  float maxSize= 0;
  float frameWidth= NSWidth([self frame]);
  int visibleSubviews= 0;
  
  size.width= 0;
  size.height= 0;
  
  for (NSView *subview in [self subviews])
  {
    NSSize minSize;
    if (!subview.isHidden && ((subview.viewFlags & RemovingFlag) == 0))
    {
      visibleSubviews++;
      
      if ([subview isKindOfClass: [MFLabelImpl class]])
        minSize= [subview preferredSizeForWidth: frameWidth];
      else        
        minSize= [subview preferredSize];
      
      if (mHorizontal)
      {
        size.width+= minSize.width;
        maxSize= MAX(maxSize, minSize.width);
        size.height= MAX(size.height, minSize.height);
      }
      else
      {
        size.width= MAX(size.width, minSize.width);
        maxSize= MAX(maxSize, minSize.height);
        size.height+= minSize.height;
      }
    }
  }
  
  if (mHomogeneous)
  {
    if (mHorizontal)
      size.width= maxSize * visibleSubviews;
    else
      size.height= maxSize * visibleSubviews;
  }
  
  size.width+= mLeftPadding + mRightPadding;
  size.height+= mTopPadding + mBottomPadding;
  if (mHorizontal)
    size.width+= (visibleSubviews-1) * mSpacing;
  else
    size.height+= (visibleSubviews-1) * mSpacing;
  
  return size;
}


- (void)didAddSubview:(NSView *)subview
{
  [self subviewMinimumSizeChanged];  
}


- (void)willRemoveSubview: (NSView *)subview
{
  // mark the subview so that it's not accounted when relayouting
  subview.viewFlags = (subview.viewFlags & ~BoxFlagMask) | RemovingFlag;
  if (!mOwner->is_destroying())
    [self subviewMinimumSizeChanged];
}

- (void)setFrame: (NSRect)frame
{
  [super setFrame: frame];
  if (!mOwner->is_destroying())
      (*mOwner->signal_resized())();
}

- (void)destroy
{
  [self removeFromSuperview];
}

#if 0
- (void)drawRect:(NSRect)rect
{
  [[NSColor redColor] set];
  NSFrameRect([self frame]);

  [[NSColor orangeColor] set];
  NSFrameRect(NSInsetRect([self frame], 5, 5));
  
  [[NSColor purpleColor] set];
  for (id view in [self subviews])
  {
    NSFrameRect([view frame]);
  }
}
#endif

static bool box_create(::mforms::Box *self, bool horiz)
{
  MFBoxImpl *box= [[[MFBoxImpl alloc] initWithObject:self] autorelease];
  
  [box setHorizontal:horiz ? YES : NO];
  
  return true;  
}


static void box_set_spacing(::mforms::Box *self, int spacing)
{
  if ( self )
  {
    MFBoxImpl* box = self->get_data();
    
    if ( box )
    {
      [box setSpacing:spacing];
    }
  }
}


static void box_set_homogeneous(::mforms::Box *self, bool flag)
{
  if ( self )
  {
    MFBoxImpl* box = self->get_data();
    
    if ( box )
    {
      [box setHomogeneous:flag];
    }
  }
}


static void box_add(::mforms::Box *self, ::mforms::View *child, bool expand, bool fill)
{
  NSView *childView = child->get_data();
  NSUInteger flags = childView.viewFlags & ~BoxFlagMask;
  id last = nil;
  if (expand)
    flags |= ExpandFlag;
  if (fill)
    flags |= FillFlag;
  
  childView.viewFlags = flags;
  
  // find the 1st subview that's packed to the end
  for (NSView *sub in [self->get_data() subviews])
  {
    if (sub.viewFlags & PackEndFlag)
    {
      last= sub;
      break;
    }
  }

  MFBoxImpl *view = self->get_data();
  [view setFreezeRelayout: YES];

  // there's nothing packed to the end, so just add to the end
  if (!last)
    [view addSubview: childView];
  else // if there's something packed to end, add it before (ie below) it
    [view addSubview: childView positioned: NSWindowBelow relativeTo: last];

  assert(childView.superview == self->get_data());
  if ([view setFreezeRelayout: NO])
  {
    [view subviewMinimumSizeChanged];
    [view resizeSubviewsWithOldSize: view.frame.size];
  }
}


static void box_add_end(::mforms::Box *self, ::mforms::View *child, bool expand, bool fill)
{
  NSView *childView = child->get_data();

  // TODO: what exactly is this about? Looks like a debugging help.
  if ([child->get_data() viewFlags] < 0)
    NSLog(@"NEGATIVE TAG IN NEW OBJECT %li", (long)[child->get_data() viewFlags]);

  NSUInteger flags = (childView.viewFlags & ~BoxFlagMask) | PackEndFlag;
  id last = nil;
  if (expand)
    flags |= ExpandFlag;
  if (fill)
    flags |= FillFlag;

  childView.viewFlags = flags;
  
  // Find the 1st subview that's packed to the end.
  MFBoxImpl *view = self->get_data();
  for (NSView *sub in view.subviews)
  {
    if (sub.viewFlags & PackEndFlag)
    {    
      last = sub;
      break;
    }
  }
  
  [view setFreezeRelayout: YES];

  if (!last)
    [view addSubview: childView positioned: NSWindowAbove relativeTo: nil];
  else
    [view addSubview: childView positioned: NSWindowBelow relativeTo: last];

  assert(childView.superview == self->get_data());
  if ([view setFreezeRelayout: NO])
    [view subviewMinimumSizeChanged];
}


static void box_remove(::mforms::Box *self, ::mforms::View *child)
{
  [self->get_data() setFreezeRelayout: YES];
  [child->get_data() removeFromSuperview];
  [self->get_data() setFreezeRelayout: NO];
}


void cf_box_init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();
  
  f->_box_impl.create= &box_create;
  f->_box_impl.set_spacing= &box_set_spacing;
  f->_box_impl.add= &box_add;
  f->_box_impl.add_end= &box_add_end;
  f->_box_impl.remove= &box_remove;
  f->_box_impl.set_homogeneous= &box_set_homogeneous;
}

@end


