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

#ifdef _MSC_VER
#pragma warning(disable : 4251) // class ... needs to have dll-interface to be used by clients
                                // Warning caused by Google templates not using dllexport.
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstddef>

#include "grt/common.h"

#include "module_db_mysql.h"
#include "diff/diffchange.h"
#include "diff/grtdiff.h"
#include "grtdb/diff_dbobjectmatch.h"
#include "db_mysql_diffsqlgen.h"
#include "db_mysql_params.h"

#include "db_mysql_diffsqlgen_grant.h"
#include "../res/reporting_includes/basic_text_report.txt.tpl.varnames.h"

namespace mtemplate {
  class DictionaryInterface;
};

class ActionGenerateReport : public DiffSQLGeneratorBEActionInterface {
  std::string fname;
  mtemplate::DictionaryInterface* dictionary;
  mtemplate::DictionaryInterface* current_table_dictionary;
  mtemplate::DictionaryInterface* current_schema_dictionary;

  bool has_attributes, has_partitioning; //, schema_altered;

public:
  ActionGenerateReport(grt::StringRef template_filename);
  virtual ~ActionGenerateReport();

  std::string generate_output();

  std::string object_name(const GrtNamedObjectRef obj) const;
  std::string trigger_name(const GrtNamedObjectRef obj) const;

  // create table
  void create_table_props_begin(db_mysql_TableRef);
  void create_table_props_end(db_mysql_TableRef);

  void create_table_columns_begin(db_mysql_TableRef);
  void create_table_column(db_mysql_ColumnRef);
  void create_table_columns_end(db_mysql_TableRef);

  void create_table_indexes_begin(db_mysql_TableRef);
  void create_table_index(db_mysql_IndexRef, bool gen_create_index);
  void create_table_indexes_end(db_mysql_TableRef);

  void create_table_fks_begin(db_mysql_TableRef);
  void create_table_fk(db_mysql_ForeignKeyRef);
  void create_table_fks_end(db_mysql_TableRef);

  void create_table_engine(grt::StringRef);
  void create_table_next_auto_inc(grt::StringRef);
  void create_table_password(grt::StringRef);
  void create_table_delay_key_write(grt::IntegerRef);
  void create_table_charset(grt::StringRef);
  void create_table_collate(grt::StringRef);
  void create_table_merge_union(grt::StringRef);
  void create_table_merge_insert(grt::StringRef);
  void create_table_pack_keys(grt::StringRef);
  void create_table_checksum(grt::IntegerRef);
  void create_table_row_format(grt::StringRef);
  void create_table_key_block_size(grt::StringRef);
  void create_table_avg_row_length(grt::StringRef);
  void create_table_min_rows(grt::StringRef);
  void create_table_max_rows(grt::StringRef);
  void create_table_comment(grt::StringRef);
  void create_table_data_dir(grt::StringRef);
  void create_table_index_dir(grt::StringRef);

  // drop table
  void drop_table(db_mysql_TableRef);

  // alter table
  void alter_table_props_begin(db_mysql_TableRef);
  void alter_table_name(db_mysql_TableRef, grt::StringRef);
  void alter_table_engine(db_mysql_TableRef, grt::StringRef);
  void alter_table_next_auto_inc(db_mysql_TableRef, grt::StringRef);
  void alter_table_password(db_mysql_TableRef, grt::StringRef);
  void alter_table_delay_key_write(db_mysql_TableRef, grt::IntegerRef);
  void alter_table_charset(db_mysql_TableRef, grt::StringRef);
  void alter_table_collate(db_mysql_TableRef, grt::StringRef);
  void alter_table_comment(db_mysql_TableRef, grt::StringRef);
  void alter_table_merge_union(db_mysql_TableRef, grt::StringRef);
  void alter_table_merge_insert(db_mysql_TableRef, grt::StringRef);
  void alter_table_pack_keys(db_mysql_TableRef, grt::StringRef);
  void alter_table_checksum(db_mysql_TableRef, grt::IntegerRef);
  void alter_table_row_format(db_mysql_TableRef, grt::StringRef);
  void alter_table_key_block_size(db_mysql_TableRef, grt::StringRef);
  void alter_table_avg_row_length(db_mysql_TableRef, grt::StringRef);
  void alter_table_min_rows(db_mysql_TableRef, grt::StringRef);
  void alter_table_max_rows(db_mysql_TableRef, grt::StringRef);
  void alter_table_connection_string(db_mysql_TableRef, grt::StringRef);

  void alter_table_generate_partitioning(db_mysql_TableRef table, const std::string& part_type,
                                         const std::string& part_expr, int part_count, const std::string& subpart_type,
                                         const std::string& subpart_expr,
                                         grt::ListRef<db_mysql_PartitionDefinition> part_defs);
  void alter_table_drop_partitioning(db_mysql_TableRef table);
  void alter_table_add_partition(db_mysql_PartitionDefinitionRef part, bool is_range);
  void alter_table_drop_partition(const std::string& part_name);
  void alter_table_reorganize_partition(db_mysql_PartitionDefinitionRef old_part,
                                        db_mysql_PartitionDefinitionRef new_part, bool is_range);
  void alter_table_partition_count(db_mysql_TableRef, grt::IntegerRef);
  void alter_table_partition_definitions(db_mysql_TableRef, grt::StringRef);
  void alter_table_props_end(db_mysql_TableRef);

  void alter_table_columns_begin(db_mysql_TableRef);
  void alter_table_add_column(db_mysql_TableRef, std::map<std::string, std::string>, db_mysql_ColumnRef column,
                              db_mysql_ColumnRef after);
  void alter_table_drop_column(db_mysql_TableRef, db_mysql_ColumnRef);
  void alter_table_change_column(db_mysql_TableRef table, db_mysql_ColumnRef org_col, db_mysql_ColumnRef mod_col,
                                 db_mysql_ColumnRef after, bool modified,
                                 std::map<std::string, std::string> column_rename_map);
  void alter_table_columns_end(db_mysql_TableRef);

  void alter_table_indexes_begin(db_mysql_TableRef);
  void alter_table_add_index(db_mysql_IndexRef);
  void alter_table_drop_index(db_mysql_IndexRef);
  void alter_table_indexes_end(db_mysql_TableRef);
  void alter_table_change_index(db_mysql_IndexRef orgIndex, db_mysql_IndexRef newIndex);

  void alter_table_fks_begin(db_mysql_TableRef);
  void alter_table_add_fk(db_mysql_ForeignKeyRef);
  void alter_table_drop_fk(db_mysql_ForeignKeyRef);
  void alter_table_fks_end(db_mysql_TableRef);

  // triggers create/drop
  void create_trigger(db_mysql_TriggerRef, bool for_alter);
  void drop_trigger(db_mysql_TriggerRef, bool for_alter);

  // views create/drop
  void create_view(db_mysql_ViewRef);
  void drop_view(db_mysql_ViewRef);

  // routines create/drop5
  void create_routine(db_mysql_RoutineRef, bool for_alter);
  void drop_routine(db_mysql_RoutineRef, bool for_alter);

  // users create/drop
  void create_user(db_UserRef);
  void drop_user(db_UserRef);

  // schema create/drop
  void create_schema(db_mysql_SchemaRef);
  void drop_schema(db_mysql_SchemaRef);

  // alter schema
  void alter_schema_props_begin(db_mysql_SchemaRef);
  void alter_schema_name(db_mysql_SchemaRef, grt::StringRef value);
  void alter_schema_default_charset(db_mysql_SchemaRef, grt::StringRef value);
  void alter_schema_default_collate(db_mysql_SchemaRef, grt::StringRef value);
  void alter_schema_props_end(db_mysql_SchemaRef);
  virtual void disable_list_insert(const bool flag){};
};
