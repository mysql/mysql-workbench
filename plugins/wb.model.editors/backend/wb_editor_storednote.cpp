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

#include "base/util_functions.h"
#include "base/string_utilities.h"

#include "wb_editor_storednote.h"

#include "grts/structs.workbench.physical.h"
#include "grt/exceptions.h"

#include "mforms/code_editor.h"
#include "mforms/toolbar.h"

#include "grtsqlparser/mysql_parser_services.h"

using namespace base;
using namespace parsers;

static struct {
  const char *label;
  const char *name;
} inclusion_positions[] = {{"Do not include", ""},
                           {"Top of script", "top_file"},
                           {"Before DDL", "before_ddl"},
                           {"After DDL", "after_ddl"},
                           {"Before Inserts", "before_inserts"},
                           {"After Inserts", "after_inserts"},
                           {"Bottom of script", "bottom_file"},
                           {NULL, NULL}};

StoredNoteEditorBE::StoredNoteEditorBE(const GrtStoredNoteRef &note) : bec::BaseEditor(note), _note(note) {
  _ignored_object_fields_for_ui_refresh.insert("lastChangeDate");
}

bool StoredNoteEditorBE::is_script() {
  return _note.is_instance(db_Script::static_class_name());
}

MySQLEditor::Ref StoredNoteEditorBE::get_sql_editor() {
  if (!_sql_editor) {
    workbench_physical_ModelRef model(workbench_physical_ModelRef::cast_from(_note->owner()));
    MySQLParserServices::Ref services = MySQLParserServices::get();
    MySQLParserContext::Ref context =
      services->createParserContext(model->catalog()->characterSets(), model->catalog()->version(), "", false);
    MySQLParserContext::Ref autocomplete_context =
      services->createParserContext(model->catalog()->characterSets(), model->catalog()->version(), "", false);
    _sql_editor = MySQLEditor::create(context, autocomplete_context, {});

    scoped_connect(_sql_editor->text_change_signal(),
                   std::bind(&StoredNoteEditorBE::do_partial_ui_refresh, this, (int)BaseEditor::RefreshTextChanged));

    if (is_script()) {
      mforms::ToolBar *tbar = _sql_editor->get_toolbar();
      mforms::ToolBarItem *item;
      db_ScriptRef script(db_ScriptRef::cast_from(_note));

      std::string syncvalue = inclusion_positions[0].label, fwvalue = inclusion_positions[0].label;
      std::vector<std::string> sync_choices, fw_choices;
      for (int i = 0; inclusion_positions[i].label != NULL; i++) {
        if (strcmp(inclusion_positions[i].name, "after_inserts") != 0 &&
            strcmp(inclusion_positions[i].name, "before_inserts") != 0)
          sync_choices.push_back(inclusion_positions[i].label);
        fw_choices.push_back(inclusion_positions[i].label);
        if (strcmp(inclusion_positions[i].name, script->synchronizeScriptPosition().c_str()) == 0)
          syncvalue = inclusion_positions[i].label;
        if (strcmp(inclusion_positions[i].name, script->forwardEngineerScriptPosition().c_str()) == 0)
          fwvalue = inclusion_positions[i].label;
      }

      item = mforms::manage(new mforms::ToolBarItem(mforms::ExpanderItem));
      tbar->add_item(item);

      item = mforms::manage(new mforms::ToolBarItem(mforms::LabelItem));
      item->set_text(_("Synchronization:"));
      tbar->add_item(item);

      item = mforms::manage(new mforms::ToolBarItem(mforms::SelectorItem));
      item->set_selector_items(sync_choices);
      item->set_name("syncscript");
      item->set_tooltip(_("Position to insert this in synchronization output scripts"));
      item->signal_activated()->connect(std::bind(&StoredNoteEditorBE::changed_selector, this, item));
      item->set_text(syncvalue);
      tbar->add_item(item);

      item = mforms::manage(new mforms::ToolBarItem(mforms::LabelItem));
      item->set_text(_("Forward Engineering:"));
      tbar->add_item(item);

      item = mforms::manage(new mforms::ToolBarItem(mforms::SelectorItem));
      item->set_selector_items(fw_choices);
      item->set_name("forwardscript");
      item->signal_activated()->connect(std::bind(&StoredNoteEditorBE::changed_selector, this, item));
      item->set_tooltip(_("Position to insert this in forward engineering output scripts"));
      item->set_text(fwvalue);
      tbar->add_item(item);
    }

    // Remove syntax highlighting if this is a note.
    if (!is_script()) {
      _sql_editor->get_editor_control()->set_language(mforms::LanguageNone);
      _sql_editor->set_sql_check_enabled(false);
    }
  }
  return _sql_editor;
}

void StoredNoteEditorBE::changed_selector(mforms::ToolBarItem *item) {
  std::string value = item->get_text();
  std::string s;
  for (int i = 0; inclusion_positions[i].label != NULL; i++)
    if (strcmp(inclusion_positions[i].label, value.c_str()) == 0) {
      s = inclusion_positions[i].name;
      break;
    }

  bec::AutoUndoEdit undo(this);
  if (item->getInternalName() == "syncscript") {
    db_ScriptRef::cast_from(_note)->synchronizeScriptPosition(s);
    undo.end(base::strfmt(_("Change sync output position for %s"), get_name().c_str()));
  } else {
    db_ScriptRef::cast_from(_note)->forwardEngineerScriptPosition(s);
    undo.end(base::strfmt(_("Change forward eng. output position for %s"), get_name().c_str()));
  }
}

void StoredNoteEditorBE::set_text(grt::StringRef text) {
  // XXX replace this using module wrapper class
  grt::Module *module = grt::GRT::get()->get_module("Workbench");
  if (!module)
    throw std::runtime_error("Workbench module not found");

  grt::BaseListRef args(true);

  args.ginsert(_note->filename());
  args.ginsert(text);

  module->call_function("setAttachedFileContents", args);

  _note->lastChangeDate(base::fmttime(0, DATETIME_FMT));
}

grt::StringRef StoredNoteEditorBE::get_text(bool &isutf8) {
  grt::Module *module = grt::GRT::get()->get_module("Workbench");
  if (!module)
    throw std::runtime_error("Workbench module not found");

  grt::BaseListRef args(true);

  args.ginsert(_note->filename());

  grt::StringRef value(grt::StringRef::cast_from(module->call_function("getAttachedFileContents", args)));

  if (!g_utf8_validate(value.c_str(), (gssize)strlen(value.c_str()), NULL)) {
    isutf8 = false;
    return "";
  }
  isutf8 = true;

  return value;
}

void StoredNoteEditorBE::set_name(const std::string &name) {
  if (_note->name() != name) {
    workbench_physical_ModelRef model(workbench_physical_ModelRef::cast_from(_note->owner()));

    if (!model.is_valid())
      throw std::logic_error("Note owner not set");

    grt::ListRef<GrtStoredNote> notes(model->notes());
    for (size_t c = notes.count(), i = 0; i < c; i++) {
      GrtStoredNoteRef note(notes[i]);

      if (note != _note && *note->name() == name)
        throw bec::validation_error(_("Duplicate note name."));
    }

    bec::AutoUndoEdit undo(this, _note, "name");
    _note->name(name);
    undo.end(strfmt(_("Rename '%s' to '%s'"), _note->name().c_str(), name.c_str()));
  }
}

std::string StoredNoteEditorBE::get_name() {
  return _note->name();
}

//--------------------------------------------------------------------------------------------------

std::string StoredNoteEditorBE::get_title() {
  std::string result = is_script() ? base::strfmt("%s - Script", get_name().c_str())
                                   : base::strfmt("%s - Stored Note", get_name().c_str());
  if (is_editor_dirty())
    result += "*";

  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Loads the note text from the GRT into the editor.
 */
void StoredNoteEditorBE::load_text() {
  bool isUTF8;

  grt::StringRef text = get_text(isUTF8);
  MySQLEditor::Ref editor = get_sql_editor();
  mforms::CodeEditor *code_editor = editor->get_editor_control();
  if (isUTF8)
    code_editor->set_text_keeping_state(text.c_str());
  else
    code_editor->set_text(_("Data is not UTF8 encoded and cannot be displayed."));
  code_editor->reset_dirty();
}

//--------------------------------------------------------------------------------------------------

void StoredNoteEditorBE::commit_changes() {
  MySQLEditor::Ref editor = get_sql_editor();
  mforms::CodeEditor *code_editor = editor->get_editor_control();
  if (code_editor->is_dirty()) {
    std::pair<const char *, size_t> text = code_editor->get_text_ptr();
    set_text(grt::StringRef(text.first));

    // Note: the dirty state of the editor does *not* indicate the dirty state of the note object
    //       or even the model file. Instead it only reflects if the text differs from the content
    //       of the note object. WBContext instead keeps track of changed attachments.
    code_editor->reset_dirty();
  }
}

//--------------------------------------------------------------------------------------------------
