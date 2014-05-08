/*
 * Copyright (c) 2014 Oracle and/or its affiliates. All rights reserved.
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

#import "RecordGridView.h"
#import "MResultsetViewer.h"
#include "sqlide/recordset_be.h"

using namespace mforms;


static RecordGrid* create_record_grid(boost::shared_ptr<Recordset> rset)
{
  return new RecordGridView(rset);
}


void cf_record_grid_init()
{
  mforms::RecordGrid::register_factory(create_record_grid);
}



RecordGridView::RecordGridView(Recordset::Ref rset)
{
  viewer = [[MResultsetViewer alloc] initWithRecordset: rset];

  set_data([[viewer gridView] enclosingScrollView]);
}

RecordGridView::~RecordGridView()
{
  [viewer release];
}

int RecordGridView::get_column_count()
{
  return [[viewer gridView] numberOfColumns];
}


int RecordGridView::get_column_width(int column)
{
  return (int)[[[viewer gridView] tableColumnWithIdentifier: [NSString stringWithFormat:@"%i", column]] width];
}


void RecordGridView::set_column_width(int column, int width)
{
  [[[viewer gridView] tableColumnWithIdentifier: [NSString stringWithFormat:@"%i", column]] setWidth: width];
}


bool RecordGridView::current_cell(size_t &row, int &column)
{
  MGridView *grid = [viewer gridView];

  if ([grid selectedRowIndex] >= 0 && [grid selectedColumnIndex] >= 0)
  {
    row = [grid selectedRowIndex];
    column = [grid selectedColumnIndex];
    return true;
  }
  return false;
}


void RecordGridView::set_current_cell(size_t row, int column)
{
  [[viewer gridView] selectCellAtRow: row column: column];
}

