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

#ifndef _MDC_AREA_GROUP_H_
#define _MDC_AREA_GROUP_H_

#include "mdc_group.h"

namespace mdc {

  class CanvasItem;

  class MYSQLCANVAS_PUBLIC_FUNC AreaGroup : public Group {
  public:
    AreaGroup(Layer *layer);
    virtual ~AreaGroup();

    virtual void set_selected(bool flag);

    virtual void move_item(CanvasItem *item, const base::Point &pos);

    virtual bool can_render_gl() {
      return true;
    }

    virtual void repaint(const base::Rect &clipArea, bool direct);

    void repaint_contents(const base::Rect &localClipArea, bool direct);

  protected:
    bool _dragged;
    bool _drag_selects_contents;

    virtual void update_bounds();
    base::Rect constrain_rect_to_bounds(const base::Rect &rect);

    virtual bool on_click(CanvasItem *target, const base::Point &point, MouseButton button, EventState state);
    virtual bool on_button_press(CanvasItem *target, const base::Point &point, MouseButton button, EventState state);
    virtual bool on_button_release(CanvasItem *target, const base::Point &point, MouseButton button, EventState state);
    virtual bool on_drag(CanvasItem *target, const base::Point &point, EventState state);
  };

} // end of mdc namespace

#endif
