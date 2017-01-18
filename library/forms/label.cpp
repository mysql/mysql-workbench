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

Label::Label(const std::string &text, bool right_align) {
  _label_impl = &ControlFactory::get_instance()->_label_impl;

  _label_impl->create(this);

  set_text(text);
  if (right_align)
    set_text_align(MiddleRight);
}

Label::Label() {
  _label_impl = &ControlFactory::get_instance()->_label_impl;

  _label_impl->create(this);
}

void Label::set_text(const std::string &text) {
  _label_impl->set_text(this, text);
}

void Label::set_color(const std::string &color) {
  _label_impl->set_color(this, color);
}

void Label::set_text_align(Alignment align) {
  _label_impl->set_text_align(this, align);
}

void Label::set_style(LabelStyle style) {
  _label_impl->set_style(this, style);
}

void Label::set_wrap_text(bool flag) {
  _label_impl->set_wrap_text(this, flag);
}
