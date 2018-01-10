/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "wb_editor_note.h"
#include "base/string_utilities.h"

NoteEditorBE::NoteEditorBE(const workbench_model_NoteFigureRef &note) : bec::BaseEditor(note), _note(note) {
}

bool NoteEditorBE::should_close_on_delete_of(const std::string &oid) {
  if (_note.id() == oid || _note->owner().id() == oid)
    return true;

  return false;
}

void NoteEditorBE::set_text(const std::string &text) {
  if (*_note->text() != text) {
    bec::AutoUndoEdit undo(this, _note, "text");
    _note->text(text);
    undo.end(_("Change Note Text"));
  }
}

std::string NoteEditorBE::get_text() {
  return _note->text();
}

void NoteEditorBE::set_name(const std::string &name) {
  if (name != *_note->name()) {
    bec::AutoUndoEdit undo(this, _note, "name");
    _note->name(name);
    undo.end(_("Change Note Name"));
  }
}

std::string NoteEditorBE::get_name() {
  return _note->name();
}

std::string NoteEditorBE::get_title() {
  return base::strfmt("%s - Note", get_name().c_str());
}
