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

#include "editor_base.h"
#include "mforms/code_editor.h"

using namespace bec;

//--------------------------------------------------------------------------------------------------

UndoObjectChangeGroup::UndoObjectChangeGroup(const std::string &object_id, const std::string &member)
  : _object_id(object_id), _member(member) {
}

//--------------------------------------------------------------------------------------------------

bool UndoObjectChangeGroup::matches_group(UndoGroup *group) const {
  UndoObjectChangeGroup *other = dynamic_cast<UndoObjectChangeGroup *>(group);
  if (!other)
    return false;

  return other->_object_id == _object_id && _member == other->_member;
}

//----------------- BaseEditor ---------------------------------------------------------------------

BaseEditor::BaseEditor(const grt::Ref<GrtObject> &object) : _ignore_object_changes_for_ui_refresh(0), _object(object) {
  _ignored_object_fields_for_ui_refresh.insert("oldName");

  _ignored_object_changes_for_ui_refresh = 0;
  if (object.is_valid())
    add_listeners(object);
}

//--------------------------------------------------------------------------------------------------

BaseEditor::~BaseEditor() {
  //_object->reset_references(); Cannot work. The editor doesn't own the object.
}

//--------------------------------------------------------------------------------------------------

std::string BaseEditor::get_form_context_name() const {
  return "editor"; // WB_CONTEXT_EDITOR;
}

//--------------------------------------------------------------------------------------------------

void BaseEditor::add_listeners(const grt::Ref<GrtObject> &object) {
  scoped_connect(object->signal_changed(),
                 std::bind(&BaseEditor::object_member_changed, this, std::placeholders::_1, std::placeholders::_2));
  scoped_connect(object->signal_list_changed(), std::bind(&BaseEditor::on_object_changed, this));
}

//--------------------------------------------------------------------------------------------------

void BaseEditor::object_member_changed(const std::string &member, const grt::ValueRef &ovalue) {
  if (_ignored_object_fields_for_ui_refresh.find(member) == _ignored_object_fields_for_ui_refresh.end())
    on_object_changed();
}

//--------------------------------------------------------------------------------------------------

void BaseEditor::freeze_refresh_on_object_change() {
  _ignore_object_changes_for_ui_refresh++;
}

//--------------------------------------------------------------------------------------------------

bool BaseEditor::is_refresh_frozen() {
  return _ignore_object_changes_for_ui_refresh > 0;
}

//--------------------------------------------------------------------------------------------------

void BaseEditor::thaw_refresh_on_object_change(bool discard_pending) {
  if (_ignore_object_changes_for_ui_refresh > 0)
    _ignore_object_changes_for_ui_refresh--;
  if (_ignore_object_changes_for_ui_refresh == 0) {
    if (_ignored_object_changes_for_ui_refresh > 0 && !discard_pending)
      on_object_changed();
    _ignored_object_changes_for_ui_refresh = 0;
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Replaces the current object (e.g. on re-parse/reset).
 */
void BaseEditor::set_object(GrtObjectRef value) {
  _object = value;
  on_object_changed();
};

//--------------------------------------------------------------------------------------------------

void BaseEditor::apply_changes_to_live_object() {
  commit_changes();
  reset_editor_undo_stack();
}

//--------------------------------------------------------------------------------------------------

void bec::BaseEditor::revert_changes_to_live_object() {
  refresh_live_object();
  reset_editor_undo_stack();
}

//--------------------------------------------------------------------------------------------------

/**
 * Checks if the editor can be closed and returns true if so.
 * Must be called in the context of the main thread.
 */
bool BaseEditor::can_close() {
  if (!UIForm::can_close())
    return false;

  if (is_editor_dirty())
    return false;
  return true;
}

//--------------------------------------------------------------------------------------------------

void BaseEditor::on_object_changed() {
  if (_ignore_object_changes_for_ui_refresh == 0) {
    // calling ui_refresh from here will cause refresh to be called from the GRT thread
    // which must not happen. delaying it to be executing when idle will make it
    // get called on main thread
    if (bec::GRTManager::get()->in_main_thread())
      do_ui_refresh();
    else
      _ui_refresh_conn = bec::GRTManager::get()->run_once_when_idle(std::bind(&RefreshUI::do_ui_refresh, this));
  } else
    _ignored_object_changes_for_ui_refresh++;
}

//--------------------------------------------------------------------------------------------------

void BaseEditor::undo_applied() {
  _ui_refresh_conn = bec::GRTManager::get()->run_once_when_idle(std::bind(&RefreshUI::do_ui_refresh, this));
}

//--------------------------------------------------------------------------------------------------

void BaseEditor::run_from_grt(const std::function<void()> &slot) {
  bec::GRTManager::get()->get_dispatcher()->execute_sync_function(
    "editor action", std::bind(std::bind(&base::run_and_return_value<grt::ValueRef>, slot)));
}

//--------------------------------------------------------------------------------------------------

bool BaseEditor::is_editor_dirty() {
  if (!has_editor())
    return false;

  MySQLEditor::Ref editor = get_sql_editor();
  if (editor) {
    mforms::CodeEditor *code_editor = editor->get_editor_control();
    return (code_editor != NULL) ? code_editor->is_dirty() : false;
  }
  return false;
}

//--------------------------------------------------------------------------------------------------
