/*
 * Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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

#import "MGridView.h"

#import "RecordGridView.h"
#import "MResultsetViewer.h"

#include "mforms/menubar.h"

#include "sqlide/recordset_be.h"
#include "base/string_utilities.h"
#include "base/log.h"

DEFAULT_LOG_DOMAIN("RecordGridView");

using namespace mforms;


static GridView* create_record_grid(std::shared_ptr<Recordset> rset)
{
  return new RecordGridView(rset);
}


void cf_record_grid_init()
{
  mforms::GridView::register_factory(create_record_grid);
}


@interface RecordGridObserver : NSObject
{
  std::map<NSTableView*, RecordGridView*> gridView;
}
- (void)columnDidResize:(NSNotification*)notif;
@end

@implementation RecordGridObserver

- (void)columnDidResize:(NSNotification*)notif
{
  std::map<NSTableView*, RecordGridView*>::iterator iter = gridView.find(notif.object);
  if (iter != gridView.end())
  {
    id theColumn = notif.userInfo[@"NSTableColumn"];
    NSInteger i = [iter->first.tableColumns indexOfObject: theColumn];
    if (i != NSNotFound)
      (*iter->second->signal_column_resized())((int)i - 1);
  }
}


- (void)observeViewer:(RecordGridView*)viewer
{
  gridView[viewer->control().gridView] = viewer;

  [[NSNotificationCenter defaultCenter] addObserver: self
                                           selector: @selector(columnDidResize:)
                                               name: NSTableViewColumnDidResizeNotification
                                             object: viewer->control().gridView];
}

- (void)forgetViewer:(RecordGridView*)viewer
{
  [[NSNotificationCenter defaultCenter] removeObserver: self
                                                  name: NSTableViewColumnDidResizeNotification
                                                object: viewer->control().gridView];
}

@end

static RecordGridObserver *observer = nil;
RecordGridView::RecordGridView(Recordset::Ref rset)
{
  if (!observer)
    observer = [[RecordGridObserver alloc] init];

  viewer = [[MResultsetViewer alloc] initWithRecordset: rset];

  [observer observeViewer: this];
  set_data(viewer.view);
}

RecordGridView::~RecordGridView()
{
  [observer forgetViewer: this];
  viewer = nil;
  set_data(nil);
}

int RecordGridView::get_column_count()
{
  return (int)viewer.gridView.numberOfColumns;
}


int RecordGridView::get_column_width(int column)
{
  return (int)[viewer.gridView tableColumnWithIdentifier: [NSString stringWithFormat:@"%i", column]].width;
}


void RecordGridView::set_column_width(int column, int width)
{
  [viewer.gridView tableColumnWithIdentifier: [NSString stringWithFormat:@"%i", column]].width = width;
}


void RecordGridView::set_column_header_indicator(int column, ColumnHeaderIndicator indicator)
{
  [viewer setHeaderIndicator: (int)indicator forColumn: column];
}


bool RecordGridView::current_cell(size_t &row, int &column)
{
  MGridView *grid = viewer.gridView;

  if (grid.selectedRowIndex >= 0 && grid.selectedColumnIndex >= 0)
  {
    row = grid.selectedRowIndex;
    column = grid.selectedColumnIndex;
    return true;
  }
  return false;
}


void RecordGridView::set_current_cell(size_t row, int column)
{
  [viewer.gridView selectCellAtRow: (int)row column: column];
}


static void set_clicked_column(RecordGridView *grid, void *table)
{
  NSTableView *gridView = (__bridge NSTableView*)table;
  NSPoint point = [gridView convertPoint: gridView.window.mouseLocationOutsideOfEventStream fromView: nil];
  NSInteger column = [gridView columnAtPoint: NSMakePoint(point.x, 20)];
  grid->clicked_header_column((int)column - 1);
}


void RecordGridView::set_header_menu(ContextMenu *menu)
{
  menu->signal_will_show()->connect(std::bind(set_clicked_column, this, (__bridge void *)viewer.gridView));
  viewer.gridView.headerView.menu = menu->get_data();
}


void RecordGridView::set_font(const std::string &font_desc)
{
  std::string name;
  float size;
  bool bold;
  bool italic;
  if (base::parse_font_description(font_desc, name, size, bold, italic))
  {
      NSFont *font = [[NSFontManager sharedFontManager] fontWithFamily: @(name.c_str())
                                                                traits: (bold? NSBoldFontMask: 0) | (italic? NSItalicFontMask: 0)
                                                                weight: bold ? 9 : 5
                                                                  size: size];
      [viewer setFont:font];
  }
  else
    logError("Invalid font specification: %s\n", font_desc.c_str());
}


void RecordGridView::update_columns()
{
  [viewer rebuildColumns];
  [viewer refresh];
}

