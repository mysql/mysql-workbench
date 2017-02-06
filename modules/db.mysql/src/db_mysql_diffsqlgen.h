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
*
* The entities marked as "to be removed" can be probably safely removed,
* unless I did something wrong. Their implmenetations were moved to the generator call-back.
* I think implementations for at least some of them are already removed, (compiler doesn't
* complain as they are not used anywhere).
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

  // to be removed
  std::string generate_create(db_mysql_ColumnRef column);
  // to be removed
  std::string generate_create(db_mysql_IndexRef index, std::string table_q_name, bool separate_index);
  // to be removed
  std::string generate_create(db_mysql_ForeignKeyRef fk);

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
  // to be removed
  std::string generate_add_column(db_mysql_TableRef table, db_mysql_ColumnRef column, db_mysql_ColumnRef after,
                                  Column_rename_map);
  // to be removed
  std::string generate_drop_column(db_mysql_ColumnRef column);
  // to be removed
  std::string generate_change_column(db_mysql_TableRef table, db_mysql_ColumnRef org_col, db_mysql_ColumnRef mod_col,
                                     db_mysql_ColumnRef after, bool modified, Column_rename_map);
  void generate_alter(grt::ListRef<db_mysql_Column> columns, const grt::MultiChange *);

  // table indices
  // to be removed
  std::string generate_add_index(db_mysql_IndexRef index);
  // to be removed
  std::string generate_drop_index(db_mysql_IndexRef index);
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

  // to be removed
  std::string generate_drop_partitioning(db_mysql_Table);
  // to be removed
  std::string generate_partitioning(db_mysql_TableRef table, const std::string &part_type, const std::string &part_expr,
                                    int part_count, const std::string &subpart_type, const std::string &subpart_expr,
                                    grt::ListRef<db_mysql_PartitionDefinition> part_defs);
  // to be removed
  std::string generate_change_partition_count(db_mysql_TableRef table, int newcount);
  // to be removed
  std::string generate_add_partition(db_mysql_PartitionDefinitionRef part, bool is_range);
  // to be removed
  std::string generate_drop_partitions(const std::list<std::string> &part_names_list);
  // to be removed
  std::string generate_reorganize_partition(db_mysql_PartitionDefinitionRef old_part,
                                            db_mysql_PartitionDefinitionRef new_part, bool is_range);

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

  /**
   * The 2 routines below - remember() and remember_alter() are used to store the gerneated SQL.
   * remember() just adds SQL strings to a list and remember_alter() can store to both a list or a map
   * so that later one can access object's SQL by object GRT value. remember_alter() also allows storing
   * several SQL strings per object, which is needed in some cases.
   *
   * Historically the remember() function appeared earlier than remember_alter() and the later one is
   * more general so theoretially remember() can be removed at all, and all code refactored to use remember_alter()
   * only.
   */
  void remember(const GrtNamedObjectRef &obj, const std::string &sql);
  /**
   * in case of ALTERs there could be > 1 statement to remember
   * so we use grt::StringListRefs as needed
   */
  void remember_alter(const GrtNamedObjectRef &obj, const std::string &sql);

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
