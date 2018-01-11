/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _LF_TABLE_H_
#define _LF_TABLE_H_

#include "mforms/table.h"

#include "lf_view.h"
#include <gtkmm/alignment.h>

namespace mforms {
  namespace gtk {

    class TableImpl : public ViewImpl {
    protected:
      Gtk::Grid *_grid;
      Gtk::Box *_outerBox;
      // We need this to mimic Table functionality so we know if
      // we're not trying to add to many elements
      int _rowCount;
      int _colCount;
      virtual Gtk::Widget *get_outer() const;
      virtual Gtk::Widget *get_inner() const;

      TableImpl(::mforms::Table *self);

      virtual ~TableImpl();

      static bool create(::mforms::Table *self);

      static void set_row_count(Table *self, int count);

      static void set_col_count(Table *self, int count);

      static void add(Table *self, View *child, int left, int right, int top, int bottom, int flags);

      static void remove(Table *self, View *child);

      static void set_row_spacing(Table *self, int space);

      static void set_col_spacing(Table *self, int space);

      static void set_homogeneous(Table *self, bool flag);

      virtual void set_padding_impl(int left, int top, int right, int bottom);

    public:
      static void init();
    };
  }
}

#endif /* _LF_TABLE_H_ */
