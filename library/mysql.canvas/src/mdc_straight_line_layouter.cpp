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

#include "mdc_straight_line_layouter.h"
#include "mdc_connector.h"

using namespace mdc;
using namespace base;

StraightLineLayouter::StraightLineLayouter(Connector *sconn, Connector *econn) : _start_conn(sconn), _end_conn(econn) {
  sconn->set_update_handler(std::bind(&StraightLineLayouter::connector_changed, this, std::placeholders::_1));
  econn->set_update_handler(std::bind(&StraightLineLayouter::connector_changed, this, std::placeholders::_1));

  _start = sconn->get_position();
  _end = econn->get_position();
}

StraightLineLayouter::~StraightLineLayouter() {
  delete _start_conn;
  delete _end_conn;
}

std::vector<Point> StraightLineLayouter::get_points() {
  std::vector<Point> p(2);
  p[0] = _start;
  p[1] = _end;

  return p;
}

Point StraightLineLayouter::get_start_point() {
  return _start;
}

Point StraightLineLayouter::get_end_point() {
  return _end;
}

void StraightLineLayouter::update() {
  connector_changed(_start_conn);
}

void StraightLineLayouter::connector_changed(Connector *conn) {
  Point p = conn->get_position();
  bool changed = false;

  if (conn == _start_conn) {
    mdc::CanvasItem *item = conn->get_connected_item();
    if (item)
      p = item->get_intersection_with_line_to(_end);

    if (_start != p) {
      _start = p;
      changed = true;
    }

    if (_end_conn) {
      item = _end_conn->get_connected_item();
      if (item) {
        p = item->get_intersection_with_line_to(_start);
        if (_end != p) {
          _end = p;
          changed = true;
        }
      }
    }
  }

  if (conn == _end_conn) {
    mdc::CanvasItem *item = conn->get_connected_item();
    if (item)
      p = item->get_intersection_with_line_to(_start);

    if (_end != p) {
      _end = p;
      changed = true;
    }

    if (_start_conn) {
      item = _start_conn->get_connected_item();
      if (item) {
        p = item->get_intersection_with_line_to(_end);
        if (_start != p) {
          _start = p;
          changed = true;
        }
      }
    }
  }
  if (changed)
    _changed();
}
