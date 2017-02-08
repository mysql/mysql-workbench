/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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
