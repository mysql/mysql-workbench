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

Selector::Selector(SelectorStyle style) : _updating(false), _editable(style == SelectorCombobox) {
  _selector_impl = &ControlFactory::get_instance()->_selector_impl;

  _selector_impl->create(this, style);
}

void Selector::clear() {
  _selector_impl->clear(this);
}

int Selector::add_item(const std::string &item) {
  _updating = true;
  int r = _selector_impl->add_item(this, item);
  _updating = false;
  return r;
}

void Selector::add_items(const std::list<std::string> &items) {
  _updating = true;
  _selector_impl->add_items(this, items);
  _updating = false;
}

void Selector::set_selected(int index) {
  _updating = true;
  _selector_impl->set_index(this, index);
  _updating = false;
}

std::string Selector::get_item_title(int index) {
  return _selector_impl->get_item(this, index);
}

std::string Selector::get_string_value() {
  return _selector_impl->get_text(this);
}

int Selector::index_of_item_with_title(const std::string &title) {
  for (int i = 0; i < get_item_count(); i++)
    if (get_item_title(i) == title)
      return i;
  return -1;
}

int Selector::get_selected_index() {
  return _selector_impl->get_index(this);
}

int Selector::get_item_count() {
  return _selector_impl->get_item_count(this);
}

void Selector::callback() {
  if (!_updating)
    _signal_changed();
}

void Selector::set_value(const std::string &value) {
  // Try to set text from the list
  // Otherwise force text to be set (only for SelectorCombo)
  const int i = index_of_item_with_title(value);
  if (i >= 0)
    set_selected(i);
  else if (_editable)
    _selector_impl->set_value(this, value);
}
