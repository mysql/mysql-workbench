/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "select_option_dialog.h"

#include "mforms/uistyle.h"
#include "mforms/utilities.h"

SelectOptionDialog::SelectOptionDialog(const std::string &title, const std::string &description,
                                       std::vector<std::string> &options, SelectorStyle style)
  : mforms::Form(0), _top_vbox(false), _bottom_hbox(true), _option_box(style) {
  set_title(title);
  set_name("Select Option Dialog");
  setInternalName("select_option_dialog");
  _top_vbox.set_padding(MF_WINDOW_PADDING);
  _top_vbox.set_spacing(MF_TABLE_ROW_SPACING);
  _top_vbox.add(&_description, false, true);
  _top_vbox.add(&_option_box, false, true);
  _top_vbox.add(&_bottom_hbox, false, true);

  _bottom_hbox.set_spacing(MF_BUTTON_SPACING);

  Utilities::add_end_ok_cancel_buttons(&_bottom_hbox, &_ok_button, &_cancel_button);

  _description.set_text_align(TopLeft);
  _description.set_text(description);

  _ok_button.set_text("OK");
  _cancel_button.set_text("Cancel");

  _ok_button.enable_internal_padding(true);
  _cancel_button.enable_internal_padding(true);

  std::vector<std::string>::iterator end = options.end(), it = options.begin();

  while (it != end) {
    _option_box.add_item(*it);
    it++;
  }

  set_content(&_top_vbox);
}

SelectOptionDialog::~SelectOptionDialog(void) {
}

std::string SelectOptionDialog::run() {
  bool valid_selection = false;
  std::string selection = "";

  center();

  // Return the newly created connection object.
  while (!valid_selection) {
    if (run_modal(&_ok_button, &_cancel_button)) {
      selection = _option_box.get_string_value();

      valid_selection = validate ? validate(selection) : true;
    } else {
      selection = "";
      valid_selection = true;
    }
  }

  return selection;
}
