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

#import "MResultsetViewer.h"
#include "sqlide/recordset_be.h"
#import "MQResultSetCell.h"
#import "MQIndicatorCell.h"
#import "GRTIconCache.h"
#import "MCPPUtilities.h"
#import "MVerticalLayoutView.h"

#include "mforms/toolbar.h"

static int onRefresh(MResultsetViewer *self);
static NSImage *ascendingSortIndicator= nil;
static NSImage *descendingSortIndicator= nil;

@implementation MResultsetViewer



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
  NSTableColumn *tableColumn= [mTableView tableColumnWithIdentifier:[NSString stringWithFormat:@"%i", column]];
  switch (indicator)
  {
    case 0:
      [mTableView setIndicatorImage: nil inTableColumn:tableColumn];
      break;
    case 1:
      [mTableView setIndicatorImage: ascendingSortIndicator inTableColumn:tableColumn];
      break;
    case -1:
      [mTableView setIndicatorImage: descendingSortIndicator inTableColumn:tableColumn];
      break;
  }
}


- (void)rebuildColumns
{
  for (id column in [[mTableView tableColumns] reverseObjectEnumerator])
  {
    if ([column identifier])
      [mTableView removeTableColumn: column];
  }

  if (mWarnedManyColumns == 0 && (*mData)->get_column_count() > 300)
  {
    if (NSRunAlertPanel(@"Too Many Columns",
                        @"The resultset for your query contains too many columns, which may be very slow to display.\nHowever, as a workaround, manual resizing of columns can be disabled to speed up display and scrolling.",
                        @"Disable Column Resizing", @"Continue", nil) == NSAlertDefaultReturn)
      mWarnedManyColumns = 1;
    else
      mWarnedManyColumns = -1;
  }

  float rowHeight = 0;
  for (int index= 0, count= (*mData)->get_column_count(); index < count; ++index)
  {
    std::string label= base::sanitize_utf8((*mData)->get_column_caption(index));
    //bec::GridModel::ColumnType type= (*mData)->get_column_type(index);
    NSTableColumn *column= [[[NSTableColumn alloc] initWithIdentifier: [NSString stringWithFormat:@"%i", index]] autorelease];

    [[column headerCell] setTitle: [NSString stringWithUTF8String: label.c_str()]];

    [column setEditable: YES];
    
    [column setDataCell: [[[MQResultSetCell alloc] init] autorelease]];
    [[column dataCell] setEditable: YES];
    [[column dataCell] setLineBreakMode: NSLineBreakByTruncatingTail];
    if (mFont)
    {
      [[column dataCell] setFont: mFont];
      rowHeight = MAX(rowHeight, [[column dataCell] cellSize].height + 1);
    }
    if (mWarnedManyColumns == 1)
      [column setResizingMask: 0];
    
    [mTableView addTableColumn: column];
  }
  if (rowHeight > 0)
    [mTableView setRowHeight: rowHeight];
}


- (void)dealloc
{
  [NSObject cancelPreviousPerformRequestsWithTarget: self];
  (*mData)->refresh_ui_signal.disconnect_all_slots();
  
  std::for_each(mSigConns.begin(), mSigConns.end(), boost::bind(&boost::signals2::connection::disconnect, _1));
  delete mData;
  [mView release];
  [mFont release];
  [super dealloc];
}


- (void)setFont:(NSFont*)font
{
  [mFont autorelease];
  mFont = [font retain];

  float rowHeight = 0;
  for (int index= 0, count= (*mData)->get_column_count(); index <= count; ++index)
  {
    NSTableColumn *column= [[mTableView tableColumns] objectAtIndex: index];
    if (mFont)
    {
      [[column dataCell] setFont: mFont];
      rowHeight = MAX(rowHeight, [[column dataCell] cellSize].height + 1);
    }
  }
  if (rowHeight > 0)
    [mTableView setRowHeight: rowHeight];
}

/*
static int processTaskMessage(int msgType, const std::string &message, const std::string &detail,
                              MResultsetViewer *self)
{
  if (msgType == grt::ErrorMsg)
    self->mGotError = YES;
  
  if (!message.empty())
  {
    if (self->mErrorMessage)
    {
      [self->mErrorMessage autorelease];
      self->mErrorMessage = [[NSString stringWithFormat:@"%@\n%@", self->mErrorMessage, [NSString stringWithUTF8String:message.c_str()]] retain];
    }
    else
      self->mErrorMessage = [[NSString stringWithUTF8String: message.c_str()] retain];
  }
//  (*self->mData)->grtm()->replace_status_text(message);
  
  return 0;
}*/

- (void)refreshGrid
{
  mPendingRefresh = NO;
  [mTableView reloadData];
}

static int onRefreshWhenIdle(MResultsetViewer *self)
{
  // Do table refresh only if it isn't currently in edit mode or this will
  // stop any ongoing edit action (and has other side effects like a misplaced selection).
  if (!self->mPendingRefresh && [self->mTableView editedRow] == -1)
  {
    self->mPendingRefresh = YES;
    [self performSelector: @selector(refreshGrid) withObject:nil afterDelay: 0];
  }
  return 0;
}

- (void)clickedTable:(id)sender
{
  (*self->mData)->set_edited_field([mTableView clickedRow], [mTableView clickedColumn]);
  
  [mTableView editColumn:[mTableView clickedColumn]
                     row:[mTableView clickedRow]
               withEvent:[NSApp currentEvent]
                  select:YES];
}


- (BOOL)hasPendingChanges
{
  int upd_count= 0, ins_count= 0, del_count= 0;
  (*self->mData)->pending_changes(upd_count, ins_count, del_count);
  return upd_count>0 || ins_count>0 || del_count>0;
}

  
static void record_edit(MResultsetViewer *self)
{
  [self->mTableView editColumn: [self->mTableView selectedColumnIndex]
                           row: [self->mTableView selectedRowIndex]
                     withEvent: nil
                        select: NO];
}

static void record_add(MResultsetViewer *self)
{
  [self->mTableView scrollRowToVisible: (*self->mData)->count()-1];
  [self->mTableView selectCellAtRow: (*self->mData)->count()-1 column: 1];
  
  [self->mTableView editColumn: [self->mTableView selectedColumnIndex]
                           row: [self->mTableView selectedRowIndex]
                     withEvent: nil
                        select: NO];
}
  
  
static void record_del(MResultsetViewer *self)
{
  [self->mTableView deleteBackward:nil];
}

static void selected_record_changed(MResultsetViewer *self)
{
  [self->mTableView scrollRowToVisible: (*self->mData)->edited_field_row()-1];
  [self->mTableView deselectAll: nil];
  [self->mTableView selectCellAtRow: (*self->mData)->edited_field_row() column: (*self->mData)->edited_field_column()];
}
  
- (id)initWithRecordset:(Recordset::Ref)rset
{
  if (!ascendingSortIndicator)
  {
    ascendingSortIndicator= [[NSImage imageNamed:@"NSAscendingSortIndicator"] retain];
    descendingSortIndicator= [[NSImage imageNamed:@"NSDescendingSortIndicator"] retain];
  }
  
  self= [super init];
  if (self)
  {
    [NSBundle loadNibNamed:@"WbResultsetView" owner:self];
    
    mData= new Recordset::Ref();
    *mData= rset;
    
    [mTableView setRecordset: mData->get()];

    (*mData)->update_edited_field = boost::bind(selected_record_changed, self);
    (*mData)->tree_changed_signal()->connect(boost::bind(onRefreshWhenIdle, self));
    
    (*mData)->refresh_ui_signal.connect(boost::bind(onRefresh, self));
    //(*mData)->task->msg_cb(boost::bind(processTaskMessage, _1, _2, _3, self));
    
    [mTableView setIntercellSpacing: NSMakeSize(0, 1)];
    [mTableView selectionChangedActionTarget:self];
    [mTableView setSelectionChangedAction:@selector(handleNSTableViewSelectionIsChangingNotification:)];
    [mTableView setAllowsMultipleSelection: YES];

    [[mTableView enclosingScrollView] setBorderType: NSNoBorder];
    
    mforms::ToolBar *tbar = (*mData)->get_toolbar();
    if (tbar->find_item("record_edit"))
    {
      tbar->find_item("record_edit")->signal_activated()->connect(boost::bind(record_edit, self));
      tbar->find_item("record_add")->signal_activated()->connect(boost::bind(record_add, self));
      tbar->find_item("record_del")->signal_activated()->connect(boost::bind(record_del, self));
    }
    [self rebuildColumns];
    //onRefresh(self);
  }
  return self;
}

- (void)handleNSTableViewSelectionIsChangingNotification:(NSNotification*)note
{
  (*self->mData)->set_edited_field([mTableView selectedRowIndex], [mTableView selectedColumnIndex]-1) ;
}

- (boost::shared_ptr<Recordset>)recordset
{
  return *mData;
}

- (NSView*)view
{
  return mView;
}

- (MGridView*)gridView
{
  return mTableView;
}

- (void)activateToolbarItem:(id)sender
{ // leftover toolbar item handlers, these should be added back to the toolbar maybe
  {
    std::string action = [[[sender cell] representedObject] UTF8String];

    int selectedColumnIndex = [(MGridView*)mTableView selectedColumnIndex];
    int selectedRowIndex = [(MGridView*)mTableView selectedRowIndex];

    std::vector<int> rows; 
    rows.push_back(selectedRowIndex);
    
    if (!(*mData)->action_list().trigger_action(action, rows, selectedColumnIndex)
        && !(*mData)->action_list().trigger_action(action))
    {
      if (action == "record_first")
      {
        [mTableView scrollRowToVisible: 0];
        [mTableView selectCellAtRow: 0 column: 1];
      }
      else if (action == "record_back")
      {
        int row = [mTableView selectedRowIndex] - 1;
        if (row < 0)
          row = 0;
        [mTableView scrollRowToVisible: row];
        [mTableView selectCellAtRow: row column: 1];
      }
      else if (action == "record_next")
      {
        int row = [mTableView selectedRowIndex] + 1;
        if (row >= (int)(*mData)->count()-1)
          row = (*mData)->count()-1;
        [mTableView scrollRowToVisible: row];
        [mTableView selectCellAtRow: row column: 1];
      }
      else if (action == "record_last")
      {
        [mTableView scrollRowToVisible: (*mData)->count()-1];
        [mTableView selectCellAtRow: (*mData)->count()-1 column: 1];
      }
      else if (action == "record_wrap_vertical")
      {
      }
      else if (action == "record_sort_asc")
      {
        int column = [mTableView selectedColumnIndex]-1;
        if (column >= 0)
        {
          (*mData)->sort_by(column, 1, false);
        }
      }
      else if (action == "record_sort_desc")
      {
        int column = [mTableView selectedColumnIndex]-1;
        if (column >= 0)
        {
          (*mData)->sort_by(column, -1, false);
        }
      }
      else
        NSLog(@"unhandled toolbar action %s", action.c_str());
    }
  }
/*  
  if (mErrorMessage)
  {
    NSRunAlertPanel(@"Error", @"%@", @"OK", nil, nil, mErrorMessage);
    [mErrorMessage release];
    mErrorMessage= nil;
    mGotError= NO;
  }*/
}


- (void)refresh
{
//  [self rebuildColumns];
  [mTableView reloadData];
}


- (void)refreshFull
{
  (*mData)->refresh();
}

static int onRefresh(MResultsetViewer *self)
{
  [self refresh];
  return 0;
}


- (void)fixupLayout
{
  NSRect rect= [[mTableView enclosingScrollView] frame];
  
  if (NSHeight(rect) + 20 != NSHeight([mView frame]))
  {
    rect.size.height= NSHeight([mView frame]) - 20;
    [[mTableView enclosingScrollView] setFrame: rect];
  }
}


- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
  if (mData && [aTableColumn identifier] != nil)
  {
    int columnIndex = [[aTableColumn identifier] intValue];
    if ((*mData)->get_column_type(columnIndex) != bec::GridModel::BlobType)
    {
      std::string text;
      (*mData)->get_field_repr(rowIndex, columnIndex, text);
      text = bec::replace_string(text, "\n", " ");
      return [NSString stringWithCPPString: text];
    }
    return @"";
  }
  return nil;
}


- (void)tableView:(NSTableView *)aTableView setObjectValue:(id)anObject forTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
  if (mData && [aTableColumn identifier] != nil)
  {
    if (anObject == nil)
    {
      if (!(*mData)->is_field_null(rowIndex, [[aTableColumn identifier] intValue]))
        (*mData)->set_field_null(rowIndex, [[aTableColumn identifier] intValue]);
    }
    else
    {
      std::string new_text= [anObject UTF8String];
      std::string old_text;
      (*mData)->get_field(rowIndex, [[aTableColumn identifier] intValue], old_text);
      
      if (old_text != new_text)
      {
        size_t oldRowCount= (*mData)->count();
        
        (*mData)->set_field(rowIndex, [[aTableColumn identifier] intValue], 
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
  if ([aTableColumn identifier] != nil)
  {
    int columnIndex = [[aTableColumn identifier] intValue];    
    
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
    [aCell setSelected: [(MGridView*)aTableView selectedRowIndex] == rowIndex];
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
  if (mData && [aTableColumn identifier] != nil)
    return !(*mData)->is_readonly() && 
        (*mData)->get_column_type([[aTableColumn identifier] intValue]) != bec::GridModel::BlobType;
  return NO;
}

- (void)close
{
  (*mData)->close();
}

- (void) tableView: (NSTableView *) tableView
  didClickTableColumn: (NSTableColumn *) tableColumn
{
  if ([tableColumn identifier])
  {
    int column_index= [[tableColumn identifier] intValue];  
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

