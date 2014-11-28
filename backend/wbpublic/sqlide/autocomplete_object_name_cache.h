/* 
 * Copyright (c) 2013, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "wbpublic_public_interface.h"
#include <sqlite/connection.hpp>
#include "grts/structs.db.mgmt.h"
#include "base/threading.h"
#include "cppdbc.h"

class WBPUBLICBACKEND_PUBLIC_FUNC AutoCompleteCache
{
public:
  // Note: feedback can be called from the worker thread. Make the necessary arrangements.
  //       It comes with parameter true if the cache update is going on, otherwise false.
  AutoCompleteCache(const std::string &connection_id,
                    boost::function<base::RecMutexLock (sql::Dbc_connection_handler::Ref &)> get_connection,
                    const std::string &cache_dir,
                    boost::function<void (bool)> feedback);

  // Data retrieval functions.
  std::vector<std::string> get_matching_schema_names(const std::string &prefix = "");
  std::vector<std::string> get_matching_table_names(const std::string &schema = "",
                                                    const std::string &prefix = "");
  std::vector<std::string> get_matching_view_names(const std::string &schema = "",
                                                   const std::string &prefix = "");
  std::vector<std::string> get_matching_column_names(const std::string &schema,
                                                     const std::string &table,
                                                     const std::string &prefix = "");
  std::vector<std::string> get_matching_procedure_names(const std::string &schema = "",
                                                        const std::string &prefix = "");
  std::vector<std::string> get_matching_function_names(const std::string &schema = "",
                                                       const std::string &prefix = "");
  std::vector<std::string> get_matching_trigger_names(const std::string &schema = "",
                                                      const std::string &table = "",
                                                      const std::string &prefix = "");

  std::vector<std::string> get_matching_udf_names(const std::string &prefix = "");
  std::vector<std::string> get_matching_variables(const std::string &prefix = ""); // System vars only.
  std::vector<std::string> get_matching_engines(const std::string &prefix = "");
  
  std::vector<std::string> get_matching_logfile_groups(const std::string &prefix = ""); // Only useful for NDB cluster.
  std::vector<std::string> get_matching_tablespaces(const std::string &prefix = ""); // Only useful for NDB cluster.

  // Data refresh functions. To be called from outside when data objects are created or destroyed.
  void refresh_schema_list();
  bool refresh_schema_cache_if_needed(const std::string &schema);
  void refresh_tables(const std::string &schema);
  void refresh_views(const std::string &schema);
  void refresh_columns(const std::string &schema, const std::string &table);
  void refresh_triggers(const std::string &schema, const std::string &table);
  void refresh_udfs();
  void refresh_tablespaces();    // Logfile groups and tablespaces are unqualified,
  void refresh_logfile_groups(); // event though they belong to a specific table.

  // Update functions that can also be called from outside.
  void update_schemas(const std::vector<std::string> &schemas);
  void update_tables(const std::string &schema, const std::list<std::string> &tables);
  void update_views(const std::string &schema, const std::list<std::string> &tables);
  void update_procedures(const std::string &schema, const std::list<std::string> &tables);
  void update_functions(const std::string &schema, const std::list<std::string> &tables);

  // Status functions.
  bool is_schema_list_fetch_done();
  bool is_schema_tables_fetch_done(const std::string &schema);
  bool is_schema_table_columns_fetch_done(const std::string &schema, const std::string &table);
  bool is_schema_functions_fetch_done(const std::string &schema);
  bool is_schema_procedure_fetch_done(const std::string &schema);

  void shutdown();
  
  ~AutoCompleteCache();
private:
  struct RefreshTask {
    enum RefreshType {
      RefreshSchemas,
      RefreshTables,
      RefreshViews,
      RefreshProcedures,
      RefreshFunctions,
      RefreshColumns,
      RefreshTriggers,
      RefreshUDFs,
      RefreshVariables,
      RefreshEngines,
      RefreshLogfileGroups,
      RefreshTableSpaces,
    } type;
    std::string schema_name;
    std::string table_name;

    RefreshTask()
    {
      type = RefreshSchemas;
    }
    
    RefreshTask(RefreshType type_, const std::string &schema, const std::string &table)
    {
      type = type_;
      schema_name = schema;
      table_name = table;
    }
  };

  void init_db();

  static void *_refresh_cache_thread(void *);
  void refresh_cache_thread();
  void refresh_schemas_w();
  void refresh_tables_w(const std::string &schema);
  void refresh_views_w(const std::string &schema);
  void refresh_functions_w(const std::string &schema);
  void refresh_procedures_w(const std::string &schema);
  void refresh_columns_w(const std::string &schema, const std::string &table);
  void refresh_triggers_w(const std::string &schema, const std::string &table);

  void refresh_udfs_w();
  void refresh_variables_w();
  void refresh_engines_w();
  void refresh_logfile_groups_w();
  void refresh_tablespaces_w();

  void update_object_names(const std::string &cache, const std::vector<std::string> &objects);
  void update_object_names(const std::string &cache,
                           const std::string &schema,
                           const std::list<std::string> &objects);
  void update_object_names(const std::string &cache,
                           const std::string &schema,
                           const std::string &table,
                           const std::vector<std::string> &objects);

  std::vector<std::string> get_matching_objects(const std::string &cache, const std::string &prefix);
  std::vector<std::string> get_matching_objects(const std::string &cache,
                                                const std::string &schema,
                                                const std::string &prefix);
  std::vector<std::string> get_matching_objects(const std::string &cache,
                                                const std::string &schema,
                                                const std::string &table,
                                                const std::string &prefix);
  void touch_schema_record(const std::string &schema);
  bool is_fetch_done(const std::string &cache, const std::string &schema);

  bool get_pending_refresh(RefreshTask &task);
  void add_pending_refresh(RefreshTask::RefreshType type, const std::string &schema = "",
                           const std::string &table = "");
  void create_worker_thread();
  
  base::RecMutex _sqconn_mutex;
  sqlite::connection *_sqconn;

  GThread *_refresh_thread;
  base::Semaphore _cache_working;

  base::RecMutex _shutdown_mutex;

  base::RecMutex _pending_mutex;

  std::list<RefreshTask> _pending_tasks;

  std::string _connection_id;
  boost::function<base::RecMutexLock (sql::Dbc_connection_handler::Ref &)> _get_connection;
  boost::function<void (bool)> _feedback;

  bool _shutdown;
};
