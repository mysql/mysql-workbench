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

//--------------------------------------------------------------------------------------------------

ListBox::ListBox(bool multi_select) : _updating(false) {
  _listbox_impl = &ControlFactory::get_instance()->_listbox_impl;

  _listbox_impl->create(this, multi_select);
}

//--------------------------------------------------------------------------------------------------

void ListBox::clear() {
  _updating = true;
  _listbox_impl->clear(this);
  _updating = false;
}

//--------------------------------------------------------------------------------------------------

void ListBox::set_heading(const std::string &text) {
  _listbox_impl->set_heading(this, text);
}

//--------------------------------------------------------------------------------------------------

size_t ListBox::add_item(const std::string &item) {
  return _listbox_impl->add_item(this, item);
}

//--------------------------------------------------------------------------------------------------

void ListBox::add_items(const std::list<std::string> &items) {
  _listbox_impl->add_items(this, items);
}

//--------------------------------------------------------------------------------------------------

void ListBox::remove_index(size_t index) {
  _listbox_impl->remove_index(this, index);
}

//--------------------------------------------------------------------------------------------------

void ListBox::remove_indexes(const std::vector<size_t> &indexes) {
  _listbox_impl->remove_indexes(this, indexes);
}

//--------------------------------------------------------------------------------------------------

void ListBox::set_selected(ssize_t index) {
  _updating = true;
  _listbox_impl->set_index(this, index);
  _updating = false;
}

//--------------------------------------------------------------------------------------------------

std::string ListBox::get_string_value() {
  return _listbox_impl->get_text(this);
}

//--------------------------------------------------------------------------------------------------

ssize_t ListBox::get_selected_index() {
  return _listbox_impl->get_index(this);
}

//--------------------------------------------------------------------------------------------------

std::vector<size_t> ListBox::get_selected_indices() {
  return _listbox_impl->get_selected_indices(this);
}

//--------------------------------------------------------------------------------------------------

void ListBox::selection_changed() {
  if (!_updating)
    _signal_changed();
}

//--------------------------------------------------------------------------------------------------

size_t ListBox::get_count() {
  return _listbox_impl->get_count(this);
}

//--------------------------------------------------------------------------------------------------

std::string ListBox::get_string_value_from_index(size_t index) {
  return _listbox_impl->get_string_value_from_index(this, index);
}
