/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "types.h"
#include "aalcommon.h"

#include <iostream>

namespace geometry {

  class ACCESSIBILITY_PUBLIC Point : public mga::SerializableObject {
  public:
    int x, y;

    Point() : x(0), y(0) {}
    Point(int x, int y) : x(x), y(y) {}
    Point(const Point &p) : x(p.x), y(p.y) {}

    bool operator == (Point const& other) const;
    bool operator != (Point const& other) const;

    virtual std::string toJson() const override;
  };

  class ACCESSIBILITY_PUBLIC Size : public mga::SerializableObject {
  public:
    int width, height;

    Size() : width(0), height(0) {}
    Size(int width, int height) : width(width), height(height) {}
    Size(const Size &p) : width(p.width), height(p.height) {}

    bool operator == (Size const& other) const;
    bool operator != (Size const& other) const;

    virtual std::string toJson() const override;
  };

  class ACCESSIBILITY_PUBLIC Rectangle : public mga::SerializableObject {
  public:
    Point position;
    Size size;

    Rectangle() {}
    Rectangle(const Rectangle &rect) : position(rect.position), size(rect.size) {}
    Rectangle(int x, int y, int width, int height) : position(x, y), size(width, height) {}

    bool contains(Point const& p) const;
    bool contains(Rectangle const& r) const;
    bool empty() const { return size.width == 0 || size.height == 0; };

    int minX() const { return position.x; }
    int maxX() const { return position.x + size.width - 1; } // In a 5 pixels wide rect min is 0 and max is 4.
    int minY() const { return position.y; }
    int maxY() const { return position.y + size.height - 1; }

    bool operator == (Rectangle const& other) const;
    bool operator != (Rectangle const& other) const;

    virtual std::string toJson() const override;
  };
}

std::ostream &operator << (std::ostream &out, const geometry::Rectangle &rect);
