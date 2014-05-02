/* 
 * Copyright (c) 2008, 2013, Oracle and/or its affiliates. All rights reserved.
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

#include "DBObjectEditorBE.h"
#include "GrtTemplates.h"
#include "grtdb/editor_table.h"
#include "recordset_wr.h"

namespace MySQL {
namespace Grt {
namespace Db {

ref class TableEditorWrapper;
ref class IndexListWrapper;
ref class FKConstraintListWrapper;

public ref class TableColumnsListWrapper : public MySQL::Grt::ListModel
{
public:
  enum class ColumnListColumns {
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
    Charset = bec::TableColumnsListBE::Charset,
    Collation = bec::TableColumnsListBE::Collation,
    HasCharset = bec::TableColumnsListBE::HasCharset,
    Comment = bec::TableColumnsListBE::Comment,
    LastColumn = bec::TableColumnsListBE::LastColumn
  };

  TableColumnsListWrapper(bec::TableColumnsListBE *inn);

  List<String^>^ get_datatype_names()
  {
    return CppStringListToNative(get_unmanaged_object()->get_datatype_names());
  }


  inline bec::TableColumnsListBE *get_unmanaged_object()
  { return static_cast<bec::TableColumnsListBE *>(inner); }

  bool get_row(NodeId^ node,
               [Out] String^ %name,
               [Out] String^ %type,
               [Out] bool^ %ispk,
               [Out] bool^ %notnull,
               [Out] bool^ %unique,
               [Out] bool^ %isbinary,
               [Out] bool^ %isunsigned,
               [Out] bool^ %iszerofill,
               [Out] String^ %flags,
               [Out] String^ %defvalue,
               [Out] String^ %charset,
               [Out] String^ %collation,
               [Out] String^ %comment);

  void reorder_many(List<int> ^rows, int nindex);
};

public ref class IndexColumnsListWrapper : public ListModel
{
public:
  enum class IndexColumnsListColumns {
      Name = bec::IndexColumnsListBE::Name,
      Descending = bec::IndexColumnsListBE::Descending,
      Length = bec::IndexColumnsListBE::Length,
      OrderIndex = bec::IndexColumnsListBE::OrderIndex
  };
  
  IndexColumnsListWrapper(IndexListWrapper^ owner);

  IndexColumnsListWrapper(bec::IndexColumnsListBE *inn);

  inline bec::IndexColumnsListBE *get_unmanaged_object()
  { return static_cast<bec::IndexColumnsListBE *>(inner); }

  void set_column_enabled(NodeId^ node, bool flag);
  bool get_column_enabled(NodeId^ node);

  int get_max_order_index();
};

public ref class IndexListWrapper : public ListModel
{ 
public:
  enum class IndexListColumns {
      Name = bec::IndexListBE::Name,
      Type = bec::IndexListBE::Type,
      Comment = bec::IndexListBE::Comment
  };
  
  IndexListWrapper(TableEditorWrapper^ owner);

  IndexListWrapper(bec::IndexListBE *inn);

  inline bec::IndexListBE *get_unmanaged_object()
  { return static_cast<bec::IndexListBE *>(inner); }
    
  IndexColumnsListWrapper^ get_columns();
  
  //db_Index get_selected_index();
  void select_index(NodeId^ node);

  // cannot create a wrapper instance here
  //TableEditorWrapper *get_owner() { return _owner; }
};

public ref class FKConstraintColumnsListWrapper : public ListModel
{
public:
  enum class FKConstraintColumnsListColumns {
    Enabled = bec::FKConstraintColumnsListBE::Enabled,
    Column = bec::FKConstraintColumnsListBE::Column,
    RefColumn = bec::FKConstraintColumnsListBE::RefColumn
  };
  
  FKConstraintColumnsListWrapper(FKConstraintListWrapper^ owner);

  FKConstraintColumnsListWrapper(bec::FKConstraintColumnsListBE *inn);

  inline bec::FKConstraintColumnsListBE *get_unmanaged_object()
  { return static_cast<bec::FKConstraintColumnsListBE *>(inner); }

  List<String^>^ get_ref_columns_list(NodeId ^node, bool filtered);

  bool set_column_is_fk(NodeId^ node, bool flag);
  bool get_column_is_fk(NodeId^ node);
};

public ref class FKConstraintListWrapper : public ListModel
{
public:
  enum class FKConstraintListColumns {
      Name = bec::FKConstraintListBE::Name,
      OnDelete = bec::FKConstraintListBE::OnDelete,
      OnUpdate = bec::FKConstraintListBE::OnUpdate,
      RefTable = bec::FKConstraintListBE::RefTable,
      Comment = bec::FKConstraintListBE::Comment,
      Index = bec::FKConstraintListBE::Index,
      ModelOnly = bec::FKConstraintListBE::ModelOnly,
  };

  FKConstraintListWrapper(TableEditorWrapper^ owner);

  FKConstraintListWrapper(bec::FKConstraintListBE *inn);

  inline bec::FKConstraintListBE *get_unmanaged_object()
  { return static_cast<bec::FKConstraintListBE *>(inner); }
  
  void select_fk(NodeId^ node);

  FKConstraintColumnsListWrapper^ get_columns();
};

public ref class TableEditorWrapper : public DBObjectEditorBE
{
protected:
  TableEditorWrapper(bec::TableEditorBE *inn)
    : DBObjectEditorBE(inn)
  {}

public:
  enum class PartialRefreshes
  {
    RefreshColumnMoveUp    = bec::TableEditorBE::RefreshColumnMoveUp,
    RefreshColumnMoveDown  = bec::TableEditorBE::RefreshColumnMoveDown,
    RefreshColumnList      = bec::TableEditorBE::RefreshColumnList,
    RefreshColumnCollation = bec::TableEditorBE::RefreshColumnCollation,
  };

  bec::TableEditorBE *get_unmanaged_object()
  { return static_cast<bec::TableEditorBE *>(inner); }

  IndexListWrapper^ get_indexes();

  FKConstraintListWrapper^ get_fks();

  MySQL::Grt::Db::RecordsetWrapper^ get_inserts_model()
  {
    return Ref2Ptr<::Recordset, MySQL::Grt::Db::RecordsetWrapper>(
      get_unmanaged_object()->get_inserts_model());
  }

  Control ^get_inserts_panel(Control ^grid);


  // table options
  //...

  // column editing
  NodeId^ add_column(String^ name);

  void remove_column(NodeId^ column);

  //db_Column get_column_with_name(const std::string &name);

  // fk editing
  NodeId^ add_fk(String^ name);

  void remove_fk(NodeId^ fk);

  NodeId^ add_fk_with_columns(List<NodeId ^> ^columns);

  // index editing
  NodeId^ add_index(String^ name);

  void remove_index(NodeId^ index);

  NodeId^ add_index_with_columns(List<NodeId ^> ^columns);

  List<String^>^ get_index_types();
};

} // namespace Db
} // namespace Grt
} // namespace MySQL
