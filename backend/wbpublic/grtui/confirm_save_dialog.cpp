/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "confirm_save_dialog.h"

ConfirmSaveDialog::ConfirmSaveDialog(mforms::Form *owner, const std::string &window_title, const std::string &caption)
  : mforms::Form(owner), _box(false), _checkboxes(false), _item_count(0) {
  set_title(window_title);
  set_name("Save");
  setInternalName("save_dialog");
  set_content(&_box);

  _box.set_padding(25);
  _box.set_spacing(12);

  _caption.set_style(mforms::BigBoldStyle);
  _caption.set_text(caption);
  _box.add(&_caption, false, true);

  _box.add(&_scroller, true, true);
  _scroller.add(&_checkboxes);

  _checkboxes.set_spacing(8);
  _checkboxes.set_padding(8);

  {
    mforms::Box *bbox = mforms::manage(new mforms::Box(true));
    bbox->set_spacing(12);
    _box.add(bbox, false, true);
    mforms::Button *discard = mforms::manage(new mforms::Button());
    _review_button.set_text(("Review Changes"));
    _cancel_button.set_text(("Cancel"));
    discard->set_text(("Discard Changes"));

    scoped_connect(discard->signal_clicked(), std::bind(&ConfirmSaveDialog::discard_clicked, this));
#ifdef __APPLE__
    bbox->add(discard, false, true);
    bbox->add_end(&_review_button, false, true);
    bbox->add_end(&_cancel_button, false, true);
#else
    bbox->add_end(&_cancel_button, false, true);
    bbox->add_end(discard, false, true);
    bbox->add_end(&_review_button, false, true);
#endif
  }

  _result = ReviewChanges;

  set_size(500, 300);
  center();
}

void ConfirmSaveDialog::discard_clicked() {
  _result = DiscardChanges;
  end_modal(true);
}

void ConfirmSaveDialog::add_item(const std::string &group, const std::string &name) {
  if (_last_group != group) {
    _last_group = group;
    mforms::Label *label = mforms::manage(new mforms::Label(group));
    _checkboxes.add(label, false, true);
  }
  add_item(name);
}

void ConfirmSaveDialog::add_item(const std::string &name) {
  mforms::Label *cb = mforms::manage(new mforms::Label(name));

  if (_last_group.empty())
    cb->set_text(name);
  else
    cb->set_text("    " + name);

  _item_count++;
  _checkboxes.add(cb, false, true);
}

ConfirmSaveDialog::Result ConfirmSaveDialog::run() {
  if (!mforms::Form::run_modal(&_review_button, &_cancel_button))
    return Cancel;

  return _result;
}
