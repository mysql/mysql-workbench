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

#include <mforms/panel.h>

namespace mforms {
  class ScrollPanel;

  enum ScrollPanelFlags {
    ScrollPanelNoFlags = 0,
    ScrollPanelBordered = (1 << 0),
    ScrollPanelDrawBackground = (1 << 1),
    ScrollPanelNoAutoScroll = (1 << 2)
  };

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct MFORMS_EXPORT ScrollPanelImplPtrs {
    bool (*create)(ScrollPanel *, ScrollPanelFlags flags);

    void (*add)(ScrollPanel *, View *);
    void (*remove)(ScrollPanel *);

    void (*set_visible_scrollers)(ScrollPanel *, bool vertical, bool horizontal);
    void (*set_autohide_scrollers)(ScrollPanel *, bool flag);
    void (*scroll_to_view)(ScrollPanel *, View *);

    base::Rect (*get_content_rect)(ScrollPanel *);
    void (*scroll_to)(ScrollPanel *, int x, int y);
  };
#endif
#endif

  /** A container panel with scrollbars.

   Single item container that will show scrollbars to accommodate arbitrarily sized
   contents. */
  class MFORMS_EXPORT ScrollPanel : public Container {
  public:
    /** Constructor.

     @param flags -
       @li ScrollPanelBordered - whether a single pixel border should be drawn around the contents
       @li ScrollPanelDrawBackground - whether the background of the panel should be transparent
     */
    ScrollPanel(ScrollPanelFlags flags = ScrollPanelNoFlags);
    ~ScrollPanel();

    /** Sets whether the vertical and horizontal scrollbars should be created.

     By default, both scrollbars are created. */
    void set_visible_scrollers(bool vertical, bool horizontal);

    /** Sets whether scrollbars are automatically hidden when the contents fit in the
     available space. */
    void set_autohide_scrollers(bool flag);

    /** Sets content view. */
    void add(View *child);
    /** Removes the content view. */
    void remove();

    /* Tries to scroll to given child. In other words to make sure child is visible. */
    void scroll_to_view(View *child);

    base::Rect get_content_rect();

    /* Scroll to the given x, y.*/
    void scroll_to(int x, int y);

  protected:
    ScrollPanelImplPtrs *_spanel_impl;
  };
};
