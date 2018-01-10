/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MFORMS_RECORD_GRID_VIEW_H_
#define _MFORMS_RECORD_GRID_VIEW_H_

#include "mforms/gridview.h"

@class MResultsetViewer;

namespace mforms {
  class RecordGridView : public mforms::GridView {
    MResultsetViewer *viewer;

  public:
    RecordGridView(std::shared_ptr<Recordset> rset);
    virtual ~RecordGridView();

    virtual int get_column_count();
    virtual int get_column_width(int column);
    virtual void set_column_width(int column, int width);
    virtual void set_column_header_indicator(int column, ColumnHeaderIndicator order);

    virtual bool current_cell(size_t &row, int &column);
    virtual void set_current_cell(size_t row, int column);

    virtual void set_font(const std::string &font);
    virtual void set_header_menu(ContextMenu *menu);

    virtual void update_columns();

    MResultsetViewer *control() {
      return viewer;
    }
  };
};
#endif
