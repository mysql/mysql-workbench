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

#import "MResultsetViewer.h"
#include "sqlide/recordset_be.h"
#import "MQResultSetCell.h"
#import "MQIndicatorCell.h"
#import "GRTIconCache.h"
#import "MCPPUtilities.h"
#import "MVerticalLayoutView.h"

#include "mforms/toolbar.h"

static NSImage *ascendingSortIndicator= nil;
static NSImage *descendingSortIndicator= nil;

@interface MResultsetViewer()
{
  NSMutableArray *nibObjects;
  NSFont *mFont;

  std::list<boost::signals2::connection> mSigConns;
  std::shared_ptr<Recordset> *mData;

  int mWarnedManyColumns;
  BOOL mPendingRefresh;
}

@end

@implementation MResultsetViewer

@synthesize view;
@synthesize gridView;

+ (void)initialize
{
  ascendingSortIndicator = [NSImage imageNamed:@"NSAscendingSortIndicator"];
  descendingSortIndicator = [NSImage imageNamed:@"NSDescendingSortIndicator"];
}

- (instancetype)init
{
  return [self initWithRecordset: std::shared_ptr<Recordset>()];
}

- (instancetype)initWithRecordset: (Recordset::Ref)rset
{
  if (!rset)
    return nil;

  self = [super init];
  if (self != nil)
  {
    NSBundle *bundle = [NSBundle bundleForClass: self.class];
    NSMutableArray *temp;
    BOOL loaded = [bundle loadNibNamed: @"WbResultsetView" owner: self topLevelObjects: &temp];
    if (loaded)
    {
      nibObjects = temp;
      mData = new Recordset::Ref();
      *mData = rset;

      [gridView setRecordset: mData->get()];

      (*mData)->update_edited_field = std::bind(selected_record_changed, (__bridge void *)self);
      (*mData)->tree_changed_signal()->connect(std::bind(onRefreshWhenIdle, (__bridge void *)self));

      (*mData)->refresh_ui_signal.connect(std::bind(onRefresh, (__bridge void *)self));
      (*mData)->rows_changed = std::bind(onRefresh, (__bridge void *)self);

      gridView.intercellSpacing = NSMakeSize(0, 1);
      gridView.actionDelegate = self;
      gridView.allowsMultipleSelection = YES;

      (gridView.enclosingScrollView).borderType = NSNoBorder;

      mforms::ToolBar *tbar = (*mData)->get_toolbar();
      if (tbar->find_item("record_edit"))
      {
        tbar->find_item("record_edit")->signal_activated()->connect(std::bind(record_edit, (__bridge void *)self));
        tbar->find_item("record_add")->signal_activated()->connect(std::bind(record_add, (__bridge void *)self));
        tbar->find_item("record_del")->signal_activated()->connect(std::bind(record_del, (__bridge void *)self));
      }
      [self rebuildColumns];
    }
  }
  return self;
}

- (void)dealloc
{
  [NSObject cancelPreviousPerformRequestsWithTarget: self];
  (*mData)->refresh_ui_signal.disconnect_all_slots();

  std::for_each(mSigConns.begin(), mSigConns.end(), std::bind(&boost::signals2::connection::disconnect, std::placeholders::_1));
  delete mData;
}

// for use by mforms
static const char *viewFlagsKey = "viewFlagsKey";

- (NSInteger)viewFlags
{
  NSNumber *value = objc_getAssociatedObject(self, viewFlagsKey);
  return value.intValue;
}

- (void)setViewFlags: (NSInteger)value
{
  objc_setAssociatedObject(self, viewFlagsKey, @(value), OBJC_ASSOCIATION_RETAIN);
}
//


- (void)setHeaderIndicator:(int)indicator forColumn:(int)column
{
  NSTableColumn *tableColumn= [gridView tableColumnWithIdentifier:[NSString stringWithFormat:@"%i", column]];
  switch (indicator)
  {
    case 0:
      [gridView setIndicatorImage: nil inTableColumn:tableColumn];
      break;
    case 1:
      [gridView setIndicatorImage: ascendingSortIndicator inTableColumn:tableColumn];
      break;
    case -1:
      [gridView setIndicatorImage: descendingSortIndicator inTableColumn:tableColumn];
      break;
  }
}


- (void)rebuildColumns
{
  for (NSUInteger i = gridView.tableColumns.count - 1; i > 0; --i) {
    [gridView removeTableColumn: gridView.tableColumns[i]];
  }
  
  if (mWarnedManyColumns == 0 && (*mData)->get_column_count() > 300)
  {
    NSAlert *alert = [NSAlert new];
    alert.messageText = @"Too Many Columns";
    alert.informativeText = @"The resultset for your query contains too many columns, which may be very slow to display."
      "\nHowever, as a workaround, manual resizing of columns can be disabled to speed up display and scrolling.";
    alert.alertStyle = NSAlertStyleWarning;
    [alert addButtonWithTitle: @"Disable Column Resizing"];
    [alert addButtonWithTitle: @"Ignore"];

    if ([alert runModal] == NSAlertFirstButtonReturn)
      mWarnedManyColumns = 1;
    else
      mWarnedManyColumns = -1;
  }

  float rowHeight = 0;
  for (size_t index = 0, count = (*mData)->get_column_count(); index < count; ++index)
  {
    std::string label= base::sanitize_utf8((*mData)->get_column_caption(index));
    NSTableColumn *column= [[NSTableColumn alloc] initWithIdentifier: [NSString stringWithFormat: @"%zu", index]];

    [column.headerCell setTitle: @(label.c_str())];

    [column setEditable: YES];
    
    column.dataCell = [[MQResultSetCell alloc] init];
    [column.dataCell setEditable: YES];
    [column.dataCell setLineBreakMode: NSLineBreakByTruncatingTail];
    if (mFont)
    {
      [column.dataCell setFont: mFont];
      rowHeight = MAX(rowHeight, [[column dataCell] cellSize].height + 1);
    }
    if (mWarnedManyColumns == 1)
      column.resizingMask = 0;
    
    [gridView addTableColumn: column];
  }
  if (rowHeight > 0)
    gridView.rowHeight = rowHeight;
}

- (void)setFont:(NSFont*)font
{
  mFont = font;

  float rowHeight = 0;
  for (NSTableColumn *column in gridView.tableColumns)
  {
    if (mFont)
    {
      [column.dataCell setFont: mFont];
      rowHeight = MAX(rowHeight, [[column dataCell] cellSize].height + 1);
    }
  }
  if (rowHeight > 0)
    gridView.rowHeight = rowHeight;
}

- (void)refreshGrid
{
  mPendingRefresh = NO;
  [gridView reloadData];
}

static int onRefreshWhenIdle(void *viewer_)
{
  MResultsetViewer *viewer = (__bridge MResultsetViewer *)viewer_;

  // Do table refresh only if it isn't currently in edit mode or this will
  // stop any ongoing edit action (and has other side effects like a misplaced selection).
  if (!viewer->mPendingRefresh && viewer.gridView.editedRow == -1)
  {
    viewer->mPendingRefresh = YES;
    [viewer performSelector: @selector(refreshGrid) withObject:nil afterDelay: 0];
  }
  return 0;
}

- (void)clickedTable:(id)sender
{
  (*self->mData)->set_edited_field(gridView.clickedRow, gridView.clickedColumn);
  
  [gridView editColumn: gridView.clickedColumn
                   row: gridView.clickedRow
             withEvent: NSApp.currentEvent
                select: YES];
}


- (BOOL)hasPendingChanges
{
  int upd_count= 0, ins_count= 0, del_count= 0;
  (*self->mData)->pending_changes(upd_count, ins_count, del_count);
  return upd_count>0 || ins_count>0 || del_count>0;
}

  
static void record_edit(void *view)
{
  MResultsetViewer *viewer = (__bridge MResultsetViewer *)view;
  [viewer.gridView editColumn: viewer.gridView.selectedColumnIndex
                          row: viewer.gridView.selectedRowIndex
                    withEvent: nil
                       select: NO];
}

static void record_add(void *view)
{
  MResultsetViewer *viewer = (__bridge MResultsetViewer *)view;
  [viewer.gridView scrollRowToVisible: (*viewer->mData)->count() - 1];
  [viewer.gridView selectCellAtRow: (int)(*viewer->mData)->count() - 1 column: 1];

  [viewer.gridView editColumn: viewer.gridView.selectedColumnIndex
                          row: viewer.gridView.selectedRowIndex
                    withEvent: nil
                       select: NO];
}

static void record_del(void *view)
{
  MResultsetViewer *viewer = (__bridge MResultsetViewer *)view;
  [viewer.gridView deleteSelectedRows];
}

static void selected_record_changed(void *theViewer)
{
  MResultsetViewer *viewer = (__bridge MResultsetViewer *)theViewer;
  [viewer.gridView scrollRowToVisible: (*viewer->mData)->edited_field_row()-1];
  [viewer.gridView deselectAll: nil];
  [viewer.gridView selectCellAtRow: (int)(*viewer->mData)->edited_field_row() column: (int)(*viewer->mData)->edited_field_column()];
}

- (void)actionTriggered
{
  (*self->mData)->set_edited_field(gridView.selectedRowIndex, gridView.selectedColumnIndex - 1) ;
}

- (std::shared_ptr<Recordset>)recordset
{
  return *mData;
}

- (void)activateToolbarItem:(id)sender
{
  // TODO: leftover toolbar item handlers, these should be added back to the toolbar maybe.
  {
    std::string action = [[sender cell].representedObject UTF8String];

    int selectedColumnIndex = gridView.selectedColumnIndex;
    int selectedRowIndex = gridView.selectedRowIndex;

    std::vector<int> rows; 
    rows.push_back(selectedRowIndex);
    
    if (!(*mData)->action_list().trigger_action(action, rows, selectedColumnIndex)
        && !(*mData)->action_list().trigger_action(action))
    {
      if (action == "record_first")
      {
        [gridView scrollRowToVisible: 0];
        [gridView selectCellAtRow: 0 column: 1];
      }
      else if (action == "record_back")
      {
        int row = gridView.selectedRowIndex - 1;
        if (row < 0)
          row = 0;
        [gridView scrollRowToVisible: row];
        [gridView selectCellAtRow: row column: 1];
      }
      else if (action == "record_next")
      {
        size_t row = gridView.selectedRowIndex + 1;
        if (row >= (*mData)->count() - 1)
          row = (*mData)->count() - 1;
        [gridView scrollRowToVisible: row];
        [gridView selectCellAtRow: (int)row column: 1];
      }
      else if (action == "record_last")
      {
        [gridView scrollRowToVisible: (*mData)->count()-1];
        [gridView selectCellAtRow: (int)(*mData)->count() - 1 column: 1];
      }
      else if (action == "record_wrap_vertical")
      {
      }
      else if (action == "record_sort_asc")
      {
        int column = gridView.selectedColumnIndex - 1;
        if (column >= 0)
        {
          (*mData)->sort_by(column, 1, false);
        }
      }
      else if (action == "record_sort_desc")
      {
        int column = gridView.selectedColumnIndex - 1;
        if (column >= 0)
        {
          (*mData)->sort_by(column, -1, false);
        }
      }
      else
        NSLog(@"unhandled toolbar action %s", action.c_str());
    }
  }
}

- (void)refresh
{
  [gridView reloadData];
}

- (void)refreshFull
{
  (*mData)->refresh();
}

static int onRefresh(void *viewer)
{
  [(__bridge id)viewer refresh];
  return 0;
}


- (void)fixupLayout
{
  NSRect rect = gridView.enclosingScrollView.frame;
  
  if (NSHeight(rect) + 20 != NSHeight(view.frame))
  {
    rect.size.height= NSHeight(view.frame) - 20;
    gridView.enclosingScrollView.frame = rect;
  }
}


- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
  if (mData && aTableColumn.identifier != nil)
  {
    int columnIndex = aTableColumn.identifier.intValue;
    if ((*mData)->get_column_type(columnIndex) != bec::GridModel::BlobType)
    {
      std::string text;
      (*mData)->get_field_repr(rowIndex, columnIndex, text);
      text = base::replaceString(text, "\n", " ");
      return [NSString stringWithCPPString: text];
    }
    return @"";
  }
  return nil;
}


- (void)tableView:(NSTableView *)aTableView setObjectValue:(id)anObject forTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
  if (mData && aTableColumn.identifier != nil)
  {
    if (anObject == nil)
    {
      if (!(*mData)->is_field_null(rowIndex, aTableColumn.identifier.intValue))
        (*mData)->set_field_null(rowIndex, aTableColumn.identifier.intValue);
    }
    else
    {
      std::string new_text= [anObject UTF8String];
      std::string old_text;
      (*mData)->get_field(rowIndex, aTableColumn.identifier.intValue, old_text);
      
      if (old_text != new_text)
      {
        size_t oldRowCount= (*mData)->count();
        
        (*mData)->set_field(rowIndex, aTableColumn.identifier.intValue, 
                            new_text);
        
        if ((*mData)->count() > oldRowCount)
          [aTableView noteNumberOfRowsChanged];
      }
    }
  }
  else if (mData && aTableColumn == nil)
  {
    // delete row
    (*mData)->delete_node(rowIndex);
    [aTableView noteNumberOfRowsChanged];
  }
}


- (void) tableView: (NSTableView*) aTableView
   willDisplayCell: (id) aCell
    forTableColumn: (NSTableColumn*) aTableColumn
               row: (NSInteger) rowIndex;
{
  if (aTableColumn.identifier && ![aTableColumn.identifier isEqualToString: @""])
  {
    int columnIndex = aTableColumn.identifier.intValue;    
    
    if (columnIndex >= 0)
    {
      [aCell setIsNull: (*mData)->is_field_null(rowIndex, columnIndex)];
      [aCell setIsBlob: (*mData)->get_column_type(columnIndex) == bec::GridModel::BlobType];
    }
    else
    {
      [aCell setIsNull: NO];
      [aCell setIsBlob: NO];
    }
  }
  else
  {
    [aCell setSelected: ((MGridView*)aTableView).selectedRowIndex == rowIndex];
  }
}


- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView
{
  if (mData)
    return (*mData)->count();
  return 0;
}


- (BOOL)tableView:(NSTableView *)aTableView shouldEditTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
  if (mData && aTableColumn.identifier != nil)
    return !(*mData)->is_readonly() && 
        (*mData)->get_column_type(aTableColumn.identifier.intValue) != bec::GridModel::BlobType;
  return NO;
}

- (void)close
{
  (*mData)->close();
}

- (void) tableView: (NSTableView *) tableView
  didClickTableColumn: (NSTableColumn *) tableColumn
{
  if (tableColumn.identifier)
  {
    int column_index= tableColumn.identifier.intValue;  
    ::bec::GridModel::SortColumns sort_columns= (*mData)->sort_columns();
    int sort_order= 1; // ascending (1) on first click, descending (-1) on second, then toggling
    for (::bec::GridModel::SortColumns::const_iterator i= sort_columns.begin(), end= sort_columns.end(); i != end; ++i)
    {
      if ((int)i->first == column_index)
      {
        sort_order= (1 == i->second) ? -1 : 0;
        break;
      }
    }
    (*mData)->sort_by(column_index, sort_order, true);

    [self setHeaderIndicator: sort_order forColumn: column_index];
  }
  else
  {
    // reset sorting if clicked the dummy column - change this to Select All, to match behaviour in Windows
    //[mTableView setIndicatorImage:nil inTableColumn:tableColumn];
    //(*mData)->sort_by(0, 0, false);
  }
}

@end

