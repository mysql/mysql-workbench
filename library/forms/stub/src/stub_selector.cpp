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

#include "../stub_selector.h"

namespace mforms {
  namespace stub {

    //------------------------------------------------------------------------------
    SelectorWrapper::SelectorWrapper(::mforms::Selector *self, ::mforms::SelectorStyle style) : ViewWrapper(self) {
    }

    //------------------------------------------------------------------------------
    SelectorWrapper::~SelectorWrapper() {
    }

    //------------------------------------------------------------------------------
    bool SelectorWrapper::create(::mforms::Selector *self, ::mforms::SelectorStyle style) {
      return true;
    }

    //------------------------------------------------------------------------------
    void SelectorWrapper::clear(::mforms::Selector *self) {
    }

    //------------------------------------------------------------------------------
    int SelectorWrapper::add_item(::mforms::Selector *self, const std::string &item) {
      return 0;
    }

    //------------------------------------------------------------------------------
    void SelectorWrapper::add_items(::mforms::Selector *self, const std::list<std::string> &items) {
    }

    //------------------------------------------------------------------------------
    std::string SelectorWrapper::get_item(::mforms::Selector *self, int index) {
      return "";
    }

    //------------------------------------------------------------------------------
    std::string SelectorWrapper::get_text(::mforms::Selector *self) {
      return "";
    }

    //------------------------------------------------------------------------------
    void SelectorWrapper::set_index(::mforms::Selector *self, int index) {
    }

    //------------------------------------------------------------------------------
    int SelectorWrapper::get_index(::mforms::Selector *self) {
      return -1;
    }

    //------------------------------------------------------------------------------
    int SelectorWrapper::get_item_count(::mforms::Selector *self) {
      return -1;
    }

    //------------------------------------------------------------------------------
    void SelectorWrapper::set_value(::mforms::Selector *self, const std::string &) {
    }

    //------------------------------------------------------------------------------
    void SelectorWrapper::init() {
      ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

      f->_selector_impl.create = &SelectorWrapper::create;
      f->_selector_impl.clear = &SelectorWrapper::clear;
      f->_selector_impl.add_item = &SelectorWrapper::add_item;
      f->_selector_impl.add_items = &SelectorWrapper::add_items;
      f->_selector_impl.get_item = &SelectorWrapper::get_item;
      f->_selector_impl.set_index = &SelectorWrapper::set_index;
      f->_selector_impl.get_index = &SelectorWrapper::get_index;
      f->_selector_impl.get_text = &SelectorWrapper::get_text;
      f->_selector_impl.get_item_count = &SelectorWrapper::get_item_count;
      f->_selector_impl.set_value = &SelectorWrapper::set_value;
    }
  }
}
