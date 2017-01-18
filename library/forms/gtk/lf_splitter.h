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
#ifndef _LF_SPLITTER_H_
#define _LF_SPLITTER_H_

#include "mforms/splitter.h"

#include "lf_view.h"

namespace mforms {
  namespace gtk {

    class SplitterImpl : public ViewImpl {
    protected:
      Gtk::Paned *_paned;

      virtual Gtk::Widget *get_outer() const {
        return _paned;
      }

      SplitterImpl(::mforms::Splitter *self, bool horiz);

      static bool create(::mforms::Splitter *self, bool horiz);
      static void add(Splitter *self, View *child, int minwidth, bool fixed);
      static void remove(Splitter *self, View *child);
      static void set_divider_position(Splitter *self, int pos);
      static int get_divider_position(Splitter *self);
      static void set_expanded(Splitter *self, bool first, bool expand);

    public:
      static void init();

      virtual ~SplitterImpl();
    };
  }
}

#endif /* _LF_SPLITTER_H_ */
