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

  public
    class SelectorWrapper : public ViewWrapper {
    protected:
      SelectorWrapper(mforms::Selector *backend);

      static bool create(mforms::Selector *backend, mforms::SelectorStyle style);
      static void clear(mforms::Selector *backend);
      static int add_item(mforms::Selector *backend, const std::string &item);
      static void add_items(mforms::Selector *backend, const std::list<std::string> &items);
      static std::string get_text(mforms::Selector *backend);
      static void set_index(mforms::Selector *backend, int index);
      static std::string get_item(mforms::Selector *backend, int index);
      static int get_index(mforms::Selector *backend);
      static int get_item_count(mforms::Selector *backend);
      static void set_value(mforms::Selector *backend, const std::string &value);

    public:
      static void init();
    };
  };
};
