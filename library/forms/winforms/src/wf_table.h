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

#pragma once

namespace MySQL {
  namespace Forms {

    ref class Table;

  public
    class TableWrapper : public ViewWrapper {
    protected:
      TableWrapper(mforms::View *view);

      virtual void set_padding(int left, int top, int right, int bottom);

      static bool create(mforms::Table *backend);
      static void add(mforms::Table *backend, mforms::View *child, int left, int right, int top, int bottom, int flags);
      static void remove(mforms::Table *backend, mforms::View *child);
      static void set_row_count(mforms::Table *backend, int count);
      static void set_column_count(mforms::Table *backend, int count);
      static void set_row_spacing(mforms::Table *backend, int space);
      static void set_column_spacing(mforms::Table *backend, int space);
      static void set_homogeneous(mforms::Table *backend, bool value);

    public:
      static void init();
    };
  };
};
