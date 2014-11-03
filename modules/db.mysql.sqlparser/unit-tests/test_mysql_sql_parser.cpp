/* 
 * Copyright (c) 2011, 2014, Oracle and/or its affiliates. All rights reserved.
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

BEGIN_TEST_DATA_CLASS(mysql_sql_parser)
public:
  WBTester wbt;
  SqlFacade::Ref sql_facade;
  
  ParserContext::Ref context;
  MySQLParserServices::Ref services;

  db_mgmt_RdbmsRef rdbms;
  DictRef options;

  void test_import_sql(int test_no, const char *old_schema_name = NULL, const char *new_schema_name= NULL);

END_TEST_DATA_CLASS

TEST_MODULE(mysql_sql_parser, "SQL Parser (MySQL)");

TEST_FUNCTION(10)
{
  wbt.create_new_document();
  GRT *grt = wbt.grt;

  ensure_equals("loaded physycal model count", wbt.wb->get_document()->physicalModels().count(), 1U);

  options = DictRef(grt);
  options.set("gen_fk_names_when_empty", IntegerRef(0));

  rdbms = wbt.wb->get_document()->physicalModels().get(0)->rdbms();

  sql_facade = SqlFacade::instance_for_rdbms(rdbms);
  ensure("failed to get sqlparser module", (NULL != sql_facade));

  services = MySQLParserServices::get(grt);
  context = MySQLParserServices::createParserContext(rdbms->characterSets(), rdbms->version(), false);
}

//--------------------------------------------------------------------------------------------------

void Test_object_base<mysql_sql_parser>::test_import_sql(int test_no, const char *old_schema_name,
  const char *new_schema_name)
{
  static const char* TEST_DATA_DIR = "data/modules_grt/wb_mysql_import/sql/";

  // Set filenames & messages based on test number.
  std::string number_string = base::to_string(test_no);
  std::string test_message = "SQL (" + number_string + ")";
  std::string test_sql_filename = TEST_DATA_DIR + number_string + ".sql";
  std::string test_catalog_state_filename = TEST_DATA_DIR + number_string + ".xml";
  std::string res_catalog_state_filename = TEST_DATA_DIR + number_string + "_res.xml";

  GRT* grt = rdbms.get_grt();

  // Create and init a new catalog.
  db_mysql_CatalogRef res_catalog(grt);
  res_catalog->version(rdbms->version());
  res_catalog->defaultCharacterSetName("utf8");
  res_catalog->defaultCollationName("utf8_general_ci");
  grt::replace_contents(res_catalog->simpleDatatypes(), rdbms->simpleDatatypes());

  // We have actually 2 parser tests here for now: the old server based parser and the new ANTLR one.
  // Parse the sql with the old parser.
  sql_facade->parseSqlScriptFileEx(res_catalog, test_sql_filename, options);

  // Rename the schema if asked.
  if (old_schema_name && new_schema_name)
    sql_facade->renameSchemaReferences(res_catalog, old_schema_name, new_schema_name);

  // Serialize the catalog to file (not necessary for this test, but for manual checks).
  grt->serialize(res_catalog, res_catalog_state_filename);

  // Unserialize the result so we can compare that with the generated catalog.
  db_CatalogRef test_catalog = db_mysql_CatalogRef::cast_from(ValueRef(grt->unserialize(test_catalog_state_filename)));

  grt_ensure_equals(test_message.c_str(), res_catalog, test_catalog);

  // Same steps as above but using the ANTLR parser.
  // todo
}

// Table
TEST_FUNCTION(20)
{
  for (int i = 0; i < 18; ++i)
    test_import_sql(i);
}

// Index
TEST_FUNCTION(30)
{
  test_import_sql(50);
  test_import_sql(51);
}

// View
TEST_FUNCTION(40)
{
  test_import_sql(100);
  test_import_sql(101);
}

// Routines
TEST_FUNCTION(50)
{
  test_import_sql(150);
  test_import_sql(151);
  test_import_sql(152);
}

// Triggers
TEST_FUNCTION(60)
{
  test_import_sql(200);
}

// Events
TEST_FUNCTION(70)
{
  /*
  for (int i = 250; i < 254; ++i)
    test_import_sql(i);
  */
}

// Other language constructs.
TEST_FUNCTION(80)
{
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

// Real workd schemata (many objects) + other tasks.
TEST_FUNCTION(90)
{
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

END_TESTS
