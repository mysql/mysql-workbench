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

#include "mdc_box_handle.h"
#include "mdc_interaction_layer.h"
#include "mdc_canvas_view.h"

using namespace mdc;
using namespace base;

BoxHandle::BoxHandle(InteractionLayer *ilayer, CanvasItem *item, const Point &pos) : ItemHandle(ilayer, item, pos) {
  set_color(Color(1, 1, 1));
}

BoxHandle::~BoxHandle() {
}

Rect BoxHandle::get_bounds() const {
  Rect r;

  // try to unscale the zoom factor, so the handles are +/- the same size regardless of the zoom level
  if (_draggable) {
    double size = floor(8 / _layer->get_view()->get_zoom());
    r.pos.x = _pos.x - size / 2;
    r.pos.y = _pos.y - size / 2;
    r.size.width = size;
    r.size.height = size;
  } else {
    double size = floor(6 / _layer->get_view()->get_zoom());
    r.pos.x = _pos.x - size / 2;
    r.pos.y = _pos.y - size / 2;
    r.size.width = size;
    r.size.height = size;
  }
  return r;
}
