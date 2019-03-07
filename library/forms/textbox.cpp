/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "base/string_utilities.h"
#include "base/log.h"
#include "mforms/mforms.h"

DEFAULT_LOG_DOMAIN(DOMAIN_MFORMS_BE)

using namespace mforms;

TextBox::TextBox(ScrollBars scroll_bars) : _updating(false) {
  _textbox_impl = &ControlFactory::get_instance()->_textbox_impl;

  _textbox_impl->create(this, scroll_bars);
}

void TextBox::set_value(const std::string &text) {
  _updating = true;
  _textbox_impl->set_text(this, text);
  _updating = false;
}

void TextBox::append_text(const std::string &text, bool scroll_to_end) {
  _textbox_impl->append_text(this, text, scroll_to_end);
}

//--------------------------------------------------------------------------------------------------

/**
 * Convert the given text to utf-8, using the given encoding.
 * If this conversion fails simply add the text as it is to the text box.
 */
void TextBox::append_text_with_encoding(const std::string &text, const std::string &encoding, bool scroll_to_end) {
    if (encoding.empty() || base::tolower(encoding) == "utf8" || base::tolower(encoding) == "utf-8")
    _textbox_impl->append_text(this, text, scroll_to_end);
  else {
    gchar *temp = g_convert(text.c_str(), -1, "utf-8", encoding.c_str(), NULL, NULL, NULL);
    std::string converted_text;

    if (temp == NULL) {
      converted_text = text;
      logWarning("Cannot convert '%s' from %s to UTF-8\n", text.c_str(), encoding.c_str());
    } else {
      converted_text = temp;
      g_free(temp);
    }
    _textbox_impl->append_text(this, converted_text, scroll_to_end);
  }
}

//--------------------------------------------------------------------------------------------------

void TextBox::set_bordered(bool flag) {
  _textbox_impl->set_bordered(this, flag);
}

void TextBox::set_read_only(bool flag) {
  _textbox_impl->set_read_only(this, flag);
}

void TextBox::set_padding(int pad) {
  _textbox_impl->set_padding(this, pad);
}

void TextBox::set_monospaced(bool flag) {
  _textbox_impl->set_monospaced(this, flag);
}

std::string TextBox::get_string_value() {
  return _textbox_impl->get_text(this);
}

void TextBox::get_selected_range(int &start, int &end) {
  _textbox_impl->get_selected_range(this, start, end);
}

void TextBox::clear() {
  _textbox_impl->clear(this);
}

void TextBox::callback() {
  if (!_updating)
    _signal_changed();
}

//--------------------------------------------------------------------------------------------------

bool TextBox::key_event(KeyCode code, ModifierKey modifiers, const std::string &text) {
  // Return true if the key event can be further processed by the sender.
  // Return false if it is handled in backend code.
  if (_key_event_signal.empty())
    return true;

  return *_key_event_signal(code, modifiers, text);
}

//--------------------------------------------------------------------------------------------------
