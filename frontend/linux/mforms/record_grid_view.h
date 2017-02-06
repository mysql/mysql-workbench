/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MFORMS_RECORD_GRID_VIEW_H_
#define _MFORMS_RECORD_GRID_VIEW_H_

#include "mforms/gridview.h"

class RecordsetView;
using mforms::ColumnHeaderIndicator;

// namespace mforms
//{
class RecordGridView : public mforms::GridView {
  RecordsetView *viewer;

  void columns_resized(const std::vector<int> cols) {
    (*signal_columns_resized())(cols);
  }
  void column_right_clicked(int c, int x, int y);

public:
  RecordGridView(std::shared_ptr<Recordset> rset);
  virtual ~RecordGridView();

  virtual int get_column_count();
  virtual int get_column_width(int column);
  virtual void set_column_width(int column, int width);
  virtual void update_columns();

  virtual bool current_cell(size_t &row, int &column);
  virtual void set_current_cell(size_t row, int column);

  virtual void set_column_header_indicator(int column, ColumnHeaderIndicator order);

  virtual void set_font(const std::string &font);
  //};
};
#endif
