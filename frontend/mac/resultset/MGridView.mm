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


#import "MGridView.h"
#include "recordset_be.h"

#import "MCPPUtilities.h"
#include "mforms/menubar.h"

static std::vector<int> get_indexes(NSIndexSet *iset, NSInteger clickedRow);

@implementation MGridView

- (void)setRecordset:(Recordset*)rset
{
  mRecordset = rset;
}


- (BOOL)becomeFirstResponder
{
  [self setNeedsDisplay: YES];
  return [super becomeFirstResponder];
}


- (BOOL)resignFirstResponder
{
  [self setNeedsDisplay: YES];
  return [super resignFirstResponder];
}


- (void) mouseDown: (NSEvent*) event;
{
  NSPoint localPoint = [self convertPoint: [event locationInWindow]
                                 fromView: nil];
  NSInteger column= [self columnAtPoint: localPoint];
  
  if (column <= 0 || ([event modifierFlags] & (NSShiftKeyMask | NSCommandKeyMask)))
    // if dragging from indicator, multi-row selection is OK
    [self setAllowsMultipleSelection: YES];
  else
    [self setAllowsMultipleSelection: NO];
  
  mOSelectedRowIndex = mSelectedRowIndex;
  mOSelectedColumnIndex = mSelectedColumnIndex;
  
  if (column < 0)
    mSelectedColumnIndex = [self numberOfColumns] - 1;
  else
    mSelectedColumnIndex= column;
  mSelectedRowIndex = [self rowAtPoint: localPoint];
  if (mSelectedRowIndex < 0)
    [self deselectAll: nil];
  
  if (selectionChangedActionTarget)
    [selectionChangedActionTarget performSelector:selectionChangedAction];

  [self setNeedsDisplay: YES];
  
  [super mouseDown: event];
}


- (void)copy:(id)sender
{
  if (mRecordset)
  {
    if ([self selectedColumnIndex] > 0)
      mRecordset->copy_field_to_clipboard([self selectedRowIndex], [self selectedColumnIndex]-1, false);
    else
      mRecordset->copy_rows_to_clipboard(get_indexes([self selectedRowIndexes], [self selectedRowIndex]), ",", true);
  }
}


- (BOOL)isEditable
{
  return mRecordset && !mRecordset->is_readonly();
}

- (void)paste:(id)sender
{
  if (mRecordset)
    mRecordset->paste_rows_from_clipboard([self selectedRowIndex]);
}


- (void)selectAll:(id)sender
{
  mSelectedColumnIndex = 0;
  [self setAllowsMultipleSelection: YES];
  [super selectAll: sender];
  [self setNeedsDisplay: YES];
}


- (void)editColumn:(NSInteger)columnIndex row:(NSInteger)rowIndex withEvent:(NSEvent *)theEvent select:(BOOL)flag
{
  [super editColumn: columnIndex row: rowIndex withEvent: theEvent select: flag];
  [self selectRowIndexes: [NSIndexSet indexSetWithIndex: rowIndex]
    byExtendingSelection: NO];
}


- (void)selectRowIndexes:(NSIndexSet *)indexes byExtendingSelection:(BOOL)extend
{
  if ([indexes count] > 1)
    mSelectedColumnIndex = -1;
  
  if (mSelectedColumnIndex <= 0)
  {
    [super selectRowIndexes: indexes byExtendingSelection: extend];
    mSelectedRowIndex = [indexes lastIndex];
  }
  else
  {
    // if there's a column selected, then there can only be a single cell selected
    [super selectRowIndexes: indexes byExtendingSelection: NO];
    if ([indexes count] > 0)
      mSelectedRowIndex = [indexes lastIndex];
    else
      mSelectedRowIndex = -1;
    NSPoint localPoint = [self convertPoint: [[NSApp currentEvent] locationInWindow]
                                   fromView: nil];
    NSInteger column = [self columnAtPoint: localPoint];
    if (column > 0 && column < [self numberOfColumns])
      mSelectedColumnIndex = column;
  }
  
  if (selectionChangedActionTarget)
    [selectionChangedActionTarget performSelector:selectionChangedAction];
}


- (void)rightMouseDown: (NSEvent*)event
{
  if ([[self selectedRowIndexes] count] == 1)
  {
    NSPoint localPoint = [self convertPoint: [event locationInWindow]
                                   fromView: nil];
    NSInteger row = [self rowAtPoint: localPoint];
    
    [self selectRowIndexes: [NSIndexSet indexSetWithIndex: row]
      byExtendingSelection: NO];
    [self setNeedsDisplay: YES];
  }
  [super rightMouseDown: event];
}

/* unneeded
- (void) mouseUp: (NSEvent*) event;
{
  NSPoint localPoint = [self convertPoint: [event locationInWindow]
                                 fromView: nil];
  NSInteger column= [self columnAtPoint: localPoint];
  if (column < 0)
    mSelectedColumnIndex = [self numberOfColumns] - 1;
  else
    mSelectedColumnIndex= column;
  mSelectedRowIndex = [self rowAtPoint: localPoint];
  
  if (selectionChangedActionTarget)
    [selectionChangedActionTarget performSelector:selectionChangedAction];
  
  [self setNeedsDisplay: YES];
  
  if (column >= 0 && mSelectedRowIndex == mOSelectedRowIndex && mSelectedColumnIndex == mOSelectedColumnIndex)
  {
    if (mSelectedRowIndex >= 0 && mSelectedColumnIndex >= 0 &&
        [[self delegate] respondsToSelector: @selector(tableView:shouldEditTableColumn:row:)] &&      
        [[self delegate] tableView: self
             shouldEditTableColumn: [[self tableColumns] objectAtIndex: mSelectedColumnIndex]
                               row: mSelectedRowIndex])
      [self editColumn: mSelectedColumnIndex
                   row: mSelectedRowIndex
             withEvent: event
                select: YES];
  }
  
  [super mouseUp: event];
}*/


- (void)selectCellAtRow:(int)row column:(int)column
{
  mSelectedColumnIndex = column;
  [self selectRowIndexes: [NSIndexSet indexSetWithIndex: row]
    byExtendingSelection: (column <= 0)];
  [self setNeedsDisplay:YES];
}


- (void) keyDown: (NSEvent*) event;
{
  unsigned short key = [event keyCode];
 
  switch (key)
  {
    case 36: // Return
    case 76: // Enter
      if (mSelectedRowIndex >= 0 && mSelectedColumnIndex > 0 &&
        [[self delegate] tableView: self
             shouldEditTableColumn: [self tableColumns][mSelectedColumnIndex]
                               row: mSelectedRowIndex])
      {
        // Start edit.
        [self editColumn: mSelectedColumnIndex
                     row: mSelectedRowIndex
               withEvent: event
                  select: YES];
        return;
      }
      break;
    case 48: // Tab (only called when there's no editing going on)
    {
      NSUInteger modifiers = [event modifierFlags];
      bool shift = (modifiers & NSShiftKeyMask) != 0;

      if (shift)
      {
        // Move backward.
        mSelectedColumnIndex--;
        if (mSelectedColumnIndex < 1) // Column 0 is only the current-row-indicator.
        {
          // Continue on the previous line if we reached the beginning of the current line.
          if (mSelectedRowIndex > 0)
          {
            [self selectRowIndexes: [NSIndexSet indexSetWithIndex: mSelectedRowIndex-1]
              byExtendingSelection: NO];
            mSelectedColumnIndex = [self numberOfColumns] - 1;
          }
          else
            mSelectedColumnIndex++; // Restore previous column index. We cannot move further.
        }
        mSelectedColumnIndex = MIN(mSelectedColumnIndex, [self numberOfColumns] - 1);
      }
      else
      {
        // Move forward.
        mSelectedColumnIndex++;
        if (mSelectedColumnIndex == [self numberOfColumns])
        {
          // Continue on the next line if we reached the end of the current line.
          if (mSelectedRowIndex < [self numberOfRows] - 1)
          {
            [self selectRowIndexes: [NSIndexSet indexSetWithIndex: mSelectedRowIndex+1]
              byExtendingSelection: NO];
            mSelectedColumnIndex = 1;
          }
          else
            mSelectedColumnIndex--; // Restore previous column index. We cannot move further.
        }
        mSelectedColumnIndex = MIN(mSelectedColumnIndex, [self numberOfColumns] - 1);
      }
      
      if (selectionChangedActionTarget)
        [selectionChangedActionTarget performSelector:selectionChangedAction];
      
      break;
    }
    case 123: // Left
      mSelectedColumnIndex -= 1;
      mSelectedColumnIndex = MAX(mSelectedColumnIndex, 1);
      if (selectionChangedActionTarget)
        [selectionChangedActionTarget performSelector:selectionChangedAction];
      break;
    case 124: // Right
      mSelectedColumnIndex += 1;
      mSelectedColumnIndex = MIN(mSelectedColumnIndex, [self numberOfColumns] - 1);
      if (selectionChangedActionTarget)
        [selectionChangedActionTarget performSelector:selectionChangedAction];
      break;
    case 125: // Down
      [self selectRowIndexes: [NSIndexSet indexSetWithIndex: MIN(mSelectedRowIndex+1, [self numberOfRows]-1)]
        byExtendingSelection: NO];
      if (selectionChangedActionTarget)
        [selectionChangedActionTarget performSelector:selectionChangedAction];
      break;
    case 121: // PgDown
      if (selectionChangedActionTarget)
        [selectionChangedActionTarget performSelector:selectionChangedAction];
      [super keyDown: event]; // let original handler do the page down
      mSelectedRowIndex = [self rowAtPoint: NSMakePoint(0, NSMaxY([self visibleRect])-[self rowHeight]/3)];
      if (mSelectedRowIndex < 0)
        mSelectedRowIndex = [self numberOfRows] - 1;
      [self selectRowIndexes: [NSIndexSet indexSetWithIndex: mSelectedRowIndex]
        byExtendingSelection: NO];
      break;
    case 126: // Up
      if (selectionChangedActionTarget)
        [selectionChangedActionTarget performSelector:selectionChangedAction];
      [self selectRowIndexes: [NSIndexSet indexSetWithIndex: MAX(mSelectedRowIndex-1, 0)]
        byExtendingSelection: NO];
      break;
    case 116: // PgUp
      if (selectionChangedActionTarget)
        [selectionChangedActionTarget performSelector:selectionChangedAction];
      [super keyDown: event]; // let original handler do the page up
      mSelectedRowIndex = [self rowAtPoint: NSMakePoint(0, NSMinY([self visibleRect])+[self rowHeight]/3)];
      [self selectRowIndexes: [NSIndexSet indexSetWithIndex: mSelectedRowIndex]
        byExtendingSelection: NO];
      break;
    case 51: // Backspace
      if (mSelectedRowIndex >= 0 && mSelectedColumnIndex > 0 &&
          [[self delegate] tableView: self
               shouldEditTableColumn: [self tableColumns][mSelectedColumnIndex]
                                 row: mSelectedRowIndex])
      {
        [[self dataSource] tableView: self
                      setObjectValue: nil
                      forTableColumn: [self tableColumns][mSelectedColumnIndex]
                                 row: mSelectedRowIndex];
      }
      break;
    default:
      [super keyDown: event];
      break;
  }
  
  if (key != 121 && key != 116) // except for page keys
  {
    [self scrollRowToVisible: mSelectedRowIndex];
    [self scrollColumnToVisible: mSelectedColumnIndex];
  }
  [self setNeedsDisplay: YES];
}


- (void)cancelOperation:(id)sender
{
  // Abort editing if Escape pressed
  if ([self currentEditor])
  {
    [self abortEditing];
    [[self window] makeFirstResponder: self];
  }
}


- (NSCell *)preparedCellAtColumn:(NSInteger)column row:(NSInteger)row
{
  NSCell *cell = [super preparedCellAtColumn: column row: row];

  if (mSelectedColumnIndex > 0)
  {
    // clear highlight in the cell if a single cell is selected and this is not the one
    // this allows the tableview to have a grid like selection
    if (column != mSelectedColumnIndex && [cell isHighlighted]
        && row == mSelectedRowIndex)
    {
      [cell setBackgroundStyle: NSBackgroundStyleLight];
      [cell setHighlighted: NO];
    }
  }
  
  return cell;
}


- (void)textDidEndEditing:(NSNotification *)notification
{
  // Let the ancestor do its job first (e.g. write the value back if needed).
  [super textDidEndEditing: notification];

  int textMovement = [[notification userInfo][@"NSTextMovement"] intValue];
  if (textMovement == NSTabTextMovement)
  {
    [[self window] makeFirstResponder: self];
    mSelectedColumnIndex++;
    if (mSelectedColumnIndex == [self numberOfColumns])
    {
      mSelectedColumnIndex = 1;
      mSelectedRowIndex++;
      if (mSelectedRowIndex > [self numberOfRows]-1)
        mSelectedRowIndex = [self numberOfRows]-1;
    }
    if (mSelectedRowIndex >= 0 && mSelectedColumnIndex >= 0 &&
        [[self delegate] tableView: self
             shouldEditTableColumn: [self tableColumns][mSelectedColumnIndex]
                               row: mSelectedRowIndex])
    {
      [self editColumn: mSelectedColumnIndex row: mSelectedRowIndex withEvent: [NSApp currentEvent] select: YES];
    }
  }
  else if (textMovement == NSBacktabTextMovement)
  {
    [[self window] makeFirstResponder: self];
    mSelectedColumnIndex--;
    if (mSelectedColumnIndex == 0)
    {
      mSelectedColumnIndex = [self numberOfColumns]-1;
      mSelectedRowIndex--;
      if (mSelectedRowIndex < 0)
        mSelectedRowIndex = 0;
    }
    if (mSelectedRowIndex >= 0 && mSelectedColumnIndex >= 0 &&
        [[self delegate] tableView: self
             shouldEditTableColumn: [self tableColumns][mSelectedColumnIndex]
                               row: mSelectedRowIndex])
    {
      [self editColumn: mSelectedColumnIndex row: mSelectedRowIndex withEvent: [NSApp currentEvent] select: YES];
    }
  }
}

- (void)deleteBackward:(id)sender
{
  [[self dataSource] tableView: self
                setObjectValue: nil
                forTableColumn: nil
                           row: mSelectedRowIndex];
}


- (int) selectedColumnIndex;
{
  return mSelectedColumnIndex;
}



- (int) selectedRowIndex;
{
  return mSelectedRowIndex;
}


- (void)highlightSelectionInClipRect:(NSRect)clipRect
{
  // don't highlight the entire row
}


- (void)setSelectionChangedAction:(SEL)aSelector
{
  selectionChangedAction= aSelector;
}

- (void)selectionChangedActionTarget:(id)target;
{
  selectionChangedActionTarget = target;
}


static std::vector<int> get_indexes(NSIndexSet *iset, NSInteger clickedRow)
{
  std::vector<int> indexes;
  NSUInteger index = [iset firstIndex];
  while (index != NSNotFound)
  {
    indexes.push_back(index);
    index = [iset indexGreaterThanIndex: index];
  }
//  std::reverse(indexes.begin(), indexes.end());
  
  if (indexes.empty() && clickedRow >= 0)
    indexes.push_back(clickedRow);
  
  return indexes;
}


- (NSMenu *)menuForEvent:(NSEvent *)theEvent
{
  if (mRecordset)
  {      
    std::vector<int> rows = get_indexes([self selectedRowIndexes], [self selectedRowIndex]);
    mRecordset->update_selection_for_menu(rows, [self selectedColumnIndex]-1);

    return mRecordset->get_context_menu()->get_data();
  }
  return [super menuForEvent: theEvent];
}


@end


