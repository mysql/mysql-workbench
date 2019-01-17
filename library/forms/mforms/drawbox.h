/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "base/accessibility.h"
#include "base/geometry.h"

#include "mforms/base.h"
#include "mforms/view.h"

#include "cairo/cairo.h"

namespace mforms {
  class DrawBox;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct DrawBoxImplPtrs {
    bool (*create)(DrawBox *);
    void (*set_needs_repaint)(DrawBox *);
    void (*set_needs_repaint_area)(DrawBox *, int, int, int, int); // XXX Windows, Linux
    void (*add)(DrawBox *, View *, Alignment alignment);
    void (*remove)(DrawBox *, View *);
    void (*move)(DrawBox *, View *, int x, int y);
    void (*drawFocus)(DrawBox*, cairo_t *, const base::Rect);
  };

#endif
#endif

  struct FocusableArea {
   public:
    std::function<void()> activate;
    std::function<void()> showContextMenu;
    std::function<base::Rect()> getBounds;
  };

  class MFORMS_EXPORT DrawBox : public View, public base::Accessible {
  private:
    std::vector<FocusableArea> _focusableList;
    int _focusedItem;
    int _lastFocusedItem;

  protected:
    void drawFocus(cairo_t *cr, const base::Rect r);

  public:
    DrawBox();

    /** Adds a child view to the box. There's no layout support here. Use a Box if you need that.
     * The subview is placed and size according to what is specified for it. */
    void add(View *view, Alignment alignment);

    /** Removal of a child view. */
    void remove(View *view);

    /** Move the given child window to a fixed position. Automatically sets the alignment to NoAlign. */
    void move(View *child, int x, int y);

    virtual void set_layout_dirty(bool value) override;
    void set_padding(int left, int top, int right, int bottom);

    void set_needs_repaint();
    void set_needs_repaint_area(int x, int y, int w, int h);
#ifndef DOXYGEN_SHOULD_SKIP_THIS
    virtual base::Size getLayoutSize(base::Size proposedSize);

#ifndef SWIG
    virtual void repaint(cairo_t *cr, int x, int y, int w, int h);
    virtual void cancel_operation(){}

    virtual void set_name(const std::string &name) override {
      _name = name;
    }

    virtual base::Rect getAccessibilityBounds() override {
      return base::Rect(get_x(), get_y(), get_width(), get_height());
    }

    virtual std::string getAccessibilityDescription() override {
      return _name;
    }

    virtual Role getAccessibilityRole() override {
      return base::Accessible::RoleNone;
    }

    void addFocusableArea(FocusableArea fArea);
    void clearFocusableAreas();
    bool setFocusOnArea(const base::Point p);


    virtual bool keyPress(KeyCode code, ModifierKey modifiers) override;
    virtual bool focusIn() override;
    virtual bool focusOut() override;
    virtual bool mouse_down(mforms::MouseButton button, int x, int y) override;

#endif
#endif

  private:
    DrawBoxImplPtrs *_drawbox_impl;
    std::string _name;
  };
};
