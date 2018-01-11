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

TextEntry::TextEntry(TextEntryType type) : _updating(false) {
  _textentry_impl = &ControlFactory::get_instance()->_textentry_impl;

  _textentry_impl->create(this, type);
}

void TextEntry::set_value(const std::string &text) {
  _updating = true;
  _textentry_impl->set_text(this, text);
  _updating = false;
}

void TextEntry::set_max_length(int len) {
  _textentry_impl->set_max_length(this, len);
}

std::string TextEntry::get_string_value() {
  return _textentry_impl->get_text(this);
}

void TextEntry::callback() {
  if (!_updating)
    _signal_changed();
}

void TextEntry::action(TextEntryAction action) {
  _signal_action(action);
}

void TextEntry::set_read_only(bool flag) {
  _textentry_impl->set_read_only(this, flag);
}

//--------------------------------------------------------------------------------------------------

void TextEntry::set_placeholder_text(const std::string &text) {
  if (_textentry_impl->set_placeholder_text)
    _textentry_impl->set_placeholder_text(this, text);
}

//--------------------------------------------------------------------------------------------------

void TextEntry::set_placeholder_color(const std::string &color) {
  if (_textentry_impl->set_placeholder_color)
    _textentry_impl->set_placeholder_color(this, color);
}

//--------------------------------------------------------------------------------------------------

void TextEntry::set_bordered(bool flag) {
  if (_textentry_impl->set_bordered)
    _textentry_impl->set_bordered(this, flag);
}

//--------------------------------------------------------------------------------------------------

void TextEntry::cut() {
  _textentry_impl->cut(this);
}

//--------------------------------------------------------------------------------------------------

void TextEntry::copy() {
  _textentry_impl->copy(this);
}

//--------------------------------------------------------------------------------------------------

void TextEntry::paste() {
  _textentry_impl->paste(this);
}

//--------------------------------------------------------------------------------------------------

void TextEntry::select(const base::Range &range) {
  _textentry_impl->select(this, range);
}

//--------------------------------------------------------------------------------------------------

base::Range TextEntry::get_selection() {
  return _textentry_impl->get_selection(this);
}
