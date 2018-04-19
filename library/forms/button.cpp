/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

Button::Button(ButtonType btype) : _updating(false) {
  _button_impl = &ControlFactory::get_instance()->_button_impl;

  _button_impl->create(this, btype);

#ifndef _MSC_VER
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
