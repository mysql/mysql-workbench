/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

#import "MFSplitter.h"
#import "MFMForms.h"
#import "MFBox.h"

//----------------------------------------------------------------------------------------------------------------------

@implementation MFSplitterImpl

static NSSize initialSize = { 10, 10 };

- (instancetype)initWithObject: (mforms::Splitter*)aSplitter {
  self = [super initWithFrame: NSMakeRect(10, 10, 10, 10)];
  if (self) {
    mOwner= aSplitter;
    mOwner->set_data(self);
    self.delegate = self;
    mRequestedPosition = -1;
    mResizable[0] = YES;
    mResizable[1] = YES;
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (mforms::Object*)mformsObject {
  return mOwner;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setHorizontal: (BOOL)flag {
  mHorizontal = flag;
  self.vertical = flag;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setPosition: (int)position {
  // If our size is still the initial size, schedule the size to be set later (usually happens in tabviews before being shown).
  if (NSEqualSizes(self.frame.size, initialSize) || NSEqualSizes(self.frame.size, NSZeroSize))
    mRequestedPosition = position;
  else
    [self setPosition:position ofDividerAtIndex: 0];
}

//----------------------------------------------------------------------------------------------------------------------

- (int)position {
  // If our size is still the initial size, return the requested position if set previously.
  if ((NSEqualSizes(self.frame.size, initialSize) || NSEqualSizes(self.frame.size, NSZeroSize))
      && mRequestedPosition > -1) {
    return mRequestedPosition;
  } else {
    NSRect frame = (self.subviews[0]).frame;
    return self.vertical ? NSMaxX(frame) : NSMaxY(frame);
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (NSSize)minimumSize {
  NSSize size, minSize;
  float maxSize = 0;
  int i = 0;
  
  size.width = 0;
  size.height = 0;
  
  for (NSView *subview in self.subviews) {
    if (!subview.isHidden) {
      minSize = subview.minimumSize;
      
      if (mHorizontal) {
        size.width += minSize.width;
        maxSize = MAX(maxSize, minSize.width);
        size.height = MAX(size.height, MAX(mMinSizes[i], minSize.height));
      } else {
        size.width = MAX(size.width, MAX(mMinSizes[i], minSize.width));
        maxSize = MAX(maxSize, minSize.height);
        size.height += minSize.height;
      }
    }
    i++;
  }
  minSize = super.minimumSize;
  return { MAX(size.width, minSize.width) + self.dividerThickness, MAX(size.height, minSize.height) + self.dividerThickness };
}

//----------------------------------------------------------------------------------------------------------------------

- (void)resizeSubviewsWithOldSize: (NSSize)osize {
  [super resizeSubviewsWithOldSize: osize];

  if (NSEqualSizes(osize, self.frame.size)) {
    for (id sub in self.subviews)
      [sub resizeSubviewsWithOldSize: [sub frame].size];
  }

  if (!NSEqualSizes(self.frame.size, initialSize) && !NSEqualSizes(self.frame.size, NSZeroSize)
      && mRequestedPosition > 0) {
    [self setPosition:mRequestedPosition ofDividerAtIndex:0];
    mRequestedPosition= -1;
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)splitView: (NSSplitView *)splitView canCollapseSubview: (NSView *)subview {
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)splitView: (NSSplitView *)splitView shouldAdjustSizeOfSubview: (NSView *)subview {
  if (subview == splitView.subviews.lastObject)
    return mResizable[1];
  else
    return mResizable[0];
}

//----------------------------------------------------------------------------------------------------------------------

  - (CGFloat)splitView: (NSSplitView *)splitView
constrainMaxCoordinate: (CGFloat)proposedMax
           ofSubviewAt: (NSInteger)dividerIndex {
  if (mHorizontal) {
    if (proposedMax > NSWidth(splitView.frame) - mMinSizes[1])
      proposedMax = NSWidth(splitView.frame) - mMinSizes[1];
  } else {
    if (proposedMax > NSHeight(splitView.frame) - mMinSizes[1])
      proposedMax = NSHeight(splitView.frame) - mMinSizes[1];
  }
  return proposedMax;
}

//----------------------------------------------------------------------------------------------------------------------

  - (CGFloat)splitView: (NSSplitView *)splitView
constrainMinCoordinate: (CGFloat)proposedMin
           ofSubviewAt: (NSInteger)dividerIndex {
  if (proposedMin < mMinSizes[0])
    proposedMin = mMinSizes[0];
  return proposedMin;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)splitViewDidResizeSubviews: (NSNotification *)notification {
  mOwner->position_changed();
}

//----------------------------------------------------------------------------------------------------------------------

- (NSRect)splitView: (NSSplitView *)splitView
      effectiveRect: (NSRect)proposedEffectiveRect
       forDrawnRect: (NSRect)drawnRect
   ofDividerAtIndex: (NSInteger)dividerIndex {
  // if the divider is too thin, increase effective rect by 2px to make it less impossible to drag
  if (splitView.vertical) {
    if (proposedEffectiveRect.size.width < 2) {
      proposedEffectiveRect.origin.x -= 1;
      proposedEffectiveRect.size.width += 2;
    }
  } else {
    if (proposedEffectiveRect.size.height < 2) {
      proposedEffectiveRect.origin.y -= 1;
      proposedEffectiveRect.size.height += 2;
    }
  }
  return proposedEffectiveRect;
}

//----------------------------------------------------------------------------------------------------------------------

static bool splitter_create(::mforms::Splitter *self, bool horiz, bool thin) {
  MFSplitterImpl *splitter = [[MFSplitterImpl alloc] initWithObject: self];
  
  [splitter setHorizontal: horiz ? YES : NO];
  if (thin)
    splitter.dividerStyle = NSSplitViewDividerStyleThin;
  
  return true;  
}

//----------------------------------------------------------------------------------------------------------------------

static void splitter_set_divider_position(::mforms::Splitter *self, int pos) {
  MFSplitterImpl* splitter = self->get_data();
  if (splitter) {
    [splitter setPosition: pos];
  }
}

//----------------------------------------------------------------------------------------------------------------------

static int splitter_get_divider_position(::mforms::Splitter *self) {
  MFSplitterImpl* splitter = self->get_data();
  if (splitter)
    return splitter.position;

  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

static void splitter_add(::mforms::Splitter *self, ::mforms::View *child, int minsize, bool fixed) {
  NSUInteger idx;
  MFSplitterImpl *impl = self->get_data();
  if ((idx = impl.subviews.count) > 2) {
    NSLog(@"Attempt to add subview to splitter with 2 items already");
    return;
  }
  impl->mResizable[idx] = !fixed;
  impl->mMinSizes[idx] = minsize;
  [impl addSubview: child->get_data()];
}

//----------------------------------------------------------------------------------------------------------------------

static void splitter_remove(::mforms::Splitter *self, ::mforms::View *child) {
  MFSplitterImpl *impl = self->get_data();
  if ([impl.subviews indexOfObject: child->get_data()] == 0)
    impl->mMinSizes[0] = impl->mMinSizes[1];
  [child->get_data() removeFromSuperview];
}

//----------------------------------------------------------------------------------------------------------------------

static void splitter_set_expanded(::mforms::Splitter *self, bool first, bool expand) {
  MFSplitterImpl *impl = self->get_data();
  
  NSView *view1  = impl.subviews[0];
	NSView *view2 = impl.subviews[1];

  if (first) {
    if (!expand == [impl isSubviewCollapsed: view1])
      return;
    
    if (expand) {
      [view1 setHidden: NO];
      NSRect frame = view1.frame;
      [impl setPosition: frame.size.width ofDividerAtIndex: 0];
    } else {
      [view1 setHidden: YES];
      [impl setPosition: 0 ofDividerAtIndex: 0];
    }
  } else {
    if (!expand == [impl isSubviewCollapsed: view2])
      return;
    
    if (expand) {
      [view2 setHidden: NO];
      CGFloat dividerThickness = impl.dividerThickness;

      NSRect frame1 = view1.frame;
      NSRect frame2 = view2.frame;
      
      if (impl.vertical) { // Is the splitter bar vertical (not the layout)?
        // Adjust left frame size.
        frame1.size.width = (frame1.size.width - frame2.size.width - dividerThickness);
        frame2.origin.x = frame1.size.width + dividerThickness;
        [view1 setFrameSize: frame1.size];
        view2.frame = frame2;
      } else {
        // Adjust top frame size.
        frame1.size.height = (frame1.size.height - frame2.size.height - dividerThickness);
        frame2.origin.y = frame1.size.height + dividerThickness;
        [view1 setFrameSize: frame1.size];
        view2.frame = frame2;
      }
    } else {
      NSRect frame1 = view1.frame;
      NSRect overallFrame = impl.frame;
      [view2 setHidden: YES];
      if (impl.vertical) {
        [view1 setFrameSize: NSMakeSize(overallFrame.size.width, frame1.size.height)];
      } else {
        [view1 setFrameSize: NSMakeSize(frame1.size.width, overallFrame.size.height)];
      }
    }
  }
  [impl setNeedsLayout: YES];
}

//----------------------------------------------------------------------------------------------------------------------

void cf_splitter_init() {
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();
  
  f->_splitter_impl.create= &splitter_create;
  f->_splitter_impl.set_divider_position= &splitter_set_divider_position;
  f->_splitter_impl.get_divider_position= &splitter_get_divider_position;
  f->_splitter_impl.add= &splitter_add;
  f->_splitter_impl.remove= &splitter_remove;
  f->_splitter_impl.set_expanded = &splitter_set_expanded;
}

@end


