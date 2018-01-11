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

#include "mdc_interaction_layer.h"
#include "mdc_canvas_view.h"
#include "mdc_algorithms.h"
#include "mdc_item_handle.h"
#include "mdc_draw_util.h"

using namespace mdc;
using namespace base;

InteractionLayer::InteractionLayer(CanvasView *view) : Layer(view) {
  _dragging_handle = 0;
  _selection_started = false;
  _selection_started_by_us = false;
  _dragging_rectangle = false;
}

void InteractionLayer::draw_selection(const Rect &clip) {
  CairoCtx *cr = _owner->cairoctx();
  Point p1 = _selection_start.round();
  Point p2 = _selection_end.round();

  if (fabs(p1.x - p2.x) <= 1 || fabs(p1.y - p2.y) <= 1)
    return;

  points_reorder(p1, p2);

  if (_owner->has_gl()) {
    Color fill_color = Color(0.8f, 0.8f, 0.9f, 0.6f);
    Color border_color = Color(0.5f, 0.5f, 0.6f, 0.9f);
    Rect bounds = Rect(p1.x + 1.5, p1.y + 1.5, floor(p2.x - p1.x) - 2, floor(p2.y - p1.y) - 2);
    gl_box(bounds, border_color, fill_color);
  } else {
    cr->save();
    cr->rectangle(p1.x + 1.5, p1.y + 1.5, floor(p2.x - p1.x) - 2, floor(p2.y - p1.y) - 2);
    cr->set_color(Color(0.9, 0.9, 0.9, 0.4));
    cr->set_line_width(1);
    cr->fill_preserve();
    cr->set_color(Color(0.5, 0.5, 0.6, 0.9));
    cr->stroke();
    cr->restore();
  }
}

void InteractionLayer::draw_dragging_rectangle() {
  CairoCtx *cr = _owner->cairoctx();
  Point p1 = _dragging_rectangle_start;
  Point p2 = _dragging_rectangle_end;

  points_reorder(p1, p2);

  if (_owner->has_gl()) {
    Color fill_color = Color(0.6f, 0.6f, 0.9f, 0.6f);
    Color border_color = Color(0.5f, 0.5f, 0.6f, 0.9f);
    Rect bounds = Rect(p1.x + 0.5, p1.y + 0.5, p2.x - p1.x - 2, p2.y - p1.y - 2);
    gl_box(bounds, border_color, fill_color);
  } else {
    cr->save();
    cr->rectangle(p1.x + 0.5, p1.y + 0.5, p2.x - p1.x - 2, p2.y - p1.y - 2);
    cr->set_color(Color(0.6, 0.6, 0.9, 0.4));
    cr->set_line_width(1);
    cr->fill_preserve();
    cr->set_color(Color(0.5, 0.5, 0.6, 0.9));
    cr->stroke();
    cr->restore();
  }
}

void InteractionLayer::repaint(const Rect &bounds) {
  if (_selection_started)
    draw_selection(bounds);

  if (_dragging_rectangle)
    draw_dragging_rectangle();

  if (_active_area.width() > 0 && _active_area.height() > 0) {
    CairoCtx *cr = _owner->cairoctx();
    Size vsize = _owner->get_total_view_size();

    cr->save();
    cr->set_color(Color(0.0, 0.0, 0.0, 0.7));
    fill_hollow_rectangle(cr, Rect(Point(0, 0), vsize), _active_area);
    cr->restore();
  }

  _owner->lock();
  for (std::list<ItemHandle *>::iterator iter = _handles.begin(); iter != _handles.end(); ++iter) {
    (*iter)->repaint(_owner->cairoctx());
  }
  _owner->unlock();

  _custom_repaint(_owner->cairoctx());

  Layer::repaint(bounds);
}

void InteractionLayer::add_handle(ItemHandle *handle) {
  _handles.push_back(handle);
}

void InteractionLayer::remove_handle(ItemHandle *handle) {
  _handles.remove(handle);
}

ItemHandle *InteractionLayer::get_handle_at(const Point &pos) {
  for (std::list<ItemHandle *>::iterator iter = _handles.begin(); iter != _handles.end(); ++iter) {
    if (bounds_contain_point((*iter)->get_bounds(), pos.x, pos.y))
      return *iter;
  }
  return 0;
}

bool InteractionLayer::handle_mouse_move(const Point &pos_, EventState state) {
  Point pos = pos_;
  // make sure pos is inside the canvas area
  Size view_size = _owner->get_total_view_size();
  if (pos.x < 0)
    pos.x = 0;
  else if (pos.x >= view_size.width)
    pos.x = view_size.width - 1;
  if (pos.y < 0)
    pos.y = 0;
  else if (pos.y >= view_size.height)
    pos.y = view_size.height - 1;

  if (_dragging_rectangle) {
    update_dragging_rectangle(pos);
    return true;
  }

  if (_dragging_handle) {
    _dragging_handle->get_item()->on_drag_handle(_dragging_handle, pos, true);

    return true;
  }

  if (_selection_started_by_us) {
    update_selection_rectangle(pos, state);
    return true;
  }

  return false;
}

bool InteractionLayer::handle_mouse_button_top(MouseButton button, bool press, const Point &pos_, EventState state) {
  Point pos = pos_;
  ItemHandle *handle;

  if (button != ButtonLeft)
    return false;

  // make sure pos is inside the canvas area
  Size view_size = _owner->get_total_view_size();
  if (pos.x < 0)
    pos.x = 0;
  else if (pos.x >= view_size.width)
    pos.x = view_size.width - 1;
  if (pos.y < 0)
    pos.y = 0;
  else if (pos.y >= view_size.height)
    pos.y = view_size.height - 1;

  if (_dragging_rectangle && !press) {
    _dragging_rectangle_end = pos;

    _dragging_rectangle = false;
    return true;
  }

  if (_selection_started_by_us) {
    _selection_started_by_us = false;
    if (!press) {
      end_selection_rectangle(pos, state);
      return true;
    }
  }

  handle = get_handle_at(pos);

  if (press) {
    if (handle) {
      _dragging_handle = handle;
      _dragging_handle->set_highlighted(true);
      _dragging_pos = _owner->snap_to_grid(pos);
      return true;
    }
  } else {
    if (_dragging_handle) {
      _dragging_handle->get_item()->on_drag_handle(_dragging_handle, _owner->snap_to_grid(pos), false);
      _dragging_handle->set_highlighted(false);
      _dragging_handle = 0;
      return true;
    }
  }
  return false;
}

bool InteractionLayer::handle_mouse_button_bottom(MouseButton button, bool press, const Point &pos, EventState state) {
  if (button == ButtonLeft) {
    if (press) {
      // check if the point is inside the canvas area
      if (bounds_contain_point(Rect(Point(0, 0), _owner->get_total_view_size()), pos.x, pos.y)) {
        start_selection_rectangle(pos, state);
        _selection_started_by_us = true;
        return true;
      }
    }
  }
  return false;
}

void InteractionLayer::start_selection_rectangle(const Point &pos, EventState state) {
  _owner->get_selection()->begin_multi_selection();

  _selection_started = true;
  _selection_start = pos;
  _selection_end = pos;
  update_selection_rectangle(pos, state);
}

void InteractionLayer::end_selection_rectangle(const Point &pos, EventState state) {
  _selection_started = false;
  update_selection_rectangle(pos, state);
  _owner->get_selection()->end_multi_selection();
}

void InteractionLayer::update_selection_rectangle(const Point &end, EventState state) {
  Point p1, p2;
  Group *group = 0;
  CanvasItem *item;
  Point old_start(_selection_start);
  Point old_end(_selection_end);

  // get the item that the selection started in (to see if we're selecting inside a group)
  item = _owner->get_item_at(_selection_start);
  group = dynamic_cast<Group *>(item);

  p1 = _selection_start;
  p2 = _selection_end;
  points_reorder(p1, p2);

  if (_selection_end != end || !_selection_started) {
    _selection_end = end;

    points_reorder(old_start, old_end);

    _owner->queue_repaint(
      Rect(Point(std::min(old_start.x, _selection_start.x), std::min(old_start.y, _selection_start.y)),
           Point(std::max(old_end.x, _selection_end.x), std::max(old_end.y, _selection_end.y))));

    if (state & SShiftMask)
      _owner->select_items_inside(Rect(p1, p2), SelectAdd, group);
    else if (state & SControlMask)
      _owner->select_items_inside(Rect(p1, p2), SelectToggle, group);
    else
      _owner->select_items_inside(Rect(p1, p2), SelectSet, group);
  }
}

void InteractionLayer::set_active_area(const Rect &rect) {
  _active_area = rect;
  _owner->queue_repaint();
}

void InteractionLayer::reset_active_area() {
  _active_area = Rect(Point(0, 0), Size(0, 0));
}

void InteractionLayer::start_dragging_rectangle(const Point &pos) {
  _dragging_rectangle = true;
  _dragging_rectangle_start = _owner->snap_to_grid(pos);
  _dragging_rectangle_end = _owner->snap_to_grid(pos);
}

void InteractionLayer::update_dragging_rectangle(const Point &pos) {
  Point old_start(_dragging_rectangle_start);
  Point old_end(_dragging_rectangle_end);

  _dragging_rectangle_end = _owner->snap_to_grid(pos);

  points_reorder(old_start, old_end);

  _owner->queue_repaint(
    Rect(Point(std::min(old_start.x, _dragging_rectangle_start.x), std::min(old_start.y, _dragging_rectangle_start.y)),
         Point(std::max(old_end.x, _dragging_rectangle_end.x), std::max(old_end.y, _dragging_rectangle_end.y))));

  // _owner->set_needs_repaint();
}

Rect InteractionLayer::finish_dragging_rectangle() {
  points_reorder(_dragging_rectangle_start, _dragging_rectangle_end);
  Rect rect = Rect(_dragging_rectangle_start, _dragging_rectangle_end);

  _dragging_rectangle = false;

  _owner->queue_repaint();

  return rect;
}
