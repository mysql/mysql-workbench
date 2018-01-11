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

#ifndef _MDC_ALGORITHMS_H_
#define _MDC_ALGORITHMS_H_

#include "mdc_common.h"

// TODO: move everything of general interest (wrt. geometry) to the base list (geometry.h).
namespace mdc {

  // top is front of list, bottom is back
  template <class C>
  void restack_up(std::list<C *> &stack, C *object, C *above = 0) {
    typename std::list<C *>::iterator iter = std::find(stack.begin(), stack.end(), object);
    if (iter == stack.end())
      return;

    stack.erase(iter);
    if (above)
      stack.insert(find(stack.begin(), stack.end(), above), object);
    else
      stack.push_front(object);
  }

  template <class C>
  void restack_down(std::list<C *> &stack, C *object) {
    typename std::list<C *>::iterator iter = std::find(stack.begin(), stack.end(), object);
    if (iter == stack.end())
      return;

    stack.erase(iter);
    stack.push_back(object);
  }

  //----------------------------------------------------------------------------------------------------------------------

  bool MYSQLCANVAS_PUBLIC_FUNC intersect_lines(const base::Point &s1, const base::Point &e1, const base::Point &s2,
                                               const base::Point &e2, base::Point &intersection_ret);

  bool MYSQLCANVAS_PUBLIC_FUNC intersect_hv_lines(const base::Point &s1, const base::Point &e1, const base::Point &s2,
                                                  const base::Point &e2, base::Point &intersection_ret);

  bool MYSQLCANVAS_PUBLIC_FUNC intersect_rect_to_line(const base::Rect &rect, const base::Point &s,
                                                      const base::Point &e, base::Point &intersection1_ret,
                                                      base::Point &intersection2_ret);

  inline double angle_of_line(const base::Point &p1, const base::Point &p2) {
    double angle;
    // figure out the angle of the line in degrees
    if (p1 == p2)
      angle = 0;
    else {
      if (p2.y < p1.y)
        angle = 90.0 + atan((p2.x - p1.x) / (p2.y - p1.y)) * 180.0 / M_PI;
      else
        angle = 270.0 + atan((p2.x - p1.x) / (p2.y - p1.y)) * 180.0 / M_PI;

      angle = angle - floor(angle / 360) * 360;
    }
    return angle;
  }

  //----------------------------------------------------------------------------------------------------------------------

  /**
   * Determines whether both bounds overlap.
   *
   * @param bounds1 One of the bounds to compare.
   * @param bounds2 The other bounds to compare.
   * @return True if both bounds overlap each other, otherwise false.
   * @note Bounds must be sorted.
   */
  inline bool bounds_intersect(const base::Rect &bounds1, const base::Rect &bounds2) {
    return (bounds1.right() >= bounds2.left()) && (bounds1.left() <= bounds2.right()) &&
           (bounds1.bottom() >= bounds2.top()) && (bounds1.top() <= bounds2.bottom());
  }

  //----------------------------------------------------------------------------------------------------------------------

  /**
   * Determines whether the second bounds are completely within the first bounds.
   *
   * @param bounds1 The outer bounds to compare.
   * @param bounds2 The inner bounds to compare.
   * @return True if the second bounds are completely within the first bounds, otherwise false.
   * @note Bounds must be sorted.
   */
  inline bool bounds_contain_bounds(const base::Rect &bounds1, const base::Rect &bounds2) {
    return (bounds1.left() <= bounds2.left()) && (bounds2.right() <= bounds1.right()) &&
           (bounds1.top() <= bounds2.top()) && (bounds2.bottom() <= bounds1.bottom());
  }

  //----------------------------------------------------------------------------------------------------------------------

  /**
   * Determines whether the given bounds include the given point.
   *
   * @param bounds The bounds to check the point against.
   * @param x The horizontal coordinate to check.
   * @param y The vertical coordinate to check.
   * @return True if the point is within the bounds, otherwise false.
   * @note Bounds must be sorted.
   */
  inline bool bounds_contain_point(const base::Rect &bounds, double x, double y) {
    return (bounds.right() >= x) && (bounds.pos.x <= x) && (bounds.bottom() >= y) && (bounds.pos.y <= y);
  }

  //----------------------------------------------------------------------------------------------------------------------

  /**
   * Examines the given bounds and returns whether it is empty or not.
   *
   * @param bounds The bounds to examine.
   * @return True if the bounds are empty, false otherwise.
   */
  inline bool bounds_are_empty(const base::Rect &bounds) {
    return (bounds.size.width == 0) && (bounds.size.height == 0);
  }

  //----------------------------------------------------------------------------------------------------------------------

  inline base::Rect clip_bound(const base::Rect &bounds, const base::Rect &clip) {
    double x1 = bounds.left();
    double x2 = bounds.right();
    double y1 = bounds.top();
    double y2 = bounds.bottom();

    if (x1 < clip.left())
      x1 = clip.left();
    if (y1 < clip.top())
      y1 = clip.top();
    if (x2 > clip.right())
      x2 = clip.right();
    if (y2 > clip.bottom())
      y2 = clip.bottom();

    return base::Rect(base::Point(x1, y1), base::Size(x2 - x1, y2 - y1));
  }

  //----------------------------------------------------------------------------------------------------------------------

  inline base::Point bounds_center(const base::Rect &bounds) {
    return base::Point(bounds.left() + bounds.width() / 2, bounds.top() + bounds.height() / 2);
  }

  //----------------------------------------------------------------------------------------------------------------------

  inline base::Rect expand_bound(const base::Rect &bounds, double dx, double dy) {
    base::Rect r = bounds;

    r.pos.x -= dx;
    r.pos.y -= dy;
    r.size.width += dx * 2;
    r.size.height += dy * 2;
    return r;
  }

  //----------------------------------------------------------------------------------------------------------------------

  inline double points_distance(const base::Point &p1, const base::Point &p2) {
    return sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
  }

  double MYSQLCANVAS_PUBLIC_FUNC point_line_distance(const base::Point &p1, const base::Point &p2,
                                                     const base::Point &p);

  //----------------------------------------------------------------------------------------------------------------------

  inline void points_reorder(base::Point &topleft, base::Point &bottomright) {
    double tmp;

    if (topleft.x > bottomright.x) {
      tmp = topleft.x;
      topleft.x = bottomright.x;
      bottomright.x = tmp;
    }
    if (topleft.y > bottomright.y) {
      tmp = topleft.y;
      topleft.y = bottomright.y;
      bottomright.y = tmp;
    }
  }
};

#endif /* _MDC_ALGORITHMS_H_ */
