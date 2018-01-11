/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mforms/box.h"
#include "mforms/drawbox.h"
#include "cairo/cairo.h"

/**
 * The section box is a container with a hidable area (collapse/expand). It provides a header with action buttons
 * and can use tabs for switchable content.
 */
namespace mforms {

  class HeaderBox;

  class MFORMS_EXPORT SectionBox : public Box {
    friend class HeaderBox;

  private:
    HeaderBox* _header;
    View* _content;
    std::string _title;
    bool _expandable;
    bool _expanded;
    cairo_surface_t* _unexpandable_icon;
    cairo_surface_t* _unexpanded_icon;
    cairo_surface_t* _expanded_icon;

  public:
    SectionBox(bool expandable, const std::string& title, bool header_mode = false);
    ~SectionBox();

    void set_content(View* page);
    void toggle();
    void set_expanded(bool expanded);
    bool get_expanded() {
      return _expanded;
    }
  };

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  // Private class, so don't expose it to Python.
  class HeaderBox : public DrawBox {
  private:
    SectionBox* _owner;
    double _caption_offset;
    double _icon_left;
    double _icon_right;
    double _icon_top;
    double _icon_bottom;
    bool _header_mode; // Draw in special style (rounded corners and gradient).

    void draw_background(cairo_t* cr, int width, int height);

  public:
    HeaderBox(SectionBox* owner, bool header_mode);

    void repaint(cairo_t* cr, int x, int y, int w, int h);
    bool mouse_down(mforms::MouseButton button, int x, int y);
  };
#endif
#endif
}
