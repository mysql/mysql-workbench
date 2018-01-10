/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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
    class ListBoxWrapper : public ViewWrapper {
    protected:
      ListBoxWrapper(mforms::ListBox *backend);

      static bool create(mforms::ListBox *backend, bool multi_select);
      static void clear(mforms::ListBox *backend);
      static void set_heading(mforms::ListBox *backend, const std::string &text);
      static void add_items(mforms::ListBox *backend, const std::list<std::string> &items);
      static size_t add_item(mforms::ListBox *backend, const std::string &item);
      static void remove_indexes(mforms::ListBox *backend, const std::vector<size_t> &indices);
      static void remove_index(mforms::ListBox *backend, size_t index);
      static std::string get_text(mforms::ListBox *backend);
      static void set_index(mforms::ListBox *backend, ssize_t index);
      static ssize_t get_index(mforms::ListBox *backend);
      static std::vector<size_t> get_selected_indices(mforms::ListBox *backend);
      static size_t get_count(mforms::ListBox *self);
      static std::string get_string_value_from_index(mforms::ListBox *self, size_t index);

    public:
      static void init();
    };
  }
}
