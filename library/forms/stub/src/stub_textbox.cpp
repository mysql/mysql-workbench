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

#include "../stub_textbox.h"

namespace mforms {
  namespace stub {

    bool TextBoxWrapper::create(::mforms::TextBox *self, mforms::ScrollBars scroll_type) {
      return true;
    }

    void TextBoxWrapper::set_text(::mforms::TextBox *self, const std::string &text) {
    }

    void TextBoxWrapper::append_text(::mforms::TextBox *self, const std::string &text, bool scroll_to_end) {
    }

    std::string TextBoxWrapper::get_text(::mforms::TextBox *self) {
      return "";
    }

    void TextBoxWrapper::set_read_only(::mforms::TextBox *self, bool flag) {
    }

    void TextBoxWrapper::set_padding(::mforms::TextBox *self, int pad) {
    }

    void TextBoxWrapper::set_bordered(::mforms::TextBox *self, bool flag) {
    }

    void TextBoxWrapper::clear(::mforms::TextBox *self) {
    }

    void TextBoxWrapper::set_monospaced(::mforms::TextBox *self, bool flag) {
    }

    void TextBoxWrapper::get_selected_range(TextBox *self, int &start, int &end) {
    }

    TextBoxWrapper::TextBoxWrapper(::mforms::TextBox *self, mforms::ScrollBars scroll_type) : ViewWrapper(self) {
    }

    void TextBoxWrapper::init() {
      ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

      f->_textbox_impl.create = &TextBoxWrapper::create;
      f->_textbox_impl.set_bordered = &TextBoxWrapper::set_bordered;
      f->_textbox_impl.set_text = &TextBoxWrapper::set_text;
      f->_textbox_impl.get_text = &TextBoxWrapper::get_text;
      f->_textbox_impl.append_text = &TextBoxWrapper::append_text;
      f->_textbox_impl.set_read_only = &TextBoxWrapper::set_read_only;
      f->_textbox_impl.set_padding = &TextBoxWrapper::set_padding;
      f->_textbox_impl.clear = &TextBoxWrapper::clear;
      f->_textbox_impl.set_monospaced = &TextBoxWrapper::set_monospaced;
      f->_textbox_impl.get_selected_range = &TextBoxWrapper::get_selected_range;
    }
  };
};
