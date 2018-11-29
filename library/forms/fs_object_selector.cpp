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

#include <set>
#include "base/string_utilities.h"

#include "mforms/mforms.h"

using namespace mforms;

//--------------------------------------------------------------------------------------------------

/**
 * Constructor used to let the selector itself control the layout. The selector will serve as
 * container for the other (implicitly created) controls.
 */
FsObjectSelector::FsObjectSelector(bool horizontal) : Box(horizontal), _type(OpenFile), _show_hidden(false) {
  _browse_button = mforms::manage(new Button(), false);
  _browse_button->retain();
  _edit = mforms::manage(new TextEntry(), false);
  _edit->retain();

  set_spacing(10);
  add(_edit, true, true);
  add(_browse_button, false, true);
}

//--------------------------------------------------------------------------------------------------

/**
 * Constructor that can be given controls to use for its task, which might be laid out in any way
 * not supported by the selector.
 * If you use this constructor you should not add the selector to any other container or it will
 * show up empty (unless you explicitly add the given controls to this selector).
 */
FsObjectSelector::FsObjectSelector(Button* button, TextEntry* edit) : Box(true), _type(OpenFile), _show_hidden(false) {
  _browse_button = button;
  _browse_button->retain();
  _edit = edit;
  _edit->retain();
}

//--------------------------------------------------------------------------------------------------

FsObjectSelector::~FsObjectSelector() {
  _browse_button->release();
  _edit->release();
}

//--------------------------------------------------------------------------------------------------

void FsObjectSelector::initialize(const std::string& initial_path, FileChooserType type, const std::string& extensions,
                                  bool show_hidden, std::function<void()> on_validate) {
  _type = type;
  _show_hidden = show_hidden;
  _extensions = extensions;

  if (extensions.size() > 0) {
    char** parts = g_strsplit(extensions.c_str(), "|", -1);
    if (parts) {
      if (parts[0] && parts[1])
        _default_extension = parts[1];
      else if (parts[0])
        _default_extension = parts[0];

      if (_default_extension[0] == '*')
        _default_extension = _default_extension.substr(1);

      g_strfreev(parts);
    }
  }

  _edit->set_value(initial_path);
  _browse_button->set_text("...");
  _browse_button->set_name("Browse");
  _browse_button->set_size(40, -1);
  _browse_button->enable_internal_padding(false);
  _on_validate = on_validate;
  enable_file_browsing();
}

//--------------------------------------------------------------------------------------------------

void FsObjectSelector::set_filename(const std::string& path) {
  _edit->set_value(path);
}

//--------------------------------------------------------------------------------------------------

void FsObjectSelector::enable_file_browsing() {
  scoped_connect(_edit->signal_changed(), std::bind(&FsObjectSelector::filename_changed, this));
  _browse_connection =
    _browse_button->signal_clicked()->connect(std::bind(&FsObjectSelector::browse_file_callback, this));
}

//--------------------------------------------------------------------------------------------------

static std::set<mforms::TextEntry*> file_entries_set_from_browse_dialog;

void FsObjectSelector::filename_changed() {
  file_entries_set_from_browse_dialog.erase(_edit);
  if (_on_validate)
    _on_validate();
}

//--------------------------------------------------------------------------------------------------

/**
 * Used to set an external browse callback (the function called when the user clicks the ... button.
 * This allows to implement browsing of remote file systems etc.
 * Note: setting a different browse callback moves reponsibilities to this callback (e.g. setting
 * the text of the edit control, triggering validation).
 */
void FsObjectSelector::set_browse_callback(std::function<void()> browse_callback) {
  _browse_connection = _browse_button->signal_clicked()->connect(browse_callback);
}

//--------------------------------------------------------------------------------------------------

void FsObjectSelector::browse_file_callback() {
  FileChooser fsel(_type, _show_hidden);

  if (!_extensions.empty())
    fsel.set_extensions(_extensions, _default_extension);

  std::string path = _edit->get_string_value();
  if (!path.empty()) {
    if (g_file_test(path.c_str(), G_FILE_TEST_IS_DIR))
      fsel.set_directory(path);
    else {
      // Assume its a filename and get the dirname for the path.
      fsel.set_directory(base::dirname(path));
    }
  }

  if (fsel.run_modal()) {
    _edit->set_value(base::normalize_path_extension(fsel.get_path(), _default_extension));
    // emit the changed signal for the text entry
    (*signal_changed())();

// gtkmm file picker doesn't seem to confirm overwrite
#if defined(__APPLE__) || defined(_MSC_VER)
    // Keep all entries that had their contents set using the file dialog
    // so that when checking for filenames that would be overwritten, the ones from such
    // entries are skipped. Paths picked with file dialog are already confirmed against overwrite.
    file_entries_set_from_browse_dialog.insert(_edit);
#endif
  }
  if (_on_validate)
    _on_validate();
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the currently set fs object name, appending the extension if needed.
 */
std::string FsObjectSelector::get_filename() {
  return base::normalize_path_extension(_edit->get_string_value(), _default_extension);
}

//--------------------------------------------------------------------------------------------------

/**
 * Used to enable or disable edit box and browse button.
 */
void FsObjectSelector::set_enabled(bool value) {
  _edit->set_enabled(value);
  _browse_button->set_enabled(value);
}

//--------------------------------------------------------------------------------------------------

std::string FsObjectSelector::get_string_value() {
  return _edit->get_string_value();
}

//--------------------------------------------------------------------------------------------------

int FsObjectSelector::get_int_value() {
  return -1;
}

//--------------------------------------------------------------------------------------------------

bool FsObjectSelector::get_bool_value() {
  return false;
}

//--------------------------------------------------------------------------------------------------

/**
 * Takes the text from the given edit and checks if it represents an existing file that would be
 * overwritten if used. Entries which were previously selected via the selector itself are kept
 * in the file entry list and do not trigger a confirmation.
 */
bool FsObjectSelector::check_and_confirm_file_overwrite(TextEntry* entry, const std::string& extension) {
  if (file_entries_set_from_browse_dialog.find(entry) != file_entries_set_from_browse_dialog.end())
    return true;

  std::string value = base::normalize_path_extension(entry->get_string_value(), extension);

  if (g_file_test(value.c_str(), G_FILE_TEST_EXISTS)) {
    if (mforms::Utilities::show_warning(
          _("A file with the selected name already exists, do you want to replace it?"),
          base::strfmt(_("The file '%s' already exists. Replacing it will overwrite its contents."), value.c_str()),
          _("Replace"), _("Cancel")) == mforms::ResultCancel)
      return false;
  }
  return true;
}

//--------------------------------------------------------------------------------------------------

bool FsObjectSelector::check_and_confirm_file_overwrite() {
  return check_and_confirm_file_overwrite(_edit, _default_extension);
}

//--------------------------------------------------------------------------------------------------

/**
 * Clears the internal list of stored filenames, which is used to check for overwrite confirmations.
 */
void FsObjectSelector::clear_stored_filenames() {
  file_entries_set_from_browse_dialog.clear();
}

//--------------------------------------------------------------------------------------------------
