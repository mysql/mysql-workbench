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

Button::Button(ButtonType btype) : _updating(false) {
  _button_impl = &ControlFactory::get_instance()->_button_impl;

  _button_impl->create(this, btype);

#ifndef _WIN32
  // enable by default
  if (btype == PushButton)
    enable_internal_padding(true);
#endif
}

void Button::set_icon(const std::string &icon) {
  _button_impl->set_icon(this, icon);
}

void Button::set_text(const std::string &text) {
  _button_impl->set_text(this, text);
}

void Button::enable_internal_padding(bool flag) {
  _button_impl->enable_internal_padding(this, flag);
}

void Button::callback() {
  if (!_updating)
    _clicked();
}
