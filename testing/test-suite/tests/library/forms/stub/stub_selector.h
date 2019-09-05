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

#ifndef _STUB_SELECTOR_H_
#define _STUB_SELECTOR_H_

#include "stub_view.h"

namespace mforms {
  namespace stub {

    class SelectorWrapper : public ViewWrapper {
    protected:
      SelectorWrapper(::mforms::Selector *self, ::mforms::SelectorStyle style);
      static void callback(::mforms::Selector *self);
      static bool create(::mforms::Selector *self, ::mforms::SelectorStyle style);
      static void clear(::mforms::Selector *self);
      static int add_item(::mforms::Selector *self, const std::string &item);
      static void add_items(::mforms::Selector *self, const std::list<std::string> &items);
      static std::string get_item(::mforms::Selector *self, int index);
      static std::string get_text(::mforms::Selector *self);
      static void set_index(::mforms::Selector *self, int index);
      static int get_index(::mforms::Selector *self);
      static int get_item_count(::mforms::Selector *self);
      static void set_value(::mforms::Selector *self, const std::string &);

    public:
      ~SelectorWrapper();
      static void init();
    };
  }
}

#endif
