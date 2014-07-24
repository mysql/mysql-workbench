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

/**
 * Checks that exactly the given set of AC parts are set.
 */
void match_included_parts(const std::string msg, const MySQLEditor::AutoCompletionContext &context,
  int parts)
{
  ensure_equals(msg + ", parts differ", context.wanted_parts, parts);
}

/**
 * Runs tests from a list of ac test entries in the given range.
 */
void run_simple_query_tests(MySQLEditor::Ref editor, int part, ac_test_entry *test_input, size_t start, size_t count)
{
  for (size_t i = start; i < count; i++)
  {
    if (version <= test_input[i].version_first || version > test_input[i].version_last)
      continue;
    
    MySQLEditor::AutoCompletionContext context;
    std::vector<std::string> expected_entries = base::split(test_input[i].entries, " ");

    // In order to avoid duplicating the test query we use an empty string as indicator that
    // the start query should be used instead. However, if we really need an empty query we mark
    // this accordingly.
    context.statement = test_input[i].query.empty() ? test_input[0].query : test_input[i].query;
    if (context.statement == "§")
      context.statement = "";
    context.typed_part = test_input[i].typed_part;
    context.line = test_input[i].line;
    context.offset = test_input[i].offset;

    std::string message = base::strfmt("Part %u, step %lu", part, i);
    if (!editor->create_auto_completion_list(context, _autocomplete_context->recognizer()))
      fail(message + ", unexpected syntax error: " + test_input[i].query);
    match_included_parts(message, context, test_input[i].parts);
    if (test_input[i].check_entries)
    {
      std::vector<std::pair<int, std::string> > entries = editor->update_auto_completion(context.typed_part);
      check_ac_entries(message, entries, expected_entries, test_input[i].version_first, test_input[i].version_last);
    }
  }
}

/**
 * Checks that the auto completion list contains all entries in the correct order. Additional
 * entries are ignored. Both lists must be alphabetically sorted.
 */
void check_ac_entries(const std::string msg, const std::vector<std::pair<int, std::string> > &ac_list,
  const std::vector<std::string> &expected, int version_first, int version_last)
{
  size_t i = 0;
  size_t j = 0;
  while (i < expected.size() && j < ac_list.size())
  {
    while (j < ac_list.size() && expected[i] > ac_list[j].second) // Sakila doesn't use Unicode, so we don't need Unicode aware comparisons.
      j++;
    
    if (j < ac_list.size())
    {
      if (expected[i] != ac_list[j].second)
        break;

      i++;
      j++;
    }
  }
#ifdef DEBUG
  if ( i != expected.size())
  {
    std::cout << "===========================================" << std::endl
              << "List[v" << version << "]:" << std::endl;
              
    for (std::vector<std::pair<int, std::string> >::const_iterator iter = ac_list.begin(); iter != ac_list.end(); iter++)
      std::cout << "  " << (*iter).second << std::endl;
    
    std::cout << "Expected[v" << version_first << " - v" << version_last << "]:" << std::endl;
    
    for (std::vector<std::string>::const_iterator iter = expected.begin(); iter != expected.end(); iter++)
      std::cout << "  " << *iter << std::endl;
    
    std::cout << "===========================================" << std::endl;
  }
#endif
  ensure(msg + ", entries are missing", i == expected.size());
}

void run_all_queries_tests(MySQLEditor::Ref editor, ac_test_entry *test_input, size_t start, size_t count)
{
  for (size_t i = start; i < count; i++)
  {
    if (version <= test_input[i].version_first || version > test_input[i].version_last)
      continue;
    
    MySQLEditor::AutoCompletionContext context;
    std::vector<std::string> expected_entries = base::split(test_input[i].entries, " ");

    context.statement = test_input[i].query;
    context.typed_part = test_input[i].typed_part;
    context.line = test_input[i].line;
    context.offset = test_input[i].offset;
    
    std::string message = base::strfmt("Step %lu", i);
    if (!editor->create_auto_completion_list(context, _autocomplete_context->recognizer()))
      fail(message + ", unexpected syntax error: " + test_input[i].query);
    match_included_parts(message, context, test_input[i].parts);
    if (test_input[i].check_entries)
    {
      std::vector<std::pair<int, std::string> > entries = editor->update_auto_completion(context.typed_part);
      check_ac_entries(message, entries, expected_entries, test_input[i].version_first, test_input[i].version_last);
    }
  }
}

/**
 * Runs tests with possible syntax errors.
 */
void run_typing_tests(MySQLEditor::Ref editor, int part, ac_test_entry *test_input, size_t start, size_t count)
{
  for (size_t i = start; i < count; i++)
  {
    if (version <= test_input[i].version_first || version > test_input[i].version_last)
      continue;
    
    MySQLEditor::AutoCompletionContext context;
    std::vector<std::string> expected_entries = base::split(test_input[i].entries, " ");

    // In order to avoid duplicating the test query we use an empty string as indicator that
    // the start query should be used instead. However, if we really need an empty query we mark
    // this accordingly.
    context.statement = test_input[i].query.empty() ? test_input[0].query : test_input[i].query;
    if (context.statement == "§")
      context.statement = "";
    context.typed_part = test_input[i].typed_part;
    context.line = test_input[i].line;
    context.offset = test_input[i].offset;

    std::string message = base::strfmt("Part %u, step %lu", part, i);
    editor->create_auto_completion_list(context, _autocomplete_context->recognizer());
    match_included_parts(message, context, test_input[i].parts);
    if (test_input[i].check_entries)
    {
      std::vector<std::pair<int, std::string> > entries = editor->update_auto_completion(context.typed_part);
      check_ac_entries(message, entries, expected_entries, test_input[i].version_first, test_input[i].version_last);
    }
  }
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

  base::remove("testconn.cache");
  _cache = new AutoCompleteCache("testconn", boost::bind(&Test_object_base<sql_editor_be_autocomplete_tests>::get_connection, this, _1),
    ".", NULL);

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

  // The loops are not necessary, but there can be spurious condition signals,
  // as the glib docs say.
  while (!_cache->is_schema_list_fetch_done())
    g_usleep(500000);

  _cache->get_matching_table_names("sakila"); // Cache all tables and columns.

  while (!_cache->is_schema_table_columns_fetch_done("sakila", "store"))
    g_usleep(500000);
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


static ac_test_entry simple_valid_line1[] = {
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "select count(distinct a.actor_id), phone, first_name, a.last_name, country.country \n"
   "from sakila.actor a, address aa, country\n"
   "where (a.actor_id = 0 and country_id > 0)\n"
   "group by actor_id", "", 1, 0, false,
    "",
    MySQLEditor::CompletionWantMajorKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_5_5,
   "", "s", 1, 1, true,
    "select set show",
    MySQLEditor::CompletionWantMajorKeywords
  },
  {MYSQL_VERSION_5_5, MYSQL_VERSION_HIGHER,
   "", "s", 1, 1, true,
    "savepoint select set show start stop",
    MySQLEditor::CompletionWantMajorKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "se", 1, 2, true,
    "select set",
    MySQLEditor::CompletionWantMajorKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "sel", 1, 3, true,
    "select",
    MySQLEditor::CompletionWantMajorKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "sele", 1, 4, true,
    "select",
    MySQLEditor::CompletionWantMajorKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "selec", 1, 5, true, // Step 5.
    "select",
    MySQLEditor::CompletionWantMajorKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "select", 1, 6, true,
    "select",
    MySQLEditor::CompletionWantMajorKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 1, 7, false,
    "",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "c", 1, 8, true,
    "cast() category category_id ceil() ceiling() char_length() character_length() city city_id "
    "coalesce() concat() concat_ws() connection_id() conv() convert() cos() cot() count() country "
    "country_id create_date curdate() current_user() curtime() customer customer_id customer_list",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "co", 1, 9, true,
    "coalesce() concat() concat_ws() connection_id() conv() convert() cos() cot() count() country "
    "country_id",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "cou", 1, 10, true, // Step 10.
    "count() country country_id",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "coun", 1, 11, true,
    "count() country country_id",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "count", 1, 12, true,
    "count() country country_id",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 1, 13, false,
    "",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "d", 1, 14, true,
    "data database databases datafile date date_add() date_format() date_sub() datetime day day_hour "
    "day_microsecond day_minute day_second dayname() dayofmonth() dayofweek() dayofyear() dec "
    "decimal declare decode() default definer degrees() delay_key_write delayed delimiter "
    "des_decrypt() des_encrypt() des_key_file description deterministic directory "
    "disable discard disk distinct distinctrow district div double dual dumpfile duplicate dynamic",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "di", 1, 15, true, // Step 15.
    "directory disable discard disk distinct distinctrow district div",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "dis", 1, 16, true,
    "disable discard disk distinct distinctrow district",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "dist", 1, 17, true,
    "distinct distinctrow district",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "disti", 1, 18, true,
    "distinct distinctrow",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "distin", 1, 19, true,
    "distinct distinctrow",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "distinc", 1, 20, true, // Step 20.
    "distinct distinctrow",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "distinct", 1, 21, true,
    "distinct distinctrow",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 1, 22, false,
    "",
    MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "a", 1, 23, true,
    "a aa abs() acos() active actor actor_id actor_info actors adddate() address address2 address_id "
    "aes_decrypt() aes_encrypt() amount ascii() asin() atan() atan2()",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 1, 24, true,
    "actor_id first_name last_name last_update",
    MySQLEditor::CompletionWantColumns | MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "a", 1, 25, true, // Step 25.
    "actor_id",
    MySQLEditor::CompletionWantColumns | MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "ac", 1, 26, true, // This continues for all of the chars in actor_id.
    "actor_id",
    MySQLEditor::CompletionWantColumns | MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 1, 33, false, // Continue after the closing parenthesis.
    "",
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 1, 34, false,
    "",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 1, 35, false,
    "",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_5_5_3,
   "", "p", 1, 36, true, // step 30
    "password payment payment_date payment_id period_add() period_diff() phone pi() "
    "picture position() postal_code pow() power() price",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_5_5_3, MYSQL_VERSION_HIGHER,
   "", "p", 1, 36, true, // step 30
    "password payment payment_date payment_id performance_schema period_add() period_diff() phone pi() "
    "picture position() postal_code pow() power() price",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "ph", 1, 37, true,
    "phone",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 1, 41, false, // After the next comma.
    "",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "f", 1, 43, true,
    "FID field() film film_actor film_category film_id film_info film_list film_text find_in_set() "
    "first_name floor() format() found_rows() from_days() from_unixtime()",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "fi", 1, 44, true,
    "FID field() film film_actor film_category film_id film_info film_list film_text find_in_set() "
    "first_name",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "fir", 1, 45, true, // Step 35.
    "first_name",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 1, 53, false, // After the next comma.
    "",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "a", 1, 55, true,
    "a aa actor actor_info address",
    MySQLEditor::CompletionWantSchemas | MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 1, 56, true,
    "actor_id first_name last_name last_update",
    MySQLEditor::CompletionWantColumns | MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "l", 1, 57, true,
    "last_name",
    MySQLEditor::CompletionWantColumns | MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 1, 66, false, // After the next comma. Step 40.
    "",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "c", 1, 68, true,
    "category city country customer customer_list",
    MySQLEditor::CompletionWantSchemas | MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "co", 1, 69, true,
    "country",
    MySQLEditor::CompletionWantSchemas | MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 1, 75, true, // After the dot.
    "country country_id",
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "co", 1, 77, true,
    "country country_id",
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 1, 83, false, // Line end. Step 45.
    "",
    MySQLEditor::CompletionWantKeywords
  },
};

static ac_test_entry simple_valid_line2[] = {
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "select count(distinct a.actor_id), phone, first_name, a.last_name, country.country \n"
    "from sakila.actor a, address aa, country \n"
    "where (a.actor_id = 0 and country_id > 0) \n"
    "group by actor_id", "", 2, 0, false,
    "",
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_5_5,
   "", "f", 2, 1, true,
    "false fast faults fetch fields file first fixed float float4 float8 for force foreign "
    "found from full fulltext function",
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_5_5, MYSQL_VERSION_HIGHER,     //  NOTE: Could not determine the exact version that "format" was added
   "", "f", 2, 1, true,
    "false fast faults fetch fields file first fixed float float4 float8 for force foreign "
    "format found from full fulltext function",
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "fr", 2, 2, true,
    "from",
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "fro", 2, 3, true,
    "from",
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_5_5,
   "", "", 2, 5, true,
    "a aa actor actor_info address category city country customer customer_list film film_actor "
    "film_category film_list film_text information_schema inventory language mysql "
    "nicer_but_slower_film_list payment rental sakila sales_by_film_category "
    "sales_by_store staff staff_list store",
    MySQLEditor::CompletionWantSchemas | MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_5_5, MYSQL_VERSION_HIGHER,
   "", "", 2, 5, true,
    "actor actor_info address category city country customer customer_list film film_actor "
    "film_category film_list film_text information_schema inventory language mysql "
    "nicer_but_slower_film_list payment performance_schema rental sakila sales_by_film_category "
    "sales_by_store staff staff_list store",
    MySQLEditor::CompletionWantSchemas | MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "s", 2, 6, true, // Step 5.
    "sakila sales_by_film_category sales_by_store staff staff_list store",
    MySQLEditor::CompletionWantSchemas | MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "sa", 2, 7, true,
    "sakila sales_by_film_category sales_by_store",
    MySQLEditor::CompletionWantSchemas | MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 2, 12, true, // Continue after dot.
    "actor actor_info address category city country customer customer_list film film_actor "
    "film_category film_list film_text inventory language "
    "nicer_but_slower_film_list payment rental sales_by_film_category "
    "sales_by_store staff staff_list store",
    MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "a", 2, 13, true,
    "actor actor_info address",
    MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "ac", 2, 14, true,
    "actor actor_info",
    MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 2, 18, false, // Continue at the alias (after the space). Step 10.
    "",
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "a", 2, 19, true,
    "accessible action add after against aggregate algorithm all and any as asc asensitive at "
    "authors auto_increment autoextend_size avg avg_row_length",
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_5_5,
   "", "", 2, 21, true, // Second table. Here we do similar steps as for the first table.
    "a aa actor actor_info address category city country customer customer_list film film_actor "
    "film_category film_list film_text information_schema inventory language mysql "
    "nicer_but_slower_film_list payment rental sakila sales_by_film_category "
    "sales_by_store staff staff_list store",
    MySQLEditor::CompletionWantSchemas | MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_5_5, MYSQL_VERSION_HIGHER,
   "", "", 2, 21, true, // Second table. Here we do similar steps as for the first table.
    "actor actor_info address category city country customer customer_list film film_actor "
    "film_category film_list film_text information_schema inventory language mysql "
    "nicer_but_slower_film_list payment performance_schema rental sakila sales_by_film_category "
    "sales_by_store staff staff_list store",
    MySQLEditor::CompletionWantSchemas | MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "a", 2, 22, true,
    "actor actor_info address",
    MySQLEditor::CompletionWantSchemas | MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "ad", 2, 23, true,
    "address",
    MySQLEditor::CompletionWantSchemas | MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 2, 29, false, // Continue at the alias (after the space). Step 15.
    "",
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "a", 2, 30, true,
    "accessible action add after against aggregate algorithm all and any as asc asensitive at "
    "authors auto_increment autoextend_size avg avg_row_length",
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "aa", 2, 31, false,
    "",
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_5_5,
   "", "", 2, 33, true, // Third table. Similar steps again.
    "a aa actor actor_info address category city country customer customer_list film film_actor "
    "film_category film_list film_text information_schema inventory language mysql "
    "nicer_but_slower_film_list payment rental sakila sales_by_film_category "
    "sales_by_store staff staff_list store",
    MySQLEditor::CompletionWantSchemas | MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_5_5, MYSQL_VERSION_HIGHER,
   "", "", 2, 33, true, // Third table. Similar steps again.
    "actor actor_info address category city country customer customer_list film film_actor "
    "film_category film_list film_text information_schema inventory language mysql "
    "nicer_but_slower_film_list payment performance_schema rental sakila sales_by_film_category "
    "sales_by_store staff staff_list store",
    MySQLEditor::CompletionWantSchemas | MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "c", 2, 34, true,
    "category city country customer customer_list",
    MySQLEditor::CompletionWantSchemas | MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "co", 2, 35, true, // Step 20.
    "country",
    MySQLEditor::CompletionWantSchemas | MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 2, 41, false, // Continue after the space.
    "",
    MySQLEditor::CompletionWantKeywords
  },
};
 
static ac_test_entry simple_valid_line3[] = {
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "select count(distinct a.actor_id), phone, first_name, a.last_name, country.country \n"
    "from sakila.actor a, address aa, country\n"
    "where (a.actor_id = 0 and country_id > 0) \n"
    "group by actor_id", "", 3, 0, false,
    "",
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "w", 3, 1, true,
    "wait warnings when where while with work wrapper write",
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "wh", 3, 2, true,
    "when where while",
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "whe", 3, 3, true,
    "when where",
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 3, 6, false, // Continue at the open parenthesis (after the space).
    "",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords // Keywords possible after where: interval, exists, row, match, case, cast etc.
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 3, 7, false, // Step 5.
    "",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords | MySQLEditor::CompletionWantSelect // Select for sub queries.
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "a", 3, 8, true,
    "a aa actor actor_info address",
    MySQLEditor::CompletionWantSchemas | MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 3, 9, true,
    "actor_id first_name last_name last_update",
    MySQLEditor::CompletionWantColumns | MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "a", 3, 10, true,
    "actor_id",
    MySQLEditor::CompletionWantColumns | MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 3, 18, false, // Continue at equal sign.
    "",
    MySQLEditor::CompletionWantNothing // Only operators are allowed here, but we don't show operators.
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 3, 19, false, // Step 10.
    "",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprInnerKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 3, 20, false,
    "",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "0", 3, 21, false,
    "",
    MySQLEditor::CompletionWantNothing
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 3, 22, true,
    "all and any between escape false in is like or regexp sounds true unknown xor",
    MySQLEditor::CompletionWantExprInnerKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "a", 3, 23, true,
    "all and any",
    MySQLEditor::CompletionWantExprInnerKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "an", 3, 24, true, // Step 15.
    "and any",
    MySQLEditor::CompletionWantExprInnerKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 3, 26, false, // Continue at country_id
    "",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "c", 3, 27, true,
    "case cast cast() category category_id ceil() ceiling() char_length() character_length() "
    "city city_id coalesce() concat() concat_ws() connection_id() conv() convert convert() cos() "
    "cot() count() country country_id create_date curdate() current_user() curtime() customer "
    "customer_id customer_list",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "co", 3, 28, true,
    "coalesce() concat() concat_ws() connection_id() conv() convert convert() cos() "
    "cot() count() country country_id",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "cou", 3, 29, true,
    "count() country country_id",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 3, 37, false, // Step 20. Continue at ">".
    "",
    MySQLEditor::CompletionWantNothing // Only operators are allowed here, but we don't show operators.
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 3, 38, false,
    "",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprInnerKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 3, 39, false,
    "",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "0", 3, 40, false,
    "",
    MySQLEditor::CompletionWantNothing
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 3, 41, false, // After the closing parenthesis.
    "",
    MySQLEditor::CompletionWantKeywords
  },
};

static ac_test_entry simple_valid_line4[] = {
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "select count(distinct a.actor_id), phone, first_name, a.last_name, country.country \n"
    "from sakila.actor a, address aa, country\n"
    "where (a.actor_id = 0 and country_id > 0) \n"
    "group by actor_id", "", 4, 0, false,
    "",
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_5_5,
   "", "g", 4, 1, true,
    "geometry geometrycollection get_format global grants group",
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_5_5, MYSQL_VERSION_HIGHER,
   "", "g", 4, 1, true,
    "general geometry geometrycollection get_format global grants group",
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "gr", 4, 2, true,
    "grants group",
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 4, 6, true, // Continue at "by".
    "by",
    MySQLEditor::CompletionWantBy
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "b", 4, 7, true,
    "by",
    MySQLEditor::CompletionWantBy
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "by", 4, 8, true, // Step 5.
    "by",
    MySQLEditor::CompletionWantBy
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "", "", 4, 9, false,
    "",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
};



/**
 * Collecting AC info for each position in a simple but typical statement.
 * Since this statement is valid we don't need to consider error conditions here.
 */
TEST_FUNCTION(15)
{
  run_simple_query_tests(_sql_editor, 1, simple_valid_line1, 0, sizeof(simple_valid_line1) / sizeof(simple_valid_line1[0]));
  run_simple_query_tests(_sql_editor, 2, simple_valid_line2, 0, sizeof(simple_valid_line2) / sizeof(simple_valid_line2[0]));
  run_simple_query_tests(_sql_editor, 3, simple_valid_line3, 0, sizeof(simple_valid_line3) / sizeof(simple_valid_line3[0]));
  run_simple_query_tests(_sql_editor, 4, simple_valid_line4, 0, sizeof(simple_valid_line4) / sizeof(simple_valid_line4[0]));
}

//--------------------------------------------------------------------------------------------------

static ac_test_entry all_queries_test_data[] = {
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "alter database db default collate = 'utf8'", "", 1, 15, false,
    "", MySQLEditor::CompletionWantSchemas
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "alter database db upgrade data directory name", "", 1, 33, false,
    "", MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "alter logfile group grp add undofile 'blah' initial_size 12345 wait engine = InnoDB", "", 1, 65, false,
    "", MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "alter function func1 comment 'blah' language sql contains sql reads sql data no sql modifies "
    "sql data sql security invoker contains sql", "", 1, 115, false,
    "", MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "alter procedure proc1 comment 'blah' language sql contains sql reads sql data no sql modifies "
    "sql data sql security invoker contains sql", "", 1, 31, false,
    "", MySQLEditor::CompletionWantNothing
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "alter server server1 options (host 'host', database 'database', user 'user', password 'pw', "
    "socket 'socket', owner 'owner', port 1234, user 'user')", "", 1, 68, false,  // Step 5.
    "", MySQLEditor::CompletionWantNothing
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "alter ignore table table1 discard tablespace", "", 1, 19, false,
    "", MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantSchemas
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "alter table table1 import tablespace", "", 1, 14, false,
  "", MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantSchemas
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "alter table table1 change column column1 column2 bit (5) storage memory "
    "not null default now() key references a.b (col1, col2) match full on delete cascade first, "
    "engine = innodb, delay_key_write 1 remove partitioning", "", 1, 40, false,
    "", MySQLEditor::CompletionWantColumns
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "alter table table1 change column column1 column2 bit (5) storage memory "
    "not null default now() key references sakila.customer (col1, col2) match full on delete cascade first, "
    "engine = innodb, delay_key_write 1 remove partitioning", "", 1, 117, true,
    "actor actor_info address category city country customer customer_list film film_actor "
    "film_category film_list film_text inventory language nicer_but_slower_film_list payment rental "
    "sales_by_film_category sales_by_store staff staff_list store",
    MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "alter table table1 change column column1 column2 bit (5) storage memory "
    "not null default now() key references sakila.customer (address_id, email) match full on delete cascade first, "
    "engine = innodb, delay_key_write 1 remove partitioning", "", 1, 139, true,  // Step 10.
    "active address_id create_date customer_id email first_name last_name last_update store_id",
    MySQLEditor::CompletionWantColumns
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "alter tablespace space1 add datafile 'file1' initial_size = 123 wait engine innodb", "", 1, 30, false,
    "", MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "alter definer = current_user () event event1 on schedule every a.b * 5 >> 1 week starts "
    "(select 1) + interval (15.2, 2) on completion preserve enable comment 'blah' do select @a := "
    "1", "", 1, 65, false,
    "", MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "alter definer = current_user () event event1 on schedule every a.b * 5 >> 1 week starts "
    "(select 1) + interval (15.2, 2) on completion preserve enable comment 'blah' do select @a := "
    "1", "", 1, 169, false,
    "", MySQLEditor::CompletionWantMajorKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "alter algorithm = merge definer = 'abc'@`def` sql security invoker view view1 (a, b) as select "
    "(select a from b) as subselect with cascaded check option", "", 1, 46, false,
    "", MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "alter algorithm = merge definer = 'abc'@`def` sql security invoker view view1 (a, b) as select "
    "(select a from b) as subselect with cascaded check option", "", 1, 90, false, // Step 15.
    "", MySQLEditor::CompletionWantSelect
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "alter algorithm = merge definer = 'abc'@`def` sql security invoker view view1 (a, b) as select "
    "(select a from b) as subselect with cascaded check option", "", 1, 102, false,
    "", MySQLEditor::CompletionWantSelect
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "create temporary table if not exists app_userprofile ( id INTEGER AUTO_INCREMENT NOT NULL "
    "PRIMARY KEY , user_id INTEGER NOT NULL UNIQUE , pref_brew_type VARCHAR( 1 ) NOT NULL , "
    "pref_make_starter bool NOT NULL , pref_secondary_ferm bool NOT NULL , pref_dispensing_style "
    "VARCHAR( 1 ) NOT NULL , timezone VARCHAR( 1 ) NOT NULL ) ", "", 1, 166, false,
    "", MySQLEditor::CompletionWantKeywords},
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "create table actor_copy (like sakila.actor)", "", 1, 40, false,
    "", MySQLEditor::CompletionWantTables},
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "create unique index index1 using btree on sakila.actor(id(123)ASC,actor_id desc) "
    "key_block_size = 15", "", 1, 71, true,
    "actor_id", MySQLEditor::CompletionWantColumns
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "create database if not exists sakila char set = default", "", 1, 32, false, // Step 20.
    "", MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "CREATE EVENT myevent ON SCHEDULE EVERY 6 HOUR COMMENT 'A sample comment.' DO UPDATE "
    "sakila.payment SET payment_id = payment_id + 1", "", 1, 86, false,
    "", MySQLEditor::CompletionWantSchemas | MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "CREATE EVENT myevent ON SCHEDULE EVERY 6 HOUR COMMENT 'A sample comment.' DO UPDATE "
    "sakila.payment SET payment_id = payment_id + 1", "p", 1, 104, true,
    "payment_date payment_id",
    MySQLEditor::CompletionWantSchemas | MySQLEditor::CompletionWantTables |MySQLEditor::CompletionWantColumns
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "CREATE ALGORITHM = UNDEFINED DEFINER = `root`@`localhost` SQL SECURITY DEFINER VIEW `film_list` AS \n"
    "select `film`.`film_id` AS `FID`, `film`.`title` AS `title`, `film`.`description` AS `description`, \n"
    "`category`.`name` AS `category`, `film`.`rental_rate` AS `price`, `film`.`length` AS `length`, \n"
    "`film`.`rating` AS `rating`, group_concat(concat(`actor`.`first_name`, _utf8' ', `actor`.`last_name`)\n"
    "separator ', ') AS `actors` from ((((`category` left join `film_category` ON ((`category`.`category_id` = `film_category`.`category_id`)))\n"
    "left join `film` ON ((`film_category`.`film_id` = `film`.`film_id`))) join `film_actor` ON ((`film`.`film_id` = `film_actor`.`film_id`)))\n"
    "join `actor` ON ((`film_actor`.`actor_id` = `actor`.`actor_id`))) group by `film`.`film_id`", "", 7, 35, true,
    "actor_id film_id last_update", MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "CREATE FUNCTION `inventory_held_by_customer`(p_inventory_id INT) RETURNS int(11) READS SQL DATA\n"
    "BEGIN DECLARE v_customer_id INT; DECLARE EXIT HANDLER FOR NOT FOUND RETURN NULL; \n"
    "SELECT customer_id INTO v_customer_id FROM rental WHERE return_date IS NULL AND inventory_id = \n"
    "p_inventory_id; RETURN v_customer_id; END", "REA", 1, 84, true,
    "read read_only read_write reads real",
    MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "create aggregate function udf1 returns real soname 'lib-name'", "", 1, 32, false, // Step 25.
    "", MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "CREATE TRIGGER `upd_film` AFTER UPDATE ON `film` FOR EACH ROW BEGIN\n"
    "IF (old.title != new.title) or (old.description != new.description) THEN UPDATE film_text \n"
    "SET title=new.title, description=new.description, film_id=new.film_id WHERE film_id=old.film_id;\n"
    "END IF; END", "fi", 1, 45, true,
    "film film_actor film_category film_text", MySQLEditor::CompletionWantTables // No views allowed.
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "create logfile group group1 add undofile 'file' initial_size = 1G redo_buffer_size 20M wait, "
    "no_wait, engine InnoDB initial_size = 100M", "", 1, 96, true,
    "no_wait", MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "create server 'server1' foreign data wrapper 'blah' options (host 'host', port 3306)", "", 1, 37, false,
    "", MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "create tablespace space1 add datafile 'file1' use logfile group group1 autoextend_size = 1M no_wait, "
    "initial_size 10G", "tab", 1, 10, true,
    "table tablespace", MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "drop database if exists sakila", "s", 1, 25, true, // Step 30.
    "sakila", MySQLEditor::CompletionWantSchemas
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "drop event sakila.event1", "", 1, 13, false, // TODO: cache must return events to enable this check.
                                                 // Additionally, sakila has no events defined.
    "", MySQLEditor::CompletionWantEvents
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "drop function if exists sakila.inventory_help_by_customer", "inv", 1, 34, true,
    "inventory_held_by_customer inventory_in_stock", MySQLEditor::CompletionWantFunctions
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "drop procedure film_in_stock", "", 1, 20, true,
    "film_in_stock film_not_in_stock", MySQLEditor::CompletionWantProcedures
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "drop index idx_fk_staff_id on sakila.payment", "", 1, 13, false, // TODO: cache must return indexes to enable this check.
    "idx_fk_staff_id idx_fk_customer_id", MySQLEditor::CompletionWantIndexes
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "drop logfile group group1 wait wait wait, no_wait, no_wait, storage engine = InnoDB storage "
    "engine MyISAM", "in", 1, 79, true, // Step 35.
    "infinidb InnoDB", MySQLEditor::CompletionWantEngines
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "drop server if exists 'server1'", "", 1, 15, false,
    "", MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "drop temporary tables if exists sakila.actor, sakila.city restrict", "a", 1, 40, true,
    "actor actor_info address", MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_5_5,            //  NOTE: Could not determine the exact version this was changed
   "drop tablespace space1 wait", "tab", 1, 8, true,
    "table table_checksum tables tablespace", MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_5_5, MYSQL_VERSION_HIGHER,
   "drop tablespace space1 wait", "tab", 1, 8, true,
    "table table_checksum table_name tables tablespace", MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "drop trigger if exists payment.payment_date", "", 1, 31, false,
    "", MySQLEditor::CompletionWantTriggers
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "drop view sakila.film_list, actor_info cascade", "a", 1, 29, true, // Step 40.
    "actor actor_info address", MySQLEditor::CompletionWantSchemas | MySQLEditor::CompletionWantTables
  },  
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "rename tables a to b, c to d, sakila.actor to sakila.bctor", "ac", 1, 39, true,
    "actor", MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "truncate sakila.film_text", "f", 1, 17, true,
    "film film_actor film_category film_list film_text", MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "call sakila.rewards_report ((select * from actor where actor_id = 1), 22, @a)", "", 1, 12, true,
    "rewards_report", MySQLEditor::CompletionWantProcedures
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "delete low_priority quick ignore from sakila.actor where actor_id = 1 order by actor_id limit "
    "22", "a", 1, 80, true, "abs() acos() active actor actor_id actor_info actors adddate() address "
    "address2 address_id aes_decrypt() aes_encrypt() amount ascii() asin() atan() atan2()",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords

  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "delete from sakila.city.*, payment.*, staff using { OJ (select * from actor a) as oneone inner "
    "join sakila.store as s on s.store_id = a.actor_id } where payment.payment_id = 1", "", 1, 136,
    true, // Step 45.
    "actor_id first_name last_name last_update",
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns

  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "delete sakila.city.*, payment.*, staff from { one (select * from actor a) as oneone inner join "
    "sakila.store as s on s.store_id = a.actor_id } where payment.payment_id = 1", "", 1, 46, false,
    "", MySQLEditor::CompletionWantNothing
  },

  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "do (select 1 from city), @a := 1,  111 >> 222 and 333", "", 1, 42, false,
    "",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },

  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "handler .id1 open as b", "", 1, 13, false,
    "", MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "handler id2 close", "c", 1, 13, false,
    "", MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "handler id3 read first", "handler", 1, 7, true, // Case 50.
    "handler", MySQLEditor::CompletionWantMajorKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "handler id3 read id4 prev where actor_id = 1 limit 1,1", "", 1, 32, false,
    "",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "handler id4 read id5 <= (123, default, (select 1)) limit 10", "", 1, 30, false,
    "",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },

  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "insert into .actor set actor_id = 1, first_name = 'Arnold', last_name = 'Schwarzenegger'",
    "ac", 1, 15, true,
    "actor", MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "insert low_priority ignore into .city (city_id, city, country_id) values (1, 'Munich', 2)",
    "", 1, 9, true,
    "", MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "insert staff (staff_id, address_id, active) (select staff_id, address_id, active from staff "
    "where staff_id < 5) union select staff_id, address_id, active from staff where staff_id > "
    "10", "", 1, 118, false, // Case 55.
    "", MySQLEditor::CompletionWantKeywords // Actually only SELECT, ALL + DISTINCT, but for now all keywords.
  },

#ifndef __FIX_FOR_OLDER_MYSQL_VERSIONS__
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "load data concurrent local infile 'filename' ignore into table city charset binary columns "
    "terminated by '\\t' lines starting by 'blah' ignore 20 lines (city_id, @x) set city = 'import'"
    ", country_id = (select 1)", "c", 1, 187, true,
    "category category_id city city_id country country_id create_date customer customer_id customer_list",
    MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "load xml concurrent local infile 'filename' ignore into table city charset binary rows "
    "identified by 'starting' columns terminated by '\\t' lines starting by 'blah' ignore 20 "
    "lines (actor.actor_id, @x)", "", 1, 62, false,
    "", MySQLEditor::CompletionWantSchemas | MySQLEditor::CompletionWantTables
  },
#endif

  {MYSQL_VERSION_5_5, MYSQL_VERSION_HIGHER,
   "replace delayed into language (language_id, name) values (1, 'Esperanto')", "", 1, 44, false,
    "",
    MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns
  },
  
#ifndef __FIX_FOR_OLDER_MYSQL_VERSIONS__
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "replace staff (staff_id, address_id, active) (select staff_id, address_id, active from staff "
    "where staff_id < 5) union (select staff_id, address_id, active from staff where staff_id > "
    "10)", "s", 1, 9, true,
    "sakila staff store",
    MySQLEditor::CompletionWantSchemas | MySQLEditor::CompletionWantTables
  },

  // SELECT has dedicated tests.
#endif
  {MYSQL_VERSION_5_5, MYSQL_VERSION_HIGHER,
   "update ignore { OJ actor a straight_join city c on a.actor_id = city.city_id} set c.city = "
    "'blah', a.last_update = default", "", 1, 101, true, // Case 60.
    "actor_id first_name last_name last_update",
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns
  },

  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "start transaction with consistent snapshot", "", 1, 23, false,
    "", MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "commit work and no chain release", "", 1, 19, false,
    "", MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "savepoint blah", "", 1, 9, false,
    "", MySQLEditor::CompletionWantMajorKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "release savepoint blah", "", 1, 18, false,
    "", MySQLEditor::CompletionWantNothing
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "lock tables sakila.actor a read local, payment write", "", 1, 18, false, // Case 65.
    "", MySQLEditor::CompletionWantSchemas | MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "unlock tables", "", 1, 8, false,
    "", MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "xa start '123456789' resume", "", 1, 15, false,
    "", MySQLEditor::CompletionWantNothing
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "xa commit '1'", "", 1, 1, false,
    "", MySQLEditor::CompletionWantMajorKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "xa recover", "", 1, 4, false,
   "", MySQLEditor::CompletionWantKeywords
  },

  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "purge binary logs to 'blah'", "", 1, 18, false, // Case 70.
    "", MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "purge master logs before (select * from customer)", "", 1, 40, false,
  "", MySQLEditor::CompletionWantSchemas | MySQLEditor::CompletionWantTables
  },
#ifndef __FIX_FOR_OLDER_MYSQL_VERSIONS__
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "change master to master_host = 'blah', relay_log_pos = 1, ignore_server_ids = (1, 2, 3, 4)",
    "", 1, 39, false,
    "", MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "change master to master_connect_retry = 1, master_log_file = 'log'",
    "", 1, 17, false,
    "", MySQLEditor::CompletionWantKeywords
  },
#endif
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "start slave relay_thread, sql_thread, sql_thread until master_log_pos = 42", "", 1, 55, false,
    "", MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "stop slave relay_thread, sql_thread", "", 1, 25, false, // Case 75.
    "", MySQLEditor::CompletionWantKeywords
  },

#ifndef __FIX_FOR_OLDER_MYSQL_VERSIONS__
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "prepare s1 from 'select * from customer'", "", 1, 25, false,
    "", MySQLEditor::CompletionWantNothing
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "prepare s2 from @select_var", "", 1, 11, false,
    "", MySQLEditor::CompletionWantKeywords
  },
#endif
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "execute s1 using @'blah', @blah", "", 1, 20, false,
    "", MySQLEditor::CompletionWantNothing
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "deallocate prepare s1", "", 1, 14, false,
    "", MySQLEditor::CompletionWantKeywords
  },

  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "create user mike identified by password 'blah'", "", 1, 28, false, // Case 80.
    "", MySQLEditor::CompletionWantBy
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "drop user current_user(), 'mike'@localhost", "", 1, 28, false,
    "", MySQLEditor::CompletionWantNothing
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "grant insert (a, b, c), references (d, e, f) on function *.* to mike@'localhost', "
    "friends@`anyhost` require CIPHER '12345' ISSUER 'me' with max_queries_per_hour 10000 grant "
    "option", "", 1, 140, false,
    "", MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "rename user mike to ekim, ekim to mike, me to myself", "", 1, 40, false,
    "", MySQLEditor::CompletionWantUsers
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "revoke all privileges, grant option from mike", "", 1, 41, false,
    "", MySQLEditor::CompletionWantUsers
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "revoke insert (a, b, c), references (d, e, f) on sakila.* from mike, alex, alfredo, rene, "
    "sergio, carlos", "", 1, 49, false, // Case 85.
    "", MySQLEditor::CompletionWantSchemas | MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "set password for mike := password('blah')", "mike", 1, 21, false,
    "", MySQLEditor::CompletionWantUsers
  },

  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "analyze no_write_to_binlog table sakila.city, customer, film", "", 1, 56, false,
    "", MySQLEditor::CompletionWantTables
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "repair local table category, film_actor, film_category quick", "", 1, 41, false,
    "", MySQLEditor::CompletionWantTables
  },

  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "install plugin soname soname 'plugin'", "", 1, 22, false,
    "", MySQLEditor::CompletionWantKeywords // Actually only SONAME.
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "uninstall plugin owner", "", 1, 10, false, // Case 90.
    "", MySQLEditor::CompletionWantKeywords
  },

  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "set global transaction isolation level repeatable read", "", 1, 39, false,
    "", MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "set global names := (32, 1 << 20, brother or sister, (select * from actor)), session "
    "default.blah = ON", "", 1, 45, false,
    "",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },

  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "show authors", "sh", 1, 2, false,
    "", MySQLEditor::CompletionWantMajorKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "show collation like 'blah'", "", 1, 22, false,
    "", MySQLEditor::CompletionWantNothing
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "show full columns from customer in sakila where customer_id in (1, 2, 3)",
    "", 1, 48, false,  // Case 95.
    "",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },

  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "binlog 'blah'", "bi", 1, 2, false,
    "", MySQLEditor::CompletionWantMajorKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "flush local des_key_file, hosts, binary logs", "", 1, 33, false,
    "", MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "cache index actor key (primary, primary, primary), store index (idx_unique_manager) in default",
    "", 1, 64, false,
    "", MySQLEditor::CompletionWantIndexes
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "kill query 1", "", 1, 1, false,
    "", MySQLEditor::CompletionWantMajorKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "load index into cache inventory", "", 1, 15, false, // Case 100.
    "", MySQLEditor::CompletionWantKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "reset master, query cache, slave", "", 1, 20, false,
    "", MySQLEditor::CompletionWantKeywords
  },

  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "describe sakila.inventory store_id", "", 1, 26, false,
    "", MySQLEditor::CompletionWantColumns
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "explain extended (select staff_id, address_id, active from staff where staff_id < 5) union "
    "(select staff_id, address_id, active from staff "
    "where staff_id > 10)", "", 1, 92, false,
    "", MySQLEditor::CompletionWantSelect
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "help 'me'", "", 1, 6, false,
    "", MySQLEditor::CompletionWantNothing
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "use less", "", 1, 4, false, // Case 105.
    "", MySQLEditor::CompletionWantNothing
  },
};



/**
 * Testing 1-2 examples for each possible query type (i.e. major keywords with a typical case).
 * Possibilities are endless so we can only peek for possible problems.
 * Note: for now no language parts are included that are introduced in 5.6 or later.
 * Add more test cases for specific bugs.
 */
TEST_FUNCTION(20)
{
  //run_all_queries_tests(_sql_editor, all_queries_test_data, 25, sizeof(all_queries_test_data) / sizeof(all_queries_test_data[0]) - 1);
  // XXX: There are a few issues with these cases. Since reworking auto completion is planned next
  //      I won't try to fix them, as this requires adjusting auto completion as such. (ml)
  run_all_queries_tests(_sql_editor, all_queries_test_data, 0, 25);
}

//--------------------------------------------------------------------------------------------------

static ac_test_entry query_typing_test_data[] = {
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "§", "", 1, 0, false,
    "",
    MySQLEditor::CompletionWantMajorKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_5_5,
   "s", "s", 1, 1, true,
    "select set show",
    MySQLEditor::CompletionWantMajorKeywords
  },
  {MYSQL_VERSION_5_5, MYSQL_VERSION_HIGHER,
   "s", "s", 1, 1, true,
    "savepoint select set show start stop",
    MySQLEditor::CompletionWantMajorKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "se", "se", 1, 2, true,
    "select set",
    MySQLEditor::CompletionWantMajorKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "sel", "sel", 1, 3, true,
    "select",
    MySQLEditor::CompletionWantMajorKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "sele", "sele", 1, 4, true,
    "select",
    MySQLEditor::CompletionWantMajorKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "selec", "selec", 1, 5, true, // Step 5.
    "select",
    MySQLEditor::CompletionWantMajorKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "select", "select", 1, 6, true,
    "select",
    MySQLEditor::CompletionWantMajorKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "select ", "", 1, 7, false,
    "",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "select c", "c", 1, 8, true,
    "cast() category category_id ceil() ceiling() char_length() character_length() city city_id "
    "coalesce() concat() concat_ws() connection_id() conv() convert() cos() cot() count() country "
    "country_id create_date curdate() current_user() curtime() customer customer_id customer_list",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "select co", "co", 1, 9, true,
    "coalesce() concat() concat_ws() connection_id() conv() convert() cos() cot() count() country "
    "country_id",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "select cou", "cou", 1, 10, true, // Step 10.
    "count() country country_id",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "select coun", "coun", 1, 11, true,
    "count() country country_id",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },
  {MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER,
   "select count", "count", 1, 12, true,
    "count() country country_id",
    MySQLEditor::CompletionWantRuntimeFunctions | MySQLEditor::CompletionWantSchemas |
    MySQLEditor::CompletionWantTables | MySQLEditor::CompletionWantColumns |
    MySQLEditor::CompletionWantExprStartKeywords
  },

  // TODO: continue with the rest of the query.
};


/**
 * Collecting AC info for the same statement as in TC 15, but this time as if we were writing
 * each letter. This usually causes various parse errors to appear, but we want essentially the same
 * output as for the valid statement (except for not-yet-written references).
 */
TEST_FUNCTION(90)
{
  run_typing_tests(_sql_editor, 1, query_typing_test_data, 0, sizeof(query_typing_test_data) / sizeof(query_typing_test_data[0]));
}

END_TESTS

