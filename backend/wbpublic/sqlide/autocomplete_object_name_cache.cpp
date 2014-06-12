/* 
 * Copyright (c) 2012, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include <sqlite/execute.hpp>
#include <sqlite/query.hpp>
#include <sqlite/database_exception.hpp>
#include <glib.h>

#include "autocomplete_object_name_cache.h"
#include "base/string_utilities.h"
#include "base/log.h"
#include "base/file_utilities.h"
#include "base/sqlstring.h"
#include "grt/common.h"
#include "sqlide_generics.h"

using namespace bec;

DEFAULT_LOG_DOMAIN("AutoCCache");

// What can trigger a hard refresh (fetch data from server) of the cache:
// - first time a soft refresh is requested in the session (indirectly from the get_matching_* methods)
// - explicit hard refresh from frontend (ie, refresh schema tree)

//                                                              
//                                                                      
// Editor   -->   AutoCompletionEngine   --(object names)-->   AutoCompleteCache  --> server
//                                                                                --> cache  <-- LiveSchemaTree 
//

//--------------------------------------------------------------------------------------------------

AutoCompleteCache::AutoCompleteCache(const std::string &connection_id,
  boost::function<base::RecMutexLock (sql::Dbc_connection_handler::Ref &)> get_connection,
  const std::string &cache_dir, boost::function<void (bool)> feedback)
  : _refresh_thread(0), _cache_working(1), _connection_id(connection_id), _get_connection(get_connection), _schema_list_fetched(false), _shutdown(false)
{
  _feedback = feedback;
  _sqconn = new sqlite::connection(make_path(cache_dir, _connection_id)+".cache");
  sqlite::execute(*_sqconn, "PRAGMA temp_store=MEMORY", true);
  sqlite::execute(*_sqconn, "PRAGMA synchronous=NORMAL", true);

  log_debug2("Using autocompletion cache file %s\n", (make_path(cache_dir, _connection_id)+".cache").c_str());

  // check if the DB is already initialized
  sqlite::query q(*_sqconn, "select name from sqlite_master where type='table'");
  int found = 0;
  if (q.emit())
  {
    boost::shared_ptr<sqlite::result> res(q.get_result());
    do
    {
      std::string name = res->get_string(0);
      if (name == "tables" || name == "schemas" || name == "routines" || name == "columns" || name == "meta")
        found++;
    }
    while (res->next_row());
  }
  if (found == 0)
  {
    log_debug3("Initializing cache\n");
    init_db();
  }
  else if (found != 5)
  {
    log_warning("Unexpected number of tables found in cache (%i). Recreating the cache...\n", found);
    delete _sqconn;
    base::remove(make_path(cache_dir, _connection_id)+".cache");
    _sqconn = new sqlite::connection(make_path(cache_dir, _connection_id)+".cache");
    sqlite::execute(*_sqconn, "PRAGMA temp_store=MEMORY", true);
    sqlite::execute(*_sqconn, "PRAGMA synchronous=NORMAL", true);
    init_db();
  }
}

//--------------------------------------------------------------------------------------------------

void AutoCompleteCache::shutdown()
{
  base::RecMutexLock sd_lock(_shutdown_mutex);
  _shutdown = true;

  {
    base::RecMutexLock lock(_pending_mutex);
    _pending_refresh_schema.clear();
    _feedback = NULL;
  }

  if (_refresh_thread)
  {
    log_debug2("Waiting for worker thread to finish...\n");
    g_thread_join(_refresh_thread);
    _refresh_thread = NULL;
    log_debug2("Worker thread finished.\n");
  }
}

//--------------------------------------------------------------------------------------------------

AutoCompleteCache::~AutoCompleteCache()
{
  g_assert(_shutdown);

  delete _sqconn;
}

//--------------------------------------------------------------------------------------------------

std::vector<std::string>  AutoCompleteCache::get_matching_schema_names(const std::string &prefix)
{
  // schedule a fetch in case its needed
  refresh_schema_list_cache_if_needed();

  if (!_shutdown)
  {
    // Ensures shutdown is not done while processing
    base::RecMutexLock sd_lock(_shutdown_mutex);
    base::RecMutexLock lock(_sqconn_mutex);
    sqlite::query q(*_sqconn, "SELECT name FROM schemas WHERE name LIKE ? ESCAPE '\\'");
    q.bind(1, base::escape_sql_string(prefix, true) + "%");
    if (q.emit())
    {
      std::vector<std::string> items;
      boost::shared_ptr<sqlite::result> matches(q.get_result());
      bool fetch_ongoing = false;
      do
      {
        std::string name = matches->get_string(0);
        if (name.empty()) // entry with empty name means a fetch is underway
        {
          fetch_ongoing = true;
          break;
        }
        items.push_back(name);
      }
      while (matches->next_row());
      if (!fetch_ongoing)
        return items;
      log_debug3("get_matching_schema_names(%s) returning empty list because fetch is still ongoing", prefix.c_str());
    }
  }
  return std::vector<std::string>();
}


std::vector<std::string> AutoCompleteCache::get_matching_table_names(const std::string &schema, const std::string &prefix)
{
  // schedule a fetch in case its needed
  refresh_schema_cache_if_needed(schema);

  if (!_shutdown)
  {
    // Ensures shutdown is not done while processing
    base::RecMutexLock sd_lock(_shutdown_mutex);
    base::RecMutexLock lock(_sqconn_mutex);
    sqlite::query q(*_sqconn, "SELECT name FROM tables WHERE schema LIKE ? ESCAPE '\\' AND name LIKE ? ESCAPE '\\'");
    q.bind(1, schema.size() == 0 ? "%" : base::escape_sql_string(schema, true));
    q.bind(2, base::escape_sql_string(prefix, true) + "%");
    if (q.emit())
    {
      std::vector<std::string> items;
      boost::shared_ptr<sqlite::result> matches(q.get_result());
      bool fetch_ongoing = false;
      do
      {
        std::string name = matches->get_string(0);
        if (name.empty()) // entry with empty name means a fetch is underway
        {
          fetch_ongoing = true;
          break;
        }
        items.push_back(name);
      }
      while (matches->next_row());
      if (!fetch_ongoing)
        return items;
    }
  }
  // this never got refreshed, but a fetch request must be ongoing.. we can wait and block or return an empty list
  // if (_block_on_fetches)
  return std::vector<std::string>();
}

std::vector<std::string> AutoCompleteCache::get_matching_column_names(const std::string &schema, const std::string &table, const std::string &prefix)
{
  // schedule a fetch in case its needed
  refresh_schema_cache_if_needed(schema);
  
  if (!_shutdown)
  {
    // Ensures shutdown is not done while processing
    base::RecMutexLock sd_lock(_shutdown_mutex);
    base::RecMutexLock lock(_sqconn_mutex);
    sqlite::query q(*_sqconn, "SELECT name FROM columns WHERE schema LIKE ? ESCAPE '\\' "
      "AND tabl LIKE ? ESCAPE '\\' AND name LIKE ? ESCAPE '\\'");
    q.bind(1, schema.size() == 0 ? "%" : base::escape_sql_string(schema, true));
    q.bind(2, table.size() == 0 ? "%" : base::escape_sql_string(table, true));
    q.bind(3, base::escape_sql_string(prefix, true) + "%");
  
    if (q.emit())
    {
      std::vector<std::string> items;
      boost::shared_ptr<sqlite::result> matches(q.get_result());
      bool fetch_ongoing = false;
      do
      {
        std::string name = matches->get_string(0);
        if (name.empty()) // entry with empty name means a fetch is underway
        {
          fetch_ongoing = true;
          break;
        }
        items.push_back(name);
      }
      while (matches->next_row());
      if (!fetch_ongoing)
        return items;
    }
  }
  
  // this never got refreshed, but a fetch request must be ongoing.. we can wait and block or return an empty list
  // if (_block_on_fetches)
  return std::vector<std::string>();  
}

std::vector<std::string> AutoCompleteCache::get_matching_procedure_names(const std::string &schema, const std::string &prefix)
{
  // schedule a fetch in case its needed
  refresh_schema_cache_if_needed(schema);
  
  if (!_shutdown)
  {
    // Ensures shutdown is not done while processing
    base::RecMutexLock sd_lock(_shutdown_mutex);
    base::RecMutexLock lock(_sqconn_mutex);
    sqlite::query q(*_sqconn, "SELECT name FROM routines WHERE schema LIKE ? ESCAPE '\\' "
      "AND name LIKE ? ESCAPE '\\' AND is_function=0");
    q.bind(1, schema.size() == 0 ? "%" : base::escape_sql_string(schema, true));
    q.bind(2, base::escape_sql_string(prefix, true) + "%");
    if (q.emit())
    {
      std::vector<std::string> items;
      boost::shared_ptr<sqlite::result> matches(q.get_result());
      bool fetch_ongoing = false;
      do
      {
        std::string name = matches->get_string(0);
        if (name.empty()) // entry with empty name means a fetch is underway
        {
          fetch_ongoing = true;
          break;
        }
        items.push_back(name);
      }
      while (matches->next_row());
      if (!fetch_ongoing)
        return items;
    }
  }
  
  // this never got refreshed, but a fetch request must be ongoing.. we can wait and block or return an empty list
  // if (_block_on_fetches)
  return std::vector<std::string>();
}

std::vector<std::string> AutoCompleteCache::get_matching_function_names(const std::string &schema, const std::string &prefix)
{
  // schedule a fetch in case its needed
  refresh_schema_cache_if_needed(schema);
  
  if (!_shutdown)
  {
    // Ensures shutdown is not done while processing
    base::RecMutexLock sd_lock(_shutdown_mutex);
    base::RecMutexLock lock(_sqconn_mutex);
    sqlite::query q(*_sqconn, "SELECT name FROM routines WHERE schema LIKE ? ESCAPE '\\' "
      "AND name LIKE ? ESCAPE '\\' AND is_function=1");
    q.bind(1, schema.size() == 0 ? "%" : base::escape_sql_string(schema, true));
    q.bind(2, base::escape_sql_string(prefix, true) + "%");
    if (q.emit())
    {
      std::vector<std::string> items;
      boost::shared_ptr<sqlite::result> matches(q.get_result());
      bool fetch_ongoing = false;
      do
      {
        std::string name = matches->get_string(0);
        if (name.empty()) // entry with empty name means a fetch is underway
        {
          fetch_ongoing = true;
          break;
        }
        items.push_back(name);
      }
      while (matches->next_row());
      if (!fetch_ongoing)
        return items;
    }
  }

  // this never got refreshed, but a fetch request must be ongoing.. we can wait and block or return an empty list
  // if (_block_on_fetches)
  return std::vector<std::string>();
}

/**
 *  Queries the database to check if the schema cache has been already loaded.
 */
bool AutoCompleteCache::refresh_schema_cache_if_needed(const std::string &schema)
{
  if (!_shutdown)
  {
    // Ensures shutdown is not done while processing
    base::RecMutexLock sd_lock(_shutdown_mutex);
    base::RecMutexLock lock(_sqconn_mutex);
    sqlite::query q(*_sqconn, "SELECT last_refresh FROM schemas WHERE name LIKE ? ESCAPE '\\' ");
    q.bind(1, schema.size() == 0 ? "%" : base::escape_sql_string(schema, true));
    if (q.emit())
    {
      boost::shared_ptr<sqlite::result> matches(q.get_result());
      // schema info is already loaded in cache
      if (matches->get_int(0))
      {
        log_debug3("schema %s is already cached\n", schema.c_str());
        return false;
      }
    }
  }

  // Triggers a loading task for the schema cache
  log_debug3("schema %s is not cached, populating cache...\n", schema.c_str());
  refresh_schema_cache(schema);

  return true;
}


void AutoCompleteCache::refresh_schema_list_cache_if_needed()
{
  add_pending_refresh("");
}

void AutoCompleteCache::refresh_schema_cache(const std::string &schema)
{
  log_debug("refresh schema cache for %s\n", schema.c_str());
  add_pending_refresh(schema);
}


void AutoCompleteCache::refresh_table_cache(const std::string &schema, const std::string &table)
{
  add_pending_refresh(schema + "\n" + table);
}


void AutoCompleteCache::refresh_cache_thread()
{
  log_debug2("entering worker thread\n");

//  while(!_refresh_thread); //wait until _rfresh_thread is assigned

  while (!_shutdown)
  {
    std::string schema_to_fetch;

    if (!get_pending_refresh(schema_to_fetch))
    {
        break;
    }
    
    // Ensures shutdown is not done while processing
    if (_shutdown)
      break;
    
    log_debug3("will fetch '%s'\n", schema_to_fetch.c_str());

    if (schema_to_fetch.empty())
    {
      refresh_schemas_w();
    }
    else
    {
      std::string::size_type s = schema_to_fetch.find('\n');
      try
      {
        if (s == std::string::npos)
        {
          refresh_tables_w(schema_to_fetch);

          refresh_routines_w(schema_to_fetch);
        }
        else
        {
          std::string schema = schema_to_fetch.substr(0, s);
          std::string table = schema_to_fetch.substr(s+1);
          refresh_columns_w(schema, table);
        }
      }
      catch (std::exception &exc)
      {
        log_error("Exception during update of cache for '%s': %s\n", schema_to_fetch.c_str(), exc.what()); 
      }
    }
  }



  // signal the main thread that the thread is (about to be) gone
  _refresh_thread = NULL;
  _cache_working.post();

  if (_feedback && !_shutdown)
    _feedback(false);

  log_debug2("leaving worker thread\n");

}


void *AutoCompleteCache::_refresh_cache_thread(void *data)
{
  try
  {
    AutoCompleteCache *self = reinterpret_cast<AutoCompleteCache*>(data);
    self->refresh_cache_thread();
  } catch (sql::SQLException &exc)
  {
    log_error("SQLException executing refresh_cache_thread: Error Code: %d\n, %s\n",exc.getErrorCode(), exc.what());
  }
  return NULL;
}


void AutoCompleteCache::refresh_schemas_w()
{
  std::vector<std::string> schemas;
  {
    sql::Dbc_connection_handler::Ref conn;
    base::RecMutexLock lock(_get_connection(conn));
    {
      std::auto_ptr<sql::Statement> stmt(conn->ref->createStatement());
      std::auto_ptr<sql::ResultSet> rs(stmt->executeQuery("SHOW DATABASES"));
      if (rs.get())
      {
        while (rs->next() && !_shutdown)
        {
          schemas.push_back(rs->getString(1));
        }
        log_debug2("Found %zi schemas.\n", schemas.size()); 
      }
      else
        log_debug2("No schema found.\n");
    }
  } 
  update_schemas(schemas);
}


void AutoCompleteCache::refresh_tables_w(const std::string &schema)
{
  std::vector<std::pair<std::string, bool> > tables;
  {
    sql::Dbc_connection_handler::Ref conn;

    // TODO: is it possible that the connection can be locked even it was already deleted???
    base::RecMutexLock lock(_get_connection(conn));
    {
      std::auto_ptr<sql::ResultSet> rs(conn->ref->createStatement()->executeQuery(std::string(base::sqlstring("SHOW FULL TABLES FROM !", 0) << schema)));
      if (rs.get())
      {
        while (rs->next() && !_shutdown)
        {
          std::string type = rs->getString(2);
          std::string table = rs->getString(1);
          tables.push_back(std::make_pair(table, type == "VIEW"));

          // automatically trigger fetch of columns for this table
          add_pending_refresh(schema + "\n" + table);
        }
        log_debug2("updating %zi tables...\n", tables.size());
      }
      else
        log_debug2("no tables for %s\n", schema.c_str());
    }
  }
  update_schema_tables(schema, tables, false);
}


void AutoCompleteCache::refresh_routines_w(const std::string &schema)
{
  std::vector<std::pair<std::string, bool> > routines;
  {
    sql::Dbc_connection_handler::Ref conn;
    base::RecMutexLock lock(_get_connection(conn));
    {
      std::auto_ptr<sql::ResultSet> rs(conn->ref->createStatement()->executeQuery(std::string(base::sqlstring("SHOW PROCEDURE STATUS WHERE Db=?", 0) << schema)));
      if (rs.get())
      {
        while (rs->next() && !_shutdown)
          routines.push_back(std::make_pair(rs->getString(2), false));
      }
    }
    {
      std::auto_ptr<sql::ResultSet> rs(conn->ref->createStatement()->executeQuery(std::string(base::sqlstring("SHOW FUNCTION STATUS WHERE Db=?", 0) << schema)));
      if (rs.get())
      {
        while (rs->next() && !_shutdown)
          routines.push_back(std::make_pair(rs->getString(2), true));
      }
    }
  }
  update_schema_routines(schema, routines, false);
}


void AutoCompleteCache::refresh_columns_w(const std::string &schema, const std::string &table)
{
  std::vector<std::string> columns;
  {
    sql::Dbc_connection_handler::Ref conn;
    base::RecMutexLock lock(_get_connection(conn));
    {
      std::auto_ptr<sql::Statement> statement(conn->ref->createStatement());
      std::auto_ptr<sql::ResultSet> rs(statement->executeQuery(std::string(base::sqlstring("SHOW COLUMNS FROM !.!", 0) << schema << table)));
      if (rs.get())
      {
        while (rs->next() && !_shutdown)
          columns.push_back(rs->getString(1));
      }
    }
  }
  update_table_columns(schema, table, columns);
}

#define CREATE_META \
"create table meta ("\
"   name varchar(64) primary key,"\
"   value varchar(64)"\
")"

#define CREATE_SCHEMAS \
"create table schemas ("\
"   name varchar(64) primary key,"\
"   last_refresh int default 0"\
")"

#define CREATE_TABLES \
"create table tables ("\
"  schema varchar(64) NOT NULL,"\
"  name varchar(64) NOT NULL,"\
"  is_view int default 0,"\
"  primary key (schema, name)"\
")"

#define CREATE_ROUTINES \
"create table routines ("\
"  schema varchar(64) NOT NULL,"\
"  name varchar(64) NOT NULL,"\
"  is_function int default 0,"\
"  primary key (schema, name)"\
")"


#define CREATE_COLUMNS \
"create table columns ("\
"  schema varchar(64) NOT NULL,"\
"  tabl varchar(64) NOT NULL,"\
"  name varchar(64) NOT NULL,"\
"  primary key (schema, tabl, name),"\
"  foreign key (schema, tabl) references tables (schema, name) on delete cascade"\
")"

void AutoCompleteCache::init_db()
{
  log_info("Initializing autocompletion cache for %s\n", _connection_id.c_str());
  try
  {
    sqlite::execute(*_sqconn, CREATE_META, true);
  }
  catch (std::exception &exc)
  {
    log_error("Error creating cache db.meta: %s\n", exc.what());
  }  
  try
  {
    sqlite::execute(*_sqconn, CREATE_SCHEMAS, true);
  }
  catch (std::exception &exc)
  {
    log_error("Error creating cache db.schemas: %s\n", exc.what());
  }
  try
  {
    sqlite::execute(*_sqconn, CREATE_TABLES, true);
  }
  catch (std::exception &exc)
  {
    log_error("Error creating cache db.tables: %s\n", exc.what());
  }
  try
  {
    sqlite::execute(*_sqconn, CREATE_ROUTINES, true);
  }
  catch (std::exception &exc)
  {
    log_error("Error creating cache db.routines: %s\n", exc.what());
  }
  try
  {
    sqlite::execute(*_sqconn, CREATE_COLUMNS, true);
  }
  catch (std::exception &exc)
  {
    log_error("Error creating cache db.columns: %s\n", exc.what());
  }
}


bool AutoCompleteCache::is_schema_list_fetch_done()
{
  base::RecMutexLock lock(_sqconn_mutex);
  sqlite::query q(*_sqconn, "select * from schemas");
  if (q.emit())
    return true;
  return false;
}


bool AutoCompleteCache::is_schema_tables_fetch_done(const std::string &schema)
{
  base::RecMutexLock lock(_sqconn_mutex);
  sqlite::query q(*_sqconn, "select * from tables where schema = ?");
  q.bind(1, schema);
  if (q.emit())
    return true;
  return false;
}

bool AutoCompleteCache::is_schema_table_columns_fetch_done(const std::string &schema, const std::string &table)
{
  base::RecMutexLock lock(_sqconn_mutex);
  sqlite::query q(*_sqconn, "select * from columns where schema = ? and tabl = ?");
  q.bind(1, schema);
  q.bind(2, table);
  if (q.emit())
    return true;
  return false;
}

bool AutoCompleteCache::is_schema_routines_fetch_done(const std::string &schema)
{
  base::RecMutexLock lock(_sqconn_mutex);
  sqlite::query q(*_sqconn, "select * from routines where schema = ?");
  q.bind(1, schema);
  if (q.emit())
    return true;
  return false;
}

void AutoCompleteCache::touch_schema_record(const std::string &schema)
{
  {
    sqlite::query q(*_sqconn, "select * from schemas where name=?");
    q.bind(1, schema);
    if (q.emit())
    {
      sqlite::command update(*_sqconn, "update schemas set last_refresh=strftime('%s', 'now') where name=?");
      update.bind(1, schema);
      update.emit();
      return;
    }
  }
  {
    sqlite::command insert(*_sqconn, "insert into schemas (name, last_refresh) values (?, strftime('%s', 'now'))");
    insert.bind(1, schema);
    insert.emit();
  }
}


void AutoCompleteCache::update_schemas(const std::vector<std::string> &schemas)
{
  try
  {
    if (!_shutdown)
    {
      // Ensures shutdown is not done while processing
      base::RecMutexLock sd_lock(_shutdown_mutex);
      base::RecMutexLock lock(_sqconn_mutex);
 
      std::map<std::string, int> old_schema_update_times;
      {
        sqlite::query q(*_sqconn, "select name, last_refresh from schemas");
        if (q.emit())
        {
          boost::shared_ptr<sqlite::result> matches(q.get_result());
          do
          {
            std::string name = matches->get_string(0);
            if (!name.empty()) // entry with empty name means a fetch is underway
              old_schema_update_times[name] = matches->get_int(1);
          }
          while (matches->next_row());
        }
      }

      sqlide::Sqlite_transaction_guarder trans(_sqconn, false); //will be committed when will go out of the scope
      {
        sqlite::execute del(*_sqconn, "delete from schemas");
        del.emit();
      }

      sqlite::execute insert(*_sqconn, "insert into schemas (name, last_refresh) values (?, ?)");
      for (std::vector<std::string>::const_iterator t = schemas.begin(); t != schemas.end(); ++t)
      {
        insert.bind(1, *t);
        if (old_schema_update_times.find(*t) == old_schema_update_times.end())
          insert.bind(2, 0);
        else
          insert.bind(2, old_schema_update_times[*t]);
        insert.emit();
        insert.clear();
      }

      // if there were no tables, create a dummy item signaling the update already happened
      if (schemas.empty())
      {
        sqlite::execute insert(*_sqconn, "insert into schemas (name) values ('')");
        insert.emit();
      }


      _schema_list_fetched = true;
    }
  }
  catch (std::exception &exc)
  {
    log_error("Exception caught while updating schema name cache: %s",
              exc.what());
  }
}


void AutoCompleteCache::update_schema_tables(const std::string &schema,
  const std::vector<std::pair<std::string, bool> > &tables, bool just_append)
{
  try
  {
    if (!_shutdown)
    {
      // Ensures shutdown is not done while processing
      base::RecMutexLock sd_lock(_shutdown_mutex);
      base::RecMutexLock lock(_sqconn_mutex);
 
      touch_schema_record(schema);
    
      sqlide::Sqlite_transaction_guarder  trans(_sqconn, false); //will be committed when will go out of the scope
      // Clear records for this schema if we are not just appending.
      if (!just_append)
      {
        sqlite::execute del(*_sqconn, "delete from tables where schema = ?");
        del.bind(1, schema);
        del.emit();
      }

      sqlite::execute insert(*_sqconn, "insert into tables (schema, name, is_view) values (?, ?, ?)");
      for (std::vector<std::pair<std::string, bool> >::const_iterator t = tables.begin(); t != tables.end(); ++t)
      {
        insert.bind(1, schema);
        insert.bind(2, t->first);
        insert.bind(3, t->second ? 1 : 0);
        insert.emit();
        insert.clear();
      }
      // if there were no tables, create a dummy item signaling the update already happened
      if (tables.empty())
      {
        sqlite::execute insert(*_sqconn, "insert into tables (schema, name, is_view) values (?, '', -1)");
        insert.bind(1, schema);
        insert.emit();
      }
    }    
  }
  catch (std::exception &exc)
  {
    log_error("Exception caught while updating table name cache for schema %s: %s\n", schema.c_str(),
              exc.what());
  }
}


void AutoCompleteCache::update_schema_routines(const std::string &schema,
  const std::vector<std::pair<std::string, bool> > &routines, bool just_append)
{
  try
  {
    if (!_shutdown)
    {
      // Ensures shutdown is not done while processing
      base::RecMutexLock sd_lock(_shutdown_mutex);
      base::RecMutexLock lock(_sqconn_mutex);
    
      touch_schema_record(schema);

      sqlide::Sqlite_transaction_guarder  trans(_sqconn, false); // Will be committed when we go out of the scope.

      // Clear records for this schema if we are not just appending.
      if (!just_append)
      {
        sqlite::execute del(*_sqconn, "delete from routines where schema = ?");
        del.bind(1, schema);
        del.emit();
      }

      sqlite::query insert(*_sqconn, "insert into routines (schema, name, is_function) values (?, ?, ?)");
      for (std::vector<std::pair<std::string, bool> >::const_iterator t = routines.begin(); t != routines.end(); ++t)
      {
        insert.bind(1, schema);
        insert.bind(2, t->first);
        insert.bind(3, t->second);
        insert.emit();
        insert.clear();
      }
      // if there were no routines, create a dummy item signaling the update already happened
      if (routines.empty())
      {
        sqlite::execute insert(*_sqconn, "insert into routines (schema, name, is_function) values (?, '', -1)");
        insert.bind(1, schema);
        insert.emit();
      }

    }
  }
  catch (std::exception &exc)
  {
    log_error("Exception caught while updating routine name cache for schema %s: %s\n", schema.c_str(),
              exc.what());
  }
}


void AutoCompleteCache::update_table_columns(const std::string &schema, const std::string &table,
                                             const std::vector<std::string> &columns)
{

    if (!_shutdown)
    {
      // Ensures shutdown is not done while processing
      base::RecMutexLock sd_lock(_shutdown_mutex);
      base::RecMutexLock lock(_sqconn_mutex);
      try
      {
        sqlide::Sqlite_transaction_guarder  trans(_sqconn, false); //will be committed when will go out of the scope
        // clear records for this schema
        {
          sqlite::execute del(*_sqconn, "delete from columns where schema = ? and tabl = ?");
          del.bind(1, schema);
          del.bind(2, table);
          del.emit();
        }


        sqlite::query insert(*_sqconn, "insert into columns (schema, tabl, name) values (?, ?, ?)");
        for (std::vector<std::string>::const_iterator t = columns.begin(); t != columns.end(); ++t)
        {
          insert.bind(1, schema);
          insert.bind(2, table);
          insert.bind(3, *t);
          insert.emit();
          insert.clear();
        }
        // if there were no columns, create a dummy item signaling the update already happened
        if (columns.empty())
        {
          sqlite::execute insert(*_sqconn, "insert into columns (schema, tabl, name) values (?, ?, '')");
          insert.bind(1, schema);
          insert.bind(2, table);
          insert.emit();
        }
      }
      catch (std::exception &exc)
      {
        log_error("Exception caught while updating column name cache for %s.%s: %s\n", schema.c_str(), table.c_str(),
                  exc.what());
      }
    }

}

void AutoCompleteCache::add_pending_refresh(const std::string& task)
{
  if (!_shutdown)
  {
    // Ensures shutdown is not done while processing
    base::RecMutexLock sd_lock(_shutdown_mutex);
    base::RecMutexLock lock(_pending_mutex);
    if ((!task.empty() || !_schema_list_fetched) && std::find(_pending_refresh_schema.begin(), _pending_refresh_schema.end(), task)
        == _pending_refresh_schema.end())
    {
      _pending_refresh_schema.push_back(task);
    }
  }

  // Create the worker thread if there's work to do. Does nothing if there's already a thread.
  if (_pending_refresh_schema.size() > 0)
    create_worker_thread();
}

bool AutoCompleteCache::get_pending_refresh(std::string &task)
{
  bool ret_val = false;

  if (!_shutdown)
  {
    // Ensures shutdown is not done while processing
    base::RecMutexLock sd_lock(_shutdown_mutex);
    base::RecMutexLock lock(_pending_mutex);

    if (!_pending_refresh_schema.empty())
    {
      ret_val = true;
      task = _pending_refresh_schema.front();
      _pending_refresh_schema.pop_front();
    }
  }

  return ret_val;
}

void AutoCompleteCache::create_worker_thread()
{
  // Fire up thread to start caching.
  //if (!_refresh_thread)
  if(!_cache_working.try_wait()) // If there is already working thread, just do nothing and exit.
    return;

  _refresh_thread = NULL;
//  if (!_refresh_thread)
//  {
    if (!_shutdown)
    {
      GError *error = NULL;
      log_debug3("creating worker thread\n");
//      if (_refresh_thread)
//        throw std::logic_error("Starting new AutoCompleteCache worker, but the previous one didn't die");
      _refresh_thread = base::create_thread(&AutoCompleteCache::_refresh_cache_thread, this, &error);
      if (!_refresh_thread)
      {
        log_error("Error creating autocompletion worker thread: %s\n", error ? error->message : "out of mem?");
        g_error_free(error);
      }
      else
        if (_feedback)
          _feedback(true);
    }
//  }
}
