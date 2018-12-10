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

//----------------------------------------------------------------------------------------------------------------------

Label::Label(const std::string &text, bool right_align) {
  _label_impl = &ControlFactory::get_instance()->_label_impl;

  _label_impl->create(this);

  set_text(text);
  if (right_align)
    set_text_align(MiddleRight);
}

//----------------------------------------------------------------------------------------------------------------------

Label::Label() {
  _label_impl = &ControlFactory::get_instance()->_label_impl;

  _label_impl->create(this);
}

//----------------------------------------------------------------------------------------------------------------------

void Label::set_text(const std::string &text) {
  _label_impl->set_text(this, text);
}

//----------------------------------------------------------------------------------------------------------------------

void Label::set_color(const std::string &color) {
  _label_impl->set_color(this, color);
}

//----------------------------------------------------------------------------------------------------------------------

void Label::set_text_align(Alignment align) {
  _label_impl->set_text_align(this, align);
}

//----------------------------------------------------------------------------------------------------------------------

void Label::set_style(LabelStyle style) {
  _label_impl->set_style(this, style);
}

//----------------------------------------------------------------------------------------------------------------------

void Label::set_wrap_text(bool flag) {
  _label_impl->set_wrap_text(this, flag);
}

//----------------------------------------------------------------------------------------------------------------------
