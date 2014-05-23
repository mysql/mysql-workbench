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

#pragma once

#include "sqlide/wb_sql_editor_form.h"

#include "db_sql_editor_history_wr.h"
#include "Overview.h"
#include "SQLEditorWrapper.h"

using namespace Runtime::InteropServices; // Needed for the [Out] keyword.

namespace MySQL {
namespace GUI {
namespace Workbench {

public ref class SqlEditorFormWrapper : public MySQL::Base::UIForm
{
private:
  SqlEditorForm::Ref *_ref;
  mforms::DockingPoint *_docking_point;
  MySQL::Forms::ManagedDockDelegate ^_dock_delegate_wrapper;

  VarGridModelWrapper ^_log;
  DbSqlEditorHistoryWrapper ^_history;

  RefreshUI ^_refresh_ui;

  ~SqlEditorFormWrapper();

public:
  enum class PartialRefreshType
  {
    RefreshEditorTitle      = SqlEditorForm::RefreshEditorTitle,
    RefreshRecordsetTitle   = SqlEditorForm::RefreshRecordsetTitle,
    QueryExecutionStarted   = SqlEditorForm::QueryExecutionStarted,
    SaveRecordsetChanges    = SqlEditorForm::SaveRecordsetChanges,
    DiscardRecordsetChanges = SqlEditorForm::DiscardRecordsetChanges,
  };

  typedef ManagedRef<::SqlEditorForm> ^ Ref;
  GrtThreadedTaskWrapper ^exec_sql_task;

  SqlEditorFormWrapper(boost::shared_ptr<::SqlEditorForm> *ptr);

  MySQL::Grt::ActionList ^action_list;

  GrtManager ^ grt_manager() { return gcnew GrtManager((*_ref)->grt_manager()); }

  SqlEditorWrapper ^ sql_editor() { return Ref2Ptr_<MySQLEditor, SqlEditorWrapper>((*_ref)->active_sql_editor()); }
  SqlEditorWrapper ^ sql_editor(Int32 index) { return Ref2Ptr_<MySQLEditor, SqlEditorWrapper>((*_ref)->sql_editor(index)); }
  Int32 sql_editor_count() { return (*_ref)->sql_editor_count(); }
  String ^ sql_editor_caption(Int32 index) { return CppStringToNative((*_ref)->sql_editor_caption(index)); }
  void sql_editor_caption(Int32 index, String^ caption) { ((*_ref)->sql_editor_caption(index, NativeToCppString(caption))); }
  void new_sql_script_file() { (*_ref)->new_sql_script_file(); }
  void remove_sql_editor(Int32 index) { return (*_ref)->remove_sql_editor(index); }
  Int32 active_sql_editor_index() { return (*_ref)->active_sql_editor_index(); }
  void active_sql_editor_index(Int32 val) { (*_ref)->active_sql_editor_index(val); }

  bool sql_editor_reorder(Int32 from, Int32 to) { return (*_ref)->sql_editor_reorder((*_ref)->sql_editor(from), to); }
  bool sql_editor_is_scratch(Int32 index) { return (*_ref)->sql_editor_is_scratch(index); }
  bool sql_editor_start_collapsed(Int32 index) { return (*_ref)->sql_editor_start_collapsed(index); }
  String^ sql_editor_path(Int32 index) { return CppStringToNative((*_ref)->sql_editor_path(index)); }
  Int32 sql_editor_index(SqlEditorWrapper^ editor) { return (*_ref)->sql_editor_index(*(MySQLEditor::Ref*)(void*)~editor->ref()); };

  bool sql_editor_will_close(Int32 index) { return (*_ref)->sql_editor_will_close(index); }

  void active_recordset(Int32 editor, RecordsetWrapper ^val);

  Int32 exec_sql_error_count() { return (*_ref)->exec_sql_error_count(); };
  void show_output_area();

  bool connected() { return (*_ref)->connected(); }
  void cancel_query() { (*_ref)->cancel_query(); }

  void reset() { (*_ref)->reset(); }
  bool exec_editor_sql(SqlEditorWrapper^ editor, bool current_statement_only, bool wrap_with_non_std_delimiter)
  {
    return (*_ref)->exec_editor_sql(&editor->ref(), false, current_statement_only, wrap_with_non_std_delimiter);
  }
  void commit() { (*_ref)->commit(); }
  void rollback() { (*_ref)->rollback(); }
  Int32 recordset_count(Int32 editor) { return (*_ref)->recordset_count(editor); }
  RecordsetWrapper^ recordset(Int32 editor, Int32 index);
  RecordsetWrapper^ recordset_for_key(Int32 editor, Int32 key);

  // History and log.
  VarGridModelWrapper^ log() { return _log; }
  System::Windows::Forms::ContextMenuStrip^ get_log_context_menu();
  DbSqlEditorHistoryWrapper^ history() { return _history; }
  String^ restore_sql_from_history(Int32 entry_index, List<Int32> ^detail_indexes);
  void set_log_selection(List<Int32> ^selection);

  bool can_close() { return (*_ref)->can_close(); }

  void active_schema(String^ schema) { (*_ref)->active_schema(NativeToCppString(schema)); }
  String^ active_schema() { return CppStringToNative((*_ref)->active_schema()); }

  RefreshUI^ refresh_ui() { return _refresh_ui; }

  // Native UI control factory methods.
  Windows::Forms::Control^ get_sidebar_control();
  Windows::Forms::ToolStrip^ get_editor_toolbar(Int32 index);
  Windows::Forms::Control^ get_palette_control();
  Windows::Forms::Control^ get_result_panel_for(RecordsetWrapper ^resultset);

  void dock_grid_to_result_panel(Control ^control, RecordsetWrapper ^rs);

  void set_tool_item_checked(String^ name, bool value);
  void set_docking_delegate(MySQL::Forms::ManagedDockDelegate ^theDelegate);

public:
  typedef MySQL::Grt::DelegateSlot3<void, void,
    Int32, Int32, ::Recordset::Ref, long, bool, bool> Recordset_list_changed_cb;
  void recordset_list_changed_cb(Recordset_list_changed_cb::ManagedDelegate ^cb);
private:
  Recordset_list_changed_cb ^_recordset_list_changed_cb;

public:
  typedef MySQL::Grt::DelegateSlot2<void, void, std::string, String^, bool, bool> Output_text_ui_cb;
  void output_text_ui_cb(Output_text_ui_cb::ManagedDelegate ^cb);
private:
  Output_text_ui_cb ^_output_text_ui_cb;

public:
  typedef MySQL::Grt::DelegateSlot1<Int32, Int32, Int32, Int32> Sql_editor_new_ui_cb;
  void sql_editor_new_ui_cb(Sql_editor_new_ui_cb::ManagedDelegate ^cb);
private:
  Sql_editor_new_ui_cb ^_sql_editor_new_ui_cb;
};


};  // namespace Workbench
};  // namespace GUI
};  // namespace MySQL
