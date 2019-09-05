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

#include "geometry.h"

using namespace geometry;

//----------------- Point ----------------------------------------------------------------------------------------------

bool Point::operator == (Point const& other) const {
  return other.x == x && other.y == y;
}

//----------------------------------------------------------------------------------------------------------------------

bool Point::operator != (Point const& other) const {
  return other.x != x || other.y != y;
}

//----------------------------------------------------------------------------------------------------------------------

std::string Point::toJson() const {
  return "{\"x\":" + std::to_string(x) + ",\"y\":" + std::to_string(y) + "}";
}

//----------------- Size -----------------------------------------------------------------------------------------------

bool Size::operator == (Size const& other) const {
  return other.width == width && other.height == height;
}

//----------------------------------------------------------------------------------------------------------------------

bool Size::operator != (Size const& other) const {
  return other.width != width || other.height != height;
}

//----------------------------------------------------------------------------------------------------------------------

std::string Size::toJson() const {
  return "{\"width\":" + std::to_string(width) + ",\"height\":" + std::to_string(height) + "}";
}

//----------------- Rectangle ------------------------------------------------------------------------------------------

bool Rectangle::contains(Point const& p) const {
  return minX() <= p.x && p.x <= maxX() && minY() <= p.y && p.y <= maxY();
}

//----------------------------------------------------------------------------------------------------------------------

bool Rectangle::contains(Rectangle const& r) const {
  return r.minX() >= minX() && r.maxX() <= maxX() && r.minY() >= minY() && r.maxY() <= maxY();
}

//----------------------------------------------------------------------------------------------------------------------

bool Rectangle::operator == (Rectangle const& other) const {
  return other.position == position && other.size == size;
}

//----------------------------------------------------------------------------------------------------------------------

bool Rectangle::operator != (Rectangle const& other) const {
  return other.position != position || other.size != size;
}

//----------------------------------------------------------------------------------------------------------------------

std::string Rectangle::toJson() const {
  return "{\"x\":" + std::to_string(position.x) + ",\"y\":" + std::to_string(position.y) + ",\"width\":" +
    std::to_string(size.width) + ",\"height\":" + std::to_string(size.height) + "}";
}

//----------------------------------------------------------------------------------------------------------------------

std::ostream &operator << (std::ostream &out, geometry::Rectangle const& rect) {
  out << "[x: " << rect.position.x << ", y: " << rect.position.y << ", w: " << rect.size.width << ", h: "
    << rect.size.height << "]";
  return out;
}

//----------------------------------------------------------------------------------------------------------------------
