/*
 * Copyright (c) 2013, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "GrtWrapper.h"
#include "GrtTemplates.h"
#include "DelegateWrapper.h"
#include "GrtManager.h"

#include "BaseEditorWrapper.h"

using namespace MySQL::Grt;

//--------------------------------------------------------------------------------------------------

BaseEditorWrapper::BaseEditorWrapper(bec::BaseEditor *inn) : UIForm(inn) {
}

//--------------------------------------------------------------------------------------------------

BaseEditorWrapper::~BaseEditorWrapper() {
  // The inner class must be considered gone already at this point.
  // So no refresh handler to reset there.
  refresh_ui_handler = nullptr;
  refresh_partial_ui_handler = nullptr;
}

//--------------------------------------------------------------------------------------------------

void BaseEditorWrapper::disable_auto_refresh() {
  ((bec::BaseEditor *)inner)->block_auto_refresh();
}

//--------------------------------------------------------------------------------------------------

void BaseEditorWrapper::enable_auto_refresh() {
  ((bec::BaseEditor *)inner)->unblock_auto_refresh();
}

//--------------------------------------------------------------------------------------------------

bec::BaseEditor *BaseEditorWrapper::get_unmanaged_object() {
  return static_cast<::bec::BaseEditor *>(inner);
}

//--------------------------------------------------------------------------------------------------

GrtValue ^ BaseEditorWrapper::get_object() {
  return gcnew GrtValue(get_unmanaged_object()->get_object());
}

//--------------------------------------------------------------------------------------------------

String ^ BaseEditorWrapper::get_title() {
  return CppStringToNativeRaw(get_unmanaged_object()->get_title());
}

//--------------------------------------------------------------------------------------------------

bool BaseEditorWrapper::is_editing_live_object() {
  return get_unmanaged_object()->is_editing_live_object();
}

//--------------------------------------------------------------------------------------------------

void BaseEditorWrapper::apply_changes_to_live_object() {
  get_unmanaged_object()->apply_changes_to_live_object();
}

//--------------------------------------------------------------------------------------------------

void BaseEditorWrapper::revert_changes_to_live_object() {
  get_unmanaged_object()->revert_changes_to_live_object();
}

//--------------------------------------------------------------------------------------------------

void BaseEditorWrapper::set_refresh_ui_handler(DelegateSlot0<void, void>::ManagedDelegate ^ slot) {
  refresh_ui_handler = gcnew DelegateSlot0<void, void>(slot);
  get_unmanaged_object()->set_refresh_ui_slot(refresh_ui_handler->get_slot());
}

//--------------------------------------------------------------------------------------------------

void BaseEditorWrapper::set_refresh_partial_ui_handler(DelegateSlot1<void, void, int, int>::ManagedDelegate ^ slot) {
  refresh_partial_ui_handler = gcnew DelegateSlot1<void, void, int, int>(slot);
  get_unmanaged_object()->set_partial_refresh_ui_slot(refresh_partial_ui_handler->get_slot());
}

//--------------------------------------------------------------------------------------------------

MySQL::Grt::GRT ^ BaseEditorWrapper::get_grt() {
  return gcnew MySQL::Grt::GRT;
}

//--------------------------------------------------------------------------------------------------

void BaseEditorWrapper::show_exception(String ^ title, String ^ detail) {
  MessageBox::Show(String::Format("An error has occurred while performing the requested action:\n{0}", detail), title,
                   MessageBoxButtons::OK, MessageBoxIcon::Error);
}

//--------------------------------------------------------------------------------------------------

void BaseEditorWrapper::show_validation_error(String ^ title, String ^ reason) {
  MessageBox::Show(String::Format("Cannot change property:\n{0}", reason), title, MessageBoxButtons::OK,
                   MessageBoxIcon::Error);
}

//--------------------------------------------------------------------------------------------------

bool BaseEditorWrapper::should_close_on_delete_of(String ^ oid) {
  return get_unmanaged_object()->should_close_on_delete_of(NativeToCppString(oid));
}

//--------------------------------------------------------------------------------------------------

bool BaseEditorWrapper::is_editor_dirty() {
  return get_unmanaged_object()->is_editor_dirty();
}

//--------------------------------------------------------------------------------------------------

void BaseEditorWrapper::reset_editor_undo_stack() {
  get_unmanaged_object()->reset_editor_undo_stack();
}

//--------------------------------------------------------------------------------------------------
