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

#include <mforms/base.h>
#include <mforms/view.h>

#include "cairo/cairo.h"
#include "base/geometry.h"

/**
 * This class implements a popup window class which can be used for context menus or drop down menus.
 * A popup is always a modal window and cannot be used to embed other controls nor can itself be embedded.
 */
namespace mforms {
  enum PopupStyle {
    PopupPlain, // A simple popup window, similar to context menus, but non-modal.
    PopupBezel, // A semi-transparent black and modal window with rounded corners and a white border.
  };

  class Popup;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct PopupImplPtrs {
    bool (*create)(Popup *, PopupStyle);
    void (*destroy)(Popup *);
    void (*set_needs_repaint)(Popup *);
    void (*set_size)(Popup *, int, int);
    int (*show)(Popup *, int, int);
    base::Rect (*get_content_rect)(Popup *);
    void (*set_modal_result)(Popup *, int result);
  };
#endif
#endif

  class MFORMS_EXPORT Popup : public Object {
  public:
    Popup(PopupStyle style);
    ~Popup();

    void set_needs_repaint();
    void set_size(int width, int height);
    int show(int spot_x, int spot_y);
    base::Rect get_content_rect();
    void set_modal_result(int result);

    void closed();

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
    boost::signals2::signal<void()> *on_close() {
      return &_on_close;
    }

    virtual void repaint(cairo_t *cr, int x, int y, int w, int h) {
    }

    virtual bool mouse_down(mforms::MouseButton button, int x, int y) {
      return false;
    }
    virtual bool mouse_up(mforms::MouseButton button, int x, int y) {
      return false;
    }
    virtual bool mouse_click(mforms::MouseButton button, int x, int y) {
      return false;
    }
    virtual bool mouse_double_click(mforms::MouseButton button, int x, int y) {
      return false;
    }
    virtual bool mouse_enter() {
      return false;
    }
    virtual bool mouse_leave() {
      return false;
    }
    virtual bool mouse_move(mforms::MouseButton button, int x, int y) {
      return false;
    }
#endif
#endif
  protected:
    PopupImplPtrs *_popup_impl;
    boost::signals2::signal<void()> _on_close; // Callback for non-modal popups.
  };
};
