/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "text_input_dialog.h"
#include "grt/common.h"
#include "base/string_utilities.h"

using namespace grtui;

TextInputDialog::TextInputDialog(mforms::Form *owner) : mforms::Form(owner, mforms::FormResizable), _button_box(true) {
  set_name("Input Dialog");
  setInternalName("input_dialog");
  _table.set_padding(12);

  _table.set_row_count(3);
  _table.set_column_count(2);

  _table.add(&_description, 1, 2, 0, 1);
  _table.add(&_caption, 0, 1, 1, 2);
  _table.add(&_input, 1, 2, 1, 2);
  _table.set_row_spacing(8);
  _table.set_column_spacing(8);

  _table.add(&_button_box, 0, 2, 2, 3);

  _button_box.set_spacing(8);
  _cancel_button.set_text(_("Cancel"));
  _cancel_button.enable_internal_padding(true);
  _ok_button.set_text(_("OK"));
  _ok_button.enable_internal_padding(true);

  _button_box.add_end(&_cancel_button, false, true);
  _button_box.add_end(&_ok_button, false, true);

  set_content(&_table);

  set_size(350, 150);
}

void TextInputDialog::set_description(const std::string &text) {
  _description.set_text(text);
}

void TextInputDialog::set_caption(const std::string &text) {
  _caption.set_text(text);
}

void TextInputDialog::set_value(const std::string &text) {
  _input.set_value(text);
}

std::string TextInputDialog::get_value() {
  return _input.get_string_value();
}

bool TextInputDialog::run() {
  return run_modal(&_ok_button, &_cancel_button);
}
