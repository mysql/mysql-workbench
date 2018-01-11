/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MDC_ITEM_HANDLE_H_
#define _MDC_ITEM_HANDLE_H_

#include "mdc_common.h"

namespace mdc {

  class InteractionLayer;
  class CanvasItem;

  class MYSQLCANVAS_PUBLIC_FUNC ItemHandle {
  public:
    ItemHandle(InteractionLayer *ilayer, CanvasItem *item, const base::Point &pos);
    virtual ~ItemHandle();

    virtual void repaint(CairoCtx *cr);
    virtual base::Rect get_bounds() const = 0;

    void move(const base::Point &point);
    base::Point get_position() const {
      return _pos;
    };

    CanvasItem *get_item() const {
      return _item;
    };

    void set_highlighted(bool flag);
    void set_draggable(bool flag);
    bool is_draggable() {
      return _draggable;
    }

    void set_tag(int tag) {
      _tag = tag;
    };
    inline int get_tag() {
      return _tag;
    };

    void set_color(const base::Color &color);
    void paint_gl(base::Rect &r);

  protected:
    CanvasItem *_item;
    InteractionLayer *_layer;
    base::Color _color;
    GLuint _display_list;

    base::Point _pos;

    int _tag;

    bool _dirty;
    bool _highlighted;
    bool _draggable;
  };

} // end of mdc namespace

#endif
