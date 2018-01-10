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

#include "var_grid_model_wr.h"

namespace MySQL {
  namespace Grt {
    namespace Db {

    public
      ref class RecordsetWrapper : public MySQL::Grt::VarGridModelWrapper {
      public:
        typedef ManagedRef<::Recordset> ^ Ref;
        RecordsetWrapper(Ref ref);
        RecordsetWrapper(IntPtr nref_ptr);
        Ref ref() {
          return _ref;
        }
        IntPtr ref_intptr() {
          return ~_ref;
        }

      private:
        Ref _ref;
        ~RecordsetWrapper();

      public:
        MySQL::Grt::ActionList ^ action_list;
        void register_edit_actions();

        long long key() {
          return _ref->key();
        }
        String ^ caption() { return CppStringToNative(_ref->caption()); } void caption(String ^ value) {
          _ref->caption(NativeToCppString(value));
        }
        bool can_close() {
          return _ref->can_close();
        }
        bool close() {
          if ((void*)~_ref != NULL)
            return _ref->close();
          return true;
        }

        String ^ status_text() { return CppStringToNative(_ref->status_text()); }

          int row_count() {
          return (int)_ref->row_count();
        }

        void pending_changes(int % upd_count, int % ins_count, int % del_count);
        bool has_pending_changes() {
          return _ref->has_pending_changes();
        }
        void apply_changes() {
          _ref->apply_changes();
        }
        void rollback() {
          _ref->rollback();
        }

        void limit_rows(bool value) {
          _ref->limit_rows(value);
        }
        bool limit_rows() {
          return _ref->limit_rows();
        }
        bool limit_rows_applicable() {
          return _ref->limit_rows_applicable();
        }
        int limit_rows_count() {
          return _ref->limit_rows_count();
        }

        int real_row_count() {
          return (int)_ref->real_row_count();
        }

        void sort_by(int column, int direction, bool retaining) {
          _ref->sort_by((::ColumnId)column, direction, retaining);
        }

        bool delete_nodes(List<NodeIdWrapper ^> ^ nodes);

        bool has_column_filters() {
          return _ref->has_column_filters();
        }
        bool has_column_filter(int column) {
          return _ref->has_column_filter((::ColumnId)column);
        }
        String ^ get_column_filter_expr(int column) {
          return CppStringToNative(_ref->get_column_filter_expr((::ColumnId)column));
        } void set_column_filter(int column, System::String ^ filter_expr) {
          _ref->set_column_filter((::ColumnId)column, NativeToCppString(filter_expr));
        }
        void reset_column_filter(int column) {
          _ref->reset_column_filter((::ColumnId)column);
        }
        void reset_column_filters() {
          _ref->reset_column_filters();
        }
        int column_filter_icon_id() {
          return (int)_ref->column_filter_icon_id();
        }

        String ^ data_search_string() {
          return CppStringToNative(_ref->data_search_string());
        } void set_data_search_string(String ^ value) {
          _ref->set_data_search_string(NativeToCppString(value));
        }
        void reset_data_search_string() {
          _ref->reset_data_search_string();
        }

        void copy_rows_to_clipboard(List<int> ^ indeces);
        void copy_field_to_clipboard(int row, int column) {
          _ref->copy_field_to_clipboard(row, column);
        }

        void set_flush_ui_changes_cb(DelegateSlot0<void, void>::ManagedDelegate ^ apply);

        bool inserts_editor() {
          return _ref->inserts_editor();
        }

        String^ getFont() {
          return CppStringToNative(_ref->getFont());
        }

        float getFontSize() {
          return _ref->getFontSize();
        }

      private:
        DelegateSlot0<void, void> ^ _flush_ui_changes;

      public:
        GrtThreadedTaskWrapper ^ task;

        System::Windows::Forms::ContextMenuStrip ^ get_context_menu(List<int> ^ indexes, int clicked_column);

        delegate MySQL::Base::IRecordsetView ^ CreateRecordsetViewForWrapper(RecordsetWrapper ^ wrapper);

        // used by the main program to initialize mforms::RecordGridView
        // we can't create a direct delegate that will create a RecordsetView from a std::shared_ptr<Recordset>, because
        // RecordsetView
        // is in C# and can't pass around std::shared_ptr values, so we do it in 2 stage callback
        static void init_mforms(CreateRecordsetViewForWrapper ^ deleg);

      private:
        static MySQL::Base::IRecordsetView ^
          wrap_and_create_recordset_view(IntPtr /* to a std::shared_ptr<Recordset> ptr */ rset);
        static CreateRecordsetViewForWrapper ^ create_recordset_for_wrapper = nullptr;
      };

    }; // namespace Db
  };   // namespace Grt
};     // namespace MySQL
