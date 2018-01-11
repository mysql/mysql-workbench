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
