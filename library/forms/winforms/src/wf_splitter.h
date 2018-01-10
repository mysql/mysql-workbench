/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

  public
    class SplitterWrapper : public ViewWrapper {
    protected:
      SplitterWrapper(mforms::Splitter *backend);

      static bool create(mforms::Splitter *backend, bool horizontal);
      static void add(mforms::Splitter *backend, mforms::View *view, int min_size, bool fixed);
      static void remove(mforms::Splitter *backend, mforms::View *view);
      static void set_divider_position(mforms::Splitter *backend, int position);
      static int get_divider_position(mforms::Splitter *backend);
      static void set_expanded(mforms::Splitter *backend, bool first, bool expand);

    public:
      static void init();
    };
  };
};
