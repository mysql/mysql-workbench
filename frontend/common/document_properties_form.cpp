/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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
#include "base/ui_form.h"

#include "mforms/label.h"

#include "grt.h"

#include "grts/structs.app.h"
#include "grts/structs.h"

#include "grt/editor_base.h"

#include "workbench/wb_context_ui.h"

#include "document_properties_form.h"

DocumentPropertiesForm::DocumentPropertiesForm()
  : Form(NULL, mforms::FormResizable), _bottom_box(true), _text(mforms::BothScrollBars) {
  set_title(_("Document Properties"));
  set_name("Document Properties");
  setInternalName("document_properties");

  set_content(&_table);
  _table.set_padding(TOP_FORM_PADDING);
  _table.set_row_spacing(8);
  _table.set_column_spacing(4);
  _table.set_row_count(8);
  _table.set_column_count(2);

  add_control(_("Name:"), &_entry1);
  add_control(_("Version:"), &_entry2);
  add_control(_("Author:"), &_entry3);
  add_control(_("Project:"), &_entry4);
  add_control(_("Created:"), &_entry5);
  add_control(_("Last Changed:"), &_entry6);
  add_control(_("Description:"), &_text, true);

  _entry5.set_enabled(false);
  _entry6.set_enabled(false);

  _table.add(&_bottom_box, 0, 2, 7, 8, mforms::HFillFlag | mforms::VFillFlag);
  _bottom_box.set_spacing(8);

  scoped_connect(_ok_button.signal_clicked(), std::bind(&DocumentPropertiesForm::ok_clicked, this));
  scoped_connect(_cancel_button.signal_clicked(), std::bind(&DocumentPropertiesForm::cancel_clicked, this));

  _ok_button.enable_internal_padding(true);
  _cancel_button.enable_internal_padding(true);

  _bottom_box.add_end(&_ok_button, false, true);
  _ok_button.set_text(_(" OK "));

  _bottom_box.add_end(&_cancel_button, false, true);
  _cancel_button.set_text(_("Cancel"));

  set_size(400, 420);

  center();

  pull_values();
}

void DocumentPropertiesForm::push_values() {
  std::string caption, version, author, project, dateCreated, dateChanged, description;

  // fetch values from backend
  caption = _entry1.get_string_value();
  version = _entry2.get_string_value();
  author = _entry3.get_string_value();
  project = _entry4.get_string_value();
  dateCreated = _entry5.get_string_value();
  dateChanged = _entry6.get_string_value();

  description = _text.get_string_value();

  wb::WBContextUI::get()->set_doc_properties(caption, version, author, project, dateCreated, dateChanged, description);
}

void DocumentPropertiesForm::pull_values() {
  std::string caption, version, author, project, dateCreated, dateChanged, description;

  // fetch values from backend
  wb::WBContextUI::get()->get_doc_properties(caption, version, author, project, dateCreated, dateChanged, description);

  _entry1.set_value(caption);
  _entry2.set_value(version);
  _entry3.set_value(author);
  _entry4.set_value(project);
  _entry5.set_value(dateCreated);
  _entry6.set_value(dateChanged);

  _text.set_value(description);
}

static void destroy(mforms::Object *object) {
  delete object;
}

DocumentPropertiesForm::~DocumentPropertiesForm() {
  std::for_each(_widgets.begin(), _widgets.end(), std::bind(&destroy, std::placeholders::_1));
}

void DocumentPropertiesForm::add_control(const std::string &caption, mforms::View *control, bool expand) {
  mforms::Label *label;
  int row = (int)_widgets.size();
  label = new mforms::Label();

  label->set_text(caption);
  label->set_text_align(mforms::TopRight);

  _table.add(label, 0, 1, row, row + 1, mforms::VFillFlag | mforms::HFillFlag);
  _table.add(control, 1, 2, row, row + 1,
             (expand ? mforms::VExpandFlag : 0) | mforms::HFillFlag | mforms::VFillFlag | mforms::HExpandFlag);

  _widgets.push_back(label);
}

void DocumentPropertiesForm::ok_clicked() {
  push_values();
}

void DocumentPropertiesForm::cancel_clicked() {
}

void DocumentPropertiesForm::show() {
  run_modal(&_ok_button, &_cancel_button);
}
