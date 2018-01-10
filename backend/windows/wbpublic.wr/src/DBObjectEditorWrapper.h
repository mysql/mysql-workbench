/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "grtdb/editor_dbobject.h"
#include "GrtWrapper.h"
#include "GrtTemplates.h"
#include "BaseEditorWrapper.h"
#include "DelegateWrapper.h"

namespace MySQL {
  namespace Grt {
    namespace Db {

    public
      ref class DBObjectEditorWrapper : public BaseEditorWrapper {
      protected:
        DBObjectEditorWrapper(bec::DBObjectEditorBE *inn) : BaseEditorWrapper(inn) {
        }

      public:
        bec::DBObjectEditorBE *get_unmanaged_object() {
          return static_cast<::bec::DBObjectEditorBE *>(inner);
        }

        String ^ get_name() { return CppStringToNative(get_unmanaged_object()->get_name()); }

          void set_name(String ^ name) {
          get_unmanaged_object()->set_name(NativeToCppString(name));
        }

        String ^ get_comment() { return CppStringToNative(get_unmanaged_object()->get_comment()); }

          void set_comment(String ^ descr) {
          get_unmanaged_object()->set_comment(NativeToCppString(descr));
        }

        String ^ get_sql() { return CppStringToNative(get_unmanaged_object()->get_sql()); }

          bool is_sql_commented() {
          return get_unmanaged_object()->is_sql_commented();
        }

        void set_sql_commented(bool flag) {
          get_unmanaged_object()->set_sql_commented(flag);
        }

        // Helpers
        GrtValue ^
          get_catalog() { return gcnew GrtValue(static_cast<::bec::DBObjectEditorBE *>(inner)->get_catalog()); }

          GrtValue
          ^ get_schema() { return gcnew GrtValue(static_cast<::bec::DBObjectEditorBE *>(inner)->get_schema()); }

          String
          ^ get_schema_name() { return CppStringToNative(get_unmanaged_object()->get_schema_name()); }

          List<String ^> ^
          get_all_table_names() {
            return CppStringListToNative(static_cast<::bec::DBObjectEditorBE *>(inner)->get_all_table_names());
          }

          List<String ^> ^
          get_schema_table_names() {
            return CppStringListToNative(static_cast<::bec::DBObjectEditorBE *>(inner)->get_schema_table_names());
          }

          List<String ^> ^
          get_table_column_names(String ^ table_name) {
            List<String ^> ^ val = CppStringListToNative(
              static_cast<::bec::DBObjectEditorBE *>(inner)->get_table_column_names(NativeToCppString(table_name)));
            return val;
          }

          List<String ^> ^
          get_table_column_names(GrtValue ^ table) {
            return CppStringListToNative(static_cast<::bec::DBObjectEditorBE *>(inner)->get_table_column_names(
              db_TableRef::cast_from(table->get_unmanaged_object())));
          }

          List<String ^> ^
            get_charset_list() {
            return CppStringListToNative(static_cast<::bec::DBObjectEditorBE *>(inner)->get_charset_list());
          }

          List<String ^> ^
          get_charset_collation_list() {
            return CppStringListToNative(static_cast<::bec::DBObjectEditorBE *>(inner)->get_charset_collation_list());
          }

          List<String ^> ^
            get_charset_collation_list(String ^charset) {
            return CppStringListToNative(static_cast<::bec::DBObjectEditorBE *>(inner)->get_charset_collation_list(NativeToCppString(charset)));
          }

          private:
        // typedef void (*DBObjectEditorWrapper::VOID_STRINGVECTOR_CB)(const std::vector<std::string>&);
      };

    } // namespace Db
  }   // namespace Grt
} // namespace MySQL
