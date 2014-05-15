/* 
 * Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "base/util_functions.h"
#include "base/string_utilities.h"

#include "wb_editor_storednote.h"

#include "grts/structs.workbench.physical.h"
#include "grt/exceptions.h"

#include "mforms/code_editor.h"

#include "grtsqlparser/mysql_parser_services.h"

using namespace base;
using namespace parser;

StoredNoteEditorBE::StoredNoteEditorBE(bec::GRTManager *grtm, const GrtStoredNoteRef &note)
: bec::BaseEditor(grtm, note), _note(note)
{
  _ignored_object_fields_for_ui_refresh.insert("lastChangeDate");
}


bool StoredNoteEditorBE::is_script()
{
  return _note.is_instance(db_Script::static_class_name());
}


MySQLEditor::Ref StoredNoteEditorBE::get_sql_editor()
{
  if (!_sql_editor)
  {
    workbench_physical_ModelRef model(workbench_physical_ModelRef::cast_from(_note->owner()));
    MySQLParserServices::Ref services = MySQLParserServices::get(get_grt());
    ParserContext::Ref context = services->createParserContext(model->catalog()->characterSets(), model->catalog()->version(), false);
    _sql_editor = MySQLEditor::create(get_grt(), context);

    scoped_connect(_sql_editor->text_change_signal(),
      boost::bind(&StoredNoteEditorBE::do_partial_ui_refresh, this, (int)BaseEditor::RefreshTextChanged));

    // Remove syntax highlighting if this is a note.
    if (!is_script())
    {
      _sql_editor->get_editor_control()->set_language(mforms::LanguageNone);
      _sql_editor->set_sql_check_enabled(false);
    }
  }
  return _sql_editor;
}


void StoredNoteEditorBE::set_text(grt::StringRef text)
{  
  //XXX replace this using module wrapper class
  grt::Module *module= get_grt()->get_module("Workbench");
  if (!module)
    throw std::runtime_error("Workbench module not found");

  grt::BaseListRef args(get_grt());

  args.ginsert(_note->filename());
  args.ginsert(text);

  module->call_function("setAttachedFileContents", args);
  
  _note->lastChangeDate(base::fmttime(0, DATETIME_FMT));
}


grt::StringRef StoredNoteEditorBE::get_text(bool &isutf8)
{
  //XXX replace this using module wrapper class
  grt::Module *module= get_grt()->get_module("Workbench");
  if (!module)
    throw std::runtime_error("Workbench module not found");

  grt::BaseListRef args(get_grt());

  args.ginsert(_note->filename());

  grt::StringRef value(grt::StringRef::cast_from(module->call_function("getAttachedFileContents", args)));

  if (!g_utf8_validate(value.c_str(), (gssize)strlen(value.c_str()), NULL))
  {
    isutf8= false;
    return "";
  }
  isutf8= true;

  return value;
}


void StoredNoteEditorBE::set_name(const std::string &name)
{
  if (_note->name() != name)
  {
    workbench_physical_ModelRef model(workbench_physical_ModelRef::cast_from(_note->owner()));

    if (!model.is_valid())
      throw std::logic_error("Note owner not set");

    grt::ListRef<GrtStoredNote> notes(model->notes());
    for (size_t c= notes.count(), i= 0; i < c; i++)
    {
      GrtStoredNoteRef note(notes[i]);

      if (note != _note && *note->name() == name)
        throw bec::validation_error(_("Duplicate note name."));
    }

    bec::AutoUndoEdit undo(this, _note, "name");
    _note->name(name);
    undo.end(strfmt(_("Rename '%s' to '%s'"), _note->name().c_str(), name.c_str()));
  }
}


std::string StoredNoteEditorBE::get_name()
{
  return _note->name();
}

//--------------------------------------------------------------------------------------------------

std::string StoredNoteEditorBE::get_title()
{
  std::string result = is_script() ? base::strfmt("%s - Script", get_name().c_str()) : base::strfmt("%s - Stored Note", get_name().c_str());
  if (is_editor_dirty())
    result += "*";
  
  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Loads the note text from the GRT into the editor.
 */
void StoredNoteEditorBE::load_text()
{
  bool isUTF8;

  grt::StringRef text = get_text(isUTF8);
  MySQLEditor::Ref editor = get_sql_editor();
  mforms::CodeEditor* code_editor = editor->get_editor_control();
  if (isUTF8)
    code_editor->set_text_keeping_state(text.c_str());
  else
    code_editor->set_text(_("Data is not UTF8 encoded and cannot be displayed."));
  code_editor->reset_dirty();
}

//--------------------------------------------------------------------------------------------------

void StoredNoteEditorBE::commit_changes()
{
  MySQLEditor::Ref editor = get_sql_editor();
  mforms::CodeEditor* code_editor = editor->get_editor_control();
  if (code_editor->is_dirty())
  {
    std::pair<const char*, size_t> text = code_editor->get_text_ptr();
    set_text(grt::StringRef(text.first));

    // Note: the dirty state of the editor does *not* indicate the dirty state of the note object
    //       or even the model file. Instead it only reflects if the text differs from the content
    //       of the note object. WBContext instead keeps track of changed attachments.
    code_editor->reset_dirty();
  }
}

//--------------------------------------------------------------------------------------------------
