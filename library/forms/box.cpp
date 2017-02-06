/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "base/log.h"

#include "mforms/mforms.h"

using namespace mforms;

Box::Box(bool horiz) {
  _is_horizontal = horiz;
  _box_impl = &ControlFactory::get_instance()->_box_impl;

  _box_impl->create(this, horiz);
}

void Box::add(View *subview, bool expand, bool fill) {
  cache_view(subview);
  _box_impl->add(this, subview, expand, fill); // Add to platform dependent hierarchy.
}

void Box::add_end(View *subview, bool expand, bool fill) {
  cache_view(subview);
  _box_impl->add_end(this, subview, expand, fill);
}

void Box::remove(View *subview) {
  _box_impl->remove(this, subview);
  remove_from_cache(subview);
}

void Box::set_homogeneous(bool flag) {
  _box_impl->set_homogeneous(this, flag);
}

void Box::set_spacing(int space) {
  _box_impl->set_spacing(this, space);
}

bool mforms::Box::is_horizontal() {
  return _is_horizontal;
}
