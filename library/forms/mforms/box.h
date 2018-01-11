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

#pragma once

#include <map>

#include "mforms/container.h"

namespace mforms {

  class Box;
  class Button;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct BoxImplPtrs {
    bool (*create)(Box *self, bool horizontal);
    void (*add)(Box *self, View *child, bool expand, bool fill);
    void (*add_end)(Box *self, View *child, bool expand, bool fill);
    void (*remove)(Box *self, View *child);
    void (*set_homogeneous)(Box *self, bool);
    void (*set_spacing)(Box *self, int);
  };
#endif
#endif

  /** Places child views sequentially in a vertical or horizontal layout.

   The container will use all available space in its parent and layout child views
   so that their minimal sizes are respected. If there is leftover space in the Box,
   it will be evenly distributed among child views that are marked to expand.
   */
  class MFORMS_EXPORT Box : public Container {
  public:
    /** Constructor.

     @param horiz - true to place child views from left to right. false to place them from
       top to bottom.
     */
    Box(bool horiz);

    /** Adds a child view, from left to right or top to bottom.

     @param subview - subview to be added
     @param expand - whether the subview should expand to use leftover space in the box
     @param fill - whether the subview should be resized to fill all allocated space or
     use only its minimum required size. If not set then the subview is centered within
     the computed space (vertically in horizontal layout, horizontally in vertical layout).
     */
    void add(View *subview, bool expand, bool fill = true);

    /** Adds a child view, from right to left or bottom to top.

     @param subview - subview to be added
     @param expand - whether the subview should expand to use leftover space in the box
     @param fill - whether the subview should be resized to fill all allocated space or
     use only its minimum required size. Only makes sense if the target size is larger
     than the preferred view size (if set to expand, homogeneous mode enabled).
     */
    void add_end(View *subview, bool expand, bool fill = true);

    /** Remove a subview */
    virtual void remove(View *subview);

    /** Sets whether all child views should have the same space. */
    void set_homogeneous(bool flag);

    /** Sets spacing between child items. */
    void set_spacing(int space);

    /** Returns the orientation of the box. */
    bool is_horizontal();

  protected:
    BoxImplPtrs *_box_impl;
    bool _is_horizontal;
  };
};
