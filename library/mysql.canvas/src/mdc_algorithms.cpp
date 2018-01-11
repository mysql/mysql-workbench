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

#include "mdc_algorithms.h"

using namespace mdc;
using namespace base;

bool mdc::intersect_hv_lines(const Point &s1, const Point &e1, const Point &s2, const Point &e2,
                             Point &intersection_ret) {
  if (e1.y == s1.y) {
    if (e2.y == s2.y)
      return false; // parallel lines
    if ((e1.y > std::max(e2.y, s2.y)) || (e1.y < std::min(e2.y, s2.y)) || (e2.x > std::max(e1.x, s1.x)) ||
        (e2.x < std::min(e1.x, s1.x)))
      return false; // line is out
    if ((s1 == e1) || (s2 == e2))
      return false;
    intersection_ret.x = e2.x;
    intersection_ret.y = e1.y;
    return true;
  } else if (s2.x == e2.x)
    return false;
  if ((e1.x > std::max(e2.x, s2.x)) || (e1.x < std::min(e2.x, s2.x)) || (e2.y > std::max(e1.y, s1.y)) ||
      (e2.y < std::min(e1.y, s1.y)))
    return false; // line is out
  if ((s1 == e1) || (s2 == e2))
    return false;
  intersection_ret.x = e1.x;
  intersection_ret.y = e2.y;
  return true;
}

bool mdc::intersect_lines(const Point &s1, const Point &e1, const Point &s2, const Point &e2, Point &intersection_ret) {
  double a1, b1;
  double a2, b2;

  a1 = e1.y - s1.y;
  b1 = s1.x - e1.x;

  a2 = e2.y - s2.y;
  b2 = s2.x - e2.x;

  double d = a1 * b2 - a2 * b1;
  if (fabs(d) <= 0.000000001)
    return false;
  else {
    double c1 = e1.x * s1.y - s1.x * e1.y;
    double c2 = e2.x * s2.y - s2.x * e2.y;
    double x = floor((b1 * c2 - b2 * c1) / d + 0.5);
    double y = floor((a2 * c1 - a1 * c2) / d + 0.5);

    if (floor(std::min(s1.x, e1.x)) <= x && x <= ceil(std::max(s1.x, e1.x)) && floor(std::min(s1.y, e1.y)) <= y &&
        y <= ceil(std::max(s1.y, e1.y)) && floor(std::min(s2.x, e2.x)) <= x && x <= ceil(std::max(s2.x, e2.x)) &&
        floor(std::min(s2.y, e2.y)) <= y && y <= ceil(std::max(s2.y, e2.y))) {
      intersection_ret.x = x;
      intersection_ret.y = y;
      return true;
    }
    return false;
  }
}

bool mdc::intersect_rect_to_line(const Rect &rect, const Point &s, const Point &e, Point &intersection1_ret,
                                 Point &intersection2_ret) { // XXX optimize this
  std::vector<Point> intersections;
  Point p;

  if (intersect_lines(s, e, rect.top_left(), rect.top_right(), p))
    intersections.push_back(p);
  if (intersect_lines(s, e, rect.bottom_left(), rect.bottom_right(), p))
    intersections.push_back(p);
  if (intersect_lines(s, e, rect.top_left(), rect.bottom_left(), p))
    intersections.push_back(p);
  if (intersect_lines(s, e, rect.top_right(), rect.bottom_right(), p))
    intersections.push_back(p);

  if (intersections.size() > 1) {
    intersection1_ret = intersections[0];
    intersection2_ret = intersections[1];
    return true;
  } else if (intersections.size() == 1) {
    intersection1_ret = intersections[0];
    intersection2_ret = intersections[0];
    return true;
  }
  return false;
}

double mdc::point_line_distance(const Point &p1, const Point &p2, const Point &p) {
  double line_size_sqr;
  double u;
  Point inters;

  line_size_sqr = (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);

  u = ((p.x - p1.x) * (p2.x - p1.x) + (p.y - p1.y) * (p2.y - p1.y)) / line_size_sqr;

  if (u < 0.0 || u > 1.0)
    return INFINITY;

  inters.x = p1.x + u * (p2.x - p1.x);
  inters.y = p1.y + u * (p2.y - p1.y);

  return points_distance(p, inters);
}
