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

#include "workbench/wb_overview.h"
#include "GrtTemplates.h"
#include "DelegateWrapper.h"

#include "SQLEditorFormWrapper.h"
#include "objimpl/wrapper/mforms_ObjectReference_impl.h"
#include "sqlide/wb_sql_editor_result_panel.h"

#include "mforms/dockingpoint.h"
#include "mforms/view.h"
#include "mforms/toolbar.h"
#include "mforms/tabview.h"

using namespace System::Windows::Forms;

using namespace MySQL;
using namespace MySQL::Forms;
using namespace MySQL::GUI::Workbench;

//--------------------------------------------------------------------------------------------------

SqlEditorFormWrapper::SqlEditorFormWrapper(std::shared_ptr<SqlEditorForm> *ptr) {
  _docking_point = NULL;
  _dock_delegate_wrapper = nullptr;

  _set_busy_tab_cb = nullptr;

  SqlEditorForm *inner = (SqlEditorForm *)ptr->get();

  _ref = new SqlEditorForm::Ref(*ptr);

  UIForm::init((*_ref).get());

  _log = Ref2Ptr_<::VarGridModel, VarGridModelWrapper>((*_ref)->log());
  _history = Ref2Ptr<::DbSqlEditorHistory, DbSqlEditorHistoryWrapper>((*_ref)->history());

  exec_sql_task = gcnew GrtThreadedTaskWrapper((*_ref)->exec_sql_task.get());
}

//--------------------------------------------------------------------------------------------------

SqlEditorFormWrapper::~SqlEditorFormWrapper() {
  (*_ref)->close();
  if (_docking_point != NULL)
    _docking_point->release();
  delete _dock_delegate_wrapper;

  delete exec_sql_task;
  delete _history;
  delete _log;
  inner = NULL;
  delete _ref;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorFormWrapper::show_output_area() {
  (*_ref)->show_output_area();
}

//--------------------------------------------------------------------------------------------------

void SqlEditorFormWrapper::output_text_ui_cb(Output_text_ui_cb::ManagedDelegate ^ cb) {
  _output_text_ui_cb = gcnew Output_text_ui_cb(cb);
  (*_ref)->output_text_slot = _output_text_ui_cb->get_slot();
}

//--------------------------------------------------------------------------------------------------

ContextMenuStrip ^ SqlEditorFormWrapper::get_log_context_menu() {
  // TODO: replace this by an own wrapper for the log (not just a generic VarGridModel).
  return dynamic_cast<ContextMenuStrip ^>(ObjectMapper::GetManagedComponent((*_ref)->log()->get_context_menu()));
}

//--------------------------------------------------------------------------------------------------

String ^ SqlEditorFormWrapper::restore_sql_from_history(Int32 entry_index, List<Int32> ^ detail_indexes) {
  std::list<Int32> indexes = NativeListToCppList<Int32, Int32>(detail_indexes);
  return CppStringToNative((*_ref)->restore_sql_from_history(entry_index, indexes));
}

//--------------------------------------------------------------------------------------------------

void SqlEditorFormWrapper::set_log_selection(List<Int32> ^ selection) {
  std::vector<Int32> sel = NativeListToCppVector<Int32, Int32>(selection);
  (*_ref)->log()->set_selection(sel);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the native control which is behind the task side bar implementation, so the (C#) UI can
 * embed it. The sidebar is created on the way if not yet done.
 */
Control ^ SqlEditorFormWrapper::get_sidebar_control() {
  return dynamic_cast<Control ^>(ObjectMapper::GetManagedComponent((*_ref)->get_sidebar()));
}

//--------------------------------------------------------------------------------------------------

/**
 * Managed control for the palette window for docking.
 */
Control ^ SqlEditorFormWrapper::get_palette_control() {
  return dynamic_cast<Control ^>(ObjectMapper::GetManagedComponent((*_ref)->get_side_palette()));
}

//--------------------------------------------------------------------------------------------------

void SqlEditorFormWrapper::set_tool_item_checked(String ^ name, bool value) {
  (*_ref)->set_tool_item_checked(NativeToCppString(name), value);
}

//--------------------------------------------------------------------------------------------------

void SqlEditorFormWrapper::set_docking_delegate(ManagedDockDelegate ^ theDelegate) {
  if (_docking_point != NULL)
    _docking_point->release();

  if (_dock_delegate_wrapper != nullptr)
    delete _dock_delegate_wrapper;
  _dock_delegate_wrapper = theDelegate;

  // We don't let the docking point delete our native delegate because it is managed by the
  // managed delegate.
  _docking_point = mforms::manage(new mforms::DockingPoint(theDelegate->get_unmanaged_delegate(), false));
  (*_ref)->set_tab_dock(_docking_point);
}

//--------------------------------------------------------------------------------------------------

void SqlEditorFormWrapper::set_busy_tab_cb(Set_busy_tab_cb::ManagedDelegate ^ cb) {
  _set_busy_tab_cb = gcnew Set_busy_tab_cb(cb);
  (*_ref)->set_busy_tab = _set_busy_tab_cb->get_slot();
}

//--------------------------------------------------------------------------------------------------

void SqlEditorFormWrapper::set_post_query_cb(Post_query_cb::ManagedDelegate ^ cb) {
  _post_query_cb = gcnew Post_query_cb(cb);
  (*_ref)->post_query_slot = _post_query_cb->get_slot();
}

//--------------------------------------------------------------------------------------------------

void SqlEditorFormWrapper::view_switched() {
  _docking_point->view_switched();
}
