/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "stub_view.h"

namespace mforms {
  namespace stub {

    class SplitterWrapper : public ViewWrapper {
    protected:
      SplitterWrapper(::mforms::Splitter *self, bool horiz, bool thin) : ViewWrapper(self) {
      }

#ifdef __APPLE__
      static bool create(::mforms::Splitter *self, bool horiz, bool thin) {
        return true;
      }
#else
      static bool create(::mforms::Splitter *self, bool horiz) {
        return true;
      }
#endif

      static void add(Splitter *self, View *child, int minwidth, bool fixed) {
      }

      static void remove(Splitter *self, View *child) {
      }

      static void set_divider_position(Splitter *self, int pos) {
      }

      static int get_divider_position(Splitter *self) {
        return 0;
      }

      static void set_expanded(Splitter *self, bool first, bool expand) {
      }

    public:
      static void init() {
        ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

        f->_splitter_impl.create = &SplitterWrapper::create;
        f->_splitter_impl.add = &SplitterWrapper::add;
        f->_splitter_impl.remove = &SplitterWrapper::remove;
        f->_splitter_impl.set_divider_position = &SplitterWrapper::set_divider_position;
        f->_splitter_impl.get_divider_position = &SplitterWrapper::get_divider_position;
        f->_splitter_impl.set_expanded = &SplitterWrapper::set_expanded;
      }

      virtual ~SplitterWrapper() {
      }
    };
  }
}
