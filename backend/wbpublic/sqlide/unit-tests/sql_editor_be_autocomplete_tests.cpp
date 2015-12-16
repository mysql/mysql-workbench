/* 
 * Copyright (c) 2012, 2015, Oracle and/or its affiliates. All rights reserved.
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
#include "base/file_utilities.h"

#include "grtdb/db_helpers.h"
#include "grtdb/db_object_helpers.h"
#include "sqlide/wb_sql_editor_form.h"
#include "sqlide/autocomplete_object_name_cache.h"
#include "sqlide/sql_editor_be.h"

#include "grtsqlparser/mysql_parser_services.h"

#include "tut_mysql_versions.h"

using namespace bec;
using namespace wb;

struct ac_test_entry
{
  int version_first;  //  First supported version for this entry
  int version_last;   //  Last supported version for this entry
  
  std::string query;
  std::string typed_part;
  int line;
  int offset;

  bool check_entries;
  std::string entries;
  int parts;
};

BEGIN_TEST_DATA_CLASS(sql_editor_be_autocomplete_tests)
protected:
  WBTester _tester;
  MySQLEditor::Ref _sql_editor;
  GrtVersionRef _version;

  base::RecMutex _connection_mutex;
  sql::Dbc_connection_handler::Ref _conn;
  AutoCompleteCache *_cache;
  parser::ParserContext::Ref _autocomplete_context;
  int version;

public:
TEST_DATA_CONSTRUCTOR(sql_editor_be_autocomplete_tests)
  : _conn(new sql::Dbc_connection_handler()), _cache(NULL)
{
  populate_grt(_tester.grt, _tester);

  // Auto completion needs a cache for object name look up, so we have to set up one
  // with all bells and whistles.
}

TEST_DATA_DESTRUCTOR(sql_editor_be_autocomplete_tests)
{
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


TEST_MODULE(sql_editor_be_autocomplete_tests, "SQL code completion tests");

/**
 * Setup for the cache.
 */
TEST_FUNCTION(5)
{
  db_mgmt_ConnectionRef connectionProperties(_tester.grt);
  setup_env(_tester.grt, connectionProperties);

  sql::DriverManager *dm = sql::DriverManager::getDriverManager();
  _conn->ref = dm->getConnection(connectionProperties);

  std::auto_ptr<sql::Statement> stmt(_conn->ref->createStatement());

  sql::ResultSet *res = stmt->executeQuery("SELECT VERSION() as VERSION");
  if (res && res->next())
  {
    std::string version_string = res->getString("VERSION");
    _version = parse_version(_tester.grt, version_string);
  }
  delete res;

  ensure("Server version is invalid", _version.is_valid());

  _tester.get_rdbms()->version(_version);
  version = (int)(_version->majorNumber() * 10000 + _version->minorNumber() * 100 + _version->releaseNumber());

  base::remove("testconn.cache");
  _cache = new AutoCompleteCache("testconn", boost::bind(&Test_object_base<sql_editor_be_autocomplete_tests>::get_connection, this, _1),
    ".", NULL);

  // Copy a current version of the code editor configuration file to the test data folder.
  gchar *contents;
  gsize length;
  GError *error = NULL;
  if (g_file_get_contents("../../res/wbdata/code_editor.xml", &contents, &length, &error))
  {
    ensure("Could not write editor configuration to target file",
      g_file_set_contents("data/code_editor.xml", contents, length, &error) == TRUE);
    g_free(contents);
  }
  else
    fail("Could not copy code editor configuration");

  parser::MySQLParserServices::Ref services = parser::MySQLParserServices::get(_tester.grt);
  parser::ParserContext::Ref context = services->createParserContext(_tester.get_rdbms()->characterSets(),
    _version, false);

  _autocomplete_context = services->createParserContext(_tester.get_rdbms()->characterSets(),
    _version, false);

  _sql_editor = MySQLEditor::create(_tester.grt, context, _autocomplete_context);
  _sql_editor->set_current_schema("sakila");
  _sql_editor->set_auto_completion_cache(_cache);

  // We don't set up the sakila schema. This is needed in so many places, it should simply exist.
  std::vector<std::string> list = _cache->get_matching_schema_names("sakila");
  if (list.empty())
  {
    // Sakila not yet fetched. Give it a moment and try again.
    g_usleep(1000000);
    list = _cache->get_matching_schema_names("sakila");
  }
  ensure("Could not get the schema list from the cache", !list.empty());

  bool found = false;
  for (std::vector<std::string>::const_iterator i = list.begin(); i != list.end(); ++i)
  {
    if (*i == "sakila")
    {
      found = true;
      break;
    }
  }
  ensure("Sakila could not be found", found); 
}

/**
 * Another prerequisites test. See that the cache contains needed objects.
 */
TEST_FUNCTION(10)
{
  std::vector<std::string> list = _cache->get_matching_schema_names("sakila");
  int found = 0;
  for (std::vector<std::string>::const_iterator i = list.begin(); i != list.end(); ++i)
  {
    if (*i == "sakila" || *i == "mysql")
      found++;
  }
  ensure_equals("Sakila schema missing. Is the DB set up properly?", found, 1);
}

//--------------------------------------------------------------------------------------------------

/**
 * Collecting AC info for each position in a simple but typical statement.
 */
TEST_FUNCTION(15)
{
}

//--------------------------------------------------------------------------------------------------

/**
 * Testing 1-2 examples for each possible query type (i.e. major keywords with a typical case).
 * Possibilities are endless so we can only peek for possible problems.
 * Note: for now no language parts are included that are introduced in 5.6 or later.
 * Add more test cases for specific bugs.
 */
TEST_FUNCTION(20)
{
}

//--------------------------------------------------------------------------------------------------

/**
 * Collecting AC info for the same statement as in TC 15, but this time as if we were writing
 * each letter. This usually causes various parse errors to appear, but we want essentially the same
 * output as for the valid statement (except for not-yet-written references).
 */
TEST_FUNCTION(90)
{
}

END_TESTS

