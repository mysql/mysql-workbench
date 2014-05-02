/* 
 * Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
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



#import "MFTable.h"
#import "MFMForms.h"


@interface MFTableCell : NSObject
{
@public
  NSView *mView;
  int mLeftAttachment;
  int mRightAttachment;
  int mTopAttachment;
  int mBottomAttachment;
  BOOL mHorizontalExpand;
  BOOL mVerticalExpand;
  BOOL mHorizontalFill;
  BOOL mVerticalFill;
}
@end

@implementation MFTableCell
@end




@implementation MFTableImpl

- (id)initWithObject:(::mforms::Table*)aTable
{
  self= [super initWithFrame:NSMakeRect(0,0,1,1)];
  if (self)
  {
    mOwner = aTable;
    mOwner->set_data(self);
    
    mTableCells= [[NSMutableArray array] retain];
  }
  return self;
}

- (mforms::Object*)mformsObject
{
  return mOwner;
}


- (void)dealloc
{
  [mTableCells release];
  mTableCells= nil;
  [super dealloc];
}

//--------------------------------------------------------------------------------------------------

STANDARD_MOUSE_HANDLING(self) // Add handling for mouse events.

//--------------------------------------------------------------------------------------------------

- (BOOL)isFlipped
{
  return YES;
}


- (void)setHomogeneous:(bool)flag
{
  mHomogeneous= flag;
}


- (void)setRowSpacing:(float)spacing
{
  mRowSpacing= spacing;
}

- (void)setColumnSpacing:(float)spacing
{
  mColumnSpacing= spacing;
}

#if 0
- (void)drawRect:(NSRect)rect
{
  //[[NSColor redColor] set];
  //NSFrameRect([self frame]);
  
  [[NSColor blueColor] set];
  NSFrameRect(NSInsetRect([self bounds], 1, 1));
  
//  [[NSColor purpleColor] set];
//  NSFrameRect(NSInsetRect([self frame], 5, 5));
  
  [[NSColor orangeColor] set];
  for (id view in [self subviews])
  {
    NSFrameRect([view frame]);
  }
}
#endif
 

- (NSSize)minimumSize
{
  if (mOwner == NULL || mOwner->is_destroying())
    return NSZeroSize;
  
  if (mRowCount > 0 && mColumnCount > 0)
  {
    int widths[mColumnCount];
    int heights[mRowCount];
    
    for (int i= 0; i < mRowCount; i++)
      heights[i]= 0;
    for (int i= 0; i < mColumnCount; i++)
      widths[i]= 0;
    
    // Go for each cell entry and apply its preferred size to the proper cells,
    // after visibility state and bounds are set.
    // Keep expand states so we can apply them later.
    for (MFTableCell *cell in mTableCells)
    {
      NSSize cellSize;

      if ([cell->mView isHidden]) continue;

      cellSize= [cell->mView preferredSize];

      int widthPerCell= cellSize.width / (cell->mRightAttachment - cell->mLeftAttachment);
      int heightPerCell= cellSize.height / (cell->mBottomAttachment - cell->mTopAttachment);
      
      for (int i= cell->mLeftAttachment; i < cell->mRightAttachment; i++)
      {
        if (widthPerCell > widths[i])
          widths[i]= widthPerCell;
      }
      for (int i= cell->mTopAttachment; i < cell->mBottomAttachment; i++)
      {
        if (heightPerCell > heights[i])
          heights[i]= heightPerCell;
      }
    }
    
    // Handle homogeneous mode.
    if (mHomogeneous)
    {
      int max= 0;
      for (int i= 0; i < mRowCount; i++)
        if (heights[i] > max)
          max= heights[i];
      for (int i= 0; i < mRowCount; i++)
        heights[i]= max;
      
      max= 0;
      for (int i= 0; i < mColumnCount; i++)
        if (widths[i] > max)
          max= widths[i];      
      for (int i= 0; i < mColumnCount; i++)
        widths[i]= max;
    }
    
    // Compute overall size
    NSSize minSize= NSMakeSize(mLeftPadding + mRightPadding, mTopPadding + mBottomPadding);
    for (int i= 0; i < mRowCount; i++)
    {
      minSize.height += heights[i];
    }
    minSize.height += (mRowCount - 1) * mRowSpacing;
    for (int i= 0; i < mColumnCount; i++)
      minSize.width += widths[i];
    minSize.width += (mColumnCount - 1) * mColumnSpacing;
    
    return minSize;
  }
  return NSMakeSize(mLeftPadding + mRightPadding, mTopPadding + mBottomPadding);
}


- (void)resizeSubviewsWithOldSize:(NSSize)oldBoundsSize
{
  if (mRowCount > 0 && mColumnCount > 0)
  {
    // adapted from wf_table.cpp
    
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
    // over all childs with expand flag set.
    // Similar for columns.
    
    // If not in homogeneous mode do:
    // 1) If the sum of all widths is smaller than the control width then distribute the remaining
    //     space over all columns for which the expand flag is set.
    // 2) Same for all rows.
    
    int heights[mRowCount];
    for (int i= 0; i < mRowCount; i++)
      heights[i]= 0;
    int widths[mColumnCount];
    for (int i= 0; i < mColumnCount; i++)
      widths[i]= 0;
    
    BOOL verticalExpandState[mRowCount];
    for (int i= 0; i < mRowCount; i++)
      verticalExpandState[i]= false;
    BOOL horizontalExpandState[mColumnCount];
    for (int i= 0; i < mColumnCount; i++)
      horizontalExpandState[i]= false;
    
    // Go for each cell entry and apply its preferred size to the proper cells,
    // after visibility state and bounds are set.
    // Keep expand states so we can apply them later.
    for (MFTableCell *entry in mTableCells)
    {
      NSSize entryMinSize;
      
      if ([entry->mView isHidden])
          continue;
      
      entryMinSize= [entry->mView preferredSize];
      
      int widthPerCell= entryMinSize.width / (entry->mRightAttachment - entry->mLeftAttachment);
      int heightPerCell= entryMinSize.height / (entry->mBottomAttachment - entry->mTopAttachment);
      
      // Set all cells to the computed partial size if it is larger than what was found so far.
      // On the way apply the expand flag to all cells that are covered by that entry.
      
      // Expanding cells is only enabled if there is no other entry with only one cell size
      // which is expanded. In this case this single cell will get expanded only.
      BOOL canExpandHorizontally= entry->mHorizontalExpand;
      if (canExpandHorizontally && (entry->mRightAttachment - entry->mLeftAttachment > 1))
      {
        // Check if there is at least one single-cell entry that is expanded and which is
        // within the attachment range of this entry.
        for (MFTableCell* otherEntry in mTableCells)
          if ((otherEntry->mRightAttachment - otherEntry->mLeftAttachment == 1) &&
              (otherEntry->mLeftAttachment >= entry->mLeftAttachment) &&
              (otherEntry->mRightAttachment <= entry->mRightAttachment))
          {
            canExpandHorizontally= NO;
            break;
          }
      }
      for (int i= entry->mLeftAttachment; i < entry->mRightAttachment; i++)
      {
        if (canExpandHorizontally)
          horizontalExpandState[i]= YES;
        if (widthPerCell > widths[i])
          widths[i]= widthPerCell;
      }
      
      BOOL canExpandVertically= entry->mVerticalExpand;
      if (canExpandVertically && (entry->mBottomAttachment - entry->mTopAttachment > 1))
      {
        // Check if there is at least one single-cell entry that is expanded and which is
        // within the attachment range of this entry.
        for (MFTableCell* otherEntry in mTableCells)
          if ((otherEntry->mBottomAttachment - otherEntry->mTopAttachment == 1) &&
              (otherEntry->mTopAttachment >= entry->mTopAttachment) &&
              (otherEntry->mBottomAttachment <= entry->mBottomAttachment))
          {
            canExpandVertically= NO;
            break;
          }
      }
      for (int i= entry->mTopAttachment; i < entry->mBottomAttachment; i++)
      {
        if (canExpandVertically)
          verticalExpandState[i]= YES;
        if (heightPerCell > heights[i])
          heights[i]= heightPerCell;
      }
    }
    
    NSSize tableSize= [self frame].size;
    tableSize.width -= (mLeftPadding + mRightPadding);
    tableSize.height -= (mTopPadding + mBottomPadding);
    
    // Handle homogeneous mode.
    if (mHomogeneous)
    {
      int max= 0;
      for (int i= 0; i < mRowCount; i++)
        if (heights[i] > max)
          max= heights[i];
      
      // If the sum of all resized rows still does not fill the full height
      // then distribute the remaining space too to make them fill.
      if (tableSize.height > max * mRowCount)
        max = tableSize.height / mRowCount;
      for (int i= 0; i < mRowCount; i++)
        heights[i]= max;
      max= 0;
      for (int i= 0; i < mColumnCount; i++)
        if (widths[i] > max)
          max= widths[i];
      
      if (tableSize.width > max * mColumnCount)
        max = tableSize.width / mColumnCount;
      for (int i= 0; i < mColumnCount; i++)
        widths[i]= max;
    }
    
    // Compute overall size and handle expanded entries.
    NSSize newSize= NSMakeSize(0, 0);
    for (int i= 0; i < mRowCount; i++)
      newSize.height += heights[i];
    newSize.height += (mRowCount - 1) * mRowSpacing;
    for (int i= 0; i < mColumnCount; i++)
      newSize.width += widths[i];
    newSize.width += (mColumnCount - 1) * mColumnSpacing;
    
    
    // Handle expansion of cells, which only applies to the table if it is not set to homogeneous mode.
    // The following test will also fail if homogeneous mode is set because the cell sizes 
    // have been adjusted already to fill the entire table.
    if (newSize.width < tableSize.width)
    {
      int expandCount= 0;
      for (int i= 0; i < mColumnCount; i++)
        if (horizontalExpandState[i])
          expandCount++;
      if (expandCount > 0)
      {
        int fraction= (tableSize.width - newSize.width) / expandCount;
        for (int i= 0; i < mColumnCount; i++)
          if (horizontalExpandState[i])
            widths[i] += fraction;
      }
    }
    
    if (newSize.height < tableSize.height)
    {
      int expandCount= 0;
      for (int i= 0; i < mRowCount; i++)
        if (verticalExpandState[i])
          expandCount++;
      if (expandCount > 0)
      {
        int fraction= (tableSize.height - newSize.height) / expandCount;
        for (int i= 0; i < mRowCount; i++)
          if (verticalExpandState[i])
            heights[i] += fraction;
      }
    }
    
    // Compute target bounds from cell sizes. Compute one more column/row used as right/bottom border.
    int rowStarts[mRowCount + 1];
    if (mCenterContents && tableSize.height > newSize.height)
      rowStarts[0]= (tableSize.height - newSize.height) / 2;
    else
      rowStarts[0]= mTopPadding;
    for (int i= 1; i <= mRowCount; i++)
      rowStarts[i]= rowStarts[i - 1] + heights[i - 1] + mRowSpacing;
    
    int columnStarts[mColumnCount + 1];
    if (mCenterContents && tableSize.width > newSize.width)
      columnStarts[0]= (tableSize.width - newSize.width) / 2;
    else
      columnStarts[0]= mLeftPadding;
    for (int i= 1; i <= mColumnCount; i++)
      columnStarts[i]= columnStarts[i - 1] + widths[i - 1] + mColumnSpacing;
    
    // Apply target bounds to cell content.
    for (MFTableCell *entry in mTableCells)
    {
      if ([entry->mView isHidden])
        continue;
      
      NSRect entryBounds;
      NSSize availSize;
      entryBounds.origin.x= columnStarts[entry->mLeftAttachment];
      entryBounds.origin.y= rowStarts[entry->mTopAttachment];
      entryBounds.size.width= columnStarts[entry->mRightAttachment] - columnStarts[entry->mLeftAttachment] - mColumnSpacing;
      entryBounds.size.height= rowStarts[entry->mBottomAttachment] - rowStarts[entry->mTopAttachment]- mRowSpacing;
      availSize = entryBounds.size;

      if ([entry->mView widthIsFixed])
        entryBounds.size.width= NSWidth([entry->mView frame]);
      if ([entry->mView heightIsFixed])
        entryBounds.size.height= NSHeight([entry->mView frame]);
      
      if (!entry->mVerticalFill && [entry->mView heightIsFixed])
        entryBounds.origin.y += (availSize.height - NSHeight(entryBounds))/2;
      if (!entry->mHorizontalFill && [entry->mView widthIsFixed])
        entryBounds.origin.x += (availSize.width - NSWidth(entryBounds))/2;
      
      if (NSEqualRects([entry->mView frame], entryBounds))
        [entry->mView resizeSubviewsWithOldSize: entryBounds.size];
      else
        [entry->mView setFrame:entryBounds];
    }
  }
}


- (void)willRemoveSubview:(NSView *)subview
{
  for (MFTableCell *cell in mTableCells)
  {
    if (cell->mView == subview)
    {
      [mTableCells removeObject:cell];
      break;
    }
  }
  [self subviewMinimumSizeChanged];
}


- (void)subviewMinimumSizeChanged
{
  if (!mOwner->is_destroying())
    [super subviewMinimumSizeChanged];
}


- (void)addSubview:(NSView*)view left:(int)left right:(int)right top:(int)top bottom:(int)bottom flags:(mforms::TableItemFlags)flags
{
  if (left < 0 || left >= mColumnCount || right < left || right > mColumnCount)
    throw std::invalid_argument("Invalid column coordinate passed to table.add()");
  if (top < 0 || top >= mRowCount || bottom < top || bottom > mRowCount)
    throw std::invalid_argument("Invalid row coordinate passed to table.add()");
  
  [self addSubview:view];
    
  MFTableCell *cell= [[[MFTableCell alloc] init] autorelease];
  
  cell->mView= view;
  cell->mLeftAttachment= MAX(left, 0);
  cell->mRightAttachment= right;
  cell->mTopAttachment= MAX(top, 0);
  cell->mBottomAttachment= bottom;
  cell->mVerticalExpand= flags & mforms::VExpandFlag ? YES : NO;
  cell->mHorizontalExpand= flags & mforms::HExpandFlag ? YES : NO;
  cell->mVerticalFill= flags & mforms::VFillFlag ? YES : NO;
  cell->mHorizontalFill= flags & mforms::HFillFlag ? YES : NO;
  
  [mTableCells addObject:cell];
  
  // auto-adjust row/column count if necessary
  if (mColumnCount < right-1)
    mColumnCount= right;
  if (mRowCount < bottom-1)
    mRowCount= bottom;
  
  [self subviewMinimumSizeChanged];
}


- (void)setRowCount:(int)count
{
  mRowCount= count;
}

- (void)setColumnCount:(int)count
{
  mColumnCount= count;
}

- (void)setPaddingLeft:(float)lpad right:(float)rpad top:(float)tpad bottom:(float)bpad
{
  if (lpad < 0 && rpad < 0 && tpad < 0 && bpad < 0)
  {
    mLeftPadding= 0;
    mRightPadding= 0;
    mTopPadding= 0;
    mBottomPadding= 0;
    mCenterContents = YES;
  }
  else
  {
    mLeftPadding= lpad;
    mRightPadding= rpad;
    mTopPadding= tpad;
    mBottomPadding= bpad;
    mCenterContents = NO;
  }
}

static bool table_create(::mforms::Table *self)
{
  [[[MFTableImpl alloc] initWithObject:self] autorelease];
    
  return true;  
}


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


static void table_add(::mforms::Table *self, ::mforms::View *child, int left, int right, int top, int bottom, int flags)
{
  NSView *view = child->get_data();
  view.viewFlags |= flags;
  
  [self->get_data() addSubview: view
                          left: left
                         right: right
                           top: top
                        bottom: bottom
                         flags: (mforms::TableItemFlags)flags];
}


static void table_remove(::mforms::Table *self, ::mforms::View *child)
{
  [child->get_data() removeFromSuperview];
}


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
