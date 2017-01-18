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
#include "base/file_utilities.h"

#include "grtdb/db_helpers.h"
#include "grtdb/db_object_helpers.h"
#include "sqlide/wb_sql_editor_form.h"
#include "code-completion/mysql_object_names_cache.h"
#include "sqlide/sql_editor_be.h"

#include "grtsqlparser/mysql_parser_services.h"

#include "tut_mysql_versions.h"

using namespace bec;
using namespace wb;
namespace ph = std::placeholders;

struct ac_test_entry {
  int version_first; //  First supported version for this entry
  int version_last;  //  Last supported version for this entry

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
WBTester *_tester;
MySQLEditor::Ref _sql_editor;
GrtVersionRef _version;

base::RecMutex _connection_mutex;
sql::Dbc_connection_handler::Ref _conn;
MySQLObjectNamesCache *_cache;
parser::MySQLParserContext::Ref _autocomplete_context;
int version;

public:
TEST_DATA_CONSTRUCTOR(sql_editor_be_autocomplete_tests) : _conn(new sql::Dbc_connection_handler()), _cache(NULL) {
  _tester = new WBTester();
  populate_grt(*_tester);

  // Auto completion needs a cache for object name look up, so we have to set up one
  // with all bells and whistles.
}

// TEST_DATA_DESTRUCTOR(sql_editor_be_autocomplete_tests)
//{
////  _cache->shutdown();
////  delete _cache;
//}

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

TEST_MODULE(sql_editor_be_autocomplete_tests, "SQL code completion tests");

/**
 * Setup for the cache.
 */
TEST_FUNCTION(5) {
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);
  setup_env(connectionProperties);

  sql::DriverManager *dm = sql::DriverManager::getDriverManager();
  _conn->ref = dm->getConnection(connectionProperties);

  std::auto_ptr<sql::Statement> stmt(_conn->ref->createStatement());

  sql::ResultSet *res = stmt->executeQuery("SELECT VERSION() as VERSION");
  if (res && res->next()) {
    std::string version_string = res->getString("VERSION");
    _version = parse_version(version_string);
  }
  delete res;

  ensure("Server version is invalid", _version.is_valid());

  _tester->get_rdbms()->version(_version);
  version = (int)(_version->majorNumber() * 10000 + _version->minorNumber() * 100 + _version->releaseNumber());

  _cache = new MySQLObjectNamesCache(
    std::bind(&Test_object_base<sql_editor_be_autocomplete_tests>::runSQL, this, ph::_1), std::function<void(bool)>());

  // Copy a current version of the code editor configuration file to the test data folder.
  gchar *contents;
  gsize length;
  GError *error = NULL;
  if (g_file_get_contents("../../res/wbdata/code_editor.xml", &contents, &length, &error)) {
    ensure("Could not write editor configuration to target file",
           g_file_set_contents("data/code_editor.xml", contents, length, &error) == TRUE);
    g_free(contents);
  } else
    fail("Could not copy code editor configuration");

  parser::MySQLParserServices::Ref services = parser::MySQLParserServices::get();
  parser::MySQLParserContext::Ref context =
    services->createParserContext(_tester->get_rdbms()->characterSets(), _version, false);

  _autocomplete_context = services->createParserContext(_tester->get_rdbms()->characterSets(), _version, false);

  _sql_editor = MySQLEditor::create(context, _autocomplete_context);
  _sql_editor->set_current_schema("sakila");
  _sql_editor->set_auto_completion_cache(_cache);

  // We don't set up the sakila schema. This is needed in so many places, it should simply exist.
  std::vector<std::string> list = _cache->getMatchingSchemaNames("sakila");
  if (list.empty()) {
    // Sakila not yet fetched. Give it a moment and try again.
    g_usleep(1000000);
    list = _cache->getMatchingSchemaNames("sakila");
  }
  ensure("Could not get the schema list from the cache", !list.empty());

  bool found = false;
  for (std::vector<std::string>::const_iterator i = list.begin(); i != list.end(); ++i) {
    if (*i == "sakila") {
      found = true;
      break;
    }
  }
  ensure("Sakila could not be found", found);
}

/**
 * Another prerequisites test. See that the cache contains needed objects.
 */
TEST_FUNCTION(10) {
  std::vector<std::string> list = _cache->getMatchingSchemaNames("sakila");
  int found = 0;
  for (std::vector<std::string>::const_iterator i = list.begin(); i != list.end(); ++i) {
    if (*i == "sakila" || *i == "mysql")
      found++;
  }
  ensure_equals("Sakila schema missing. Is the DB set up properly?", found, 1);
}

//--------------------------------------------------------------------------------------------------
// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  _sql_editor.reset();
  _cache->shutdown();
  delete _cache;
  delete _tester;
}

END_TESTS
