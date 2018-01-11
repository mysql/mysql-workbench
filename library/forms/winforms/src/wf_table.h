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
