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

#include "../stub_textentry.h"

namespace mforms {
  namespace stub {

    bool TextEntryWrapper::create(::mforms::TextEntry *self, TextEntryType type) {
      return true;
    }

    void TextEntryWrapper::set_text(::mforms::TextEntry *self, const std::string &text) {
    }

    void TextEntryWrapper::set_max_length(::mforms::TextEntry *self, int len) {
    }

    std::string TextEntryWrapper::get_text(::mforms::TextEntry *self) {
      return "";
    }

    void TextEntryWrapper::set_read_only(::mforms::TextEntry *self, bool flag) {
    }

    void TextEntryWrapper::set_placeholder_text(TextEntry *self, const std::string &text) {
    }

    //--------------------------------------------------------------------------------------------------

    void TextEntryWrapper::set_placeholder_color(TextEntry *self, const std::string &color) {
    }

    //--------------------------------------------------------------------------------------------------

    void TextEntryWrapper::set_bordered(TextEntry *self, bool flag) {
    }

    //--------------------------------------------------------------------------------------------------

    void TextEntryWrapper::cut(TextEntry *self) {
    }

    //--------------------------------------------------------------------------------------------------

    void TextEntryWrapper::copy(TextEntry *self) {
    }

    //--------------------------------------------------------------------------------------------------

    void TextEntryWrapper::paste(TextEntry *self) {
    }

    //--------------------------------------------------------------------------------------------------

    void TextEntryWrapper::select(TextEntry *self, const base::Range &range) {
    }

    //--------------------------------------------------------------------------------------------------

    base::Range TextEntryWrapper::getSelection(TextEntry *self) {
      return base::Range();
    }

    //--------------------------------------------------------------------------------------------------

    TextEntryWrapper::TextEntryWrapper(::mforms::TextEntry *self, TextEntryType type) : ViewWrapper(self) {
    }

    void TextEntryWrapper::activated(mforms::TextEntry *self) {
    }

    void TextEntryWrapper::init() {
      ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

      f->_textentry_impl.create = &TextEntryWrapper::create;
      f->_textentry_impl.set_text = &TextEntryWrapper::set_text;
      f->_textentry_impl.set_max_length = &TextEntryWrapper::set_max_length;
      f->_textentry_impl.get_text = &TextEntryWrapper::get_text;
      f->_textentry_impl.set_read_only = &TextEntryWrapper::set_read_only;
      f->_textentry_impl.set_placeholder_text = &TextEntryWrapper::set_placeholder_text;
      f->_textentry_impl.set_placeholder_color = &TextEntryWrapper::set_placeholder_color;
      f->_textentry_impl.set_bordered = &TextEntryWrapper::set_bordered;
      f->_textentry_impl.cut = &TextEntryWrapper::cut;
      f->_textentry_impl.copy = &TextEntryWrapper::copy;
      f->_textentry_impl.paste = &TextEntryWrapper::paste;
      f->_textentry_impl.select = &TextEntryWrapper::select;
      f->_textentry_impl.get_selection = &TextEntryWrapper::getSelection;
    }
  };
};
