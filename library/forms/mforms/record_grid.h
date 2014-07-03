/*
 * Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MFORMS_RECORD_GRID_H_
#define _MFORMS_RECORD_GRID_H_

#include "mforms/native.h"
#include <boost/shared_ptr.hpp>

#ifndef SWIG
class Recordset;
#endif

namespace mforms {
  class ContextMenu;

  enum ColumnHeaderIndicator
  {
    NoIndicator = 0,
    SortDescIndicator = -1,
    SortAscIndicator = 1,
  };

  // This is a skeleton/proxy/facade class for a Recordset grid view
  // The implementation of the view itself is in frontend/platform specific code
  // The main use for this class is to provide an interface for SWIG
  // and to allow the frontend to register a factory function to allocate the
  // concrete implementation.
  class MFORMS_EXPORT RecordGrid : public NativeContainer
  {
  public:
    virtual int get_column_count() = 0;
    virtual int get_column_width(int column) = 0;
    virtual void set_column_width(int column, int width) = 0;

    virtual void set_column_header_indicator(int column, ColumnHeaderIndicator order) = 0; //TODO Windows

    virtual bool current_cell(size_t &row, int &column) = 0;
    virtual void set_current_cell(size_t row, int column) = 0;

    virtual void set_font(const std::string &font) = 0; // TODO Windows

    virtual void set_header_menu(ContextMenu *menu); // TODO Windows
    int get_clicked_header_column() { return _clicked_header_column; }

#ifndef SWIG
    static RecordGrid* create(boost::shared_ptr<Recordset> rset);

    static void register_factory(RecordGrid* (*create)(boost::shared_ptr<Recordset> rset));
#endif

    // TODO must be emited from Windows
    boost::signals2::signal<void (int)>* signal_column_resized() { return &_signal_column_resized; }

    ContextMenu *header_menu() { return _header_menu; }

#ifndef SWIG
    void clicked_header_column(int column);
#endif
  protected:
    RecordGrid();
  private:
    boost::signals2::signal<void (int)> _signal_column_resized;
    ContextMenu *_header_menu;
    int _clicked_header_column;
  };
};

#endif
