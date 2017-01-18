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
  if (encoding.empty() || encoding == "utf8" || encoding == "utf-8" || encoding == "UTF-8")
    _textbox_impl->append_text(this, text, scroll_to_end);
  else {
    gchar *temp = g_convert(text.c_str(), -1, "utf-8", encoding.c_str(), NULL, NULL, NULL);
    std::string converted_text;

    if (temp == NULL) {
      converted_text = text;
      g_warning("Cannot convert '%s' from %s to UTF-8", text.c_str(), encoding.c_str());
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
