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

#pragma once

#include <mforms/base.h>
#include <mforms/container.h>

namespace mforms {

  class Table;

  enum TableItemFlags {
    NoFillExpandFlag = 0,   //!< Neither expand nor fill.
    VExpandFlag = (1 << 0), //!< whether item should try using vertical space leftover from table
    HExpandFlag = (1 << 1), //!< same for horizontal expansion
    VFillFlag = (1 << 2),   //!< whether all allocated vertical space should be filled, or just centered in it
    HFillFlag = (1 << 3),   //!< same for horizontal filling

    FillAndExpand = HFillFlag | VFillFlag | VExpandFlag | HExpandFlag,
  };

  inline TableItemFlags operator|(TableItemFlags a, TableItemFlags b) {
    return (TableItemFlags)((int)a | (int)b);
  }

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct MFORMS_EXPORT TableImplPtrs {
    bool (*create)(Table *);

    void (*set_row_count)(Table *self, int);
    void (*set_column_count)(Table *self, int);

    void (*add)(Table *self, View *, int, int, int, int, int);
    void (*remove)(Table *self, View *);

    void (*set_row_spacing)(Table *self, int);
    void (*set_column_spacing)(Table *self, int);

    void (*set_homogeneous)(Table *self, bool);
  };
#endif
#endif

  /** Container to layout child items in a grid of rows and columns.
   */
  class MFORMS_EXPORT Table : public Container {
  public:
    Table();

    /** Sets number of rows */
    void set_row_count(int c);
    /** Sets number of columns */
    void set_column_count(int c);

    /** Sets spacing between rows. */
    void set_row_spacing(int s);
    /** Sets spacing between columns. */
    void set_column_spacing(int s);

    /** Sets whether sizing of table children should be equal among items */
    void set_homogeneous(bool value);

    /** Adds an item to the table.

     @param left - left column to put the item in
     @param right - column that the item ends at (ie at least left+1)
     @param top - top row to put the item in
     @param bottom - row that the item ends at (ie at least top+1)
     @param flags -
    */
    void add(View *subview, int left, int right, int top, int bottom,
             int flags = HExpandFlag | VExpandFlag | HFillFlag | VFillFlag);

    /** Remove a child item from the table */
    virtual void remove(View *sv);

  protected:
    TableImplPtrs *_table_impl;
  };
};
