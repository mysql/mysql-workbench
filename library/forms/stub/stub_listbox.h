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

#ifndef _STUB_LISTBOX_H_
#define _STUB_LISTBOX_H_

#include "stub_view.h"

namespace mforms {
  namespace stub {

    class ListBoxWrapper : public ViewWrapper {
    protected:
      ListBoxWrapper(mforms::ListBox *self, bool multi_select);
      static void selection_changed(mforms::ListBox *self);
      static bool create(mforms::ListBox *self, bool multi_select);
      static void clear(mforms::ListBox *self);
      static size_t add_item(mforms::ListBox *self, const std::string &item);
      static void add_items(mforms::ListBox *self, const std::list<std::string> &items);
      static void remove_index(mforms::ListBox *self, size_t index);
      static void remove_indexes(mforms::ListBox *self, const std::vector<size_t> &items);
      static std::string get_text(mforms::ListBox *self);
      static void set_index(mforms::ListBox *self, ssize_t index);
      static ssize_t get_index(mforms::ListBox *self);
      static void set_heading(ListBox *self, const std::string &text);
      static std::vector<size_t> get_selected_indices(ListBox *self);

    public:
      static void init();
    };
  }
}

#endif /* _STUB_LISTBOX_H_ */
