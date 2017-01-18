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

#include "mdc_line_segment_handle.h"

using namespace mdc;
using namespace base;

LineSegmentHandle::LineSegmentHandle(InteractionLayer *ilayer, CanvasItem *item, const Point &pos, bool vertical)
  : ItemHandle(ilayer, item, pos) {
  set_color(Color(0.4, 0.7, 1));
  _vertical = vertical;
}

LineSegmentHandle::~LineSegmentHandle() {
}

void LineSegmentHandle::set_vertical(bool flag) {
  _vertical = flag;
}

Rect LineSegmentHandle::get_bounds() const {
  Rect r;
  Point pos(_pos.round());

  if (_vertical) {
    r.pos.x = pos.x - 2.5;
    r.pos.y = pos.y - 6.5;
    r.size.width = 5;
    r.size.height = 12;
  } else {
    r.pos.x = pos.x - 6.5;
    r.pos.y = pos.y - 2.5;
    r.size.width = 12;
    r.size.height = 5;
  }

  return r;
}
