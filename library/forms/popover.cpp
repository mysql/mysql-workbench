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

#include "mforms/mforms.h"

using namespace mforms;

//--------------------------------------------------------------------------------------------------

Popover::Popover(PopoverStyle style) {
  _popover_impl = &ControlFactory::get_instance()->_popover_impl;
  _popover_impl->create(this, style);
}

//--------------------------------------------------------------------------------------------------

Popover::~Popover() {
  if (_popover_impl->destroy)
    _popover_impl->destroy(this);
}

//--------------------------------------------------------------------------------------------------

void Popover::set_content(View* content) {
  _popover_impl->set_content(this, content);
}

//--------------------------------------------------------------------------------------------------

void Popover::show(int x, int y, StartPosition position) {
  _popover_impl->show(this, x, y, position);
}

//--------------------------------------------------------------------------------------------------

void Popover::show_and_track(View* owner, int x, int y, StartPosition position) {
  _popover_impl->show_and_track(this, owner, x, y, position);
}

//--------------------------------------------------------------------------------------------------

void Popover::set_size(int width, int height) {
  _popover_impl->set_size(this, width, height);
}

//--------------------------------------------------------------------------------------------------

void Popover::close() {
  _popover_impl->close(this);
}

//--------------------------------------------------------------------------------------------------
