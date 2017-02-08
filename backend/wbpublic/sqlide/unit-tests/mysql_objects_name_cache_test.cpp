/*
 * Copyright (c) 2012, 2017, Oracle and/or its affiliates. All rights reserved.
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
#include "code-completion/mysql_object_names_cache.h"

using namespace grt;
using namespace wb;
namespace ph = std::placeholders;

BEGIN_TEST_DATA_CLASS(autocompletion_cache_test)
public:
base::RecMutex _connection_mutex;
sql::Dbc_connection_handler::Ref _conn;
MySQLObjectNamesCache *_cache;

TEST_DATA_CONSTRUCTOR(autocompletion_cache_test) : _conn(new sql::Dbc_connection_handler()), _cache(0) {
// We want to print this out only once, not for every test, so we put it here
// as this is the first test that runs usually.
#ifdef _WIN32
  TCHAR path[MAX_PATH];
  GetCurrentDirectory(MAX_PATH, path);
  printf("\nTests running in: %s\n\n", base::wstring_to_string(path).c_str());
#endif

  grt::GRT::get()->scan_metaclasses_in("../../res/grt/");
  grt::GRT::get()->end_loading_metaclasses();

  // Because tests are executed in alphabetic order this is the first one.
  // Hence we set up the sakila db in server here.
  setup_sakila_db();
}

TEST_DATA_DESTRUCTOR(autocompletion_cache_test) {
  //   cleanup_sakila_db();
  _cache->shutdown();
  delete _cache;
}

std::vector<std::pair<std::string, std::string>> runSQL(const std::string &query) {
  std::vector<std::pair<std::string, std::string>> result;

  // RecMutexLock lock(ensure_valid_aux_connection());
  {
    std::auto_ptr<sql::Statement> statement(_conn->ref->createStatement());
    std::auto_ptr<sql::ResultSet> rs(statement->executeQuery(query));
    if (rs.get()) {
      unsigned columnCount = rs->getMetaData()->getColumnCount();
      if (columnCount > 0) {
        while (rs->next()) {
          if (columnCount > 1)
            result.push_back({rs->getString(1), rs->getString(2)});
          else
            result.push_back({rs->getString(1), ""});
        }
      }
    }
  }

  return result;
}

END_TEST_DATA_CLASS;

TEST_MODULE(autocompletion_cache_test, "autocompletion object name cache");

TEST_FUNCTION(2) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  setup_env(connectionProperties);

  sql::DriverManager *dm = sql::DriverManager::getDriverManager();

  _conn->ref = dm->getConnection(connectionProperties);
}

TEST_FUNCTION(3) {
  _cache = new MySQLObjectNamesCache(std::bind(&Test_object_base<autocompletion_cache_test>::runSQL, this, ph::_1),
                                     std::function<void(bool)>());

  // Right after creation the schema list is empty. Retrieval has just been set up.
  std::vector<std::string> list = _cache->getMatchingSchemaNames("sakila");
  ensure("Schema list is not empty", list.empty());

  // After creation a first data retrieval starts automatically in the background.
  // Wait a moment to have that finished.
  g_usleep(2000000);

  // Retrieve db objects in the sakila schema. So they are available when we ask for them
  // in the following tests.
  _cache->loadSchemaObjectsIfNeeded("sakila");
  g_usleep(2000000);
}

static void ensure_list_equals(const char *what, const std::vector<std::string> &list, const char **comp) {
  std::vector<std::string>::const_iterator i;
  int j = 0;
  try {
    for (i = list.begin(); i != list.end() && comp[j] != NULL; ++i, ++j) {
      ensure_equals(what, *i, comp[j]);
    }
    ensure(what, comp[j] == NULL);
    ensure(what, i == list.end());
  } catch (...) {
    // TODO: this should be part of the TUT message, otherwise we might not see it.
    g_message("Result list:");
    for (i = list.begin(); i != list.end(); ++i)
      g_message("  %s", i->c_str());
    g_message("Expected list:");
    for (j = 0; comp[j]; j++)
      g_message("  %s", comp[j]);
    throw;
  }
}

TEST_FUNCTION(10) {
  std::vector<std::string> list = _cache->getMatchingSchemaNames("");
  int found = 0;

  // This time the schema list should contain sakila and mysql.
  for (std::vector<std::string>::const_iterator i = list.begin(); i != list.end(); ++i) {
    if (*i == "sakila" || *i == "mysql")
      found++;
  }
  ensure_equals("known schema name matches", found, 2);

  // match just sakila
  list = _cache->getMatchingSchemaNames("sakila");
  found = 0;
  for (std::vector<std::string>::const_iterator i = list.begin(); i != list.end(); ++i) {
    if (*i == "sakila" || *i == "mysql")
      found++;
  }
  ensure_equals("known schema name matches (sakila)", found, 1);
}

TEST_FUNCTION(12) {
  static const char *sakila_ac[] = {"actor", "actor_info", NULL};
  static const char *sakila_actor_[] = {"actor_info", NULL};

  _cache->loadSchemaObjectsIfNeeded("sakila");

  // Wait for the refresh to settle. The functions to check if a fetch is done
  // are really unreliable and should be revised.
  g_usleep(1000000);

  std::vector<std::string> list = _cache->getMatchingTableNames("sakila", "ac");
  std::vector<std::string> list2 = _cache->getMatchingViewNames("sakila", "ac");
  std::copy(list2.begin(), list2.end(), std::back_inserter(list));
  ensure_list_equals("tables sakila.ac*", list, sakila_ac);

  list = _cache->getMatchingTableNames("sakila", "actor_");
  list2 = _cache->getMatchingViewNames("sakila", "ac");
  std::copy(list2.begin(), list2.end(), std::back_inserter(list));
  ensure_list_equals("tables sakila.actor_*", list, sakila_actor_);
}

TEST_FUNCTION(14) {
  static const char *sakila_inv[] = {"inventory_held_by_customer", "inventory_in_stock", NULL};

  std::vector<std::string> list = _cache->getMatchingFunctionNames("sakila", "inv");
  ensure_list_equals("functions sakila.inv*", list, sakila_inv);
}

TEST_FUNCTION(16) {
  static const char *sakila_fi[] = {"film_in_stock", "film_not_in_stock", NULL};

  std::vector<std::string> list = _cache->getMatchingProcedureNames("sakila", "fi");
  ensure_list_equals("procedures sakila.fi*", list, sakila_fi);
}

TEST_FUNCTION(18) {
  // columns
  static const char *sakila_a[] = {"actor_id", NULL};

  std::vector<std::string> list = _cache->getMatchingColumnNames("sakila", "actor", "a");
  ensure_list_equals("columns sakila.actor.a*", list, sakila_a);
}

// Everything again reusing cache
TEST_FUNCTION(19) {
  _cache->shutdown();
  delete _cache;
  _cache = new MySQLObjectNamesCache(std::bind(&Test_object_base<autocompletion_cache_test>::runSQL, this, ph::_1),
                                     std::function<void(bool)>());

  g_usleep(2000000);

  _cache->loadSchemaObjectsIfNeeded("sakila");
  g_usleep(2000000);
}

TEST_FUNCTION(20) {
  std::vector<std::string> list;

  list = _cache->getMatchingSchemaNames("");
  int found = 0;

  // This time the schema list should contain sakila and mysql.
  for (std::vector<std::string>::const_iterator i = list.begin(); i != list.end(); ++i) {
    if (*i == "sakila" || *i == "mysql")
      found++;
  }
  ensure_equals("known schema name matches", found, 2);

  // match just sakila
  list = _cache->getMatchingSchemaNames("sakila");
  found = 0;
  for (std::vector<std::string>::const_iterator i = list.begin(); i != list.end(); ++i) {
    if (*i == "sakila" || *i == "mysql")
      found++;
  }
  ensure_equals("known schema name matches (sakila*)", found, 1);
}

TEST_FUNCTION(22) {
  static const char *sakila_ac[] = {"actor", "actor_info", NULL};
  static const char *sakila_actor_[] = {"actor_info", NULL};

  std::vector<std::string> list = _cache->getMatchingTableNames("sakila", "ac");
  std::vector<std::string> list2 = _cache->getMatchingViewNames("sakila", "ac");
  std::copy(list2.begin(), list2.end(), std::back_inserter(list));
  ensure_list_equals("tables sakila.ac*", list, sakila_ac);

  list = _cache->getMatchingTableNames("sakila", "actor_");
  list2 = _cache->getMatchingViewNames("sakila", "actor_");
  std::copy(list2.begin(), list2.end(), std::back_inserter(list));
  ensure_list_equals("tables sakila.actor_*", list, sakila_actor_);
}

TEST_FUNCTION(24) {
  static const char *sakila_inv[] = {"inventory_held_by_customer", "inventory_in_stock", NULL};

  std::vector<std::string> list = _cache->getMatchingFunctionNames("sakila", "inv");
  ensure_list_equals("functions sakila.inv*", list, sakila_inv);
}

TEST_FUNCTION(26) {
  static const char *sakila_fi[] = {"film_in_stock", "film_not_in_stock", NULL};

  std::vector<std::string> list = _cache->getMatchingProcedureNames("sakila", "fi");
  ensure_list_equals("procedures sakila.fi*", list, sakila_fi);
}

TEST_FUNCTION(28) {
  static const char *sakila_a[] = {"actor_id", NULL};

  std::vector<std::string> list = _cache->getMatchingColumnNames("sakila", "actor", "a");
  ensure_list_equals("columns sakila.actor.a*", list, sakila_a);
}

END_TESTS
