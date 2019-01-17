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

#import "MFTable.h"
#import "MFMForms.h"
#import "MFLabel.h"

//----------------------------------------------------------------------------------------------------------------------

struct CellEntry {
  NSView *view;
  bool isVisible;
  NSRect frame;
  int leftAttachment;
  int rightAttachment;
  int topAttachment;
  int bottomAttachment;
  bool horizontalExpand;
  bool verticalExpand;
  bool horizontalFill;
  bool verticalFill;
};

//----------------------------------------------------------------------------------------------------------------------

@interface MFTableImpl ()  {
  float mRowSpacing;
  float mColumnSpacing;
  int mRowCount;
  int mColumnCount;
  BOOL mHomogeneous;

  BOOL mHorizontalCenter;
  BOOL mVerticalCenter;

  std::vector<CellEntry> content;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation MFTableImpl

- (instancetype)initWithObject:(::mforms::Table*)aTable
{
  self = [super initWithFrame: NSMakeRect(0, 0, 1, 1)];
  if (self)
  {
    mOwner = aTable;
    mOwner->set_data(self);
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (mforms::Object*)mformsObject
{
  return mOwner;
}

//----------------------------------------------------------------------------------------------------------------------

STANDARD_MOUSE_HANDLING(self) // Add handling for mouse events.

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)isFlipped
{
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setHomogeneous:(bool)flag
{
  mHomogeneous= flag;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setRowSpacing:(float)spacing
{
  mRowSpacing= spacing;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setColumnSpacing:(float)spacing
{
  mColumnSpacing= spacing;
}

//----------------------------------------------------------------------------------------------------------------------

#if 0
- (void)drawRect:(NSRect)rect
{
  [[NSColor blueColor] set];
  NSFrameRect(NSInsetRect([self bounds], 1, 1));
  
  [[NSColor orangeColor] set];
  for (id view in [self subviews])
  {
    NSFrameRect([view frame]);
  }
}
#endif

//----------------------------------------------------------------------------------------------------------------------

#pragma mark - Layout

/**
 * Applies the computed bounds to each view. Adjusts position if the control does not fill the given
 * space depending on the fill flag.
 *
 * @param list The list of cell entries with their bounds to be applied.
 */
static void applyBounds(std::vector<CellEntry> &list)
{
  for (auto &entry : list)
  {
    if (!entry.isVisible)
      continue;

    entry.view.autoresizingMask = 0;
    NSRect newFrame = entry.view.frame;

    // Resize the view to fill the available space if it is larger than that or
    // the fill flag is set.
    if (entry.horizontalFill || entry.frame.size.width < newFrame.size.width)
      newFrame.size.width = entry.frame.size.width;
    if (entry.verticalFill || entry.frame.size.height < newFrame.size.height)
      newFrame.size.height = entry.frame.size.height;

    newFrame.origin.x = entry.frame.origin.x + (entry.frame.size.width - newFrame.size.width) / 2;
    newFrame.origin.y = entry.frame.origin.y + (entry.frame.size.height - newFrame.size.height) / 2;

    entry.view.frame = newFrame;
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Computes the entire layout of the table. This includes size and position of client views.
 *
 * @param proposedSize The size to start from layouting. Since super ordinated controls may impose
 *                     a layout size we need to honor that (especially important for auto wrapping
 *                     labels).
 * @param resizeChildren Tells the function whether the computed child view bounds should be applied
 *                       (when doing a relayout) or not (when computing the preferred size).
 * @return The resulting size of the table.
 */
- (NSSize)computeLayout: (NSSize)proposedSize resizeChildren: (BOOL)doResize
{
  // Layouting the grid goes like this:
  // * Compute all row heights + column widths.
  // * Apply the resulting cell sizes to all attached children.
  // * Adjust the size of the container if necessary.

  // To compute all row heights and widths do:
  // 1) For each cell entry
  // 2)   Keep the expand state for all cells it covers.
  // 3)   Compute the base cell sizes for all cells it covers (e.g. for the width: take the control width and distribute
  //      it evenly over all cells covered from left to right attachment).
  // 4)   Compare the cell size with what has been computed overall so far. Replace any cell size for
  //      the control which is larger than what is stored so far by that larger value.

  // If in homogeneous mode do:
  // Find the tallest row and apply its height to all other rows so they are all at the same height.
  // Similar for columns.

  // If the sum of all rows is smaller then the current container height distribute the difference evenly
  // over all children with expand flag set.
  // Similar for columns.

  // If not in homogeneous mode do:
  // 1) If the sum of all widths is smaller than the control width then distribute the remaining
  //     space over all columns for which the expand flag is set.
  // 2) Same for all rows.

  std::vector<int> heights;
  heights.resize(mRowCount); // Default initializer sets to 0 (same for the other vectors).
  std::vector<int> widths;
  widths.resize(mColumnCount);

  std::vector<bool> verticalExpandState;
  verticalExpandState.resize(mRowCount);
  std::vector<bool> horizontalExpandState;
  horizontalExpandState.resize(mColumnCount);

  float horizontalPadding = mLeftPadding + mRightPadding;
  float verticalPadding = mTopPadding + mBottomPadding;

  if (!mHorizontalCenter)
    proposedSize.width -= horizontalPadding;
  if (!mVerticalCenter)
    proposedSize.height -= verticalPadding;
  bool useHorizontalCentering = doResize && mHorizontalCenter;
  bool useVerticalCentering = doResize && mVerticalCenter;

  NSSize newSize = { 0, 0 };

  // First round: sort list for increasing column span count, so we can process smallest entries first.
  std::sort(content.begin(), content.end(), [](const CellEntry &lhs, const CellEntry &rhs) {
    return (lhs.rightAttachment - lhs.leftAttachment) < (rhs.rightAttachment - rhs.leftAttachment);
  });

  // Go for each cell entry and apply its preferred size to the proper cells,
  // after visibility state and bounds are set.
  // Keep expand states so we can apply them later.
  for (auto &entry : content)
  {
    entry.isVisible = !entry.view.isHidden && (entry.rightAttachment > entry.leftAttachment)
      && (entry.bottomAttachment > entry.topAttachment);
    if (!entry.isVisible)
      continue;

    // Check if the width of the entry is larger than what we have already.
    // While we are at it, keep the expand state in the associated cells.
    // However, if the current entry is expanding and covers a column which is already set to expand
    // then don't apply expansion, as we only want to expand those columns.
    int currentWidth = 0;
    bool doExpand = entry.horizontalExpand;
    if (doExpand)
    {
      useHorizontalCentering = false; // Expansion disables auto centering.
      for (int i = entry.leftAttachment; i < entry.rightAttachment; i++)
      {
        if (horizontalExpandState[i])
        {
          doExpand = false;
          break;
        }
      }
    }
    for (int i = entry.leftAttachment; i < entry.rightAttachment; i++)
    {
      currentWidth += widths[i];
      if (doExpand)
        horizontalExpandState[i] = true;
    }

    entry.view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    entry.frame.origin = NSMakePoint(0, 0);
    entry.frame.size = [entry.view preferredSize: NSMakeSize(currentWidth, 0)];

    // Ensure integral sizes.
    entry.frame.size.width = ceil(entry.frame.size.width);
    entry.frame.size.height = ceil(entry.frame.size.height);

    // Set all cells to the computed partial size if it is larger than what was found so far.
    // On the way apply the expand flag to all cells that are covered by that entry.

    // If the width of the entry is larger then distribute the difference to all cells it covers.
    if (entry.frame.size.width > currentWidth)
    {
      // The fraction is a per-cell value and computed by an integer div (we cannot add partial pixels).
      // Hence we might have a rest, which is less than the span size. Distribute this rest over all spanned cell too.
      int fraction = (entry.frame.size.width - currentWidth) / (entry.rightAttachment - entry.leftAttachment);
      int rest = int(entry.frame.size.width - currentWidth) % int(entry.rightAttachment - entry.leftAttachment);

      for (int i = entry.leftAttachment; i < entry.rightAttachment; i++)
      {
        widths[i] += fraction;
        if (rest > 0)
        {
          widths[i]++;
          rest--;
        }
      }
    }
  }

  // Once we got the minimal width we need to compute the real width as the height computation depends
  // on the final column widths (e.g. for wrapping labels that change their height depending on their width).
  // Handle homogeneous mode.
  if (mHomogeneous)
  {
    int max= 0;
    for (int i= 0; i < mColumnCount; i++)
      if (widths[i] > max)
        max = widths[i];

    for (int i= 0; i < mColumnCount; i++)
      widths[i] = max;
  }

  // Compute overall width and handle expanded entries.
  for (int i= 0; i < mColumnCount; i++)
    newSize.width += widths[i];
  newSize.width += (mColumnCount - 1) * mColumnSpacing;

  // Do auto sizing the table if enabled. Apply minimal bounds in any case.
  if (newSize.width > proposedSize.width || ((self.autoresizingMask & NSViewWidthSizable) != 0 && !useHorizontalCentering))
  {
    proposedSize.width = newSize.width;
    if (proposedSize.width < self.minimumSize.width - horizontalPadding)
      proposedSize.width = self.minimumSize.width - horizontalPadding;
  }

  // Handle expansion of cells.
  //if (doResize)
  {
    if (!useHorizontalCentering && (newSize.width < proposedSize.width))
    {
      int expandCount = 0;
      for (int i = 0; i < mColumnCount; i++)
        if (horizontalExpandState[i])
          expandCount++;
      if (expandCount > 0)
      {
        int fraction= (proposedSize.width - newSize.width) / expandCount;
        for (int i= 0; i < mColumnCount; i++)
          if (horizontalExpandState[i])
            widths[i] += fraction;
      }

      // Re-compute again the overall width.
      newSize.width = 0;
      for (int i= 0; i < mColumnCount; i++)
        newSize.width += widths[i];
      newSize.width += (mColumnCount - 1) * mColumnSpacing;
    }
  }

  // Second round: Now that we have all final widths compute the heights. Start with sorting the entries
  // list for increasing row span count, so we can process smallest entries first.
  std::sort(content.begin(), content.end(), [](const CellEntry &lhs, const CellEntry &rhs) {
    return (lhs.bottomAttachment - lhs.topAttachment) < (rhs.bottomAttachment - rhs.topAttachment);
  });

  // Go for each cell entry and apply its preferred size to the proper cells.
  // Keep expand states so we can apply them later.
  for (auto &entry : content)
  {
    // Visibility state and bounds where already determined in the first round.
    if (!entry.isVisible)
      continue;

    // Set all cells to the computed partial size if it is larger than what was found so far.
    // On the way apply the expand flag to all cells that are covered by that entry.

    // Check if the height of the entry is larger than what we have already.
    // Same expansion handling here as for horizontal expansion.
    int currentHeight = 0;
    bool doExpand = entry.verticalExpand;
    if (doExpand)
    {
      useVerticalCentering = false; // Expansion disables auto centering.
      for (int i = entry.topAttachment; i < entry.bottomAttachment; i++)
      {
        if (verticalExpandState[i])
        {
          doExpand = false;
          break;
        }
      }
    }
    for (int i = entry.topAttachment; i < entry.bottomAttachment; i++)
    {
      currentHeight += heights[i];
      if (doExpand)
        verticalExpandState[i]= true;
    }

    // For controls that change height depending on their width we need another preferred size computation.
    // Currently this applies only for wrapping labels.
    if ([entry.view isKindOfClass: MFLabelImpl.class] && [[(MFLabelImpl*)entry.view cell] wraps])
    {
      int currentWidth = 0;
      for (int i= entry.leftAttachment; i < entry.rightAttachment; i++)
        currentWidth += widths[i];
      entry.frame.size = [entry.view preferredSize: NSMakeSize(currentWidth, 0)];

      // Ensure integral sizes.
      entry.frame.size.width = ceil(entry.frame.size.width);
      entry.frame.size.height = ceil(entry.frame.size.height);
    }

    // If the height of the entry is larger then distribute the difference to all cells it covers.
    if (entry.frame.size.height > currentHeight)
    {
      int fraction = (entry.frame.size.height - currentHeight) / (entry.bottomAttachment - entry.topAttachment);
      int rest = int(entry.frame.size.height - currentHeight) % int(entry.bottomAttachment - entry.topAttachment);

      for (int i = entry.topAttachment; i < entry.bottomAttachment; i++)
      {
        heights[i] += fraction;
        if (rest > 0)
        {
          heights[i]++;
          rest--;
        }
      }
    }
  }

  // Handle homogeneous mode.
  if (mHomogeneous)
  {
    int max = 0;
    for (int i = 0; i < mRowCount; i++)
      if (heights[i] > max)
        max = heights[i];

    for (int i = 0; i < mRowCount; i++)
      heights[i] = max;
  }

  // Compute overall size and handle expanded entries.
  for (int i = 0; i < mRowCount; i++)
    newSize.height += heights[i];
  newSize.height += (mRowCount - 1) * mRowSpacing;

  // Do auto sizing the table if enabled. Apply minimal bounds in any case.
  if (newSize.height > proposedSize.height || ((self.autoresizingMask & NSViewHeightSizable) != 0 && !useVerticalCentering))
  {
    proposedSize.height = newSize.height;
    if (proposedSize.height < self.minimumSize.height - verticalPadding)
      proposedSize.height = self.minimumSize.height - verticalPadding;
  }

  // Handle expansion of cells (vertical case). Since this can happen only if the new size is
  // less than the proposed size it does not matter for pure size computation (in that case we
  // have already our target size). Hence do it only if we are actually layouting the table.
  if (doResize)
  {
    if (!useVerticalCentering && (newSize.height < proposedSize.height))
    {
      int expandCount = 0;
      for (int i = 0; i < mRowCount; i++)
        if (verticalExpandState[i])
          expandCount++;
      if (expandCount > 0)
      {
        int fraction = (proposedSize.height - newSize.height) / expandCount;
        for (int i = 0; i < mRowCount; i++)
          if (verticalExpandState[i])
            heights[i] += fraction;
      }
    }

    // Compute target bounds from cell sizes. Compute one more column/row used as right/bottom border.
    std::vector<int> rowStarts;
    rowStarts.resize(mRowCount + 1);
    
    rowStarts[0] = mTopPadding;
    if (useVerticalCentering && (newSize.height < proposedSize.height))
      rowStarts[0] = ceil((proposedSize.height - newSize.height) / 2);

    for (int i = 1; i <= mRowCount; i++)
      rowStarts[i] = rowStarts[i - 1] + heights[i - 1] + mRowSpacing;

    std::vector<int> columnStarts;
    columnStarts.resize(mColumnCount + 1);

    columnStarts[0] = mLeftPadding;
    if (useHorizontalCentering && (newSize.width < proposedSize.width))
      columnStarts[0] = ceil((proposedSize.width - newSize.width) / 2);

    for (int i = 1; i <= mColumnCount; i++)
      columnStarts[i] = columnStarts[i - 1] + widths[i - 1] + mColumnSpacing;

    for (auto &entry : content)
    {
      if (!entry.isVisible)
        continue;

      entry.frame.origin.x = columnStarts[entry.leftAttachment];
      entry.frame.origin.y = rowStarts[entry.topAttachment];
      entry.frame.size.width = columnStarts[entry.rightAttachment] - columnStarts[entry.leftAttachment] - mColumnSpacing;
      entry.frame.size.height = rowStarts[entry.bottomAttachment] - rowStarts[entry.topAttachment] - mRowSpacing;
    }

    // Apply target bounds to cell content.
    applyBounds(content);
  }

  if (!mHorizontalCenter)
    proposedSize.width += horizontalPadding;
  if (!mVerticalCenter)
    proposedSize.height += verticalPadding;

  return proposedSize;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSSize)preferredSize: (NSSize)proposedSize
{
  self.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
  return [self computeLayout: proposedSize resizeChildren: NO];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)resizeSubviewsWithOldSize: (NSSize)oldBoundsSize
{
  if (mOwner != nullptr && !mOwner->is_destroying())
  {
    self.autoresizingMask = 0;
    [self computeLayout: self.frame.size resizeChildren: YES];
  }
}

//----------------------------------------------------------------------------------------------------------------------

#pragma mark - Management

- (void)addSubview: (NSView*)view left: (int)left right: (int)right top: (int)top bottom: (int)bottom flags: (mforms::TableItemFlags)flags
{
  if (left < 0 || left >= mColumnCount || right < left || right > mColumnCount)
    throw std::invalid_argument("Invalid column coordinate passed to table.add()");
  if (top < 0 || top >= mRowCount || bottom < top || bottom > mRowCount)
    throw std::invalid_argument("Invalid row coordinate passed to table.add()");
  
  [self addSubview: view];
    
  CellEntry cell;
  cell.view = view;
  cell.leftAttachment = left;
  cell.rightAttachment = right;
  cell.topAttachment = top;
  cell.bottomAttachment = bottom;
  cell.verticalExpand = flags & mforms::VExpandFlag ? true : false;
  cell.horizontalExpand = flags & mforms::HExpandFlag ? true : false;
  cell.verticalFill = flags & mforms::VFillFlag ? true : false;
  cell.horizontalFill = flags & mforms::HFillFlag ? true : false;
  
  content.push_back(cell);
  
  // auto-adjust row/column count if necessary
  if (mColumnCount < right - 1)
    mColumnCount = right;
  if (mRowCount < bottom - 1)
    mRowCount = bottom;
  
  if ([self setFreezeRelayout: NO])
    [self relayout];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setRowCount:(int)count
{
  mRowCount= count;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setColumnCount:(int)count
{
  mColumnCount= count;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setPaddingLeft: (float)lpad right: (float)rpad top: (float)tpad bottom: (float)bpad
{
  mLeftPadding = lpad;
  mRightPadding = rpad;
  mTopPadding = tpad;
  mBottomPadding = bpad;

  mHorizontalCenter = (lpad < 0 || rpad < 0);
  mVerticalCenter = (tpad < 0 && bpad < 0);
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityTableRole;
}

//----------------------------------------------------------------------------------------------------------------------

static bool table_create(::mforms::Table *self)
{
  return [[MFTableImpl alloc] initWithObject: self] != nil;
}

//----------------------------------------------------------------------------------------------------------------------

static void table_set_homogeneous(::mforms::Table *self, bool flag)
{
  if ( self )
  {
    MFTableImpl* table = self->get_data();
    
    if ( table )
    {
      [table setHomogeneous: flag];
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void table_set_row_spacing(::mforms::Table *self, int spacing)
{
  if ( self )
  {
    MFTableImpl* table = self->get_data();
    
    if ( table )
    {
      [table setRowSpacing:spacing];
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void table_set_column_spacing(::mforms::Table *self, int spacing)
{
  if ( self )
  {
    MFTableImpl* table = self->get_data();
    
    if ( table )
    {
      [table setColumnSpacing:spacing];
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void table_set_row_count(::mforms::Table *self, int count)
{
  if ( self )
  {
    MFTableImpl* table = self->get_data();
    
    if ( table )
    {
      [table setRowCount:count];
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void table_set_column_count(::mforms::Table *self, int count)
{
  if ( self )
  {
    MFTableImpl* table = self->get_data();
    
    if ( table )
    {
      [table setColumnCount:count];
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void table_add(::mforms::Table *self, ::mforms::View *child, int left, int right, int top, int bottom, int flags)
{
  NSView *view = child->get_data();
  view.viewFlags = ViewFlags(view.viewFlags | flags);
  
  [self->get_data() addSubview: view
                          left: left
                         right: right
                           top: top
                        bottom: bottom
                         flags: (mforms::TableItemFlags)flags];
}

//----------------------------------------------------------------------------------------------------------------------

static void table_remove(::mforms::Table *self, ::mforms::View *child)
{
  [child->get_data() removeFromSuperview];
}

//----------------------------------------------------------------------------------------------------------------------

void cf_table_init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();
  
  f->_table_impl.create= &table_create;
  f->_table_impl.set_row_spacing= &table_set_row_spacing;
  f->_table_impl.set_column_spacing= &table_set_column_spacing;
  f->_table_impl.set_row_count= &table_set_row_count;
  f->_table_impl.set_column_count= &table_set_column_count;
  f->_table_impl.add= &table_add;
  f->_table_impl.remove= &table_remove;
  f->_table_impl.set_homogeneous= &table_set_homogeneous;
}

@end

//----------------------------------------------------------------------------------------------------------------------
