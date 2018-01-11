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

#pragma once

#include "mforms/native.h"

#ifndef SWIG
class Recordset;
#endif

namespace mforms {
  class ContextMenu;

  enum ColumnHeaderIndicator {
    NoIndicator = 0,
    SortDescIndicator = -1,
    SortAscIndicator = 1,
  };

  // This is a skeleton/proxy/facade class for a Recordset grid view
  // The implementation of the view itself is in frontend/platform specific code
  // The main use for this class is to provide an interface for SWIG
  // and to allow the frontend to register a factory function to allocate the
  // concrete implementation.
  class MFORMS_EXPORT GridView : public NativeContainer {
  public:
    virtual int get_column_count() = 0;
    virtual int get_column_width(int column) = 0;
    virtual void set_column_width(int column, int width) = 0;

    virtual void update_columns() = 0;

    virtual void set_column_header_indicator(int column, ColumnHeaderIndicator order) = 0;

    virtual bool current_cell(size_t &row, int &column) = 0;
    virtual void set_current_cell(size_t row, int column) = 0;

    virtual void set_header_menu(ContextMenu *menu);
    int get_clicked_header_column() {
      return _clicked_header_column;
    }

#ifndef SWIG
    static GridView *create(std::shared_ptr<Recordset> rset);

    static void register_factory(GridView *(*create)(std::shared_ptr<Recordset> rset));
#endif

    // TODO must be emited from Windows
    boost::signals2::signal<void(int)> *signal_column_resized() {
      return &_signal_column_resized;
    }
    boost::signals2::signal<void(const std::vector<int>)> *signal_columns_resized() {
      return &_signal_columns_resized;
    }

    ContextMenu *header_menu() {
      return _header_menu;
    }

#ifndef SWIG
    void clicked_header_column(int column);
#endif
  protected:
    GridView();

  private:
    boost::signals2::signal<void(int)> _signal_column_resized;
    boost::signals2::signal<void(const std::vector<int>)> _signal_columns_resized;
    ContextMenu *_header_menu;
    int _clicked_header_column;
  };
};
