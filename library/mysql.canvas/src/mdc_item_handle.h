/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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
