/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

#import "MFMForms.h"

#import "MFBox.h"
#import "MFLabel.h"
#import "MFTextEntry.h"

#import "NSColor_extras.h"

//----------------------------------------------------------------------------------------------------------------------

@implementation MFBoxImpl

- (instancetype)initWithObject: (::mforms::Box *)aBox {
  self = [super initWithFrame:NSMakeRect(10, 10, 10, 10)];
  if (self) {
    mOwner = aBox;
    mOwner->set_data(self);
  }
  if (@available(iOS 14.0, *)) {
    self.clipsToBounds = TRUE;
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (mforms::Object *)mformsObject {
  return mOwner;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)isFlipped {
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setHorizontal: (BOOL)flag {
  mHorizontal = flag;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setHomogeneous: (BOOL)flag {
  mHomogeneous = flag;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setSpacing: (int)spacing {
  mSpacing = spacing;
}

//----------------------------------------------------------------------------------------------------------------------

struct ChildEntry {
  NSView *view;
  NSRect frame;
  bool fills;
  bool expands;
};

//----------------------------------------------------------------------------------------------------------------------

/**
 * Adjusts all child bounds to fill their container vertically (in horizontal mode)
 * or horizontally (in vertical mode). Applies the given padding value accordingly.
 *
 * Note: containerSize must not contain any padding value.
 *
 */
static void maximizeChildren(std::vector<ChildEntry> &list, bool horizontal, NSSize containerSize, int padding) {
  if (horizontal) {
    for (ChildEntry &entry : list) {
      entry.view.autoresizingMask &= -NSViewHeightSizable;
      entry.frame.origin.y = padding;
      entry.frame.size.height = containerSize.height;
    };
  } else {
    for (ChildEntry &entry : list) {
      entry.view.autoresizingMask &= -NSViewWidthSizable;
      entry.frame.origin.x = padding;

      // For labels determine the preferred height based on the new width (as they can be in auto wrap mode).
      if ([entry.view isKindOfClass: MFLabelImpl.class]) {
        NSSize size = [entry.view preferredSize: { containerSize.width, 0 }];
        entry.frame.size = { containerSize.width, ceil(size.height) };
      } else
        entry.frame.size.width = containerSize.width;
    };
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Applies the computed bounds to each view. Adjust position if the control does not fill the given
 * space depending on the fill flag.
 */
static void applyBounds(std::vector<ChildEntry> &list, bool horizontal) {
  // Accumulate additional offsets which could be caused by control constraints (i.e. a control
  // does not resize to the size we gave it). We have to adjust the computed location of following controls then.
  int verticalOffset = 0;
  int horizontalOffset = 0;

  for (ChildEntry &entry : list) {
    // The parameter horizontal tells us if the layout direction is horizontal or vertical.
    // Always resize orthogonal to the layout direction but resize in layout direction only if fill is set
    // or the computed size is smaller than the current size.
    entry.view.autoresizingMask = 0;
    NSRect newFrame = entry.view.frame;

    if (horizontal) {
      // Workaround: usually, scaling a button or text field vertically does not produce good results.
      // They are better left at their normal height, so we switch that auto-scaling off.
      // In vertical layout the scaling is controllable by the fill flag.
      // Labels are special text fields which we wanna scale.
      bool isTextEntry = [entry.view isKindOfClass: NSTextField.class] && ![entry.view isKindOfClass: MFLabelImpl.class];
      if (![entry.view isKindOfClass: NSButton.class] && !isTextEntry)
        newFrame.size.height = entry.frame.size.height;
      if (entry.fills || entry.frame.size.width < newFrame.size.width)
        newFrame.size.width = entry.frame.size.width;
    } else {
      newFrame.size.width = entry.frame.size.width;
      if (entry.fills || entry.frame.size.height < newFrame.size.height)
        newFrame.size.height = entry.frame.size.height;
    }

    // Get the resulting control size and adjust the offset for following controls and the
    // middle alignment (for controls that don't resize).
    newFrame.origin.x =
      int(entry.frame.origin.x + horizontalOffset + (entry.frame.size.width - newFrame.size.width) / 2);
    newFrame.origin.y =
      int(entry.frame.origin.y + verticalOffset + (entry.frame.size.height - newFrame.size.height) / 2);
    if ([entry.view isKindOfClass:NSButton.class]) {
      newFrame.origin.y++;
    }

    if (horizontal)
      horizontalOffset += newFrame.size.width - entry.frame.size.width;
    else
      verticalOffset += newFrame.size.height - entry.frame.size.height;

    entry.view.frame = newFrame;
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the largest width of any control in both given lists.
 */
static int getLargestWidth(std::vector<ChildEntry> &list1, std::vector<ChildEntry> &list2) {
  int max = 0;
  for (ChildEntry &entry : list1) {
    if (entry.frame.size.width > max)
      max = entry.frame.size.width;
  }

  for (ChildEntry &entry : list2) {
    if (entry.frame.size.width > max)
      max = entry.frame.size.width;
  }

  return max;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the largest height of any control in both given lists.
 */
static int getLargestHeight(std::vector<ChildEntry> &list1, std::vector<ChildEntry> &list2) {
  int max = 0;
  for (ChildEntry &entry : list1) {
    if (entry.frame.size.height > max)
      max = entry.frame.size.height;
  }

  for (ChildEntry &entry : list2) {
    if (entry.frame.size.height > max)
      max = entry.frame.size.height;
  }

  return max;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Called if the container is in "use all" mode and any remaining space in it is to be distributed
 * amongst all child controls that have the expand flag set.
 *
 * @param list The list of views to expand.
 * @param fraction The amount of pixels the controls must be resized/moved.
 * @param mirrored Do the adjustment in a mirrored fashion (for right aligned controls).
 */
static void expandHorizontally(std::vector<ChildEntry> &list, int fraction, bool mirrored) {
  for (size_t i = 0; i < list.size(); ++i) {
    if (list[i].expands) {
      list[i].frame.size.width += fraction;
      if (mirrored) {
        list[i].frame.origin.x -= fraction;

        // Move all following controls by the same amount.
        for (size_t j = i + 1; j < list.size(); ++j)
          list[j].frame.origin.x -= fraction;
      } else {
        // Move all following controls by the same amount.
        for (size_t j = i + 1; j < list.size(); ++j)
          list[j].frame.origin.x += fraction;
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Called if the container is in "use all" mode and any remaining space in it is to be distributed
 * amongst all child controls that have the expand flag set.
 *
 * @param list The list of views to expand.
 * @param fraction The amount of pixels the controls must be resized/moved.
 * @param mirrored Do the adjustment in a mirrored fashion (for bottom aligned controls).
 */
static void expandVertically(std::vector<ChildEntry> &list, int fraction, bool mirrored) {
  for (size_t i = 0; i < list.size(); ++i) {
    if (list[i].expands) {
      list[i].frame.size.height += fraction;
      if (mirrored) {
        list[i].frame.origin.y -= fraction;

        // Move all following controls by the same amount.
        for (size_t j = i + 1; j < list.size(); ++j)
          list[j].frame.origin.y -= fraction;
      } else {
        // Move all following controls by the same amount.
        for (size_t j = i + 1; j < list.size(); ++j)
          list[j].frame.origin.y += fraction;
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Computes a horizotnal layout.
 *
 * @param proposedSize The size to start from layouting. Since super ordinated controls may impose
 *                     a layout size we need to honor that (especially important for auto wrapping
 *                     labels).
 * @param resizeChildren Tells the function whether the computed client control bounds should be applied
 *                      (when doing a relayout) or not (when computing the preferred size).
 * @return The resulting size of the box.
 */
- (NSSize)computeHorizontalLayout: (NSSize)proposedSize resizeChildren: (BOOL)doResize {
  std::vector<ChildEntry> leftAligned;
  std::vector<ChildEntry> rightAligned;

  int maxHeight = 0;
  int expandCount = 0;

  float horizontalPadding = mLeftPadding + mRightPadding;
  float verticalPadding = mTopPadding + mBottomPadding;

  proposedSize.width -= horizontalPadding;
  proposedSize.height -= verticalPadding;

  // First part: setup and vertical position and size computation.
  for (NSView *view in self.subviews) {
    if (view.isHidden)
      continue;

    // Make a copy of the current bounds of the control.
    // This is used in the computation before we finally manipulate the control itself.
    ChildEntry entry;
    entry.view = view;
    entry.fills = (view.viewFlags & FillFlag) != 0;
    entry.expands = (view.viewFlags & ExpandFlag) != 0;

    // Keep track of the tallest control, so we can adjust the container's height properly.
    view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    entry.frame = {{0, 0}, [view preferredSize:{0, proposedSize.height}]};

    // Ensure integral sizes.
    entry.frame.size.width = ceil(entry.frame.size.width);
    entry.frame.size.height = ceil(entry.frame.size.height);

    // Sort control into the proper alignment list.
    if ((view.viewFlags & PackEndFlag) != 0)
      rightAligned.push_back(entry);
    else
      leftAligned.push_back(entry);

    // Keep track of the highest control, so we can adjust the container's height properly.
    if (maxHeight < entry.frame.size.height)
      maxHeight = entry.frame.size.height;

    // Count how many children have the expand flag set. This is needed for later computation.
    // Remove auto resizing in layout direction too, as this is mutual exclusive to expand mode.
    if (entry.expands)
      expandCount++;
  }

  // Adjust height of the container if it is too small or auto resizing is enabled.
  if (proposedSize.height < maxHeight || (self.autoresizingMask & NSViewHeightSizable) != 0) {
    proposedSize.height = maxHeight;
    if (proposedSize.height < self.minimumSize.height - verticalPadding)
      proposedSize.height = self.minimumSize.height - verticalPadding;
  }

  // Go again through the child list and adjust the height of each child control as well as
  // compute their vertical position.
  maximizeChildren(leftAligned, true, proposedSize, mTopPadding);
  maximizeChildren(rightAligned, true, proposedSize, mTopPadding);

  // Second part: horizontal position and size computation.
  // We can have two special cases here: distributed and "use all" mode, but only if the container
  // is not set to auto resizing in horizontal direction (otherwise it adjusts itself to the content).
  int commonWidth = 0;

  int controlCount = int(leftAligned.size() + rightAligned.size());
  if ((controlCount > 0) && mHomogeneous) {
    // In this mode we resize all controls so that they entirely fill the width of the container.
    // However, if any of the child controls has a width larger than the computed common width
    // instead use this largest width and increase the container width accordingly.
    commonWidth = (proposedSize.width - (controlCount - 1) * mSpacing) / controlCount;
    int max = getLargestWidth(leftAligned, rightAligned);
    if (max > commonWidth)
      commonWidth = max;
  }

  int offset = mLeftPadding;

  int resultingWidth = 0;
  for (ChildEntry &entry : leftAligned) {
    // Consider either a common width or the individual widths of the controls here.
    entry.frame.origin.x = offset;
    if (commonWidth > 0)
      entry.frame.size.width = commonWidth;
    offset += entry.frame.size.width + mSpacing;
  }

  if (offset > mLeftPadding) {
    // Remove the left padding we used for positioning. It's applied later.
    resultingWidth = offset - mLeftPadding;

    // Remove also the last spacing if there are no (visible) right aligned children.
    if (rightAligned.size() == 0)
      resultingWidth -= mSpacing;
  }

  // For right aligned controls we first compute relative coordinates.
  offset = -mRightPadding;
  for (ChildEntry &entry : rightAligned) {
    // Consider either a common width or the individual widths of the controls here.
    if (commonWidth > 0)
      entry.frame.size.width = commonWidth;
    entry.frame.origin.x = offset - entry.frame.size.width;
    offset -= entry.frame.size.width + mSpacing;
  }

  if (offset < -mRightPadding)
    // Remove one spacing we added too much above. Also remove the right padding we used for positioning.
    // The padding is applied later.
    resultingWidth += -offset - mSpacing - mRightPadding;

  // Adjust width of the container if it is too small or auto resizing is enabled.
  if (proposedSize.width < resultingWidth || (self.autoresizingMask & NSViewWidthSizable) != 0) {
    proposedSize.width = resultingWidth;
    if (proposedSize.width < self.minimumSize.width - horizontalPadding)
      proposedSize.width = self.minimumSize.width - horizontalPadding;
  }

  if (doResize) {
    // Distribute any free space amongst all child views which have their expand flag set. This is
    // mutually exclusive with auto resizing in layout direction.
    // Though we don't need to test the auto resizing flag since the box's width would here already be set to
    // the resulting width we computed above if it were enabled.
    if (expandCount > 0 && proposedSize.width > resultingWidth) {
      int fraction = (proposedSize.width - resultingWidth) / expandCount;
      expandHorizontally(leftAligned, fraction, false);
      expandHorizontally(rightAligned, fraction, true);
    }

    // Compute the final position of the right aligned controls.
    for (ChildEntry &entry : rightAligned)
      entry.frame = NSOffsetRect(entry.frame, proposedSize.width, 0);

    proposedSize.width += horizontalPadding;
    proposedSize.height += verticalPadding;

    applyBounds(leftAligned, true);
    applyBounds(rightAligned, true);
  } else {
    proposedSize.width += horizontalPadding;
    proposedSize.height += verticalPadding;
  }

  return proposedSize;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Computes the vertical layout of the box.
 *
 * @param proposedSize The size to start from layouting. Since super ordinated controls may impose
 *                     a layout size we need to honor that (especially important for auto wrapping
 *                     labels).
 * @param resizeChildren Tells the function whether the computed client control bounds should be applied
 *                       (when doing a relayout) or not (when computing the preferred size).
 * @return The resulting size of the box.
 */
- (NSSize)computeVerticalLayout: (NSSize)proposedSize resizeChildren: (BOOL)doResize {
  std::vector<ChildEntry> topAligned;
  std::vector<ChildEntry> bottomAligned;

  int maxWidth = 0;
  int expandCount = 0;

  int horizontalPadding = mLeftPadding + mRightPadding;
  int verticalPadding = mTopPadding + mBottomPadding;

  proposedSize.width -= horizontalPadding;
  proposedSize.height -= verticalPadding;

  // First part: setup vertical position and size computation.
  for (NSView *view in self.subviews) {
    if (view.isHidden)
      continue;

    // Make a copy of the current bounds of the control.
    // This is used in the computation before we finally manipulate the control itself.
    ChildEntry entry;
    entry.view = view;
    entry.fills = (view.viewFlags & FillFlag) != 0;
    entry.expands = (view.viewFlags & ExpandFlag) != 0;

    // Keep track of the widest control, so we can adjust the container's width properly.
    view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    entry.frame = {{ 0, 0 }, [view preferredSize: { proposedSize.width, 0 }] };

    // Sort control into the proper alignment list.
    if ((view.viewFlags & PackEndFlag) != 0)
      bottomAligned.push_back(entry);
    else
      topAligned.push_back(entry);

    if (maxWidth < entry.frame.size.width)
      maxWidth = entry.frame.size.width;

    // Count how many children have the expand flag set. This is needed for later computation.
    // Remove auto resizing in layout direction too, as this is mutual exclusive to expand mode.
    if (entry.expands)
      expandCount++;
  }

  // Adjust height of the container if it is too small or auto resizing is enabled.
  if (proposedSize.width < maxWidth || (self.autoresizingMask & NSViewWidthSizable) != 0) {
    proposedSize.width = maxWidth;
    if (proposedSize.width < self.minimumSize.width - horizontalPadding)
      proposedSize.width = self.minimumSize.width - horizontalPadding;
  }

  // Go again through the child list and adjust the width of each child control as well as
  // compute their vertical position. This will also determine if we need to adjust the overall
  // height, in case we have labels in the list.
  maximizeChildren(topAligned, false, proposedSize, mLeftPadding);
  maximizeChildren(bottomAligned, false, proposedSize, mLeftPadding);

  // Second part: horizontal position and size computation.
  // We can have two special cases here: distributed and "use all" mode, but only if the container
  // is not set to auto resizing in horizontal direction (otherwise it adjusts itself to the content).
  int commonHeight = 0;
  int controlCount = int(topAligned.size() + bottomAligned.size());
  if (controlCount > 0 && mHomogeneous) {
    // In this mode we resize all controls so that they entirely fill the width of the container.
    // However, if any of the child controls has a width larger than the computed common width
    // instead use this largest width and increase the container width accordingly.
    commonHeight = (proposedSize.height - (controlCount - 1) * mSpacing) / controlCount;
    int max = getLargestHeight(topAligned, bottomAligned);
    if (max > commonHeight)
      commonHeight = max;
  }

  int offset = mTopPadding;

  int resultingHeight = 0;
  for (ChildEntry &entry : topAligned) {
    // Consider either a common height or the individual widths of the controls here.
    entry.frame.origin.y = offset;
    if (commonHeight > 0)
      entry.frame.size.height = commonHeight;
    offset += entry.frame.size.height + mSpacing;
  }

  if (offset > mTopPadding) {
    // Remove the top padding we used for positioning. It's applied later.
    resultingHeight = offset - mTopPadding;

    // Remove also the last spacing if there are no bottom aligned children.
    if (bottomAligned.size() == 0)
      resultingHeight -= mSpacing;
  }

  // For bottom aligned controls we first compute relative coordinates.
  offset = -mBottomPadding;
  for (ChildEntry &entry : bottomAligned) {
    // Consider either a common height or the individual widths of the controls here.
    if (commonHeight > 0)
      entry.frame.size.height = commonHeight;
    entry.frame.origin.y = offset - entry.frame.size.height;
    offset -= entry.frame.size.height + mSpacing;
  }

  if (offset < -mBottomPadding)
    // Remove one spacing we added too much above. Also remove the bottom padding we used for positioning.
    // The padding is applied later.
    resultingHeight += -offset - mSpacing - mBottomPadding;

  // Adjust height of the container if it is too small or auto resizing is enabled.
  if (proposedSize.height < resultingHeight || (self.autoresizingMask & NSViewHeightSizable) != 0) {
    proposedSize.height = resultingHeight;
    if (proposedSize.height < self.minimumSize.height - verticalPadding)
      proposedSize.height = self.minimumSize.height - verticalPadding;
  }

  if (doResize) {
    // Distribute any free space amongst all child controls which have their expand flag set. This is
    // mutually exclusive with auto resizing in layout direction.
    // Though we don't need to test the auto resizing flag since the box's width would here already be set to
    // the resulting width we computed above if it were enabled.
    if (expandCount > 0 && proposedSize.height > resultingHeight) {
      int fraction = (proposedSize.height - resultingHeight) / expandCount;
      expandVertically(topAligned, fraction, false);
      expandVertically(bottomAligned, fraction, true);
    }

    // Apply the padding value again and compute the final position of the bottom aligned controls.
    proposedSize.width += horizontalPadding;
    proposedSize.height += verticalPadding;

    for (ChildEntry &entry : bottomAligned)
      entry.frame = NSOffsetRect(entry.frame, 0, proposedSize.height);

    applyBounds(topAligned, false);
    applyBounds(bottomAligned, false);
  } else {
    proposedSize.width += horizontalPadding;
    proposedSize.height += verticalPadding;
  }

  return proposedSize;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSSize)preferredSize: (NSSize)proposedSize {
  NSSize size;
  if (mHorizontal)
    size = [self computeHorizontalLayout: proposedSize resizeChildren: NO];
  else
    size = [self computeVerticalLayout: proposedSize resizeChildren: NO];
  NSSize minSize = self.minimumSize;
  return { MAX(size.width, minSize.width), MAX(size.height, minSize.height) };
}

//----------------------------------------------------------------------------------------------------------------------

- (void)resizeSubviewsWithOldSize: (NSSize)oldBoundsSize {
  if (mOwner != nullptr && !mOwner->is_destroying()) {
    NSAutoresizingMaskOptions previous = self.autoresizingMask;
    self.autoresizingMask = 0;
    if (mHorizontal)
      [self computeHorizontalLayout: self.frame.size resizeChildren: YES];
    else
      [self computeVerticalLayout: self.frame.size resizeChildren: YES];

    self.autoresizingMask = previous;
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)didAddSubview: (NSView *)subview {
  [self resizeSubviewsWithOldSize: self.frame.size];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setFrame: (NSRect)frame {
  super.frame = frame;
  if (mOwner != NULL && !mOwner->is_destroying())
    (*mOwner->signal_resized())();
}

//----------------------------------------------------------------------------------------------------------------------

#if 0
- (void)drawRect: (NSRect)rect
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

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityGroupRole;
}

//----------------------------------------------------------------------------------------------------------------------

static bool box_create(::mforms::Box *self, bool horiz) {
  MFBoxImpl *box = [[MFBoxImpl alloc] initWithObject: self];

  [box setHorizontal:horiz ? YES : NO];

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

static void box_set_spacing(::mforms::Box *self, int spacing) {
  if (self) {
    MFBoxImpl *box = self->get_data();

    if (box) {
      [box setSpacing:spacing];
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void box_set_homogeneous(::mforms::Box *self, bool flag) {
  if (self) {
    MFBoxImpl *box = self->get_data();

    if (box) {
      [box setHomogeneous:flag];
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void box_add(::mforms::Box *self, ::mforms::View *child, bool expand, bool fill) {
  NSView *childView = child->get_data();
  NSUInteger flags = childView.viewFlags & ~BoxFlagMask;
  id last = nil;
  if (expand)
    flags |= ExpandFlag;
  if (fill)
    flags |= FillFlag;

  childView.viewFlags = ViewFlags(flags);

  // find the 1st subview that's packed to the end
  for (NSView *sub in [self->get_data() subviews]) {
    if (sub.viewFlags & PackEndFlag) {
      last = sub;
      break;
    }
  }

  MFBoxImpl *view = self->get_data();
  [view setFreezeRelayout: YES];

  // there's nothing packed to the end, so just add to the end
  if (!last)
    [view addSubview:childView];
  else // if there's something packed to end, add it before (ie below) it
    [view addSubview:childView positioned:NSWindowBelow relativeTo:last];

  assert(childView.superview == self->get_data());
  if ([view setFreezeRelayout: NO])
    [view resizeSubviewsWithOldSize: view.frame.size];
}

//----------------------------------------------------------------------------------------------------------------------

static void box_add_end(::mforms::Box *self, ::mforms::View *child, bool expand, bool fill) {
  NSView *childView = child->get_data();

  NSUInteger flags = (childView.viewFlags & ~BoxFlagMask) | PackEndFlag;
  if (expand)
    flags |= ExpandFlag;
  if (fill)
    flags |= FillFlag;

  MFBoxImpl *view = self->get_data();
  [view setFreezeRelayout: YES];
  childView.viewFlags = ViewFlags(flags);

  [view addSubview:childView];

  if ([view setFreezeRelayout: NO])
    [view resizeSubviewsWithOldSize:view.frame.size];
}

//----------------------------------------------------------------------------------------------------------------------

static void box_remove(::mforms::Box *self, ::mforms::View *child) {
  [self->get_data() setFreezeRelayout: YES];
  [child->get_data() removeFromSuperview];
  [self->get_data() setFreezeRelayout: NO];
}

//----------------------------------------------------------------------------------------------------------------------

void cf_box_init() {
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_box_impl.create = &box_create;
  f->_box_impl.set_spacing = &box_set_spacing;
  f->_box_impl.add = &box_add;
  f->_box_impl.add_end = &box_add_end;
  f->_box_impl.remove = &box_remove;
  f->_box_impl.set_homogeneous = &box_set_homogeneous;
}

@end

//----------------------------------------------------------------------------------------------------------------------
