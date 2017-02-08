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
#ifndef _LF_BOX_H_
#define _LF_BOX_H_

#include "lf_view.h"

namespace mforms {
  namespace gtk {

    class BoxImpl : public ViewImpl {
    protected:
      Gtk::Box *_innerBox;

      Gtk::Box *_outerBox;

      virtual Gtk::Widget *get_outer() const {
        return _outerBox;
      }

      virtual Gtk::Widget *get_inner() const {
        return _innerBox;
      }

      BoxImpl(::mforms::Box *self, bool horiz);
      static bool create(::mforms::Box *self, bool horiz);
      static void add(Box *self, View *child, bool expand, bool fill);
      static void add_end(Box *self, View *child, bool expand, bool fill);
      static void remove(Box *self, View *child);
      static void set_homogeneous(Box *self, bool flag);
      static void set_spacing(Box *self, int spc);
      // static void set_padding(Box *self, int pad);
      virtual void set_padding_impl(int left, int top, int right, int bottom);
      virtual void set_size(int width, int height);

    public:
      static void init();

      virtual ~BoxImpl();
    };
  }
}

#endif /* _LF_BOX_H_ */
