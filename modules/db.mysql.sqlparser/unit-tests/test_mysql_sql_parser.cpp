/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _WIN32
#include <sstream>
#endif

#include <iostream>
#include <fstream>

#include "grt_test_utility.h"
#include "testgrt.h"
#include "grtsqlparser/sql_facade.h"
#include "wb_helpers.h"
#include "backend/db_mysql_sql_export.h"

#include "grtsqlparser/mysql_parser_services.h"

using namespace parser;

BEGIN_TEST_DATA_CLASS(highlevel_mysql_parser_test)
protected:
WBTester _tester;
SqlFacade::Ref _sqlFacade;

MySQLParserContext::Ref _context;
MySQLParserServices::Ref _services;

DictRef _options;

void test_import_sql(int test_no, const char *old_schema_name = NULL, const char *new_schema_name = NULL);

TEST_DATA_CONSTRUCTOR(highlevel_mysql_parser_test) {
}

END_TEST_DATA_CLASS

TEST_MODULE(highlevel_mysql_parser_test, "High level MySQL parser tests");

TEST_FUNCTION(10) {
  // init datatypes
  populate_grt(_tester);

  _options = DictRef(true);
  _options.set("gen_fk_names_when_empty", IntegerRef(0));

  _sqlFacade = SqlFacade::instance_for_rdbms(_tester.get_rdbms());
  ensure("failed to get sqlparser module", _sqlFacade != NULL);

  _services = MySQLParserServices::get();
  _context = MySQLParserServices::createParserContext(_tester.get_rdbms()->characterSets(),
                                                      _tester.get_rdbms()->version(), false);
}

//--------------------------------------------------------------------------------------------------

void Test_object_base<highlevel_mysql_parser_test>::test_import_sql(int test_no, const char *old_schema_name,
                                                                    const char *new_schema_name) {
  static const char *TEST_DATA_DIR = "data/modules_grt/wb_mysql_import/sql/";

  // Set filenames & messages based on test number.
  std::string number_string = std::to_string(test_no);
  std::string test_message = "SQL (" + number_string + ")";
  std::string test_sql_filename = TEST_DATA_DIR + number_string + ".sql";

  // We have actually 2 parser tests here for now: the old server based parser and the new ANTLR one.
  // First parse the sql with the old parser.
  //
  // The old parser has some inflexibilities (e.g. regarding key/column ordering in a CREATE TABLE)
  // so some tests fail for it now, as we use more complex sql for the new parser. Ignore those for the old parser.
  if (test_no != 8 && test_no != 9 && test_no != 16) {
    // /*
    std::string test_catalog_state_filename = TEST_DATA_DIR + number_string + ".xml";
    std::string res_catalog_state_filename = TEST_DATA_DIR + number_string + "_res.xml";

    if (g_file_test(test_catalog_state_filename.c_str(),
                    G_FILE_TEST_EXISTS)) // Some newer tests are only done for the new parser.
    {
      db_mysql_CatalogRef res_catalog(grt::Initialized);
      res_catalog->version(_tester.get_rdbms()->version());
      res_catalog->defaultCharacterSetName("utf8");
      res_catalog->defaultCollationName("utf8_general_ci");
      grt::replace_contents(res_catalog->simpleDatatypes(), _tester.get_rdbms()->simpleDatatypes());

      _sqlFacade->parseSqlScriptFileEx(res_catalog, test_sql_filename, _options);

      // Rename the schema if asked.
      if (old_schema_name && new_schema_name)
        _sqlFacade->renameSchemaReferences(res_catalog, old_schema_name, new_schema_name);

      // Serialize the catalog to file (not necessary for this test, but for manual checks).
      grt::GRT::get()->serialize(res_catalog, res_catalog_state_filename);

      // Unserialize the result so we can compare that with the generated catalog.
      db_CatalogRef test_catalog =
        db_mysql_CatalogRef::cast_from(ValueRef(grt::GRT::get()->unserialize(test_catalog_state_filename)));

      // Before comparing set the simple data types list to that of the rdbms. Its not part of the
      // parsing process we test here. The test data additionally doesn't contain full lists,
      // so we would get a test failure on that.
      grt::replace_contents(test_catalog->simpleDatatypes(), _tester.get_rdbms()->simpleDatatypes());

      grt_ensure_equals(test_message.c_str(), res_catalog, test_catalog);
    }
    //*/
  }

  // Same steps as above but using the ANTLR parser.
  {
    //*
    std::string test_catalog_state_filename = TEST_DATA_DIR + number_string + "a.xml";
    std::string res_catalog_state_filename = TEST_DATA_DIR + number_string + "a_res.xml";

    db_mysql_CatalogRef res_catalog(grt::Initialized);
    res_catalog->version(_tester.get_rdbms()->version());
    res_catalog->defaultCharacterSetName("utf8");
    res_catalog->defaultCollationName("utf8_general_ci");
    grt::replace_contents(res_catalog->simpleDatatypes(), _tester.get_rdbms()->simpleDatatypes());

    std::string sql = base::getTextFileContent(test_sql_filename);
    tut::ensure("Query failed to parse", _services->parseSQLIntoCatalog(_context, res_catalog, sql, _options) == 0);

    // Rename the schema if asked.
    if (old_schema_name && new_schema_name)
      _services->renameSchemaReferences(_context, res_catalog, old_schema_name, new_schema_name);

    // Serialize the catalog to file (not necessary for this test, but for manual checks).
    grt::GRT::get()->serialize(res_catalog, res_catalog_state_filename);

    // Unserialize the result so we can compare that with the generated catalog.
    db_CatalogRef test_catalog =
      db_mysql_CatalogRef::cast_from(ValueRef(grt::GRT::get()->unserialize(test_catalog_state_filename)));
    grt::replace_contents(test_catalog->simpleDatatypes(), _tester.get_rdbms()->simpleDatatypes());

    grt_ensure_equals(test_message.c_str(), res_catalog, test_catalog);
    //*/
  }
}

// Table
TEST_FUNCTION(20) {
  for (int i = 0; i <= 18; ++i)
    test_import_sql(i);
}

// Index
TEST_FUNCTION(30) {
  test_import_sql(50);
  test_import_sql(51);
}

// View
TEST_FUNCTION(40) {
  test_import_sql(100);
  test_import_sql(101);
}

// Routines
TEST_FUNCTION(50) {
  test_import_sql(150);
  test_import_sql(151);
  test_import_sql(152);
}

// Triggers
TEST_FUNCTION(60) {
  test_import_sql(200);
}

// Events
TEST_FUNCTION(70) {
  for (int i = 250; i < 254; ++i)
    test_import_sql(i);
}

// Other language constructs.
TEST_FUNCTION(80) {
  // Logfile group + table space
  test_import_sql(300);

  // Server link
  test_import_sql(350);

  // Alter statements
  test_import_sql(400);

  // Drop statements
  test_import_sql(450);

  // Re-use of stub tables & columns
  test_import_sql(600);
}

// Real world schemata (many objects) + other tasks.
TEST_FUNCTION(90) {
  // sakila-db: schema structures (except of triggers).
  test_import_sql(700);

  // sakila-db: inserts & triggers
  test_import_sql(701);

  // sakila-db: mysqldump file
  test_import_sql(702);

  // sakila-db: mysqldump file
  test_import_sql(703, "sakila", "new_schema_name");

  // Schema rename.
  test_import_sql(900, "test", "new_schema_name");
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  // delete _tester;
}

END_TESTS
