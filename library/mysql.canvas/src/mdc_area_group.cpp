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

#ifndef _MSC_VER
#include <float.h>
#endif

#include "mdc_area_group.h"
#include "mdc_canvas_item.h"
#include "mdc_canvas_view.h"
#include "mdc_algorithms.h"
#include "mdc_figure.h"
#include "mdc_selection.h"
#include "mdc_interaction_layer.h"

using namespace mdc;
using namespace base;

AreaGroup::AreaGroup(Layer *layer) : Group(layer), _dragged(false) {
  resize_to(Size(100, 100));

  _drag_selects_contents = false;
}

AreaGroup::~AreaGroup() {
}

void AreaGroup::repaint_contents(const Rect &localClipArea, bool direct) {
  if (_contents.size() > 0) {
    CairoCtx *cr = _layer->get_view()->cairoctx();

    if (_layer->get_view()->has_gl() && !direct) {
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glTranslated(get_position().x, get_position().y, 0.0);
    } else {
      cr->save();
      cr->translate(get_position());
    }

    for (std::list<CanvasItem *>::reverse_iterator iter = _contents.rbegin(); iter != _contents.rend(); ++iter) {
      if ((*iter)->get_visible() && (*iter)->intersects(localClipArea))
        (*iter)->repaint(localClipArea, direct);
    }
    if (_layer->get_view()->has_gl() && !direct) {
      glMatrixMode(GL_MODELVIEW);
      glPopMatrix();
    } else
      cr->restore();
  }
}

void AreaGroup::repaint(const Rect &clipArea, bool direct) {
  Rect localClipArea(clipArea);

  if (this != _layer->get_root_area_group()) {
    localClipArea.pos = localClipArea.pos - get_position();

    Layouter::repaint(localClipArea, direct);
  }
  repaint_contents(localClipArea, direct);
}

void AreaGroup::move_item(CanvasItem *item, const Point &pos) {
  Point npos = constrain_rect_to_bounds(Rect(pos, item->get_size())).pos;

  item->move_to(npos);
}

Rect AreaGroup::constrain_rect_to_bounds(const Rect &rect) {
  Rect r = rect;

  if (r.right() > get_size().width)
    r.pos.x = get_size().width - r.size.width;

  if (r.bottom() > get_size().height)
    r.pos.y = get_size().height - r.size.height;

  if (r.pos.x < 0)
    r.pos.x = 0;

  if (r.pos.y < 0)
    r.pos.y = 0;

  return r;
}

void AreaGroup::set_selected(bool flag) {
  Layouter::set_selected(flag);
}

void AreaGroup::update_bounds() {
  // no op
}

bool AreaGroup::on_button_press(CanvasItem *target, const Point &point, MouseButton button, EventState state) {
  Point p = convert_point_to(point, 0);

  _dragged = false;

  if (_drag_selects_contents)
    get_layer()->get_view()->get_interaction_layer()->start_selection_rectangle(p, state);
  else
    return Group::on_button_press(target, point, button, state);
  return false;
}

bool AreaGroup::on_button_release(CanvasItem *target, const Point &point, MouseButton button, EventState state) {
  Point p = convert_point_to(point, 0);

  if (_drag_selects_contents)
    get_layer()->get_view()->get_interaction_layer()->end_selection_rectangle(p, state);

#ifdef no_group_activate
  if (!_dragged) {
    if (get_selected())
      activate_group(true);
  }
#endif
  return Group::on_button_release(target, point, button, state);
}

bool AreaGroup::on_drag(CanvasItem *target, const Point &point, EventState state) {
  Point p = convert_point_to(point, 0);

  _dragged = true;

  if (_drag_selects_contents)
    get_layer()->get_view()->get_interaction_layer()->update_selection_rectangle(p, state);
  else
    return Group::on_drag(target, point, state);

  return true;
}

bool AreaGroup::on_click(CanvasItem *target, const Point &point, MouseButton button, EventState state) {
  if (!_dragged && accepts_selection()) {
    if (state & SControlMask) {
      if (!get_selected())
        get_layer()->get_view()->focus_item(this);
      else
        get_layer()->get_view()->focus_item(0);

      get_layer()->get_view()->get_selection()->toggle(this);
    } else if ((state & SModifierMask) == 0) {
      get_layer()->get_view()->focus_item(this);
      get_layer()->get_view()->get_selection()->set(this);
    }
  }
  return true;
}
