/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

/**
 * Definitions and implementations of geometry related types and classes. Defines all the usual base types
 * like point, rectangle and the like.
 */

#include <stdio.h>

#ifndef SWIG
#include "common.h"
#endif

namespace base {

#ifndef SWIG

  struct BASELIBRARY_PUBLIC_FUNC Point {
    double x;
    double y;

    Point();
    Point(double x, double y);

    inline Point operator+(const Point &p) const {
      return Point(p.x + x, p.y + y);
    };
    inline Point operator-(const Point &p) const {
      return Point(x - p.x, y - p.y);
    };
    inline Point operator-() const {
      return Point(-x, -y);
    };
    inline bool operator==(const Point &p) const {
      return p.x == x && p.y == y;
    };
    inline bool operator!=(const Point &p) const {
      return p.x != x || p.y != y;
    };
    inline Point round() const {
      Point p;
      p.x = ceil(x);
      p.y = ceil(y);
      return p;
    };
    inline std::string str() const {
      char buf[20];
      snprintf(buf, sizeof(buf), "{%.2f,%.2f}", x, y);
      return buf;
    };
  };

  struct BASELIBRARY_PUBLIC_FUNC Size {
    double width;
    double height;

    bool empty();

    Size();
    Size(double w, double h);

    inline Size round() const {
      Size s;
      s.width = ceil(width);
      s.height = ceil(height);
      return s;
    };
    inline std::string str() const {
      char buf[20];
      snprintf(buf, sizeof(buf), "{%.2fx%.2f}", width, height);
      return buf;
    };

    inline bool operator==(const Size &s) const {
      return s.width == width && s.height == height;
    };
    inline bool operator!=(const Size &s) const {
      return s.width != width || s.height != height;
    };
  };

  struct BASELIBRARY_PUBLIC_FUNC Rect {
    Point pos;
    Size size;
    bool use_inter_pixel; // For some drawing operations we need coordinates that are between two pixels.

    Rect();
    Rect(double x, double y, double w, double h);
    Rect(const Point &tl, const Point &br);
    Rect(const Point &apos, const Size &asize);

    bool contains(double x, double y) const;
    bool contains_flipped(double x, double y) const;
    void inflate(double horizontal, double vertical);

    double right() const;
    double bottom() const;
    inline bool empty() const {
      return (size.width <= 0) || (size.height <= 0);
    }

    double left() const;
    double top() const;
    inline double width() const {
      return size.width;
    };
    inline double height() const {
      return size.height;
    };

    inline double xcenter() const {
      return pos.x + size.width / 2;
    }
    inline double ycenter() const {
      return pos.y + size.height / 2;
    }

    // Note: these 4 routines do not move the rectangle but only a given side
    //       adjusting the width to keep all other sides constant.
    inline void set_xmin(double x) {
      size.width -= x - pos.x;
      pos.x = x;
    };
    inline void set_ymin(double y) {
      size.height -= y - pos.y;
      pos.y = y;
    };
    inline void set_xmax(double x) {
      size.width = x - pos.x;
    };
    inline void set_ymax(double y) {
      size.height = y - pos.y;
    };

    inline Point center() const {
      return Point(xcenter(), ycenter());
    }

    inline Point top_left() const {
      return Point(left(), top());
    }
    inline Point top_right() const {
      return Point(right(), top());
    }
    inline Point bottom_left() const {
      return Point(left(), bottom());
    }
    inline Point bottom_right() const {
      return Point(right(), bottom());
    }

    inline bool operator==(const Rect &r) const {
      return r.pos == pos && r.size == size;
    };
    inline bool operator!=(const Rect &r) const {
      return r.pos != pos || r.size != size;
    };

    inline std::string str() const {
      char buf[40];
      snprintf(buf, sizeof(buf), "{%.2f,%.2f  %.2fx%.2f}", pos.x, pos.y, size.width, size.height);
      return buf;
    };
  };

  /**
   * Used to specify (integer) coordinates used to place/size controls.
   */
  struct BASELIBRARY_PUBLIC_FUNC ControlBounds {
    int left;
    int top;
    int width;
    int height;

    ControlBounds();
    ControlBounds(int x, int y, int w, int h);
  };

#endif // SWIG

/**
 * Four values describing the space on each side of an area.
 */
#ifndef SWIG
  typedef struct BASELIBRARY_PUBLIC_FUNC Padding
#else
  struct Padding
#endif
  {
    int left;
    int top;
    int right;
    int bottom;

    Padding();
    Padding(int padding);
    Padding(int left, int top, int right, int bottom);

    int horizontal();
    int vertical();

  } Padding;

  /** A struct describing a range in a container. */
  struct BASELIBRARY_PUBLIC_FUNC Range {
    size_t position;
    size_t size;

    Range();
    Range(size_t position, size_t size);

    size_t end();
    bool contains_point(size_t point);
  };

} // namespace base
