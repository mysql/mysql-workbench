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

#include "base/geometry.h"

#include "mdc_bounds_magnet.h"
#include "mdc_algorithms.h"
#include "mdc_canvas_item.h"

using namespace mdc;

BoundsMagnet::BoundsMagnet(CanvasItem *owner) : Magnet(owner) {
}

base::Point BoundsMagnet::get_position_for_connector(Connector *conn, const base::Point &srcpos) const {
  return _owner->get_intersection_with_line_to(srcpos);
}

void BoundsMagnet::owner_bounds_changed(const base::Rect &obounds) {
  notify_connected();
}
