/* 
 * Copyright (c) 2007, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "workbench/wb_overview.h"
#include "GrtTemplates.h"
#include "DelegateWrapper.h"
#include "sqlide/wb_sql_editor_result_panel.h"

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

SqlEditorFormWrapper::SqlEditorFormWrapper(boost::shared_ptr<SqlEditorForm> *ptr)
{
  _docking_point = NULL;
  _dock_delegate_wrapper = nullptr;

  SqlEditorForm *inner= (SqlEditorForm*)ptr->get();

  _ref = new SqlEditorForm::Ref(*ptr);
  
  _refresh_ui = gcnew RefreshUI(inner);

  UIForm::init((*_ref).get());

  _log = Ref2Ptr_<::VarGridModel, VarGridModelWrapper>((*_ref)->log());
  _history = Ref2Ptr<::DbSqlEditorHistory, DbSqlEditorHistoryWrapper>((*_ref)->history());

  exec_sql_task = gcnew GrtThreadedTaskWrapper((*_ref)->exec_sql_task.get());

}

//--------------------------------------------------------------------------------------------------

SqlEditorFormWrapper::~SqlEditorFormWrapper()
{
  if (_docking_point != NULL)
    _docking_point->release();
  delete _dock_delegate_wrapper;

  delete _refresh_ui;
  delete exec_sql_task;
  delete _history;
  delete _log;
  (*_ref)->close();
  inner = NULL;
  delete _ref;
}

//--------------------------------------------------------------------------------------------------

RecordsetWrapper^ SqlEditorFormWrapper::recordset(Int32 editor, Int32 index)
{
  return Ref2Ptr_<::Recordset, RecordsetWrapper>((*_ref)->recordset(editor, index));
}

//--------------------------------------------------------------------------------------------------

RecordsetWrapper^ SqlEditorFormWrapper::recordset_for_key(Int32 editor, Int32 key)
{
  return Ref2Ptr_<::Recordset, RecordsetWrapper>((*_ref)->recordset_for_key(editor, key));
}

//--------------------------------------------------------------------------------------------------

void SqlEditorFormWrapper::active_recordset(Int32 editor, RecordsetWrapper ^val)
{
  if (val == nullptr)
    (*_ref)->active_recordset(editor, Recordset::Ref());
  else
    (*_ref)->active_recordset(editor, *(Recordset::Ref*)val->ref_intptr().ToPointer());
}

//--------------------------------------------------------------------------------------------------

void SqlEditorFormWrapper::show_output_area()
{
  (*_ref)->show_output_area();
}

//--------------------------------------------------------------------------------------------------

void SqlEditorFormWrapper::recordset_list_changed_cb(Recordset_list_changed_cb::ManagedDelegate ^cb)
{
  _recordset_list_changed_cb = gcnew Recordset_list_changed_cb(cb);
  (*_ref)->recordset_list_changed.connect(_recordset_list_changed_cb->get_slot());
}

//--------------------------------------------------------------------------------------------------

void SqlEditorFormWrapper::output_text_ui_cb(Output_text_ui_cb::ManagedDelegate ^cb)
{
  _output_text_ui_cb= gcnew Output_text_ui_cb(cb);
  (*_ref)->output_text_slot= _output_text_ui_cb->get_slot();
}

//--------------------------------------------------------------------------------------------------

void SqlEditorFormWrapper::sql_editor_new_ui_cb(Sql_editor_new_ui_cb::ManagedDelegate ^cb)
{
  _sql_editor_new_ui_cb = gcnew Sql_editor_new_ui_cb(cb);
  (*_ref)->sql_editor_new_ui.connect(_sql_editor_new_ui_cb->get_slot());
}

//--------------------------------------------------------------------------------------------------

ContextMenuStrip^ SqlEditorFormWrapper::get_log_context_menu()
{
  // TODO: replace this by an own wrapper for the log (not just a generic VarGridModel).
  return dynamic_cast<ContextMenuStrip ^>(ObjectMapper::GetManagedComponent((*_ref)->log()->get_context_menu()));
}

//--------------------------------------------------------------------------------------------------

String ^ SqlEditorFormWrapper::restore_sql_from_history(Int32 entry_index, List<Int32> ^detail_indexes)
{
  std::list<Int32> indexes = NativeListToCppList<Int32, Int32>(detail_indexes);
  return CppStringToNative((*_ref)->restore_sql_from_history(entry_index, indexes));
}

//--------------------------------------------------------------------------------------------------

void SqlEditorFormWrapper::set_log_selection(List<Int32> ^selection)
{
  std::vector<Int32> sel = NativeListToCppVector<Int32, Int32>(selection);
  (*_ref)->log()->set_selection(sel);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the native control which is behind the task side bar implementation, so the (C#) UI can
 * embed it. The sidebar is created on the way if not yet done.
 */
Control^ SqlEditorFormWrapper::get_sidebar_control()
{
  return dynamic_cast<Control^>(ObjectMapper::GetManagedComponent((*_ref)->get_sidebar()));
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the managed toolstrip for an individual sql editor.
 */
ToolStrip^ SqlEditorFormWrapper::get_editor_toolbar(Int32 index)
{
  return dynamic_cast<ToolStrip ^>(ObjectMapper::GetManagedComponent((*_ref)->sql_editor_toolbar(index).get()));
}

//--------------------------------------------------------------------------------------------------

/**
 * Managed control for the palette window for docking.
 */
Control^ SqlEditorFormWrapper::get_palette_control()
{
  return dynamic_cast<Control^>(ObjectMapper::GetManagedComponent((*_ref)->get_side_palette()));
}

//--------------------------------------------------------------------------------------------------

Control^ SqlEditorFormWrapper::get_result_panel_for(RecordsetWrapper ^resultset)
{
  boost::shared_ptr<SqlEditorResult> panel((*_ref)->result_panel(*(Recordset::Ref*)resultset->ref_intptr().ToPointer()));
  return dynamic_cast<Control ^>(ObjectMapper::GetManagedComponent(&*panel));
}

//--------------------------------------------------------------------------------------------------

void SqlEditorFormWrapper::dock_grid_to_result_panel(Control ^control, RecordsetWrapper ^resultset)
{
  boost::shared_ptr<SqlEditorResult> panel((*_ref)->result_panel(*(Recordset::Ref*)resultset->ref_intptr().ToPointer()));

  panel->dock_result_grid(MySQL::Forms::Native::wrapper_for_control(control));
}

//--------------------------------------------------------------------------------------------------

void SqlEditorFormWrapper::set_tool_item_checked(String^ name, bool value)
{
  (*_ref)->set_tool_item_checked(NativeToCppString(name), value);
}

//--------------------------------------------------------------------------------------------------

void SqlEditorFormWrapper::set_docking_delegate(ManagedDockDelegate ^theDelegate)
{
  if (_docking_point != NULL)
    _docking_point->release();

  if (_dock_delegate_wrapper != nullptr)
    delete _dock_delegate_wrapper;
  _dock_delegate_wrapper = theDelegate;

  // We don't let the docking point delete our native delegate because it is managed by the 
  // managed delegate.
  _docking_point = mforms::manage(new mforms::DockingPoint(theDelegate->get_unmanaged_delegate(), false));
  db_query_EditorRef qeditor((*_ref)->wbsql()->get_grt_editor_object(_ref->get()));
  qeditor->dockingPoint(mforms_to_grt(qeditor->get_grt(), _docking_point, "DockingPoint"));
}

//--------------------------------------------------------------------------------------------------
