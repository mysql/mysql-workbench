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
#ifndef _STUB_BOX_H_
#define _STUB_BOX_H_

#include "stub_view.h"

namespace mforms {
  namespace stub {

    class BoxWrapper : public ViewWrapper {
    protected:
      BoxWrapper(::mforms::Box *self, bool horiz) : ViewWrapper(self) {
      }

      static bool create(::mforms::Box *self, bool horiz) {
        return true;
      }

      static void add(Box *self, View *child, bool expand, bool fill) {
      }

      static void add_end(Box *self, View *child, bool expand, bool fill) {
      }

      static void remove(Box *self, View *child) {
      }

      static void set_homogeneous(Box *self, bool flag) {
      }

      static void set_spacing(Box *self, int spc) {
      }

      static void set_padding(Box *self, int pad) {
      }

    public:
      static void init() {
        ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

        f->_box_impl.create = &BoxWrapper::create;
        f->_box_impl.add = &BoxWrapper::add;
        f->_box_impl.add_end = &BoxWrapper::add_end;
        f->_box_impl.remove = &BoxWrapper::remove;
        f->_box_impl.set_homogeneous = &BoxWrapper::set_homogeneous;
        f->_box_impl.set_spacing = &BoxWrapper::set_spacing;
      }

      virtual ~BoxWrapper() {
      }
    };
  }
}

#endif /* _STUB_BOX_H_ */
