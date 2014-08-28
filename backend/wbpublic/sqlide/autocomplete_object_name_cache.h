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

#ifndef _AUTOCOMPLETE_OBJECT_NAME_CACHE_H_
#define _AUTOCOMPLETE_OBJECT_NAME_CACHE_H_

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

  std::vector<std::string> get_matching_schema_names(const std::string &prefix = "");
  std::vector<std::string> get_matching_table_names(const std::string &schema = "", const std::string &prefix = "");
  std::vector<std::string> get_matching_column_names(const std::string &schema, const std::string &table, const std::string &prefix = "");
  std::vector<std::string> get_matching_procedure_names(const std::string &schema = "", const std::string &prefix = "");
  std::vector<std::string> get_matching_function_names(const std::string &schema = "", const std::string &prefix = "");

  bool refresh_schema_cache_if_needed(const std::string &schema);
  void refresh_schema_list_cache_if_needed();
  void refresh_schema_cache(const std::string &schema);
  void refresh_table_cache(const std::string &schema, const std::string &table);
  
  void update_schemas(const std::vector<std::string> &schemas);
  void update_schema_tables(const std::string &schema,
                            const std::vector<std::pair<std::string, bool> > &tables, bool just_append); 
  void update_schema_routines(const std::string &schema,
                              const std::vector<std::pair<std::string, bool> > &routines, bool just_append);
  void update_table_columns(const std::string &schema, const std::string &table,
                            const std::vector<std::string> &columns);
  
  bool is_schema_list_fetch_done();
  bool is_schema_tables_fetch_done(const std::string &schema);
  bool is_schema_table_columns_fetch_done(const std::string &schema, const std::string &table);
  bool is_schema_routines_fetch_done(const std::string &schema);

  void shutdown();
  
  ~AutoCompleteCache();
private:
  void init_db();

  static void *_refresh_cache_thread(void *);
  void refresh_cache_thread();
  void refresh_schemas_w();
  void refresh_tables_w(const std::string &schema);
  void refresh_columns_w(const std::string &schema, const std::string &table);
  void refresh_routines_w(const std::string &schema);

  void touch_schema_record(const std::string &schema);

  bool get_pending_refresh(std::string &task);
  void add_pending_refresh(const std::string& task);
  void create_worker_thread();
  
  base::RecMutex _sqconn_mutex;
  sqlite::connection *_sqconn;

  GThread *_refresh_thread;
  base::Semaphore _cache_working;

  base::RecMutex _shutdown_mutex;

  base::RecMutex _pending_mutex;

  std::list<std::string> _pending_refresh_schema;

  std::string _connection_id;
  boost::function<base::RecMutexLock (sql::Dbc_connection_handler::Ref &)> _get_connection;
  boost::function<void (bool)> _feedback;

  bool _schema_list_fetched;
  bool _shutdown;
};

#endif
