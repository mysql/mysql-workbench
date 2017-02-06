/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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
#ifndef _STUB_DRAWBOX_H_
#define _STUB_DRAWBOX_H_

#include "stub_view.h"

namespace mforms {
  class DrawBox;

  namespace stub {

    class DrawBoxWrapper : public ViewWrapper {
    protected:
      DrawBoxWrapper(::mforms::DrawBox *self);
      static bool create(mforms::DrawBox *self);
      static void set_needs_repaint(mforms::DrawBox *self);
      static void add(mforms::DrawBox *, mforms::View *, mforms::Alignment alignment);
      static void remove(mforms::DrawBox *, mforms::View *);
      static void move(mforms::DrawBox *, mforms::View *, int x, int y);

    public:
      static void init();

      virtual void set_size(int width, int height);
    };
  };
};

#endif /* _STUB_DRAWBOX_H_ */
