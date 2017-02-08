/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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
