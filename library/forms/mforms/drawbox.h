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

#pragma once

#include <mforms/base.h>
#include <mforms/view.h>
#include <base/geometry.h>

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
  };

  struct MFORMS_EXPORT Accessible {
  public:
    enum Role {
      RoleNone,
      Client,
      Pane,
      Link,
      List,
      ListItem,
      PushButton,
      StaticText,
      Text,
      Outline,
      OutlineButton,
      OutlineItem
    };

    virtual ~Accessible(){};

    // This each child class should implement it's own way to get the
    // accessible name and role
    virtual std::string get_acc_name() = 0;
    virtual Role get_acc_role() = 0;

    // The rest of the accessible methods are optional on the child
    // classes, they must implement them as needed
    virtual std::string get_acc_description() {
      return "";
    }
    virtual std::string get_acc_value() {
      return "";
    }

    virtual int get_acc_child_count() {
      return 0;
    }
    virtual Accessible *get_acc_child(int index) {
      return NULL;
    }

    virtual base::Rect get_acc_bounds() {
      return base::Rect();
    }
    virtual Accessible *hit_test(int x, int y) {
      return NULL;
    }

    virtual std::string get_acc_default_action() {
      return "";
    }
    virtual void do_default_action(){};
  };
#endif
#endif

  class MFORMS_EXPORT DrawBox : public View, public Accessible {
  public:
    DrawBox();

    /** Adds a child view to the box. There's no layout support here. Use a Box if you need that.
     * The subview is placed and size according to what is specified for it. */
    void add(View *view, Alignment alignment);

    /** Removal of a child view. */
    void remove(View *view);

    /** Move the given child window to a fixed position. Automatically sets the alignment to NoAlign. */
    void move(View *child, int x, int y);

    virtual void set_layout_dirty(bool value);
    void set_padding(int left, int top, int right, int bottom);

    void set_needs_repaint();
    void set_needs_repaint_area(int x, int y, int w, int h);
#ifndef DOXYGEN_SHOULD_SKIP_THIS
    virtual base::Size getLayoutSize(base::Size proposedSize);

#ifndef SWIG
    virtual void repaint(cairo_t *cr, int x, int y, int w, int h) {
    }
    virtual void cancel_operation(){};

    virtual std::string get_acc_name() {
      return get_name();
    }
    virtual base::Rect get_acc_bounds() {
      return base::Rect(get_x(), get_y(), get_width(), get_height());
    }
    virtual Role get_acc_role() {
      return mforms::Accessible::RoleNone;
    }
#endif
#endif
  protected:
    DrawBoxImplPtrs *_drawbox_impl;
  };
};
