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
