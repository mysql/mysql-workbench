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

#include "mforms/mforms.h"

using namespace mforms;

CheckBox::CheckBox(bool square) {
  _checkbox_impl = &ControlFactory::get_instance()->_checkbox_impl;

  _checkbox_impl->create(this, square);
}

void CheckBox::set_active(bool flag) {
  _updating = true;
  _checkbox_impl->set_active(this, flag);
  _updating = false;
}

bool CheckBox::get_active() {
  return _checkbox_impl->get_active(this);
}
