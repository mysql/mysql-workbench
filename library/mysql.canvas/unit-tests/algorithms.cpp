/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "mdc.h"
#include "mdc_algorithms.h"
#include "wb_helpers.h"

using namespace mdc;
using namespace base;

BEGIN_TEST_DATA_CLASS(canvas_algorithms)
END_TEST_DATA_CLASS

TEST_MODULE(canvas_algorithms, "Canvas: algorithms");

// line intersection
TEST_FUNCTION(1) {
  Point p;
  bool flag;

  //
  // -------
  //
  //    |
  //    |
  //
  flag = intersect_lines(Point(0, 0), Point(50, 0), Point(25, 25), Point(25, 100), p);
  ensure("line intersection1", !flag);

  //
  //    |
  // -------
  //    |
  //
  flag = intersect_lines(Point(0, 25), Point(50, 25), Point(25, 0), Point(25, 50), p);
  ensure("line intersection2", flag);
  ensure_equals("line intersection2 x", p.x, 25);
  ensure_equals("line intersection2 y", p.y, 25);
}

TEST_FUNCTION(2) // rect intersection with line
{
  Rect rect(Point(0, 0), Size(50, 50));
  Point p1, p2;
  bool flag;

  flag = intersect_rect_to_line(rect, Point(25, 25), Point(25, 100), p1, p2);
  ensure("intersection of rect to line from center", flag);

  ensure("intersection of rect to line from center should be 1 pt", p1 == p2);

  ensure_equals("intersection of rect to line from center x", p1.x, 25);
  ensure_equals("intersection of rect to line from center y", p1.y, 50);
}

END_TESTS
