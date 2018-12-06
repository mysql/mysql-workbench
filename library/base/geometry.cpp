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

#include "base/geometry.h"

using namespace base;

//----------------- Point --------------------------------------------------------------------------

Point::Point() {
  x = 0;
  y = 0;
}

//--------------------------------------------------------------------------------------------------

Point::Point(double x, double y) {
  this->x = x;
  this->y = y;
}

//----------------- Size ---------------------------------------------------------------------------

Size::Size() {
  width = 0;
  height = 0;
}

//--------------------------------------------------------------------------------------------------

Size::Size(double w, double h) {
  width = w;
  height = h;
}

//--------------------------------------------------------------------------------------------------

bool Size::empty() {
  return (width == 0) || (height == 0);
}

//----------------- Rect ---------------------------------------------------------------------------

Rect::Rect() {
  // When drawing lines or similar, e.g. in Cairo, coordinates must be between pixels, otherwise
  // the lines appear blurry. use_inter_pixel does not affect the stored values in the rect but
  // only what is returned by left(), right() etc.
  use_inter_pixel = false;
  pos = Point(0, 0);
  size = Size(0, 0);
}

//--------------------------------------------------------------------------------------------------

Rect::Rect(double x, double y, double w, double h) {
  use_inter_pixel = false;
  pos = Point(x, y);
  size = Size(w, h);
}

//--------------------------------------------------------------------------------------------------

Rect::Rect(const Point &tl, const Point &br) {
  use_inter_pixel = false;
  pos = tl;
  size = Size(br.x - tl.x, br.y - tl.y);
};

//--------------------------------------------------------------------------------------------------

Rect::Rect(const Point &apos, const Size &asize) {
  use_inter_pixel = false;
  pos = apos;
  size = asize;
};

//--------------------------------------------------------------------------------------------------

bool Rect::contains(double x, double y) const {
  return !empty() && (x >= pos.x) && (x <= pos.x + size.width) && (y >= pos.y) && (y <= pos.y + size.height);
}

//--------------------------------------------------------------------------------------------------

bool Rect::contains_flipped(double x, double y) const {
  // For cairo text the top is actually the bottom (when using it for hit tests).
  return !empty() && (x >= pos.x) && (x <= pos.x + size.width) && (y >= pos.y - size.height) && (y <= pos.y);
}

//--------------------------------------------------------------------------------------------------

/**
 * Inflates the rectangle by adding the given amounts to the left/top coordinate and subtracting them
 * from the right/bottom coordinate.
 */
void Rect::inflate(double horizontal, double vertical) {
  pos.x += horizontal;
  size.width -= 2 * horizontal;
  pos.y += vertical;
  size.height -= 2 * vertical;
}

//--------------------------------------------------------------------------------------------------

double Rect::right() const {
  if (use_inter_pixel)
    return (int)(pos.x + size.width) + 0.5;
  else
    return pos.x + size.width;
}

//--------------------------------------------------------------------------------------------------

double Rect::bottom() const {
  if (use_inter_pixel)
    return (int)(pos.y + size.height) + 0.5;
  else
    return pos.y + size.height;
}

//--------------------------------------------------------------------------------------------------

double Rect::left() const {
  if (use_inter_pixel)
    return (int)pos.x + 0.5;
  else
    return pos.x;
};

//--------------------------------------------------------------------------------------------------

double Rect::top() const {
  if (use_inter_pixel)
    return (int)pos.y + 0.5;
  else
    return pos.y;
};

//----------------- ControlBounds ------------------------------------------------------------------

ControlBounds::ControlBounds() {
  left = 0;
  top = 0;
  width = 0;
  height = 0;
}

//--------------------------------------------------------------------------------------------------

ControlBounds::ControlBounds(int x, int y, int w, int h) {
  left = x;
  top = y;
  width = w;
  height = h;
}

//----------------- Padding ------------------------------------------------------------------------

Padding::Padding() {
  left = 0;
  top = 0;
  right = 0;
  bottom = 0;
}

//--------------------------------------------------------------------------------------------------

Padding::Padding(int padding) {
  left = padding;
  top = padding;
  right = padding;
  bottom = padding;
}

//--------------------------------------------------------------------------------------------------

Padding::Padding(int left, int top, int right, int bottom) {
  this->left = left;
  this->top = top;
  this->right = right;
  this->bottom = bottom;
}

//--------------------------------------------------------------------------------------------------

int Padding::horizontal() {
  return left + right;
}

//--------------------------------------------------------------------------------------------------

int Padding::vertical() {
  return top + bottom;
}

//----------------- Range --------------------------------------------------------------------------

Range::Range() {
  position = 0;
  size = 0;
}

//--------------------------------------------------------------------------------------------------

Range::Range(size_t position, size_t size) {
  this->position = position;
  this->size = size;
}

//--------------------------------------------------------------------------------------------------

size_t Range::end() {
  return position + size;
}

//--------------------------------------------------------------------------------------------------

bool Range::contains_point(size_t point) {
  return (point >= position) && (point - position <= size);
}

//--------------------------------------------------------------------------------------------------
