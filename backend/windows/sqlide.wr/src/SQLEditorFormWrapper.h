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

#pragma once
#pragma unmanaged
#include "sqlide/wb_sql_editor_tree_controller.h"
#include "sqlide/wb_sql_editor_form.h"
#include "sqlide/wb_sql_editor_panel.h"

#pragma managed
#include "db_sql_editor_history_wr.h"
#include "Overview.h"

using namespace Runtime::InteropServices; // Needed for the [Out] keyword.

namespace MySQL {
  namespace GUI {
    namespace Workbench {

    public
      ref class SqlEditorFormWrapper : public MySQL::Base::UIForm {
      private:
        SqlEditorForm::Ref *_ref;
        mforms::DockingPoint *_docking_point;
        MySQL::Forms::ManagedDockDelegate ^ _dock_delegate_wrapper;

        VarGridModelWrapper ^ _log;
        DbSqlEditorHistoryWrapper ^ _history;

        ~SqlEditorFormWrapper();

      public:
        typedef ManagedRef<::SqlEditorForm> ^ Ref;
        GrtThreadedTaskWrapper ^ exec_sql_task;

        SqlEditorFormWrapper(std::shared_ptr<::SqlEditorForm> *ptr);

        MySQL::Grt::ActionList ^ action_list;

        GrtManager ^ grt_manager() { return gcnew GrtManager(); }

          void new_sql_script_file() {
          (*_ref)->new_sql_script_file();
        }

        void sql_editor_reorder(MySQL::Forms::AppViewDockContent ^ page, Int32 to) {
          (*_ref)->sql_editor_reordered(dynamic_cast<SqlEditorPanel *>(page->GetBackend()), to);
        }
        String ^
          sql_editor_path(Int32 index) {
            if ((*_ref)->sql_editor_panel(index))
              return CppStringToNative((*_ref)->sql_editor_panel(index)->filename());
            else
              return "";
          }

          Int32 exec_sql_error_count() {
          return (*_ref)->exec_sql_error_count();
        };
        void show_output_area();

        void handle_tab_menu_action(String ^ action, int tab) {
          (*_ref)->handle_tab_menu_action(NativeToCppString(action), tab);
        }

        void handle_history_action(String ^ action, String ^ sql) {
          (*_ref)->handle_history_action(NativeToCppString(action), NativeToCppString(sql));
        }

        // History and log.
        VarGridModelWrapper ^ log() { return _log; } System::Windows::Forms::ContextMenuStrip ^ get_log_context_menu();
        DbSqlEditorHistoryWrapper ^ history() { return _history; } String ^
          restore_sql_from_history(Int32 entry_index, List<Int32> ^ detail_indexes);
        void set_log_selection(List<Int32> ^ selection);

        bool can_close() {
          return (*_ref)->can_close();
        }

        // Native UI control factory methods.
        System::Windows::Forms::Control ^ get_sidebar_control();
        System::Windows::Forms::Control ^ get_palette_control();

        void set_tool_item_checked(String ^ name, bool value);
        void set_docking_delegate(MySQL::Forms::ManagedDockDelegate ^ theDelegate);

        void view_switched();

      public:
        typedef MySQL::Grt::DelegateSlot2<void, void, std::string, String ^, bool, bool> Output_text_ui_cb;
        void output_text_ui_cb(Output_text_ui_cb::ManagedDelegate ^ cb);

        typedef MySQL::Grt::DelegateSlot1<void, void, int, int> Set_busy_tab_cb;
        void set_busy_tab_cb(Set_busy_tab_cb::ManagedDelegate ^ cb);

        typedef MySQL::Grt::DelegateSlot0<void, void> Post_query_cb;
        void set_post_query_cb(Post_query_cb::ManagedDelegate ^ cb);

      private:
        Output_text_ui_cb ^ _output_text_ui_cb;
        Set_busy_tab_cb ^ _set_busy_tab_cb;
        Post_query_cb ^ _post_query_cb;
      };

    }; // namespace Workbench
  };   // namespace GUI
};     // namespace MySQL
