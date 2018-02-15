/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mysql_table_editor.h"
#include "grt.h"
#include "GrtTemplates.h"

using namespace MySQL::Grt;
using namespace System;
using namespace System::Windows::Forms;
using namespace System::Collections::Generic;

namespace MySQL {
  namespace Grt {
    namespace Db {

      ref class MySQLTableEditorWrapper;

    public
      ref class MySQLTableColumnsListWrapper : public Db::TableColumnsListWrapper {
      public:
        enum class MySQLColumnListColumns {
          Name = bec::TableColumnsListBE::Name,
          Type = bec::TableColumnsListBE::Type,
          IsPK = bec::TableColumnsListBE::IsPK,
          IsNotNull = bec::TableColumnsListBE::IsNotNull,
          IsUnique = bec::TableColumnsListBE::IsUnique,
          IsBinary = bec::TableColumnsListBE::IsBinary,
          IsUnsigned = bec::TableColumnsListBE::IsUnsigned,
          IsZerofill = bec::TableColumnsListBE::IsZerofill,
          Flags = bec::TableColumnsListBE::Flags,
          Default = bec::TableColumnsListBE::Default,
          CharsetCollation = bec::TableColumnsListBE::CharsetCollation,
          Charset = bec::TableColumnsListBE::Charset,
          Collation = bec::TableColumnsListBE::Collation,
          HasCharset = bec::TableColumnsListBE::HasCharset,
          Comment = bec::TableColumnsListBE::Comment,
          IsAutoIncrement = ::MySQLTableColumnsListBE::IsAutoIncrement,
          IsAutoIncrementable = ::MySQLTableColumnsListBE::IsAutoIncrementable,

          IsGenerated = MySQLTableColumnsListBE::IsGenerated,
          GeneratedExpression = MySQLTableColumnsListBE::GeneratedExpression,
          GeneratedStorageType = MySQLTableColumnsListBE::GeneratedStorageType,
        };

        MySQLTableColumnsListWrapper(::MySQLTableColumnsListBE *inn);

        inline ::MySQLTableColumnsListBE *get_unmanaged_object() {
          return static_cast<::MySQLTableColumnsListBE *>(inner);
        }

        bool set_column_type(NodeIdWrapper ^ nodeid, GrtValue ^ value) {
          return get_unmanaged_object()->set_column_type(*nodeid->get_unmanaged_object(),
                                                         GrtObjectRef::cast_from(value->get_unmanaged_object()));
        }

        List<String ^> ^
          get_datatype_flags(NodeIdWrapper ^ nodeid) {
            return CppStringListToNative(get_unmanaged_object()->get_datatype_flags(*nodeid->get_unmanaged_object()));
          }

          void set_column_flag(NodeIdWrapper ^ nodeid, String ^ flag_name, int is_set) {
          get_unmanaged_object()->set_column_flag(*nodeid->get_unmanaged_object(), NativeToCppString(flag_name),
                                                  is_set);
        }

        int get_column_flag(NodeIdWrapper ^ nodeid, String ^ flag_name) {
          return get_unmanaged_object()->get_column_flag(*nodeid->get_unmanaged_object(), NativeToCppString(flag_name));
        }
      };

    public
      ref class MySQLIndexListWrapper : public Db::IndexListWrapper {
      public:
        MySQLIndexListWrapper(::MySQLTableIndexListBE *inner) : Db::IndexListWrapper(inner) {
        }

        enum class Columns {
          Name = ::MySQLTableIndexListBE::IndexListBE::IndexListColumns::Name,
          Type = ::MySQLTableIndexListBE::IndexListBE::IndexListColumns::Type,
          Visible = ::MySQLTableIndexListBE::IndexListBE::IndexListColumns::Visible,
          StorageType = ::MySQLTableIndexListBE::StorageType,
          RowBlockSize = ::MySQLTableIndexListBE::RowBlockSize,
          Parser = ::MySQLTableIndexListBE::Parser,
          
        };
      };

    public
      ref class MySQLTablePartitionTreeWrapper : public TreeModelWrapper {
      public:
        MySQLTablePartitionTreeWrapper(::MySQLTablePartitionTreeBE *inner) : TreeModelWrapper(inner) {
        }

        enum class Columns {
          Name = ::MySQLTablePartitionTreeBE::Name,
          Value = ::MySQLTablePartitionTreeBE::Value,
          MinRows = ::MySQLTablePartitionTreeBE::MinRows,
          MaxRows = ::MySQLTablePartitionTreeBE::MaxRows,
          DataDirectory = ::MySQLTablePartitionTreeBE::DataDirectory,
          IndexDirectory = ::MySQLTablePartitionTreeBE::IndexDirectory,
          Comment = ::MySQLTablePartitionTreeBE::Comment
        };
      };

    public
      ref class MySQLTableEditorWrapper : public Db::TableEditorWrapper {
      public:
        enum class PartialRefreshes {
          RefreshColumnList = bec::TableEditorBE::RefreshColumnList,
          RefreshColumnCollation = bec::TableEditorBE::RefreshColumnCollation,
        };

        MySQLTableEditorWrapper(GrtValue ^ arglist);
        virtual ~MySQLTableEditorWrapper();

        ::MySQLTableEditorBE *get_unmanaged_object() {
          return static_cast<::MySQLTableEditorBE *>(inner);
        }

        virtual MySQLTableColumnsListWrapper ^ get_columns();

        void set_table_option_by_name(System::String ^ name, System::String ^ value) {
          get_unmanaged_object()->set_table_option_by_name(NativeToCppString(name), NativeToCppString(value));
        }

        List<String ^> ^
          get_engines_list() { return CppStringListToNative(get_unmanaged_object()->get_engines_list()); }

          List<String ^> ^
          get_index_storage_types() { return CppStringListToNative(get_unmanaged_object()->get_index_storage_types()); }

          List<String ^> ^
          get_fk_action_options() { return CppStringListToNative(get_unmanaged_object()->get_fk_action_options()); }

          bool engine_supports_foreign_keys() {
          return get_unmanaged_object()->engine_supports_foreign_keys();
        }

        String ^ MySQLTableEditorWrapper::get_table_option_by_name(String ^ name) {
          return CppStringToNative(get_unmanaged_object()->get_table_option_by_name(NativeToCppString(name)));
        }

        void set_sql(String ^ sql) {
          get_unmanaged_object()->set_sql(NativeToCppString(sql));
        }

        Control ^ get_trigger_panel();
        void commit_changes();

        bool set_partition_type(String ^ type) {
          return get_unmanaged_object()->set_partition_type(NativeToCppString(type));
        }

        String ^ get_partition_type() { return CppStringToNative(get_unmanaged_object()->get_partition_type()); }

          void set_partition_expression(String ^ expr) {
          get_unmanaged_object()->set_partition_expression(NativeToCppString(expr));
        }

        String ^
          get_partition_expression() { return CppStringToNative(get_unmanaged_object()->get_partition_expression()); }

          void set_partition_count(int count) {
          get_unmanaged_object()->set_partition_count(count);
        }

        int get_partition_count() {
          return get_unmanaged_object()->get_partition_count();
        }

        bool set_subpartition_type(String ^ type) {
          return get_unmanaged_object()->set_subpartition_type(NativeToCppString(type));
        }

        String ^ get_subpartition_type() { return CppStringToNative(get_unmanaged_object()->get_subpartition_type()); }

          bool set_subpartition_expression(String ^ expr) {
          return get_unmanaged_object()->set_subpartition_expression(NativeToCppString(expr));
        }

        String ^
          get_subpartition_expression() {
            return CppStringToNative(get_unmanaged_object()->get_subpartition_expression());
          }

          void set_subpartition_count(int count) {
          get_unmanaged_object()->set_subpartition_count(count);
        }

        int get_subpartition_count() {
          return get_unmanaged_object()->get_subpartition_count();
        }

        MySQLTablePartitionTreeWrapper ^ get_partitions();

        // Whether partitions and sub partitions will be defined by the user or not .
        // If false, only count is needed otherwise the partitions list must be defined.
        void set_explicit_partitions(bool flag) {
          get_unmanaged_object()->set_explicit_partitions(flag);
        }

        void set_explicit_subpartitions(bool flag) {
          get_unmanaged_object()->set_explicit_subpartitions(flag);
        }

        bool get_explicit_partitions() {
          return get_unmanaged_object()->get_explicit_partitions();
        }

        bool get_explicit_subpartitions() {
          return get_unmanaged_object()->get_explicit_subpartitions();
        }

        bool is_server_version_at_least(int major, int minor, int release);
        void load_trigger_sql();
      };

    }; // namespace Db
  };   // namespace Grt
};     // namespace MySQL
