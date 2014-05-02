/* 
 * Copyright (c) 2009, 2013, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MYSQL_TABLE_EDITOR_H_
#define _MYSQL_TABLE_EDITOR_H_

#include "grtdb/editor_table.h"
#include "grts/structs.workbench.physical.h"
#include "grts/structs.db.mysql.h"

#include "mysql_support_backend_public_interface.h"

class MySQLTableEditorBE;

namespace mforms
{
  class View;
};

class MYSQLWBMYSQLSUPPORTBACKEND_PUBLIC_FUNC MySQLTableColumnsListBE : public ::bec::TableColumnsListBE
{
public:
  enum MySQLColumnListColumns
  {
    IsAutoIncrement= bec::TableColumnsListBE::LastColumn,
    IsAutoIncrementable,
  };

  virtual bool set_field(const ::bec::NodeId &node, int column, const std::string &value);
  virtual bool set_field(const ::bec::NodeId &node, int column, int value);

  MySQLTableColumnsListBE(MySQLTableEditorBE *owner);
  
  virtual bool activate_popup_item_for_nodes(const std::string &name, const std::vector<bec::NodeId> &orig_nodes);
  virtual bec::MenuItemList get_popup_items_for_nodes(const std::vector<bec::NodeId> &nodes);
protected:
  // for internal use only
  virtual bool get_field_grt(const ::bec::NodeId &node, int column, ::grt::ValueRef &value);
};



class MYSQLWBMYSQLSUPPORTBACKEND_PUBLIC_FUNC MySQLTableIndexListBE : public bec::IndexListBE
{
public:
  enum Columns {
    StorageType = bec::IndexListBE::LastColumn,
    RowBlockSize,
    Parser
  };

  MySQLTableIndexListBE(MySQLTableEditorBE *owner);

  virtual bool set_field(const ::bec::NodeId &node, int column, const std::string &value);

protected:
  virtual bool get_field_grt(const ::bec::NodeId &node, int column, grt::ValueRef &value);
};



class MYSQLWBMYSQLSUPPORTBACKEND_PUBLIC_FUNC MySQLTablePartitionTreeBE : public bec::TreeModel
{
  MySQLTableEditorBE *_owner;

public:
  // all columns are string, including min/maxRows
  enum Columns {
    Name,
    Value,
    MinRows,
    MaxRows,
    DataDirectory,
    IndexDirectory,
    Comment
  };

  MySQLTablePartitionTreeBE(MySQLTableEditorBE *owner);

  virtual void refresh() {};

  virtual int count_children(const ::bec::NodeId &parent);
  virtual ::bec::NodeId get_child(const ::bec::NodeId &parent, int index);

  virtual bool set_field(const ::bec::NodeId &node, int column, const std::string &value);

protected:
  virtual bool get_field_grt(const ::bec::NodeId &node, int column, grt::ValueRef &value);
  virtual grt::Type get_field_type(const ::bec::NodeId &node, int column);

  db_mysql_PartitionDefinitionRef get_definition(const ::bec::NodeId &node);
};


class MySQLTriggerPanel;

class MYSQLWBMYSQLSUPPORTBACKEND_PUBLIC_FUNC MySQLTableEditorBE : public ::bec::TableEditorBE
{
  friend class MySQLTriggerPanel;
public:
  MySQLTableEditorBE(::bec::GRTManager *grtm, db_mysql_TableRef table, const db_mgmt_RdbmsRef &rdbms);
  virtual ~MySQLTableEditorBE();

  virtual MySQLTableColumnsListBE *get_columns() { return &_columns; }
  virtual MySQLTableIndexListBE *get_indexes() { return &_indexes; }

  virtual db_TableRef get_table() { return table(); }

  db_mysql_TableRef table() { return _table; }

  virtual std::vector<std::string> get_index_types();
  virtual std::vector<std::string> get_index_storage_types();
  virtual std::vector<std::string> get_fk_action_options();

  // table options
  virtual void set_table_option_by_name(const std::string& name, const std::string& value);
  virtual std::string get_table_option_by_name(const std::string& name);
  std::vector<std::string> get_engines_list();
  bool engine_supports_foreign_keys();
  
  virtual bool check_column_referenceable_by_fk(const db_ColumnRef &column1, const db_ColumnRef &column2);

  // interactive functions
  std::string get_all_triggers_sql() const; // TODO: only used internally and in tests, should be protected.
  void load_trigger_sql();

  // triggers
  mforms::View *get_trigger_panel();

  virtual bool can_close();
  virtual void commit_changes();

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

  MySQLTablePartitionTreeBE *get_partitions() { return &_partitions; }

  // whether partitions and subpartitions will be defined by the user or not 
  // if false, only count is needed otherwise the partitionslist must be defined
  void set_explicit_partitions(bool flag);
  bool get_explicit_partitions();
  void set_explicit_subpartitions(bool flag);
  bool get_explicit_subpartitions();

  virtual db_TableRef create_stub_table(const std::string &schema, const std::string &table);
  
protected:
  db_mysql_TableRef _table;
  MySQLTableColumnsListBE _columns;
  MySQLTablePartitionTreeBE _partitions;
  MySQLTableIndexListBE _indexes;
  MySQLTriggerPanel *_trigger_panel;
  bool _updating_triggers;
  
  void reset_partition_definitions(int parts, int subparts);
};

#endif /* _MYSQL_TABLE_EDITOR_H_ */
