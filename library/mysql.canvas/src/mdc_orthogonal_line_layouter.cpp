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

#include "mdc_orthogonal_line_layouter.h"

#include "mdc_magnet.h"
#include "mdc_connector.h"
#include "mdc_algorithms.h"
#include "mdc_line_segment_handle.h"

using namespace mdc;
using namespace base;

#define IS_VERTICAL_ANGLE(angle) ((angle) == 90.0 || (angle) == 270.0)

OrthogonalLineLayouter::OrthogonalLineLayouter(Connector *sconn, Connector *econn) : _linfo(sconn, econn) {
  sconn->set_update_handler(std::bind(&OrthogonalLineLayouter::connector_changed, this, std::placeholders::_1));
  econn->set_update_handler(std::bind(&OrthogonalLineLayouter::connector_changed, this, std::placeholders::_1));

  _updating = false;
}

OrthogonalLineLayouter::~OrthogonalLineLayouter() {
  delete _linfo.start_connector();
  delete _linfo.end_connector();
}

std::vector<Point> OrthogonalLineLayouter::get_points() {
  std::vector<Point> p;

  for (int c = _linfo.count_sublines(), i = 0; i < c; i++) {
    std::vector<Point> tmp(get_points_for_subline(i));

    p.insert(p.end(), tmp.begin(), tmp.end());
  }

  return p;
}

Point OrthogonalLineLayouter::get_start_point() {
  return _linfo.subline_start_point(_linfo.start_subline());
}

Point OrthogonalLineLayouter::get_end_point() {
  return _linfo.subline_end_point(_linfo.end_subline());
}

void OrthogonalLineLayouter::set_segment_offset(int subline, double offset) {
  _linfo.set_subline_offset(subline, offset);
}

double OrthogonalLineLayouter::angle_of_intersection_with_rect(const Rect &rect, const Point &p) {
  double langle = angle_of_line(rect.center(), p);
  double tl_angle = angle_of_line(rect.center(), rect.top_left());
  double bl_angle = angle_of_line(rect.center(), rect.bottom_left());
  double tr_angle = angle_of_line(rect.center(), rect.top_right());
  double br_angle = angle_of_line(rect.center(), rect.bottom_right());

  if (langle >= tl_angle && langle < bl_angle)
    return 180;
  else if (langle >= bl_angle && langle < br_angle)
    return 270;
  else if ((langle >= br_angle && langle <= 360) || (langle >= 0 && langle < tr_angle))
    return 0;
  else
    return 90;
}

bool OrthogonalLineLayouter::update_start_point() {
  Point p(_linfo.start_connector()->get_position());
  mdc::CanvasItem *item = _linfo.start_connector()->get_connected_item();
  mdc::Magnet *magnet = _linfo.start_connector()->get_connected_magnet();
  double angle;

  if (item) {
    Rect item_bounds = item->get_root_bounds();
    Point pp(
      magnet->get_position_for_connector(_linfo.start_connector(), _linfo.subline_end_point(_linfo.start_subline())));
    angle = angle_of_intersection_with_rect(item_bounds, pp);
    angle = _linfo.start_connector()->get_connected_magnet()->constrain_angle(angle);
    p = pp;

    // clip angles to 90 degrees
    angle = floor((angle + 45) / 90.0) * 90.0;
    if (angle == 360)
      throw;

    // make sure the angle is ok (ie the line won't go inside the item)
    if (angle == 0 && p.x == item_bounds.left())
      angle = 180;
    else if (angle == 180 && p.x == item_bounds.right())
      angle = 0;
  } else {
    p = _linfo.subline_start_point(_linfo.start_subline());
    angle = 0;
  }

  if (_linfo.subline_start_point(_linfo.start_subline()) != p ||
      _linfo.subline_start_angle(_linfo.start_subline()) != angle) {
    _linfo.set_subline_start_point(_linfo.start_subline(), p, angle);

    return true;
  }
  return false;
}

bool OrthogonalLineLayouter::update_end_point() {
  Point p(_linfo.end_connector()->get_position());
  mdc::CanvasItem *item = _linfo.end_connector()->get_connected_item();
  mdc::Magnet *magnet = _linfo.end_connector()->get_connected_magnet();
  double angle;

  if (item) {
    Rect item_bounds = item->get_root_bounds();
    Point pp(
      magnet->get_position_for_connector(_linfo.end_connector(), _linfo.subline_start_point(_linfo.end_subline())));
    angle = angle_of_intersection_with_rect(item_bounds, pp);
    angle = _linfo.end_connector()->get_connected_magnet()->constrain_angle(angle);
    p = pp;

    // clip angles to 90 degrees
    angle = floor((angle + 45) / 90.0) * 90.0;
    if (angle == 360)
      throw;

    // make sure the angle is ok (ie the line won't go inside the item)
    if (angle == 0 && p.x == item_bounds.left())
      angle = 180;
    else if (angle == 180 && p.x == item_bounds.right())
      angle = 0;
  } else {
    p = _linfo.subline_end_point(_linfo.end_subline());
    angle = 0;
  }

  if (_linfo.subline_end_point(_linfo.end_subline()) != p || _linfo.subline_end_angle(_linfo.end_subline()) != angle) {
    _linfo.set_subline_end_point(_linfo.end_subline(), p, angle);

    return true;
  }
  return false;
}

void OrthogonalLineLayouter::update() {
  _change_pending = true;
  connector_changed(_linfo.start_connector());

  if (_change_pending)
    _changed();
}

void OrthogonalLineLayouter::connector_changed(Connector *conn) {
  bool changed = false;

  if (_updating)
    return;

  _updating = true;
  if (conn == _linfo.start_connector()) {
    changed = update_start_point();
    if (changed && _linfo.end_connector())
      update_end_point();
  } else if (conn == _linfo.end_connector()) {
    changed = update_end_point();
    if (changed && _linfo.start_connector())
      update_start_point();
  }

  if (changed) {
    //    int sl= _linfo.start_subline();

    /*
    // ensure sanity
    if (_linfo.subline_start_angle(sl) != _linfo.subline_end_angle(sl))
    {
      if (_linfo.subline_start_angle(sl) == 0 &&
        _linfo.subline_start_point(sl).x > _linfo.subline_end_point(sl).x)
      {
        _linfo.set_subline_end_point(sl,
          Point(_linfo.end_connector()->get_connected_item()->get_root_bounds().xmax(),
          _linfo.subline_end_point(sl).y),
          0);
      }
      else if (_linfo.subline_end_angle(sl) == 0 &&
        _linfo.subline_end_point(sl).x > _linfo.subline_start_point(sl).x)
      {
        _linfo.set_subline_start_point(sl,
          Point(_linfo.start_connector()->get_connected_item()->get_root_bounds().xmax(),
          _linfo.subline_start_point(sl).y),
          0);
      }
    }*/

    _change_pending = false;
    _changed();
  }
  _updating = false;
}

/*
*********************************************************************************
* @brief get points that define a part of a line
*
* Calculates the segments needed to connect the start and end points in the
* subline and return the points that define them. The subline can have 2 or
* 3 segments, depending on whether the starting and end points are perpendicular
* or parallel.
*
*  @param subline index of subline
*
*  @return vector with 3 or 4 points
*********************************************************************************
*/
std::vector<Point> OrthogonalLineLayouter::get_points_for_subline(int subline) {
  std::vector<Point> points;
  Point start(_linfo.subline_start_point(subline));
  Point end(_linfo.subline_end_point(subline));
  double start_angle = _linfo.subline_start_angle(subline);
  double end_angle = _linfo.subline_end_angle(subline);

  if (IS_VERTICAL_ANGLE(start_angle) != IS_VERTICAL_ANGLE(end_angle)) {
    // perpendicular, so 2 segments
    points.push_back(start.round());
    if (IS_VERTICAL_ANGLE(start_angle))
      points.push_back(Point(start.x, end.y).round());
    else
      points.push_back(Point(end.x, start.y).round());
    points.push_back(end.round());
  } else {
    // XXX check if inital and final segments can be merged

    points.push_back(start.round());

    if (start_angle == end_angle) {
      // start/end angles are the same, which needs special handling
      if (IS_VERTICAL_ANGLE(start_angle)) {
        double y;

        if (start_angle == 90) // goes up
          y = std::min(start.y, end.y) - 30 + _linfo.subline_offset(subline);
        else
          y = std::max(start.y, end.y) + 30 + _linfo.subline_offset(subline);

        points.push_back(Point(start.x, y).round());
        points.push_back(Point(end.x, y).round());
      } else {
        double x;

        if (start_angle == 0) // goes right
          x = std::max(start.x, end.x) + 30 + _linfo.subline_offset(subline);
        else
          x = std::min(start.x, end.x) - 30 + _linfo.subline_offset(subline);

        points.push_back(Point(x, start.y).round());
        points.push_back(Point(x, end.y).round());
      }
    } else {
      // start/end angles are parallel but not the same

      if (IS_VERTICAL_ANGLE(start_angle)) {
        double middle = (start.y + end.y) / 2 + _linfo.subline_offset(subline);

        if (start.y > end.y) {
          if (middle < end.y)
            middle = end.y;
          else if (middle > start.y)
            middle = start.y;
        } else {
          if (middle < start.y)
            middle = start.y;
          else if (middle > end.y)
            middle = end.y;
        }

        points.push_back(Point(start.x, middle).round());
        points.push_back(Point(end.x, middle).round());
      } else {
        double middle = (start.x + end.x) / 2 + _linfo.subline_offset(subline);

        if (start.x > end.x) {
          if (middle < end.x)
            middle = end.x;
          else if (middle > start.x)
            middle = start.x;
        } else {
          if (middle < start.x)
            middle = start.x;
          else if (middle > end.x)
            middle = end.x;
        }

        points.push_back(Point(middle, start.y).round());
        points.push_back(Point(middle, end.y).round());
      }
    }
    points.push_back(end.round());
  }
  return points;
}

std::vector<ItemHandle *> OrthogonalLineLayouter::create_handles(Line *line, InteractionLayer *ilayer) {
  std::vector<ItemHandle *> handles = super::create_handles(line, ilayer);

  for (int c = _linfo.count_sublines(), i = 0; i < c; i++) {
    if (!_linfo.subline_is_perpendicular(i)) {
      std::vector<Point> pts = get_points_for_subline(i);

      Point pos = Point((pts[1].x + pts[2].x) / 2, (pts[1].y + pts[2].y) / 2);

      ItemHandle *hdl =
        new LineSegmentHandle(ilayer, line, pos, !_linfo.angle_is_vertical(_linfo.subline_start_angle(i)));
      hdl->set_tag(100 + i);

      handles.push_back(hdl);
    }
  }
  return handles;
}

void OrthogonalLineLayouter::update_handles(Line *line, std::vector<ItemHandle *> &handles) {
  super::update_handles(line, handles);

  for (std::vector<ItemHandle *>::iterator iter = handles.begin(); iter != handles.end(); ++iter) {
    if ((*iter)->get_tag() >= 100 && (*iter)->get_tag() < 100 + _linfo.count_sublines()) {
      LineSegmentHandle *hdl = dynamic_cast<LineSegmentHandle *>(*iter);
      int subline = (*iter)->get_tag() - 100;

      if (!_linfo.subline_is_perpendicular(subline)) {
        std::vector<Point> pts = get_points_for_subline(subline);

        Point pos = Point((pts[1].x + pts[2].x) / 2, (pts[1].y + pts[2].y) / 2);

        hdl->move(pos);
        hdl->set_vertical(!_linfo.angle_is_vertical(_linfo.subline_start_angle(subline)));
      }
    }
  }
}

bool OrthogonalLineLayouter::handle_dragged(Line *line, ItemHandle *handle, const Point &pos, bool dragging) {
  if (handle->get_tag() >= 100 && handle->get_tag() < 100 + _linfo.count_sublines()) {
    LineSegmentHandle *hdl = dynamic_cast<LineSegmentHandle *>(handle);
    if (hdl) {
      int subline = hdl->get_tag() - 100;
      Point start = _linfo.subline_start_point(subline);
      Point end = _linfo.subline_end_point(subline);

      points_reorder(start, end);

      if (hdl->is_vertical()) {
        double offset = _linfo.subline_offset(subline) + pos.x - handle->get_position().x;

        if (_linfo.subline_start_angle(subline) != _linfo.subline_end_angle(subline)) {
          if ((start.x + end.x) / 2 + offset < start.x)
            offset = start.x - (start.x + end.x) / 2;
          else if ((start.x + end.x) / 2 + offset > end.x)
            offset = end.x - (start.x + end.x) / 2;
        }
        _linfo.set_subline_offset(subline, offset);
        return true;
      } else {
        double offset = _linfo.subline_offset(subline) + pos.y - handle->get_position().y;

        if (_linfo.subline_start_angle(subline) != _linfo.subline_end_angle(subline)) {
          if ((start.y + end.y) / 2 + offset < start.y)
            offset = start.y - (start.y + end.y) / 2;
          else if ((start.y + end.y) / 2 + offset > end.y)
            offset = end.y - (start.y + end.y) / 2;
        }
        _linfo.set_subline_offset(subline, offset);
        return true;
      }
    }
  }

  return super::handle_dragged(line, handle, pos, dragging);
}
