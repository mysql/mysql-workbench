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

//!
//!    \addtogroup Diff Diff SQL Generator
//!             Diff SQL Generator
//!
//!    @{
//!
#ifndef _DB_MYSQL_DIFFSQLGEN_H_
#define _DB_MYSQL_DIFFSQLGEN_H_

/**
 * @file db_mysql_diffsqlgen.h
 *
 * This file contains declarations for SQL DDL generation routines (CREATE, DROP, ALTER) from GRT objects.
 *
 * The SQL script generation process consists of 2 steps:
 * <br/>1. generate SQL for individual objects
 * <br/>2. put the gereneated SQL together (e.g. into a string or  a file)
 *
 * The DiffSQLGeneratorBE class together with the code generation call-back implement the first step.
 * You pass the catalog object, the call-back object, the difference object, and output container (list
 * or map) to DiffSQLGeneratorBE::process_diff_change() (see below) and get back  the output container
 * filled with SQL code for objects in catalog. Filtering is available and is set through the options map
 * (also passed to the function).
 *
 * The call-back object serves to generate the SQL ot plain text. See the DiffSQLGeneratorBEActionInterface
 * interface declaraton in module_db_mysql.h.
 */

#include "grtpp_module_cpp.h"
#include "grts/structs.db.mysql.h"

#include <set>

namespace grt {
  class DiffChange;
  class MultiChange;
};

class DiffSQLGeneratorBEActionInterface;

// copied from module_db_mysql.h
typedef std::map<std::string, std::string> SchemaObjectNameDDLMap;

/**
* class DiffSQLGeneratorBE together with text generation call-back (see DiffSQLGeneratorBEActionInterface)
* generates SQL (or plain text) for individual objects based on GRT objects and diff information.
*/
class DiffSQLGeneratorBE {
  /**
   * Column_rename_map is used to keep information about old vs new column names
   */
  typedef std::map<std::string, std::string> Column_rename_map;

  /**
   * callback class instance used for SQL/text generation
   */
  DiffSQLGeneratorBEActionInterface *callback;

  /**
   * the SQL can be stored either to a list or to a map
   */
  grt::DictRef target_map;
  grt::StringListRef target_list;
  grt::ListRef<GrtNamedObject> target_object_list;

  /**
   * processing options
   */
  bool _gen_create_index;
  bool _use_filtered_lists;
  bool _skip_foreign_keys;
  bool _skip_fk_indexes;
  bool _case_sensitive;
  bool _use_oid_as_dict_key;
  bool _separate_foreign_keys;
  std::set<std::string> _filtered_schemata, _filtered_tables, _filtered_views, _filtered_routines, _filtered_triggers,
    _filtered_users;

  /**
   * generate_create_stmt() functions are used to generate create SQL stmt based on GRT object definitions.
   * They don't use a diff object. The for_alter parameter controls whether thr result will be stored into
   * target list or map.
   */
  void generate_create_stmt(db_mysql_TableRef);
  void generate_create_stmt(db_mysql_TriggerRef, bool for_alter = false);
  void generate_create_stmt(db_mysql_ViewRef);
  void generate_create_stmt(db_mysql_RoutineRef, bool for_alter = false);
  void generate_create_stmt(db_mysql_SchemaRef);
  void generate_create_stmt(db_mysql_CatalogRef);
  void generate_create_stmt(db_UserRef user);

  /**
   * generate_drop_stmt() functions are used to generate drop SQL stmt based on GRT object definitions.
   * The for_alter parameter controls whether thr result will be stored into target list or map.
   */
  void generate_drop_stmt(db_mysql_TableRef);
  void generate_drop_stmt(db_mysql_TriggerRef, bool for_alter = false);
  void generate_drop_stmt(db_mysql_ViewRef);
  void generate_drop_stmt(db_mysql_RoutineRef, bool for_alter = false);
  void generate_drop_stmt(db_mysql_SchemaRef);
  void generate_drop_stmt(db_mysql_CatalogRef);
  void generate_drop_stmt(db_UserRef user);

  // table columns
  void generate_alter(grt::ListRef<db_mysql_Column> columns, const grt::MultiChange *);

  // table indices
  void generate_alter(grt::ListRef<db_mysql_Index> indices, const grt::MultiChange *diffchange);

  /**
   * Create/remove table FKs
   */
  std::string generate_add_fk(db_mysql_ForeignKeyRef fk);
  std::string generate_drop_fk(db_mysql_ForeignKeyRef fk);
  void generate_alter(grt::ListRef<db_mysql_ForeignKey> fks, const grt::MultiChange *diffchange);
  void generate_alter_drop(grt::ListRef<db_mysql_ForeignKey> fks, const grt::MultiChange *diffchange);

  /**
   * Parse SQL partitioning options, pass them to the generator call-back. This function is used
   * for CREATE TABLE
   *
   * @param table the table object
   */
  void generate_create_partitioning(db_mysql_TableRef table);

  /**
   * Parse SQL partitioning options, pass them to the generator call-back. This function is used
   * for ALTER TABLE
   *
   * @param table the original (unchanged) table object
   * @param table_diffchange the change as generated by diff code
   */
  void generate_set_partitioning(db_mysql_TableRef table, const grt::DiffChange *diffchange);

  /**
   * generate_alter_stmt() functions are used to generate create SQL stmt based on GRT object definition.
   * They don't use a diff object. The for_alter parameter controls whether thr result will be stored into
   * list or map.
   */
  void generate_alter_stmt(db_mysql_CatalogRef, const grt::DiffChange *);
  void generate_alter_stmt(db_mysql_SchemaRef, const grt::DiffChange *);

  enum AlterTableFlags {
    EverythingButForeignKeys = 1,
    OnlyForeignKeys = 2,
    Everything = 3,
  };
  void generate_alter_stmt(db_mysql_TableRef, const grt::DiffChange *, AlterTableFlags alter_table_flags);
  // some drops needs to go prior to the rest of alters, e.g. drop FKs to allow column rename in other tables
  void generate_alter_stmt_drops(db_mysql_TableRef, const grt::DiffChange *);
  void generate_alter_stmt(db_mysql_ViewRef old_view, db_mysql_ViewRef new_view, const grt::DiffChange *diffchange);
  void generate_routine_alter_stmt(db_mysql_RoutineRef old_routine, db_mysql_RoutineRef new_routine,
                                   const grt::DiffChange *diffchange);

  void process_trigger_alter_stmts(db_mysql_TableRef table, const grt::DiffChange *triggers_cs);

  void do_process_diff_change(grt::ValueRef org_object, grt::DiffChange *);

public:
  /**
   * DiffSQLGeneratorBE c-tor
   *
   * @param options key-value paired options,parsed in c-tor
   * @param cb callback object instance
   */
  DiffSQLGeneratorBE(grt::DictRef options, grt::DictRef dbtraits, DiffSQLGeneratorBEActionInterface *cb);

  /**
   * function that starts generating SQL into a map (dictionary)
   */
  void process_diff_change(grt::ValueRef org_object, grt::DiffChange *, grt::DictRef);
  /**
   * function that starts generating SQL into a list
   */
  void process_diff_change(grt::ValueRef org_object, grt::DiffChange *, grt::StringListRef,
                           grt::ListRef<GrtNamedObject>);
};

#endif // _DB_MYSQL_DIFFSQLGEN_H_
//!
//!     @}
//!
