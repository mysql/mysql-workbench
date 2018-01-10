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

#include "grt_python_debugger.h"
#include "grt_code_editor.h"
#include "base/file_utilities.h"
#include "base/string_utilities.h"
#include "grt_shell_window.h"

using namespace mforms;

static void embed_find_panel(mforms::CodeEditor *editor, bool show, mforms::Box *container) {
  mforms::View *panel = editor->get_find_panel();
  if (show) {
    if (!panel->get_parent())
      container->add(panel, false, true);
  } else {
    container->remove(panel);
    editor->focus();
  }
}

GRTCodeEditor::GRTCodeEditor(GRTShellWindow *owner, bool module, const std::string &language)
  : Box(false), _owner(owner), _top(false), _text() {
  _dirty = false;
  _editing_module = module;
  _language = language;

  _top.add_end(&_text, true, true);

  _text.set_show_find_panel_callback(std::bind(embed_find_panel, std::placeholders::_1, std::placeholders::_2, &_top));

  if (_language == "python")
    _text.set_language(LanguagePython);
  else if (_language == "sql")
    _text.set_language(LanguageMySQL);
  else
    _text.set_language(LanguageNone);

  if (_language == "python")
    _debugging_supported = true;
  else
    _debugging_supported = false;

  _text.set_font(bec::GRTManager::get()->get_app_option_string("workbench.general.Editor:Font"));

  add(&_top, true, true);

  scoped_connect(_text.signal_changed(),
                 std::bind(&GRTCodeEditor::text_changed, this, std::placeholders::_1, std::placeholders::_2));
}

GRTCodeEditor::~GRTCodeEditor() {
}

void GRTCodeEditor::set_path(const std::string &path) {
  _filename = path;
  _owner->set_editor_title(this, get_title());
}

void GRTCodeEditor::set_text(const std::string &text) {
  _text.set_value(text);

  // explicitly call text changed callback for dirty marker to get set
  text_changed(0, 0);
}

std::string GRTCodeEditor::get_text() {
  return _text.get_string_value();
}

std::string GRTCodeEditor::get_title() {
  if (_filename.empty())
    return "Unnamed";
  return _dirty ? base::strfmt("*%s", base::basename(_filename).c_str()) : base::basename(_filename);
}

bool GRTCodeEditor::load(const std::string &path) {
  char *data = NULL;
  gsize length = 0;
  GError *error = NULL;

  if (!g_file_get_contents(path.c_str(), &data, &length, &error)) {
    Utilities::show_error("Open File", base::strfmt("Could not open %s: %s", path.c_str(), error->message), "OK");
    g_error_free(error);
    return false;
  }

  set_text(data);
  g_free(data);
  _dirty = false;
  set_path(path); // call after resetting _dirty so the title is shown ok

  return true;
}

bool GRTCodeEditor::save(bool choose_file) {
  if (choose_file || _filename.empty()) {
    FileChooser chooser(SaveFile);

    chooser.set_title("Save File");
    if (chooser.run_modal())
      _filename = chooser.get_path();
    else
      return false;
  }

  std::string data = _text.get_string_value();
  GError *error = 0;
  if (!g_file_set_contents(_filename.c_str(), data.c_str(), (gssize)data.size(), &error)) {
    Utilities::show_error("Error Saving File",
                          base::strfmt("Could not save to %s:\n%s", _filename.c_str(), error->message), "OK", "", "");
    g_error_free(error);
    return false;
  }

  _owner->add_output(base::strfmt("Script saved as %s\n", _filename.c_str()));
  _owner->on_file_save(_filename);
  _dirty = false;

  _owner->set_editor_title(this, get_title());

  return true;
}

void GRTCodeEditor::execute() {
  {
    std::string script = _text.get_string_value();

    bool ret = _owner->execute_script(script, _language);

    if (ret)
      _owner->add_output("\nScript finished.\n");
    else
      _owner->add_output("\nError executing script.\n");
  }
}

void GRTCodeEditor::text_changed(int line, int linesAdded) {
  if (!_dirty) {
    _dirty = true;
    _owner->set_editor_title(this, get_title());
  }
}

//--------------------------------------------------------------------------------------------------

bool GRTCodeEditor::can_close() {
  if (_dirty) {
    int r;
    r = mforms::Utilities::show_message(
      "Close Editor", base::strfmt("%s has unsaved changes, would you like to save them?", get_title().c_str()),
      _("Save"), _("Cancel"), _("Don't Save"));

    if (r == mforms::ResultOk) {
      if (!save(false))
        return false;
    } else if (r == mforms::ResultCancel)
      return false;
  }

  return true;
}

//--------------------------------------------------------------------------------------------------

#ifdef _DEBUG

void GRTCodeEditor::test_markup() { /*
                                     int count = _text.line_count();

                                     if (count >= 0)
                                       _text.show_markup(LineMarkupBreakpoint, 0);
                                     if (count >= 1)
                                       _text.show_markup(LineMarkupBreakpointHit, 1);
                                     if (count >= 2)
                                       _text.show_markup(LineMarkupError, 2);
                                     if (count >= 4)
                                       _text.show_markup(LineMarkupStatement, 4);

                                     // Show all markup.
                                     if (count >= 5)
                                       _text.show_markup(LineMarkupAll, 5);

                                     // Add and remove.
                                     if (count >= 6)
                                       _text.show_markup(LineMarkupStatement | LineMarkupError, 6);
                                     if (count >= 6)
                                       _text.remove_markup(LineMarkupAll, 6);

                                     // Current line with markers.
                                     if (count >= 7)
                                       _text.show_markup(LineMarkupStatement | LineMarkupBreakpointHit, 7);

                                     // Current line with error.
                                     if (count >= 8)
                                       _text.show_markup(LineMarkupStatement | LineMarkupError, 8);

                                     // Selective deletion of a marker.
                                     if (count >= 9)
                                       _text.show_markup(LineMarkupBreakpoint | LineMarkupError, 9);
                                     if (count >= 9)
                                       _text.remove_markup(LineMarkupBreakpoint, 9);
                                   */
}

#endif

//--------------------------------------------------------------------------------------------------
