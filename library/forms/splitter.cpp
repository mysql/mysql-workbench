/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mforms/mforms.h"

using namespace mforms;

Splitter::Splitter(bool horiz, bool thin) {
  _splitter_impl = &ControlFactory::get_instance()->_splitter_impl;

#ifdef __APPLE__
  _splitter_impl->create(this, horiz, thin);
#else
  _splitter_impl->create(this, horiz);
#endif
}

void Splitter::add(View *subview, int minsize, bool fixed) {
  cache_view(subview);
  _splitter_impl->add(this, subview, minsize, fixed);
}

void Splitter::remove(View *subview) {
  _splitter_impl->remove(this, subview);
  remove_from_cache(subview);
}

void Splitter::set_divider_position(int pos) {
  _splitter_impl->set_divider_position(this, pos);
}

int Splitter::get_divider_position() {
  return _splitter_impl->get_divider_position(this);
}

void Splitter::set_expanded(bool first, bool expand) {
  _splitter_impl->set_expanded(this, first, expand);
}

void Splitter::position_changed() {
  _position_changed_signal();
}
