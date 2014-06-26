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

    virtual bool current_cell(size_t &row, int &column) = 0;
    virtual void set_current_cell(size_t row, int column) = 0;


#ifndef SWIG
    static RecordGrid* create(boost::shared_ptr<Recordset> rset);

    static void register_factory(RecordGrid* (*create)(boost::shared_ptr<Recordset> rset));
#endif

    // TODO must be emited from Windows, Linux
    boost::signals2::signal<void (int)>* signal_column_resized() { return &_signal_column_resized; }
  private:
    boost::signals2::signal<void (int)> _signal_column_resized;
  };
};

#endif
