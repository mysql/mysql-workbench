/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
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

/**
 * Special window class to be used as a popup with child controls and a special border. It is
 * used as a "fly-out" or support window similar to tooltips showing e.g. larger amounts of text
 * and can act otherwise like a normal (but floating) window.
 * Other than the mforms popup class (which is meant for menus and the like) the popover cannot
 * be transparent and is non-modal.
 * Outer form, drop shadow etc. is provided by the platform implementations.
 */
namespace mforms {
  class Popover;

  // Determines the initial position of the popover with respect to the reference point.
  enum StartPosition {
    StartLeft,  // The popover is initially left to the ref point, having its arrow pointing to the right.
    StartRight, // Similar for the other positions.
    StartAbove,
    StartBelow
  };

  enum PopoverStyle {
    PopoverStyleNormal, // With large rounded corners, tip (arrow) etc.
    PopoverStyleTooltip // Simplified version with now tip, smaller corners etc.
  };

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct MFORMS_EXPORT PopoverImplPtrs {
    bool (*create)(Popover *self, mforms::View *owner, PopoverStyle style);
    void (*destroy)(Popover *self);
    void (*set_content)(Popover *self, View *content);
    void (*set_size)(Popover *self, int, int);
    void (*show)(Popover *self, int, int, StartPosition); // Position of the popover's tip in screen coordinates.
    void (*show_and_track)(Popover *self, View *, int, int,
                           StartPosition); // Position of the popover's tip in screen coordinates.
    void (*close)(Popover *self);
    void (*setName)(Popover *self, const std::string &name);
  };
#endif
#endif

  class MFORMS_EXPORT Popover : public Object {
    PopoverImplPtrs *_popover_impl;

  public:
    Popover(mforms::View *owner, PopoverStyle style = PopoverStyleNormal);
    virtual ~Popover();

    void set_content(View *content);

    // Size of the main body. The arrow is added implicitly and is not part of the size
    // (nor is the shadow or other decoration, if any). Size must be set before showing the
    // popover as it determines the overall shape.
    void set_size(int width, int height);

    // Displays the popover so that its arrow tip is at the given screen position. The popover itself
    // will be arranged to not overlap screen borders.
    // Parameter position indicates how the popover is to position initially, with respect to the
    // reference point. The actual screen position also depends on the size of the popover and the given location.
    // The position parameter applies only to the normal popover style. The tooltip style has no
    // arrow and is positioned with the left upper corner at the given position.
    void show(int x, int y, StartPosition position);
    void close();

    /**
     * Used for a11y
     */
    void setName(const std::string &name);

    // Same as show, but the popover will call the _bound_close callback when the mouse leaves the area
    // of the tracked view (which usually is to close the popover).
    void show_and_track(View *tracked_view, int x, int y, StartPosition position);

    boost::signals2::signal<void()> *signal_close() {
      return &_bound_close;
    }

  private:
    boost::signals2::signal<void()> _bound_close;
  };
};
