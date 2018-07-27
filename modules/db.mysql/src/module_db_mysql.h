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

//!
//!    \addtogroup Diff Diff SQL Generator Module
//!             Diff SQL Generator Module
//!
//!    @{
//!

#pragma once

/**
 * @file module_db_mysql.h
 *
 * This file contains declarations for SQL script creation routiness
 *
 * The SQL script generation process consists of 2 steps:
 * <br/>1. generate SQL for individual objects
 * <br/>2. put the gereneated SQL together (e.g. into a string or  a file)
 *
 * The DbMySQLImpl class implements the second step.
 */

#include "db_mysql_public_interface.h"
#include "grtpp_module_cpp.h"
#include "interfaces/sqlgenerator.h"
#include "grtdb/db_object_helpers.h"
#include "grts/structs.db.mysql.h"
//#include "module_db_mysql_shared_code.h"

using namespace bec;

#define DbMySQL_VERSION "1.0"

class DiffSQLGeneratorBE;

/**
 * class DiffSQLGeneratorBEActionInterface defines interface to call-backs that are used to generate
 * DLL SQL or plain text description of changes
 */
class MYSQLMODULEDBMYSQL_PUBLIC_FUNC DiffSQLGeneratorBEActionInterface {
protected:
  bool _put_if_exists;
  bool _omitSchemas;
  bool _gen_use;

public:
  DiffSQLGeneratorBEActionInterface() : _put_if_exists(true), _omitSchemas(false), _gen_use(false){};
  virtual ~DiffSQLGeneratorBEActionInterface();

  // use short or full table names
  virtual void setOmitSchemas(const bool flag) {
    _omitSchemas = flag;
  };
  virtual void set_gen_use(const bool flag) {
    _gen_use = flag;
  };
  virtual void set_put_if_exists(const bool flag) {
    _put_if_exists = flag;
  };
  // create table
  virtual void create_table_props_begin(db_mysql_TableRef) = 0;
  virtual void create_table_props_end(db_mysql_TableRef) = 0;

  virtual void create_table_columns_begin(db_mysql_TableRef) = 0;
  virtual void create_table_column(db_mysql_ColumnRef) = 0;
  virtual void create_table_columns_end(db_mysql_TableRef) = 0;

  virtual void create_table_indexes_begin(db_mysql_TableRef) = 0;
  virtual void create_table_index(db_mysql_IndexRef, bool gen_create_index) = 0;
  virtual void create_table_indexes_end(db_mysql_TableRef) = 0;

  virtual void create_table_fks_begin(db_mysql_TableRef) = 0;
  virtual void create_table_fk(db_mysql_ForeignKeyRef) = 0;
  virtual void create_table_fks_end(db_mysql_TableRef) = 0;

  virtual void create_table_engine(grt::StringRef) = 0;
  virtual void create_table_next_auto_inc(grt::StringRef) = 0;
  virtual void create_table_password(grt::StringRef) = 0;
  virtual void create_table_delay_key_write(grt::IntegerRef) = 0;
  virtual void create_table_charset(grt::StringRef) = 0;
  virtual void create_table_collate(grt::StringRef) = 0;
  virtual void alter_table_comment(db_mysql_TableRef, grt::StringRef) = 0;
  virtual void create_table_merge_union(grt::StringRef) = 0;
  virtual void create_table_merge_insert(grt::StringRef) = 0;
  virtual void create_table_pack_keys(grt::StringRef) = 0;
  virtual void create_table_checksum(grt::IntegerRef) = 0;
  virtual void create_table_row_format(grt::StringRef) = 0;
  virtual void create_table_key_block_size(grt::StringRef) = 0;
  virtual void create_table_avg_row_length(grt::StringRef) = 0;
  virtual void create_table_min_rows(grt::StringRef) = 0;
  virtual void create_table_max_rows(grt::StringRef) = 0;
  virtual void create_table_comment(grt::StringRef) = 0;
  virtual void create_table_data_dir(grt::StringRef) = 0;
  virtual void create_table_index_dir(grt::StringRef) = 0;

  // drop table
  virtual void drop_table(db_mysql_TableRef) = 0;

  // alter table
  virtual void alter_table_props_begin(db_mysql_TableRef) = 0;
  virtual void alter_table_name(db_mysql_TableRef, grt::StringRef) = 0;
  virtual void alter_table_engine(db_mysql_TableRef, grt::StringRef) = 0;
  virtual void alter_table_next_auto_inc(db_mysql_TableRef, grt::StringRef) = 0;
  virtual void alter_table_password(db_mysql_TableRef, grt::StringRef) = 0;
  virtual void alter_table_delay_key_write(db_mysql_TableRef, grt::IntegerRef) = 0;
  virtual void alter_table_charset(db_mysql_TableRef, grt::StringRef) = 0;
  virtual void alter_table_collate(db_mysql_TableRef, grt::StringRef) = 0;
  virtual void alter_table_merge_union(db_mysql_TableRef, grt::StringRef) = 0;
  virtual void alter_table_merge_insert(db_mysql_TableRef, grt::StringRef) = 0;
  virtual void alter_table_pack_keys(db_mysql_TableRef, grt::StringRef) = 0;
  virtual void alter_table_checksum(db_mysql_TableRef, grt::IntegerRef) = 0;
  virtual void alter_table_row_format(db_mysql_TableRef, grt::StringRef) = 0;
  virtual void alter_table_key_block_size(db_mysql_TableRef, grt::StringRef) = 0;
  virtual void alter_table_avg_row_length(db_mysql_TableRef, grt::StringRef) = 0;
  virtual void alter_table_min_rows(db_mysql_TableRef, grt::StringRef) = 0;
  virtual void alter_table_max_rows(db_mysql_TableRef, grt::StringRef) = 0;
  virtual void alter_table_connection_string(db_mysql_TableRef, grt::StringRef) = 0;

  virtual void alter_table_generate_partitioning(db_mysql_TableRef table, const std::string& part_type,
                                                 const std::string& part_expr, int part_count,
                                                 const std::string& subpart_type, const std::string& subpart_expr,
                                                 grt::ListRef<db_mysql_PartitionDefinition> part_defs) = 0;
  virtual void alter_table_drop_partitioning(db_mysql_TableRef table) = 0;
  virtual void alter_table_add_partition(db_mysql_PartitionDefinitionRef part, bool is_range) = 0;
  virtual void alter_table_drop_partition(const std::string& part_name) = 0;
  virtual void alter_table_reorganize_partition(db_mysql_PartitionDefinitionRef old_part,
                                                db_mysql_PartitionDefinitionRef new_part, bool is_range) = 0;
  virtual void alter_table_partition_count(db_mysql_TableRef, grt::IntegerRef) = 0;
  virtual void alter_table_partition_definitions(db_mysql_TableRef, grt::StringRef) = 0;
  virtual void alter_table_props_end(db_mysql_TableRef) = 0;

  virtual void alter_table_columns_begin(db_mysql_TableRef) = 0;
  virtual void alter_table_add_column(db_mysql_TableRef, std::map<std::string, std::string>, db_mysql_ColumnRef column,
                                      db_mysql_ColumnRef after) = 0;
  virtual void alter_table_drop_column(db_mysql_TableRef, db_mysql_ColumnRef) = 0;
  virtual void alter_table_change_column(db_mysql_TableRef table, db_mysql_ColumnRef org_col,
                                         db_mysql_ColumnRef mod_col, db_mysql_ColumnRef after, bool modified,
                                         std::map<std::string, std::string> column_rename_map) = 0;
  virtual void alter_table_columns_end(db_mysql_TableRef) = 0;

  virtual void alter_table_indexes_begin(db_mysql_TableRef) = 0;
  virtual void alter_table_add_index(db_mysql_IndexRef) = 0;
  virtual void alter_table_drop_index(db_mysql_IndexRef) = 0;
  virtual void alter_table_change_index(db_mysql_IndexRef orgIndex, db_mysql_IndexRef newIndex) = 0;
  virtual void alter_table_indexes_end(db_mysql_TableRef) = 0;

  virtual void alter_table_fks_begin(db_mysql_TableRef) = 0;
  virtual void alter_table_add_fk(db_mysql_ForeignKeyRef) = 0;
  virtual void alter_table_drop_fk(db_mysql_ForeignKeyRef) = 0;
  virtual void alter_table_fks_end(db_mysql_TableRef) = 0;

  // triggers create/drop
  virtual void create_trigger(db_mysql_TriggerRef, bool for_alter) = 0;
  virtual void drop_trigger(db_mysql_TriggerRef, bool for_alter) = 0;

  // views create/drop
  virtual void create_view(db_mysql_ViewRef) = 0;
  virtual void drop_view(db_mysql_ViewRef) = 0;

  // routines create/drop
  virtual void create_routine(db_mysql_RoutineRef, bool for_alter) = 0;
  virtual void drop_routine(db_mysql_RoutineRef, bool for_alter) = 0;

  // users create/drop
  virtual void create_user(db_UserRef) = 0;
  virtual void drop_user(db_UserRef) = 0;

  // schema create/drop
  virtual void create_schema(db_mysql_SchemaRef) = 0;
  virtual void drop_schema(db_mysql_SchemaRef) = 0;

  // alter schema
  virtual void alter_schema_props_begin(db_mysql_SchemaRef) = 0;
  virtual void alter_schema_name(db_mysql_SchemaRef, grt::StringRef value) = 0;
  virtual void alter_schema_default_charset(db_mysql_SchemaRef, grt::StringRef value) = 0;
  virtual void alter_schema_default_collate(db_mysql_SchemaRef, grt::StringRef value) = 0;
  virtual void alter_schema_props_end(db_mysql_SchemaRef) = 0;
  virtual void disable_list_insert(const bool flag) = 0;
};

#define DOC_DbMySQLImpl                                          \
  "MySQL specific SQL generation and synchronization support.\n" \
  "Also contains other functions to retrieve MySQL specific parameters."

/**
 * class DbMySQLImpl is the main class responsible for SQL export/sync functionality. DbMySQLImpl runs
 * DiffSQLGeneratorBE to generate SQL for individual objects and then gathers it into a script.
 */
class MYSQLMODULEDBMYSQL_PUBLIC_FUNC DbMySQLImpl : public SQLGeneratorInterfaceImpl, public grt::ModuleImplBase {
public:
  DbMySQLImpl(grt::CPPModuleLoader* ldr);

  DEFINE_INIT_MODULE_DOC(
    DbMySQL_VERSION, "Oracle", DOC_DbMySQLImpl, grt::ModuleImplBase,
    DECLARE_MODULE_FUNCTION(DbMySQLImpl::getTargetDBMSName),
    DECLARE_MODULE_FUNCTION(DbMySQLImpl::initializeDBMSInfo),
    DECLARE_MODULE_FUNCTION(DbMySQLImpl::quoteIdentifier),
    DECLARE_MODULE_FUNCTION(DbMySQLImpl::fullyQualifiedObjectName),
    DECLARE_MODULE_FUNCTION(DbMySQLImpl::generateSQLForDifferences),
    DECLARE_MODULE_FUNCTION(DbMySQLImpl::generateReportForDifferences),
    DECLARE_MODULE_FUNCTION(DbMySQLImpl::makeSQLExportScript),
    DECLARE_MODULE_FUNCTION(DbMySQLImpl::makeSQLSyncScript),
    DECLARE_MODULE_FUNCTION(DbMySQLImpl::getTraitsForServerVersion),
    DECLARE_MODULE_FUNCTION_DOC(DbMySQLImpl::makeCreateScriptForObject, "Generates a CREATE script for the object.",
                                "object the object to be processed (Table, View, Routine etc)"),
    DECLARE_MODULE_FUNCTION(DbMySQLImpl::makeAlterScriptForObject),
    DECLARE_MODULE_FUNCTION(DbMySQLImpl::makeAlterScript),
    DECLARE_MODULE_FUNCTION(DbMySQLImpl::getKnownEngines),
    DECLARE_MODULE_FUNCTION(DbMySQLImpl::getDefaultUserDatatypes),
    DECLARE_MODULE_FUNCTION(DbMySQLImpl::getDefaultColumnValueMappings));

  virtual std::string getTargetDBMSName() override {
    return "Mysql";
  }

  virtual db_mgmt_RdbmsRef initializeDBMSInfo();

  grt::StringRef quoteIdentifier(grt::StringRef ident);

  grt::StringRef fullyQualifiedObjectName(GrtNamedObjectRef object);

  virtual grt::DictRef generateSQLForDifferences(GrtNamedObjectRef srcobj, GrtNamedObjectRef dstobj,
                                                 grt::DictRef options) override;

  /**
   * generate report (create or alter)
   */
  virtual grt::StringRef generateReportForDifferences(GrtNamedObjectRef org_object, GrtNamedObjectRef dst_object,
                                                      const grt::DictRef& options) override;

  /**
   * generate sql (create or alter) internal only
   */
  virtual ssize_t generateSQL(GrtNamedObjectRef, const grt::DictRef& options,
                              std::shared_ptr<grt::DiffChange>) override;

  /**
   * generate report (create or alter) internal only
   */
  virtual grt::StringRef generateReport(GrtNamedObjectRef org_object, const grt::DictRef& options,
                                        std::shared_ptr<grt::DiffChange>) override;

  /**
   * generate SQL export script (CREATE statements only)
   */
  virtual ssize_t makeSQLExportScript(GrtNamedObjectRef, grt::DictRef options, const grt::DictRef& createSQL,
                                      const grt::DictRef& dropSQL) override;

  /**
   * generate CREATE SQL script for an individual object
   */
  virtual std::string makeCreateScriptForObject(GrtNamedObjectRef object) override;

  /**
   * generate ALTER SQL script for an individual object
   */
  virtual std::string makeAlterScriptForObject(GrtNamedObjectRef source, GrtNamedObjectRef target,
                                               GrtNamedObjectRef obj, const grt::DictRef& diff_options);

  /**
   * generate ALTER SQL script for an object and all
   */
  virtual std::string makeAlterScript(GrtNamedObjectRef source, GrtNamedObjectRef target,
                                      const grt::DictRef& diff_options);

  /**
   * generate SQL alter script
   */
  virtual ssize_t makeSQLSyncScript(db_CatalogRef cat, grt::DictRef options, const grt::StringListRef& sql_list,
                                    const grt::ListRef<GrtNamedObject>& param2) override;

  grt::ListRef<db_mysql_StorageEngine> getKnownEngines();

  virtual grt::DictRef getTraitsForServerVersion(const int major, const int minor, const int revision) override;

  grt::ListRef<db_UserDatatype> getDefaultUserDatatypes(db_mgmt_RdbmsRef rdbms);

  grt::DictRef getDefaultColumnValueMappings() {
    return grt::DictRef(true);
  }

  virtual grt::DictRef getDefaultTraits() const override {
    return _default_traits;
  };

private:
  grt::ListRef<db_mysql_StorageEngine> _known_engines;
  grt::DictRef _default_traits;
};

//!
//!     @}
//!
