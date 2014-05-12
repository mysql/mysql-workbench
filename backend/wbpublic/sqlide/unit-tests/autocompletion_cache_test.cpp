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

#include "connection_helpers.h"
#include "test_fixture.h"
#include "base/file_utilities.h"
#include "sqlide/wb_sql_editor_form.h"
#include "sqlide/autocomplete_object_name_cache.h"


using namespace grt;
using namespace wb;

// Helper to wait for cache operations to finish.
static base::Mutex cache_mutex;
static base::Cond cache_condition;

BEGIN_TEST_DATA_CLASS(autocompletion_cache_test)
public:
  GRT _grt;
  base::RecMutex _connection_mutex;
  sql::Dbc_connection_handler::Ref _conn;
  AutoCompleteCache *_cache;

TEST_DATA_CONSTRUCTOR(autocompletion_cache_test)
  : _cache(0), _conn(new sql::Dbc_connection_handler())
{
  _grt.scan_metaclasses_in("../../res/grt/");
  _grt.end_loading_metaclasses();

  // setup the sakila db in server
  setup_sakila_db();
}

TEST_DATA_DESTRUCTOR(autocompletion_cache_test)
{
//   cleanup_sakila_db();
  _cache->shutdown();
  delete _cache;
}

base::RecMutexLock get_connection(sql::Dbc_connection_handler::Ref &conn)
{
  base::RecMutexLock lock(_connection_mutex);
  conn = _conn;
  return lock;
}

END_TEST_DATA_CLASS;

TEST_MODULE(autocompletion_cache_test, "autocompletion object name cache");

static void cache_callback(bool working)
{
  if (!working)
  {
    cache_mutex.lock();
    cache_condition.signal();
    cache_mutex.unlock();
  }
}

TEST_FUNCTION(2)
{
  db_mgmt_ConnectionRef connectionProperties(&_grt);

  setup_env(&_grt, connectionProperties);

  sql::DriverManager *dm= sql::DriverManager::getDriverManager();

  _conn->ref = dm->getConnection(connectionProperties);
}


TEST_FUNCTION(3)
{
  base::remove("testconn.cache");
  _cache = new AutoCompleteCache("testconn", boost::bind(&Test_object_base<autocompletion_cache_test>::get_connection, this, _1),
    ".", cache_callback);
}

static void ensure_list_equals(const char *what, const std::vector<std::string> &list, const char **comp)
{
  std::vector<std::string>::const_iterator i;
  int j = 0;
  try
  {
    for (i = list.begin(); i != list.end() && comp[j] != NULL; ++i, ++j)
    {
      ensure_equals(what, *i, comp[j]); 
    }
    ensure(what, comp[j] == NULL);
    ensure(what, i == list.end());
  }
  catch (...)
  {
    g_message("Result list:");
    for (i = list.begin(); i != list.end(); ++i)
      g_message("  %s", i->c_str());
    g_message("Expected list:");
    for (j = 0; comp[j]; j++)
      g_message("  %s", comp[j]);
    throw;
  }
}


TEST_FUNCTION(10)
{
  std::vector<std::string> list = _cache->get_matching_schema_names("sakila");
  // 1st time fetch the list is empty because the data is not cached and the worker will start
  // fetching
  ensure("uncached schema list (empty)", list.empty());

  _cache->refresh_schema_list_cache_if_needed(); // TODO: is this really needed? get_matching_schema_names calls it already.

  // The loops are not necessary, but there can be spurious condition signals,
  // as the glib docs say.
  while (!_cache->is_schema_list_fetch_done())
    cache_condition.wait(cache_mutex);

  list = _cache->get_matching_schema_names("");
  int found = 0;
  // this time the schema list should contain sakila and mysql
  for (std::vector<std::string>::const_iterator i = list.begin(); i != list.end(); ++i)
  {
    if (*i == "sakila" || *i == "mysql")
      found++;
  }
  ensure_equals("known schema name matches", found, 2);

  // match just sakila
  list = _cache->get_matching_schema_names("sakila");
  found = 0;
  for (std::vector<std::string>::const_iterator i = list.begin(); i != list.end(); ++i)
  {
    if (*i == "sakila" || *i == "mysql")
      found++;
  }
  ensure_equals("known schema name matches (sakila*)", found, 1);
}

TEST_FUNCTION(12)
{
  static const char *sakila_ac[] = {
    "actor",
    "actor_info",
    NULL
  };
  static const char *sakila_actor_[] = {
    "actor_info",
    NULL
  };
  std::vector<std::string> list = _cache->get_matching_table_names("sakila", "ac");

  while (!_cache->is_schema_tables_fetch_done("sakila"))
    cache_condition.wait(cache_mutex);

  // get the list now
  list = _cache->get_matching_table_names("sakila", "ac");
  ensure_list_equals("tables sakila.ac*", list, sakila_ac);

  list = _cache->get_matching_table_names("sakila", "actor_");
  ensure_list_equals("tables sakila.actor_*", list, sakila_actor_);
}

TEST_FUNCTION(14)
{
  static const char *sakila_inv[] = {
    "inventory_held_by_customer",
    "inventory_in_stock",
    NULL
  };
  std::vector<std::string> list = _cache->get_matching_function_names("sakila", "inv");

  while (!_cache->is_schema_routines_fetch_done("sakila"))
    cache_condition.wait(cache_mutex);

  // get the list now
  list = _cache->get_matching_function_names("sakila", "inv");
  ensure_list_equals("functions sakila.inv*", list, sakila_inv);
}

TEST_FUNCTION(16)
{
  static const char *sakila_fi[] = {
    "film_in_stock",
    "film_not_in_stock",
    NULL
  };
  std::vector<std::string> list = _cache->get_matching_procedure_names("sakila", "fi");

  while (!_cache->is_schema_routines_fetch_done("sakila"))
    cache_condition.wait(cache_mutex);

  // get the list now
  list = _cache->get_matching_procedure_names("sakila", "fi");
  ensure_list_equals("procedures sakila.fi*", list, sakila_fi);
}


TEST_FUNCTION(18)
{
  // columns
  static const char *sakila_a[] = {
    "actor_id",
    NULL
  };
  std::vector<std::string> list = _cache->get_matching_column_names("sakila", "actor", "a");

  while (!_cache->is_schema_table_columns_fetch_done("sakila", "actor"))
    cache_condition.wait(cache_mutex);

  // get the list now
  list = _cache->get_matching_column_names("sakila", "actor", "a");
  ensure_list_equals("columns sakila.actor.a*", list, sakila_a);
}

/// Everything again reusing cache
TEST_FUNCTION(19)
{
  _cache->shutdown();
  delete _cache;
  _cache = new AutoCompleteCache("testconn", boost::bind(&Test_object_base<autocompletion_cache_test>::get_connection, this, _1),
    ".", cache_callback);
}


TEST_FUNCTION(20)
{
  std::vector<std::string> list;

  list = _cache->get_matching_schema_names("");
  int found = 0;
  // this time the schema list should contain sakila and mysql
  for (std::vector<std::string>::const_iterator i = list.begin(); i != list.end(); ++i)
  {
    if (*i == "sakila" || *i == "mysql")
      found++;
  }
  ensure_equals("known schema name matches", found, 2);

  // match just sakila
  list = _cache->get_matching_schema_names("sakila");
  found = 0;
  for (std::vector<std::string>::const_iterator i = list.begin(); i != list.end(); ++i)
  {
    if (*i == "sakila" || *i == "mysql")
      found++;
  }
  ensure_equals("known schema name matches (sakila*)", found, 1);
}

TEST_FUNCTION(22)
{
  static const char *sakila_ac[] = {
    "actor",
    "actor_info",
    NULL
  };
  static const char *sakila_actor_[] = {
    "actor_info",
    NULL
  };
  std::vector<std::string> list = _cache->get_matching_table_names("sakila", "ac");
  ensure_list_equals("tables sakila.ac*", list, sakila_ac);

  list = _cache->get_matching_table_names("sakila", "actor_");
  ensure_list_equals("tables sakila.actor_*", list, sakila_actor_);
}

TEST_FUNCTION(24)
{
  static const char *sakila_inv[] = {
    "inventory_held_by_customer",
    "inventory_in_stock",
    NULL
  };
  std::vector<std::string> list;
  // get the list now
  list = _cache->get_matching_function_names("sakila", "inv");
  ensure_list_equals("functions sakila.inv*", list, sakila_inv);
}

TEST_FUNCTION(26)
{
  static const char *sakila_fi[] = {
    "film_in_stock",
    "film_not_in_stock",
    NULL
  };
  std::vector<std::string> list;

  // get the list now
  list = _cache->get_matching_procedure_names("sakila", "fi");
  ensure_list_equals("procedures sakila.fi*", list, sakila_fi);
}


TEST_FUNCTION(28)
{
  // Just like test 18. See if we still get the same result after all the objects have been loaded.
  static const char *sakila_a[] = {
    "actor_id",
    NULL
  };
  std::vector<std::string> list;

  // get the list now
  list = _cache->get_matching_column_names("sakila", "actor", "a");
  ensure_list_equals("columns sakila.actor.a*", list, sakila_a);
}

END_TESTS
