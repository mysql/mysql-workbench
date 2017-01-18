/*
 * Copyright (c) 2010, 2017, Oracle and/or its affiliates. All rights reserved.
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
