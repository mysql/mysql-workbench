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
 * documentation. The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "grt_wizard_form.h"
#include "grtpp_util.h"
#include "grt/common.h"
#include "wizard_view_text_page.h"
#include "base/string_utilities.h"

using namespace grtui;
using namespace base;

ViewTextPage::ViewTextPage(WizardForm *form, const char *name, Buttons buttons, const std::string &filetype)
  : WizardPage(form, name), _text(), _button_box(true), _filetype(filetype) {
  _editable = true;
  set_padding(8);

  if (buttons != 0) {
    add_end(&_button_box, false, true);
    _button_box.set_spacing(8);

    if (buttons & SaveButton) {
      _button_box.add(&_save_button, false, true);
      _save_button.enable_internal_padding(true);
      scoped_connect(_save_button.signal_clicked(), std::bind(&ViewTextPage::save_clicked, this));
      _save_button.set_text(_("Save to File..."));
      _save_button.set_tooltip(_("Save the text to a new file."));
    }
    if (buttons & CopyButton) {
      _button_box.add(&_copy_button, false, true);
      _copy_button.enable_internal_padding(true);
      scoped_connect(_copy_button.signal_clicked(), std::bind(&ViewTextPage::copy_clicked, this));
      _copy_button.set_text(_("Copy to Clipboard"));
      _copy_button.set_tooltip(_("Copy the text to the clipboard."));
    }
  }

  _text.set_language(mforms::LanguageMySQL);
  // Scintilla mac has a crash bug if this is on
  //  _text.set_features(mforms::FeatureWrapText, true);

  add_end(&_text, true, true);
}

void ViewTextPage::set_text(const std::string &text) {
  bool editable = _editable;
  if (!editable)
    set_editable(true);
  _text.set_value(text);
  if (!editable)
    set_editable(false);
}

std::string ViewTextPage::get_text() {
  return _text.get_string_value();
}

void ViewTextPage::set_editable(bool flag) {
  _editable = flag;
  _text.set_features(mforms::FeatureReadOnly, !flag);
}

void ViewTextPage::save_clicked() {
  mforms::FileChooser fsel(mforms::SaveFile);

  fsel.set_extensions(_filetype, _filetype);

  if (fsel.run_modal()) {
    std::string text = _text.get_string_value();

    try {
      base::setTextFileContent(fsel.get_path(), text);
    } catch (const std::exception &exc) {
      mforms::Utilities::show_error(
        _("Save to File"), strfmt(_("Could not save to file '%s': %s"), fsel.get_path().c_str(), exc.what()), _("OK"));
    }
  }
}

void ViewTextPage::copy_clicked() {
  mforms::Utilities::set_clipboard_text(_text.get_string_value());
}

void ViewTextPage::save_text_to(const std::string &path) {
  char *filename = g_filename_from_utf8(path.c_str(), -1, NULL, NULL, NULL);
  std::string text = get_text();
  GError *error = NULL;

  if (!g_file_set_contents(filename, text.data(), (gssize)text.size(), &error)) {
    g_free(filename);
    std::string msg = strfmt(_("Could not save to file '%s': %s"), path.c_str(), error->message);
    g_error_free(error);

    throw grt::os_error(msg);
  }
  g_free(filename);
}
