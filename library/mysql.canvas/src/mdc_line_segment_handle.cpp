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
