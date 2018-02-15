/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "grtdb/editor_table.h"
#include "grts/structs.workbench.physical.h"
#include "grts/structs.db.mysql.h"

#include "mysql_support_backend_public_interface.h"

class MySQLTableEditorBE;

namespace mforms {
  class View;
};

class MYSQLWBMYSQLSUPPORTBACKEND_PUBLIC_FUNC MySQLTableColumnsListBE : public ::bec::TableColumnsListBE {
public:
  enum MySQLColumnListColumns {
    IsAutoIncrement = bec::TableColumnsListBE::LastColumn,
    IsAutoIncrementable,
    IsGenerated,
    GeneratedStorageType,
    GeneratedExpression
  };

  virtual bool set_field(const ::bec::NodeId &node, ColumnId column, const std::string &value);
  virtual bool set_field(const ::bec::NodeId &node, ColumnId column, ssize_t value);

  MySQLTableColumnsListBE(MySQLTableEditorBE *owner);

  virtual bool activate_popup_item_for_nodes(const std::string &name, const std::vector<bec::NodeId> &orig_nodes);
  virtual bec::MenuItemList get_popup_items_for_nodes(const std::vector<bec::NodeId> &nodes);

protected:
  // for internal use only
  virtual bool get_field_grt(const ::bec::NodeId &node, ColumnId column, ::grt::ValueRef &value);
};

class MYSQLWBMYSQLSUPPORTBACKEND_PUBLIC_FUNC MySQLTableIndexListBE : public bec::IndexListBE {
public:
  enum Columns { StorageType = bec::IndexListBE::LastColumn, RowBlockSize, Parser };

  MySQLTableIndexListBE(MySQLTableEditorBE *owner);

  virtual bool set_field(const ::bec::NodeId &node, ColumnId column, const std::string &value);
  virtual bool set_field(const ::bec::NodeId &node, ColumnId column, ssize_t value);

protected:
  virtual bool get_field_grt(const ::bec::NodeId &node, ColumnId column, grt::ValueRef &value);
};

class MYSQLWBMYSQLSUPPORTBACKEND_PUBLIC_FUNC MySQLTablePartitionTreeBE : public bec::TreeModel {
  MySQLTableEditorBE *_owner;

public:
  // all columns are string, including min/maxRows
  enum Columns { Name, Value, MinRows, MaxRows, DataDirectory, IndexDirectory, Comment };

  MySQLTablePartitionTreeBE(MySQLTableEditorBE *owner);

  virtual void refresh(){};

  virtual size_t count_children(const ::bec::NodeId &parent);
  virtual ::bec::NodeId get_child(const ::bec::NodeId &parent, size_t index);

  virtual bool set_field(const ::bec::NodeId &node, ColumnId column, const std::string &value);

protected:
  virtual bool get_field_grt(const ::bec::NodeId &node, ColumnId column, grt::ValueRef &value);
  virtual grt::Type get_field_type(const ::bec::NodeId &node, ColumnId column);

  db_mysql_PartitionDefinitionRef get_definition(const ::bec::NodeId &node);
};

class MySQLTriggerPanel;

class MYSQLWBMYSQLSUPPORTBACKEND_PUBLIC_FUNC MySQLTableEditorBE : public ::bec::TableEditorBE {
  friend class MySQLTriggerPanel;

public:
  MySQLTableEditorBE(db_mysql_TableRef table);
  virtual ~MySQLTableEditorBE();

  virtual void refresh_live_object();
  virtual void commit_changes();

  virtual MySQLTableColumnsListBE *get_columns() {
    return &_columns;
  }
  virtual MySQLTableIndexListBE *get_indexes() {
    return &_indexes;
  }

  virtual std::vector<std::string> get_index_types();
  virtual std::vector<std::string> get_index_storage_types();
  virtual std::vector<std::string> get_fk_action_options();

  // table options
  virtual void set_table_option_by_name(const std::string &name, const std::string &value);
  virtual std::string get_table_option_by_name(const std::string &name);
  std::vector<std::string> get_engines_list();
  bool engine_supports_foreign_keys();

  virtual bool check_column_referenceable_by_fk(const db_ColumnRef &column1, const db_ColumnRef &column2);

  void load_trigger_sql();

  // triggers
  mforms::View *get_trigger_panel();
  void add_trigger(const std::string &timing, const std::string &event);

  virtual bool can_close();

  // partitioning
  bool set_partition_type(const std::string &type);
  std::string get_partition_type();

  void set_partition_expression(const std::string &expr);
  std::string get_partition_expression();

  void set_partition_count(int count);
  int get_partition_count();

  bool set_subpartition_type(const std::string &type);
  std::string get_subpartition_type();

  bool set_subpartition_expression(const std::string &expr);
  std::string get_subpartition_expression();

  bool subpartition_count_allowed();
  void set_subpartition_count(int count);
  int get_subpartition_count();

  MySQLTablePartitionTreeBE *get_partitions() {
    return &_partitions;
  }

  // whether partitions and sub partitions will be defined by the user or not
  // if false, only count is needed otherwise the partitions list must be defined
  void set_explicit_partitions(bool flag);
  bool get_explicit_partitions();
  void set_explicit_subpartitions(bool flag);
  bool get_explicit_subpartitions();

  virtual db_TableRef create_stub_table(const std::string &schema, const std::string &table);

protected:
  MySQLTableColumnsListBE _columns;
  MySQLTablePartitionTreeBE _partitions;
  MySQLTableIndexListBE _indexes;
  MySQLTriggerPanel *_trigger_panel;
  bool _updating_triggers;

  void reset_partition_definitions(int parts, int subparts);
};
