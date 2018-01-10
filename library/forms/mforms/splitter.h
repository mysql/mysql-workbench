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

#pragma once

#include "mforms/container.h"
#include <map>

namespace mforms {

  class Splitter;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct SplitterImplPtrs {
#ifdef __APPLE__
    bool (*create)(Splitter *self, bool horizontal, bool thin);
#else
    bool (*create)(Splitter *self, bool horizontal);
#endif
    void (*add)(Splitter *self, View *child, int minsize, bool fixed);
    void (*remove)(Splitter *self, View *child);
    void (*set_divider_position)(Splitter *self, int);
    int (*get_divider_position)(Splitter *self);
    void (*set_expanded)(Splitter *self, bool first, bool expand);
  };
#endif
#endif

  /** A splitter/split view/paned view to accommodate 2 child views with a draggable divider */
  class MFORMS_EXPORT Splitter : public Container {
  public:
    /** Constructor.

     @param horiz - whether child views should be placed side by side or below the other */
    Splitter(bool horiz, bool thin = false);

    /** Adds a child view, only 2 child views can be added.

     @param minsize - minimal size this view can have
     @param fixed - whether the panel should stay in the same size when the splitter is resized */
    void add(View *subview, int minsize = 0, bool fixed = false);

    /** Remove a child view */
    virtual void remove(View *subview);

    /** Sets/gets position of the divider */
    void set_divider_position(int position);
    int get_divider_position();

    /** Sets either the first (left/top) or the second (right/bottom) part of the splitter to visible or hidden. */
    void set_expanded(bool first, bool expand);

#ifndef SWIG
    boost::signals2::signal<void()> *signal_position_changed() {
      return &_position_changed_signal;
    }
#endif

  public:
    void position_changed();

  protected:
    SplitterImplPtrs *_splitter_impl;

    boost::signals2::signal<void()> _position_changed_signal;
  };
};
