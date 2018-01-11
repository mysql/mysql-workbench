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

#include "mdc_vertex_handle.h"

using namespace mdc;
using namespace base;

VertexHandle::VertexHandle(InteractionLayer *ilayer, CanvasItem *item, const Point &pos, bool connectable)
  : ItemHandle(ilayer, item, pos), _connectable(connectable) {
}

VertexHandle::~VertexHandle() {
}

void VertexHandle::repaint(CairoCtx *cr) {
  Rect r = get_bounds();

  cr->set_color(Color(1, 1, 1, 0.8));
  cr->set_line_width(1);
  if (_connectable) {
    cr->move_to(r.left() + r.width() / 2, r.top());
    cr->line_to(r.left(), r.top() + r.height() / 2);
    cr->line_to(r.left() + r.width() / 2, r.bottom());
    cr->line_to(r.right(), r.top() + r.height() / 2);
    cr->close_path();
  } else {
    cr->rectangle(r);
  }
  cr->fill_preserve();

  if (_highlighted)
    cr->set_color(Color(0, 1, 1, 1));
  else
    cr->set_color(Color(0.0, 0.0, 0.9, 1));
  cr->stroke();
}

Rect VertexHandle::get_bounds() const {
  Rect r;
  r.pos.x = _pos.x - 4.5;
  r.pos.y = _pos.y - 4.5;
  r.size.width = 9;
  r.size.height = 9;
  return r;
}
