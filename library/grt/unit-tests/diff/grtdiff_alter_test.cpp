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

#include "testgrt.h"
#include "backend/db_rev_eng_be.h"
#include "module_db_mysql.h"

#include "diff/changelistobjects.h"
#include "grtdb/db_helpers.h"
#include "wb_helpers.h"

using namespace parser;
using namespace tut;

#define VERBOSE_TESTING 0

#define DATABASE_TESTS 1
#define TABLE_TESTS 1
#define TABLE_PARTITION_TESTS 1
#define VIEW_TESTS 1
#define ROUTINE_TESTS 1
#define TRIGGER_TESTS 1
#define CREATE_TESTS 1

BEGIN_TEST_DATA_CLASS(grtdiff_alter_test)
protected:
WBTester *tester;
SqlFacade::Ref sql_parser;
DbMySQLImpl *diffsql_module;
grt::DbObjectMatchAlterOmf omf;
sql::ConnectionWrapper connection;

TEST_DATA_CONSTRUCTOR(grtdiff_alter_test) {
  tester = new WBTester();
  omf.dontdiff_mask = 3;
  diffsql_module = grt::GRT::get()->get_native_module<DbMySQLImpl>();
  ensure("DiffSQLGen module initialization", NULL != diffsql_module);

  // init datatypes
  populate_grt(*tester);

  std::string target_version = bec::GRTManager::get()->get_app_option_string("DefaultTargetMySQLVersion");
  if (target_version.empty())
    target_version = "5.5.49";
  tester->get_rdbms()->version(parse_version(target_version));

  // init database connection
  connection = tester->create_connection_for_import();

  sql_parser = SqlFacade::instance_for_rdbms_name("Mysql");
  ensure("failed to get sqlparser module", (NULL != sql_parser));
}

TEST_DATA_DESTRUCTOR(grtdiff_alter_test) {
  std::auto_ptr<sql::Statement> stmt(connection->createStatement());
  std::string sql_string = "DROP DATABASE IF EXISTS grtdiff_alter_test;";

  execute_script(stmt.get(), sql_string);
}

END_TEST_DATA_CLASS

TEST_MODULE(grtdiff_alter_test, "GRT: diff alter");

// db rev-eng test
TEST_FUNCTION(3) {
  std::list<std::string> schemata;
  schemata.push_back("grtdiff_alter_test");
  grt::GRT::get()->get_undo_manager()->disable();
  db_mysql_CatalogRef cat = tester->db_rev_eng_schema(schemata);
  tester->wb->flush_idle_tasks();
  tester->wb->close_document();
  tester->wb->close_document_finish();
}

static struct {
  const char *description;
  const char *object_name;
  const char *cleanup;
  const char *org;
  const char *mod;
} data[] = {
#if DATABASE_TESTS
  // CREATE and DROP tests
  {"Create database", "grtdiff_alter_test", "DROP DATABASE IF EXISTS grtdiff_alter_test2;",
   "CREATE DATABASE IF NOT EXISTS grtdiff_alter_test;",
   "CREATE DATABASE IF NOT EXISTS grtdiff_alter_test;CREATE DATABASE grtdiff_alter_test2 /*!40100 DEFAULT CHARACTER "
   "SET latin1 */;"},
  {"Drop database", "grtdiff_alter_test", "DROP DATABASE IF EXISTS grtdiff_alter_test2;",
   "CREATE DATABASE IF NOT EXISTS grtdiff_alter_test;CREATE DATABASE grtdiff_alter_test2 /*!40100 DEFAULT CHARACTER "
   "SET latin1 */;",
   "CREATE DATABASE IF NOT EXISTS grtdiff_alter_test;"},
//// ALTER DATABASE tests
//{
//  "alter CHARACTER SET",
//  "grtdiff_alter_test",
//  "DROP DATABASE IF EXISTS grtdiff_alter_test",
//  "CREATE DATABASE grtdiff_alter_test DEFAULT CHARACTER SET 'utf8'",
//  "CREATE DATABASE grtdiff_alter_test DEFAULT CHARACTER SET 'latin1'"
//}, {
//  "alter COLLATE",
//  "grtdiff_alter_test",
//  "DROP DATABASE IF EXISTS grtdiff_alter_test",
//  "CREATE DATABASE grtdiff_alter_test DEFAULT COLLATE 'utf8_general_ci'",
//  "CREATE DATABASE grtdiff_alter_test DEFAULT COLLATE 'utf8_general_cs'"
//}, {
//  "alter CHARACTER SET and COLLATE",
//  "grtdiff_alter_test",
//  "DROP DATABASE IF EXISTS grtdiff_alter_test",
//  "CREATE DATABASE grtdiff_alter_test DEFAULT CHARACTER SET 'utf8' COLLATE 'utf8_general_ci'",
//  "CREATE DATABASE grtdiff_alter_test DEFAULT CHARACTER SET 'latin1' COLLATE 'utf8_general_cs'"
//},
#endif
#if 0 // CREATE TABLE tests. TODO: add more tests, add DROP TABLE tests
    {
      "T+",
      "grtdiff_alter_test.t1",
      "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
      "CREATE DATABASE IF NOT EXISTS grtdiff_alter_test",
      "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB DEFAULT CHARSET=latin1"
    },
#endif
#if TABLE_TESTS
  // ALTER TABLE tests (ADD COLUMN)
  {"C C C+", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, t TEXT) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1"},
  {"C C C+ C+", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, t TEXT, t2 TEXT) "
   "ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"C+ C C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (t TEXT, `id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1"},
  {"C+ C C+ C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (t TEXT, `id` int(11) DEFAULT NULL, t2 TEXT, `id2` int(11) DEFAULT NULL) "
   "ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"C C+ C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t TEXT, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1"},
  {"C C+ C C+", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t TEXT, `id2` int(11) DEFAULT NULL, t2 TEXT) "
   "ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"C C+ C C+ C+", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t TEXT, `id2` int(11) DEFAULT NULL, t2 TEXT, t3 "
   "TEXT) ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"C C+ C+ C C+ C+", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t TEXT, t2 TEXT, `id2` int(11) DEFAULT NULL, t3 "
   "TEXT, t4 TEXT) ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"C+ C+ C C+ C+ C C+ C+", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (t TEXT, t2 TEXT, `id` int(11) DEFAULT NULL, t3 TEXT, t4 TEXT, `id2` int(11) "
   "DEFAULT NULL, t5 TEXT, t6 TEXT) ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  // ALTER TABLE tests (DROP COLUMN)
  {"C C C-", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, t TEXT) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1"},
  {"C C C- C-", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, t TEXT, t2 TEXT) "
   "ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1"},
  {"C- C C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (t TEXT, `id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1"},
  {"C- C C- C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (t TEXT, `id` int(11) DEFAULT NULL, t2 TEXT, `id2` int(11) DEFAULT NULL) "
   "ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1"},
  {"C C- C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t TEXT, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1"},
  {"C C- C C-", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t TEXT, `id2` int(11) DEFAULT NULL, t2 TEXT) "
   "ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1"},
  {"C C- C C- C-", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t TEXT, `id2` int(11) DEFAULT NULL, t2 TEXT, t3 "
   "TEXT) ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1"},
  {"C C- C- C C- C-", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t TEXT, t2 TEXT, `id2` int(11) DEFAULT NULL, t3 "
   "TEXT, t4 TEXT) ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1"},
  {"C- C- C C- C- C C- C-", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (t TEXT, t2 TEXT, `id` int(11) DEFAULT NULL, t3 TEXT, t4 TEXT, `id2` int(11) "
   "DEFAULT NULL, t5 TEXT, t6 TEXT) ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1"},
// ALTER TABLE tests (ADD/DROP COLUMN  mix)
#if 0 // crash due to server bug #31145
    {
      "C- C+ C C",
      "grtdiff_alter_test.t1",
      "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
      "CREATE TABLE grtdiff_alter_test.t1 (t TEXT, `id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT CHARSET=latin1",
      "CREATE TABLE grtdiff_alter_test.t1 (t2 TEXT, `id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT CHARSET=latin1"
    },
#endif
  {"C- C- C+ C C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (t TEXT, t2 TEXT, `id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) "
   "ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (t3 TEXT, `id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1"},
  {"C+ C+ C- C C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (t TEXT, `id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (t2 TEXT, t3 TEXT, `id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) "
   "ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"C- C C+ C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (t TEXT, `id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t2 TEXT, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1"},
  {"C- C C- C+ C C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (t TEXT, `id` int(11) DEFAULT NULL, t2 TEXT, `id2` int(11) DEFAULT NULL) "
   "ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t3 TEXT, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1"},
  {"C+ C C- C C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t2 TEXT, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (t TEXT, `id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1"},
  {"C+ C C+ C- C C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t2 TEXT, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (t TEXT, `id` int(11) DEFAULT NULL, t3 TEXT, t4 TEXT, `id2` int(11) DEFAULT "
   "NULL) ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"C- C C C+", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (t TEXT, `id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, t2 TEXT) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1"},
  {"C- C C C- C+ C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (t TEXT, `id` int(11) DEFAULT NULL, t2 TEXT, `id2` int(11) DEFAULT NULL) "
   "ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t3 TEXT, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1"},
  {"C+ C C C- C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, t TEXT, `id3` int(11) "
   "DEFAULT NULL) ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (t2 TEXT, `id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, `id3` int(11) "
   "DEFAULT NULL) ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"C C- C C+", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t TEXT, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, t2 TEXT) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1"},
  {"C C- C C+", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t TEXT, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, t2 TEXT) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1"},
  {"C C- C C- C+ C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t TEXT, `id2` int(11) DEFAULT NULL, t2 TEXT, `id3` "
   "int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, t3 TEXT, `id3` int(11) "
   "DEFAULT NULL) ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"C C+ C C- C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, t2 TEXT, `id3` int(11) "
   "DEFAULT NULL) ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t TEXT, `id2` int(11) DEFAULT NULL, `id3` int(11) "
   "DEFAULT NULL) ENGINE=InnoDB DEFAULT CHARSET=latin1"},
#if 0 // crash due to server bug #31145
    {
      "C C C- C+",
      "grtdiff_alter_test.t1",
      "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
      "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, t TEXT) ENGINE=InnoDB DEFAULT CHARSET=latin1",
      "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, t2 TEXT) ENGINE=InnoDB DEFAULT CHARSET=latin1"
    },
#endif
  {"C C C- C- C+", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, t TEXT, t2 TEXT) "
   "ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, t3 TEXT) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1"},
  // ALTER TABLE tests (CHANGE COLUMN position change, content change)
  {"C> C C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (t TEXT, `id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t TEXT, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1"},
  {"C C> C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t TEXT, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, t TEXT) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1"},
  {"C C< C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t TEXT, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (t TEXT, `id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1"},
  {"C*", // a test for presicion/scale
   "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` DECIMAL) ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` DECIMAL(8,2)) ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"C C*> C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t TEXT, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, t TEXT NOT NULL) "
   "ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"C C* C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t TEXT, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) "
   "ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"C* C C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (t TEXT, `id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (t int(11) DEFAULT NULL, `id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) "
   "ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"C*> C C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (t TEXT, `id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) "
   "ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"C C*> C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t TEXT, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, t int(11) DEFAULT NULL) "
   "ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"C C*< C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t TEXT, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (t int(11) DEFAULT NULL, `id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) "
   "ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"C CR C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t TEXT, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t2 TEXT, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1"},
  {"C CR> C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t TEXT, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, t2 TEXT) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1"},
  {"C CR< C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t TEXT, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (t2 TEXT, `id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1"},
  {"C CR*> C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t TEXT, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, t2 TEXT) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1"},
  {"C CR*< C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, t TEXT, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (t2  int(11) DEFAULT NULL, `id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT "
   "NULL) ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"C> C C+", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id2` int(11) DEFAULT NULL, `id` int(11) DEFAULT NULL, t TEXT) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1"},
  {"C< C+ C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id2` int(11) DEFAULT NULL, t TEXT, `id` int(11) DEFAULT NULL) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1"},
  {"C> C- C", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, t TEXT) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (t TEXT, `id` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  // TODO: test USING HASH and other indexKind values
  {"I(1)+", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, KEY `idx1` (`id`)) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1"},
  {"I(1)+ I(1)+", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, KEY `idx1` (`id`), KEY `idx2` (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1"},
  {"I(2)+", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, KEY `idx1` (`id`, "
   "`id2`)) ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"I(2)+ I(2)+", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, KEY `idx1` (`id`, "
   "`id2`), KEY `idx2` (`id`, `id2`)) ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"I(2)-", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, KEY `idx1` (`id`, "
   "`id2`)) ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1"},
  {"I(2)- I(2)-", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, KEY `idx1` (`id`, "
   "`id2`), KEY `idx2` (`id`, `id2`)) ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1"},
  {"I(2)- I(2)- I(2)+ I(2)+", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, KEY `idx1` (`id`, "
   "`id2`), KEY `idx2` (`id`, `id2`)) ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, KEY `idx3` (`id`, "
   "`id2`), KEY `idx4` (`id`, `id2`)) ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"I(2)*", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, KEY `idx1` (`id`, "
   "`id2`)) ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, KEY `idx2` (`id`)) "
   "ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"I(1)* I(2)*", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, KEY `idx1` (`id`), KEY "
   "`idx2` (`id`, `id2`)) ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, KEY `idx1` (`id`, "
   "`id2`), KEY `idx2` (`id`)) ENGINE=InnoDB DEFAULT CHARSET=latin1"},
#if 0 // TODO: fix
    {
      "I(1)* I(2)*<",
      "grtdiff_alter_test.t1",
      "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
      "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, KEY `idx1` (`id`), KEY `idx2` (`id`, `id2`)) ENGINE=InnoDB DEFAULT CHARSET=latin1",
      "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) DEFAULT NULL, `id2` int(11) DEFAULT NULL, KEY `idx2` (`id`), KEY `idx1` (`id`, `id2`)) ENGINE=InnoDB DEFAULT CHARSET=latin1"
    },
#endif
  {"PK(2)+", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', `id2` int(11) NOT NULL DEFAULT '0') "
   "ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', `id2` int(11) NOT NULL DEFAULT '0', PRIMARY "
   "KEY (`id`, `id2`)) ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"PK(2)-", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', `id2` int(11) NOT NULL DEFAULT '0', PRIMARY "
   "KEY (`id`, `id2`)) ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', `id2` int(11) NOT NULL DEFAULT '0') "
   "ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"PK(2)*", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', `id2` int(11) NOT NULL DEFAULT '0', PRIMARY "
   "KEY (`id`, `id2`)) ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', `id2` int(11) NOT NULL DEFAULT '0', PRIMARY "
   "KEY (`id`)) ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"PK(2)+ I(2)+", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', `id2` int(11) NOT NULL DEFAULT '0') "
   "ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', `id2` int(11) NOT NULL DEFAULT '0', PRIMARY "
   "KEY (`id`, `id2`), KEY `idx1` (`id`, `id2`)) ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"PK(2)- I(2)-", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', `id2` int(11) NOT NULL DEFAULT '0', PRIMARY "
   "KEY (`id`, `id2`), KEY `idx1` (`id`, `id2`)) ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', `id2` int(11) NOT NULL DEFAULT '0') "
   "ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"PK(2)* I(2)*", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', `id2` int(11) NOT NULL DEFAULT '0', PRIMARY "
   "KEY (`id`, `id2`), KEY `idx1` (`id`, `id2`)) ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', `id2` int(11) NOT NULL DEFAULT '0', PRIMARY "
   "KEY (`id`), KEY `idx1` (`id`)) ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"F+", "grtdiff_alter_test.t1",
   "DROP TABLE IF EXISTS grtdiff_alter_test.t1; DROP TABLE IF EXISTS grtdiff_alter_test.ref_t1;",

   "CREATE TABLE grtdiff_alter_test.ref_t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1;",

   "CREATE TABLE grtdiff_alter_test.ref_t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`), CONSTRAINT `c1` FOREIGN "
   "KEY (`id`) REFERENCES `grtdiff_alter_test`.`ref_t1` (`id`) ) ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"F+ F+", "grtdiff_alter_test.t1",
   "DROP TABLE IF EXISTS grtdiff_alter_test.t1; DROP TABLE IF EXISTS grtdiff_alter_test.ref_t1; DROP TABLE IF EXISTS "
   "grtdiff_alter_test.ref_t2;",

   "CREATE TABLE grtdiff_alter_test.ref_t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.ref_t2 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1;",

   "CREATE TABLE grtdiff_alter_test.ref_t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.ref_t2 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`), CONSTRAINT `c1` FOREIGN "
   "KEY (`id`) REFERENCES `grtdiff_alter_test`.`ref_t1` (`id`), CONSTRAINT `c2` FOREIGN KEY (`id`) REFERENCES "
   "`grtdiff_alter_test`.`ref_t2` (`id`) ) ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"F-", "grtdiff_alter_test.t1",
   "DROP TABLE IF EXISTS grtdiff_alter_test.t1; DROP TABLE IF EXISTS grtdiff_alter_test.ref_t1;",

   "CREATE TABLE grtdiff_alter_test.ref_t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`), CONSTRAINT `c1` FOREIGN "
   "KEY (`id`) REFERENCES `grtdiff_alter_test`.`ref_t1` (`id`) ) ENGINE=InnoDB DEFAULT CHARSET=latin1",

   "CREATE TABLE grtdiff_alter_test.ref_t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1;"},
  {"F- F-", "grtdiff_alter_test.t1",
   "DROP TABLE IF EXISTS grtdiff_alter_test.t1; DROP TABLE IF EXISTS grtdiff_alter_test.ref_t1; DROP TABLE IF EXISTS "
   "grtdiff_alter_test.ref_t2;",

   "CREATE TABLE grtdiff_alter_test.ref_t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.ref_t2 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`), CONSTRAINT `c1` FOREIGN "
   "KEY (`id`) REFERENCES `grtdiff_alter_test`.`ref_t1` (`id`), CONSTRAINT `c2` FOREIGN KEY (`id`) REFERENCES "
   "`grtdiff_alter_test`.`ref_t2` (`id`) ) ENGINE=InnoDB DEFAULT CHARSET=latin1",

   "CREATE TABLE grtdiff_alter_test.ref_t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.ref_t2 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1;"},
  {"F+ F-", "grtdiff_alter_test.t1",
   "DROP TABLE IF EXISTS grtdiff_alter_test.t1; DROP TABLE IF EXISTS grtdiff_alter_test.ref_t1; DROP TABLE IF EXISTS "
   "grtdiff_alter_test.ref_t2;",

   "CREATE TABLE grtdiff_alter_test.ref_t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.ref_t2 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`), CONSTRAINT `c1` FOREIGN "
   "KEY (`id`) REFERENCES `grtdiff_alter_test`.`ref_t1` (`id`)) ENGINE=InnoDB DEFAULT CHARSET=latin1",

   "CREATE TABLE grtdiff_alter_test.ref_t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.ref_t2 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`), CONSTRAINT `c1` FOREIGN "
   "KEY (`id`) REFERENCES `grtdiff_alter_test`.`ref_t2` (`id`)) ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"F+ F-", "grtdiff_alter_test.t1",
   "DROP TABLE IF EXISTS grtdiff_alter_test.t1; DROP TABLE IF EXISTS grtdiff_alter_test.ref_t1; DROP TABLE IF EXISTS "
   "grtdiff_alter_test.ref_t2;",

   "CREATE TABLE grtdiff_alter_test.ref_t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.ref_t2 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`), CONSTRAINT `c1` FOREIGN "
   "KEY (`id`) REFERENCES `grtdiff_alter_test`.`ref_t1` (`id`)) ENGINE=InnoDB DEFAULT CHARSET=latin1",

   "CREATE TABLE grtdiff_alter_test.ref_t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.ref_t2 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`), CONSTRAINT `c2` FOREIGN "
   "KEY (`id`) REFERENCES `grtdiff_alter_test`.`ref_t2` (`id`)) ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"F+ F-", "grtdiff_alter_test.t1",
   "DROP TABLE IF EXISTS grtdiff_alter_test.t1; DROP TABLE IF EXISTS grtdiff_alter_test.ref_t1; DROP TABLE IF EXISTS "
   "grtdiff_alter_test.ref_t2;",

   "CREATE TABLE grtdiff_alter_test.ref_t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.ref_t2 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`), CONSTRAINT `c1` FOREIGN "
   "KEY (`id`) REFERENCES `grtdiff_alter_test`.`ref_t1` (`id`)) ENGINE=InnoDB DEFAULT CHARSET=latin1",

   "CREATE TABLE grtdiff_alter_test.ref_t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.ref_t2 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`), CONSTRAINT `c2` FOREIGN "
   "KEY (`id`) REFERENCES `grtdiff_alter_test`.`ref_t1` (`id`)) ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"F+ F+ F-", "grtdiff_alter_test.t1",
   "DROP TABLE IF EXISTS grtdiff_alter_test.t1; DROP TABLE IF EXISTS grtdiff_alter_test.ref_t1; DROP TABLE IF EXISTS "
   "grtdiff_alter_test.ref_t2;",

   "CREATE TABLE grtdiff_alter_test.ref_t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.ref_t2 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`), CONSTRAINT `c1` FOREIGN "
   "KEY (`id`) REFERENCES `grtdiff_alter_test`.`ref_t1` (`id`)) ENGINE=InnoDB DEFAULT CHARSET=latin1",

   "CREATE TABLE grtdiff_alter_test.ref_t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.ref_t2 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`), CONSTRAINT `c2` FOREIGN "
   "KEY (`id`) REFERENCES `grtdiff_alter_test`.`ref_t1` (`id`), CONSTRAINT `c1` FOREIGN KEY (`id`) REFERENCES "
   "`grtdiff_alter_test`.`ref_t2` (`id`)) ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"F+ F+ F- (2)", "grtdiff_alter_test.t1",
   "DROP TABLE IF EXISTS grtdiff_alter_test.t1; DROP TABLE IF EXISTS grtdiff_alter_test.ref_t1; DROP TABLE IF EXISTS "
   "grtdiff_alter_test.ref_t2;",

   "CREATE TABLE grtdiff_alter_test.ref_t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.ref_t2 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`), CONSTRAINT `c2` FOREIGN "
   "KEY (`id`) REFERENCES `grtdiff_alter_test`.`ref_t1` (`id`), CONSTRAINT `c1` FOREIGN KEY (`id`) REFERENCES "
   "`grtdiff_alter_test`.`ref_t2` (`id`)) ENGINE=InnoDB DEFAULT CHARSET=latin1",

   "CREATE TABLE grtdiff_alter_test.ref_t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.ref_t2 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`)) ENGINE=InnoDB "
   "DEFAULT CHARSET=latin1;"
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`id`), CONSTRAINT `c1` FOREIGN "
   "KEY (`id`) REFERENCES `grtdiff_alter_test`.`ref_t1` (`id`)) ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"Rename table", "grtdiff_alter_test.t1",
   "DROP TABLE IF EXISTS grtdiff_alter_test.t1; DROP TABLE IF EXISTS grtdiff_alter_test.t2;",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t2 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"Change ENGINE attribute", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1"},
  {"Change COMMENT attribute", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",
   "CREATE DATABASE IF NOT EXISTS grtdiff_alter_test;",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1 COMMENT='some comment'"},
  {// this attribute is currently ignored during comparison
   //  "Change AUTO_INCREMENT attribute",
   //  "grtdiff_alter_test.t1",
   //  "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",
   //  "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1",
   //  "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1
   //  AUTO_INCREMENT = 2"
   //}, {
   "Change DELAY_KEY_WRITE attribute", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=InnoDB DEFAULT CHARSET=latin1 "
   "DELAY_KEY_WRITE = 1"},
  {
#if 0 // TODO: need a fix from rev-eng

      "Change UNION, INSERT_METHOD attribute",
      "grtdiff_alter_test.t1",
      "DROP TABLE IF EXISTS grtdiff_alter_test.t1; DROP TABLE IF EXISTS grtdiff_alter_test.t2; DROP TABLE IF EXISTS grtdiff_alter_test.t3;",
      "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1;"
      "CREATE TABLE grtdiff_alter_test.t2 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1;"
      "CREATE TABLE grtdiff_alter_test.t3 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1;"
      ,
      "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1;"
      "CREATE TABLE grtdiff_alter_test.t2 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1;"
      "CREATE TABLE grtdiff_alter_test.t3 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MERGE UNION=(grtdiff_alter_test.t1,grtdiff_alter_test.t2) INSERT_METHOD=LAST DEFAULT CHARSET=latin1;"
    }, {
#endif
    "Change CHARACTER SET attribute", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",
    "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1",
    "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=utf8"},
  {"Change COLLATE attribute", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=utf8 "
   "COLLATE=utf8_unicode_ci",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=utf8 "
   "COLLATE=utf8_bin"},
  {"Change PACK_KEYS attribute", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1 "
   "PACK_KEYS = 1"},
  {"Change CHECKSUM attribute", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1 "
   "CHECKSUM = 1"},
  {"Change ROW_FORMAT attribute", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1 "
   "ROW_FORMAT = COMPRESSED"},
  {"Change AVG_ROW_LENGTH attribute", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1 "
   "AVG_ROW_LENGTH = 101"},
  {"Change MIN_ROWS attribute", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1 "
   "MIN_ROWS = 2"},
  {"Change MAX_ROWS attribute", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1 "
   "MAX_ROWS = 200"},
  {"a test for bug #35265", "grtdiff_alter_test.table1",
   "DROP TABLE IF EXISTS grtdiff_alter_test.table2; DROP TABLE IF EXISTS grtdiff_alter_test.table1;",
   "CREATE  TABLE IF NOT EXISTS `grtdiff_alter_test`.`table2` ("
   " `idtable2` INT(11) NOT NULL,"
   " PRIMARY KEY (`idtable2`))"
   " ENGINE = InnoDB DEFAULT CHARSET=latin1;"
   "CREATE  TABLE IF NOT EXISTS `grtdiff_alter_test`.`table1` ("
   " `idtable1` INT(11) NOT NULL ,"
   " `table2_idtable2` INT(11) NULL ,"
   " PRIMARY KEY (`idtable1`) ,"
   " INDEX fk_table1_table2 (`table2_idtable2` ASC) ,"
   " CONSTRAINT `fk_table1_table2`"
   " FOREIGN KEY (`table2_idtable2` )"
   " REFERENCES `grtdiff_alter_test`.`table2` (`idtable2` )"
   " ON DELETE NO ACTION"
   " ON UPDATE NO ACTION)"
   " ENGINE = InnoDB DEFAULT CHARSET=latin1;",
   "CREATE  TABLE IF NOT EXISTS `grtdiff_alter_test`.`table1` ("
   " `idtable1` INT(11) NOT NULL ,"
   " PRIMARY KEY (`idtable1`) )"
   " ENGINE = InnoDB DEFAULT CHARSET=latin1;"
   "CREATE  TABLE IF NOT EXISTS `grtdiff_alter_test`.`table2` ("
   " `idtable2` INT(11) NOT NULL ,"
   " `table1_idtable1` INT(11) NULL ,"
   " PRIMARY KEY (`idtable2`) ,"
   " INDEX fk_table2_table1 (`table1_idtable1` ASC) ,"
   " CONSTRAINT `fk_table2_table1`"
   " FOREIGN KEY (`table1_idtable1` )"
   " REFERENCES `grtdiff_alter_test`.`table1` (`idtable1` )"
   " ON DELETE NO ACTION"
   " ON UPDATE NO ACTION)"
   " ENGINE = InnoDB DEFAULT CHARSET=latin1;"},
  {"a test for bug #36674", "grtdiff_alter_test.table1",
   "DROP TABLE IF EXISTS grtdiff_alter_test.table2; DROP TABLE IF EXISTS grtdiff_alter_test.table1;",

   "CREATE  TABLE IF NOT EXISTS `grtdiff_alter_test`.`table1` ("
   "  `idtable1` INT(11) NOT NULL ,"
   "  PRIMARY KEY (`idtable1`) )"
   "ENGINE = InnoDB DEFAULT CHARSET=latin1;"
   "CREATE  TABLE IF NOT EXISTS `grtdiff_alter_test`.`table2` ("
   "  `idtable2` INT(11) NOT NULL ,"
   "  `t1` INT(11) NULL ,"
   "  PRIMARY KEY (`idtable2`) )"
   "ENGINE = InnoDB DEFAULT CHARSET=latin1;",
   "CREATE  TABLE IF NOT EXISTS `grtdiff_alter_test`.`table1` ("
   "  `idtable1` INT(11) NOT NULL ,"
   "  PRIMARY KEY (`idtable1`) )"
   "ENGINE = InnoDB DEFAULT CHARSET=latin1;"
   "CREATE  TABLE IF NOT EXISTS `grtdiff_alter_test`.`table2` ("
   "  `idtable2` INT(11) NOT NULL ,"
   "  `t1` INT(11) NULL ,"
   "  PRIMARY KEY (`idtable2`) ,"
   "  INDEX fk_table2_table1 (`t1` ASC) ,"
   "  CONSTRAINT `fk_table2_table1`"
   "    FOREIGN KEY (`t1` )"
   "    REFERENCES `grtdiff_alter_test`.`table1` (`idtable1` )"
   "    ON DELETE NO ACTION"
   "    ON UPDATE NO ACTION)"
   "ENGINE = InnoDB DEFAULT CHARSET=latin1;"},
#endif // end of TABLE_TESTS
#if TABLE_PARTITION_TESTS
  {"Add HASH partitioning", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=InnoDB DEFAULT CHARSET=latin1"
   " PARTITION BY HASH (id) PARTITIONS 6"},
  {"Add KEY partitioning", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (id)) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (id)) ENGINE=InnoDB DEFAULT "
   "CHARSET=latin1"
   " PARTITION BY KEY (id) PARTITIONS 6"},
  {"Drop partitioning", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=InnoDB DEFAULT CHARSET=latin1"
   " PARTITION BY HASH (id) PARTITIONS 6",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"Partition coalescing", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=InnoDB DEFAULT CHARSET=latin1"
   " PARTITION BY HASH (id) PARTITIONS 6",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=InnoDB DEFAULT CHARSET=latin1"
   " PARTITION BY HASH (id) PARTITIONS 4"},
  {"Partition splitting", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=InnoDB DEFAULT CHARSET=latin1"
   " PARTITION BY HASH (id) PARTITIONS 4",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=InnoDB DEFAULT CHARSET=latin1"
   " PARTITION BY HASH (id) PARTITIONS 6"},
  {"Add range partitioning ", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",

   "CREATE TABLE grtdiff_alter_test.t1 (id int(11) NOT NULL DEFAULT '0', hired DATE NOT NULL)"
   " ENGINE=InnoDB DEFAULT CHARSET=latin1",

   "CREATE TABLE grtdiff_alter_test.t1 (id int(11) NOT NULL DEFAULT '0', hired DATE NOT NULL)"
   " ENGINE=InnoDB DEFAULT CHARSET=latin1"
   " PARTITION BY RANGE(YEAR(hired)) ("
   " PARTITION p1 VALUES LESS THAN (1991) ENGINE = InnoDB,"
   " PARTITION p2 VALUES LESS THAN (1996) ENGINE = InnoDB,"
   " PARTITION p3 VALUES LESS THAN (2001) ENGINE = InnoDB,"
   " PARTITION p4 VALUES LESS THAN (2005) ENGINE = InnoDB,"
   " PARTITION p5 VALUES LESS THAN (MAXVALUE) ENGINE = InnoDB)"},
  {"Remove range partitioning ", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",

   "CREATE TABLE grtdiff_alter_test.t1 (id int(11) NOT NULL DEFAULT '0', hired DATE NOT NULL)"
   " ENGINE=InnoDB DEFAULT CHARSET=latin1"
   " PARTITION BY RANGE(YEAR(hired)) ("
   " PARTITION p1 VALUES LESS THAN (1991) ENGINE = InnoDB,"
   " PARTITION p2 VALUES LESS THAN (1996) ENGINE = InnoDB,"
   " PARTITION p3 VALUES LESS THAN (2001) ENGINE = InnoDB,"
   " PARTITION p4 VALUES LESS THAN (2005) ENGINE = InnoDB)",

   "CREATE TABLE grtdiff_alter_test.t1 (id int(11) NOT NULL DEFAULT '0', hired DATE NOT NULL)"
   " ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"Range partitioning: change expression, partition LESS THAN values ", "grtdiff_alter_test.t1",
   "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",

   "CREATE TABLE grtdiff_alter_test.t1 (id int(11) NOT NULL DEFAULT '0', hired DATE NOT NULL)"
   " ENGINE=InnoDB DEFAULT CHARSET=latin1"
   " PARTITION BY RANGE(YEAR(hired)) ("
   " PARTITION p1 VALUES LESS THAN (2) ENGINE = InnoDB,"
   " PARTITION p2 VALUES LESS THAN (5) ENGINE = InnoDB,"
   " PARTITION p3 VALUES LESS THAN (10) ENGINE = InnoDB,"
   " PARTITION p4 VALUES LESS THAN (MAXVALUE) ENGINE = InnoDB)",

   "CREATE TABLE grtdiff_alter_test.t1 (id int(11) NOT NULL DEFAULT '0', hired DATE NOT NULL)"
   " ENGINE=InnoDB DEFAULT CHARSET=latin1"
   " PARTITION BY RANGE(MONTH(hired)) ("
   " PARTITION p1 VALUES LESS THAN (1991) ENGINE = InnoDB,"
   " PARTITION p2 VALUES LESS THAN (1996) ENGINE = InnoDB,"
   " PARTITION p3 VALUES LESS THAN (2001) ENGINE = InnoDB,"
   " PARTITION p4 VALUES LESS THAN (2005) ENGINE = InnoDB)"},
  {"Range partitioning: add partition", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",

   "CREATE TABLE grtdiff_alter_test.t1 (id int(11) NOT NULL DEFAULT '0', hired DATE NOT NULL)"
   " ENGINE=InnoDB DEFAULT CHARSET=latin1"
   " PARTITION BY RANGE(YEAR(hired)) ("
   " PARTITION p1 VALUES LESS THAN (6) ENGINE = InnoDB,"
   " PARTITION p2 VALUES LESS THAN (9) ENGINE = InnoDB)",

   "CREATE TABLE grtdiff_alter_test.t1 (id int(11) NOT NULL DEFAULT '0', hired DATE NOT NULL)"
   " ENGINE=InnoDB DEFAULT CHARSET=latin1"
   " PARTITION BY RANGE(YEAR(hired)) ("
   " PARTITION p1 VALUES LESS THAN (6) ENGINE = InnoDB,"
   " PARTITION p2 VALUES LESS THAN (9) ENGINE = InnoDB,"
   " PARTITION p3 VALUES LESS THAN (MAXVALUE) ENGINE = InnoDB)"},
  {
    "Range partitioning: drop 2 partitions", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",

    "CREATE TABLE grtdiff_alter_test.t1 (id int(11) NOT NULL DEFAULT '0', hired DATE NOT NULL)"
    " ENGINE=InnoDB DEFAULT CHARSET=latin1"
    " PARTITION BY RANGE(YEAR(hired)) ("
    " PARTITION p1 VALUES LESS THAN (6) ENGINE = InnoDB,"
    " PARTITION p2 VALUES LESS THAN (9) ENGINE = InnoDB,"
    " PARTITION p3 VALUES LESS THAN (12) ENGINE = InnoDB,"
    " PARTITION p4 VALUES LESS THAN (MAXVALUE) ENGINE = InnoDB)",

    "CREATE TABLE grtdiff_alter_test.t1 (id int(11) NOT NULL DEFAULT '0', hired DATE NOT NULL)"
    " ENGINE=InnoDB DEFAULT CHARSET=latin1"
    " PARTITION BY RANGE(YEAR(hired)) ("
    " PARTITION p1 VALUES LESS THAN (6) ENGINE = InnoDB,"
    " PARTITION p4 VALUES LESS THAN (MAXVALUE) ENGINE = InnoDB)",
  },
  {
    "Range partitioning: reorganize partitions", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",

    "CREATE TABLE grtdiff_alter_test.t1 (id int(11) NOT NULL DEFAULT '0', hired DATE NOT NULL)"
    " ENGINE=InnoDB DEFAULT CHARSET=latin1"
    " PARTITION BY RANGE(YEAR(hired)) ("
    " PARTITION p1 VALUES LESS THAN (6) ENGINE = InnoDB,"
    " PARTITION p2 VALUES LESS THAN (9) ENGINE = InnoDB,"
    " PARTITION p3 VALUES LESS THAN (12) ENGINE = InnoDB,"
    " PARTITION p4 VALUES LESS THAN (13) ENGINE = InnoDB)",

    "CREATE TABLE grtdiff_alter_test.t1 (id int(11) NOT NULL DEFAULT '0', hired DATE NOT NULL)"
    " ENGINE=InnoDB DEFAULT CHARSET=latin1"
    " PARTITION BY RANGE(YEAR(hired)) ("
    " PARTITION p1 VALUES LESS THAN (6) ENGINE = InnoDB,"
    " PARTITION p2 VALUES LESS THAN (9) ENGINE = InnoDB,"
    " PARTITION p3 VALUES LESS THAN (12) ENGINE = InnoDB,"
    " PARTITION p4 VALUES LESS THAN (14) ENGINE = InnoDB)",
  },
  {
    "List partitioning: reorganize partitions", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",

    "CREATE TABLE grtdiff_alter_test.t1 (id int(11) NOT NULL DEFAULT '0', hired DATE NOT NULL)"
    " ENGINE=InnoDB DEFAULT CHARSET=latin1"
    " PARTITION BY LIST(id) ("
    " PARTITION p0 VALUES IN (5,10,15),"
    " PARTITION p1 VALUES IN (6,12,18),"
    " PARTITION p2 VALUES IN (7,13,19),"
    " PARTITION p3 VALUES IN (8,14,20))",

    "CREATE TABLE grtdiff_alter_test.t1 (id int(11) NOT NULL DEFAULT '0', hired DATE NOT NULL)"
    " ENGINE=InnoDB DEFAULT CHARSET=latin1"
    " PARTITION BY LIST(id) ("
    " PARTITION p0 VALUES IN (5,10,15),"
    " PARTITION p1 VALUES IN (6,12,18),"
    " PARTITION p2 VALUES IN (3,13,19),"
    " PARTITION p3 VALUES IN (8,14,20))",
  },
  {
    "List partitioning: add, remove, change partitions", "grtdiff_alter_test.t1",
    "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",

    "CREATE TABLE grtdiff_alter_test.t1 (id int(11) NOT NULL DEFAULT '0', hired DATE NOT NULL)"
    " ENGINE=InnoDB DEFAULT CHARSET=latin1"
    " PARTITION BY LIST(id) ("
    " PARTITION p0 VALUES IN (5,10,15),"
    " PARTITION p1 VALUES IN (6,12,18),"
    " PARTITION p2 VALUES IN (7,13,19),"
    " PARTITION p3 VALUES IN (8,14,20))",

    "CREATE TABLE grtdiff_alter_test.t1 (id int(11) NOT NULL DEFAULT '0', hired DATE NOT NULL)"
    " ENGINE=InnoDB DEFAULT CHARSET=latin1"
    " PARTITION BY LIST(id) ("
    " PARTITION p0 VALUES IN (5,10,15),"
    " PARTITION p2 VALUES IN (3,13,19),"
    " PARTITION p3 VALUES IN (8,14,20),"
    " PARTITION p4 VALUES IN (6,12,18))",
  },

#endif // end of TABLE_PARTITION_TESTS
#if VIEW_TESTS
  {

    "Create view", "grtdiff_alter_test.v1", "DROP VIEW IF EXISTS grtdiff_alter_test.v1;",
    "CREATE DATABASE IF NOT EXISTS grtdiff_alter_test;",
    "CREATE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL SECURITY DEFINER VIEW `grtdiff_alter_test`.`v1` AS "
    "select 2 AS `2`;"},
  {"Drop view", "grtdiff_alter_test.v1",
   "DROP VIEW IF EXISTS grtdiff_alter_test.v1;DROP TABLE IF EXISTS grtdiff_alter_test.t1;",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1;CREATE "
   "ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL SECURITY DEFINER VIEW `grtdiff_alter_test`.`v1` AS select 2 AS "
   "`2`;",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1;"},
  {"Alter view", "grtdiff_alter_test.v1", "DROP VIEW IF EXISTS grtdiff_alter_test.v1;",
   "CREATE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL SECURITY DEFINER VIEW `grtdiff_alter_test`.`v1` AS "
   "select 1 AS `1`;",
   "CREATE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL SECURITY DEFINER VIEW `grtdiff_alter_test`.`v1` AS "
   "select 2 AS `2`;"},
  {"Rename view", "grtdiff_alter_test.v1",
   "DROP VIEW IF EXISTS grtdiff_alter_test.v1;DROP VIEW IF EXISTS grtdiff_alter_test.v2;",
   "CREATE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL SECURITY DEFINER VIEW `grtdiff_alter_test`.`v1` AS "
   "select 1 AS `1`;",
   "CREATE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL SECURITY DEFINER VIEW `grtdiff_alter_test`.`v2` AS "
   "select 1 AS `1`;"},
#endif // end of VIEW_TESTS
#if ROUTINE_TESTS
  {"Create procedure", "grtdiff_alter_test.p1", "DROP PROCEDURE IF EXISTS grtdiff_alter_test.p1;",
   "CREATE DATABASE IF NOT EXISTS grtdiff_alter_test;",
   "DELIMITER //\nCREATE DEFINER=`root`@`localhost` PROCEDURE `grtdiff_alter_test`.`p1`()\nBEGIN SELECT 1; SELECT 2; "
   "END//"},
  {"Drop procedure", "grtdiff_alter_test.p1", "DROP PROCEDURE IF EXISTS grtdiff_alter_test.p1;",
   "DELIMITER //\nCREATE DEFINER=`root`@`localhost` PROCEDURE `grtdiff_alter_test`.`p1`()\nBEGIN SELECT 1; SELECT 2; "
   "END//",
   "CREATE DATABASE IF NOT EXISTS grtdiff_alter_test;"},
  {"Change procedure", "grtdiff_alter_test.p1", "DROP PROCEDURE IF EXISTS grtdiff_alter_test.p1;",
   "DELIMITER //\nCREATE DEFINER=`root`@`localhost` PROCEDURE `grtdiff_alter_test`.`p1`()\nBEGIN SELECT 1; SELECT 2; "
   "END//",
   "DELIMITER //\nCREATE DEFINER=`root`@`localhost` PROCEDURE `grtdiff_alter_test`.`p1`()\nBEGIN SELECT 2; SELECT 1; "
   "END//"},
#endif // ROUTINE_TESTS
#if TRIGGER_TESTS
  {"Create trigger", "grtdiff_alter_test.tr1",

   "DROP TRIGGER IF EXISTS grtdiff_alter_test.tr1;DROP TABLE IF EXISTS grtdiff_alter_test.t1;",

   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1;\n",

   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1;\n"
   "USE grtdiff_alter_test;\n"
   "DELIMITER //\nCREATE\nDEFINER=`root`@`localhost`\nTRIGGER `grtdiff_alter_test`.`tr1`\nBEFORE INSERT ON "
   "`grtdiff_alter_test`.`t1`\nFOR EACH ROW\nBEGIN DELETE FROM grtdiff_alter_test.t1; END//"},
  {"Drop trigger", "grtdiff_alter_test.tr1",

   "DROP TRIGGER IF EXISTS grtdiff_alter_test.tr1;DROP TABLE IF EXISTS grtdiff_alter_test.t1;",

   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1;\n"
   "USE grtdiff_alter_test;\n"
   "DELIMITER //\nCREATE\nDEFINER=`root`@`localhost`\nTRIGGER `grtdiff_alter_test`.`tr1`\nBEFORE INSERT ON "
   "`grtdiff_alter_test`.`t1`\nFOR EACH ROW\nBEGIN DELETE FROM grtdiff_alter_test.t1; END//",

   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1;\n"},
  {"Change trigger", "grtdiff_alter_test.tr1",

   "DROP TRIGGER IF EXISTS grtdiff_alter_test.tr1;DROP TABLE IF EXISTS grtdiff_alter_test.t1;",

   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1;\n"
   "USE grtdiff_alter_test;\n"
   "DELIMITER //\nCREATE\nDEFINER=`root`@`localhost`\nTRIGGER `grtdiff_alter_test`.`tr1`\nBEFORE INSERT ON "
   "`grtdiff_alter_test`.`t1`\nFOR EACH ROW\nBEGIN DELETE FROM t1; END//",

   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1;\n"
   "USE grtdiff_alter_test;\n"
   "DELIMITER //\nCREATE\nDEFINER=`root`@`localhost`\nTRIGGER `grtdiff_alter_test`.`tr1`\nBEFORE INSERT ON "
   "`grtdiff_alter_test`.`t1`\nFOR EACH ROW\nBEGIN DELETE FROM t2; END//"},
#endif // TRIGGER_TESTS
#ifdef CREATE_TESTS
  {"Create test 1", "grtdiff_alter_test.tr1", "drop table if exists grtdiff_alter_test.create_test_t1;", "",
   "CREATE TABLE grtdiff_alter_test.create_test_t1 ("
   "`id` int(11) NOT NULL DEFAULT '0') DEFAULT CHARSET=latin1 ENGINE=InnoDB;"},
  {"Create test: data types", "grtdiff_alter_test.tr1", "drop table if exists grtdiff_alter_test.types_check;", "",
   "CREATE TABLE grtdiff_alter_test.types_check ("
   "`b4` bit(1) DEFAULT NULL,  "
   "`ti` tinyint(4) DEFAULT NULL,  "
   "`si` smallint(6) DEFAULT NULL,  "
   "`mi` mediumint(9) DEFAULT NULL,  "
   "`i` int(11) DEFAULT NULL,  "
   "`i2` int(11) DEFAULT NULL,  "
   "`bi` bigint(20) DEFAULT NULL, "
   "`r` double DEFAULT NULL,  "
   "`d` double DEFAULT NULL,  "
   "`f` float DEFAULT NULL,  "
   "`dc` decimal(10,0) DEFAULT NULL,  "
   "`num` decimal(10,2) DEFAULT NULL,  "
   "`dt` date DEFAULT NULL,  "
   "`tm` time DEFAULT NULL,"
   "`tmst` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,  "
   "`dttm` datetime DEFAULT NULL,  "
   "`yr` year(4) DEFAULT NULL,  "
   "`ch` char(32) CHARACTER SET utf8 DEFAULT NULL,"
   "`vchr` varchar(128) DEFAULT NULL, "
   "`bnr` binary(32) DEFAULT NULL,  "
   "`vbnr` varbinary(32) DEFAULT NULL,  "
   "`tblb` tinyblob,  "
   "`blb` blob,  "
   "`mblb` mediumblob,  "
   "`lblb` longblob,  "
   "`ttxt` tinytext,  "
   "`mtxt` mediumtext CHARACTER SET latin1 COLLATE latin1_bin,"
   "`enm` enum('one','two','three') DEFAULT NULL,  "
   "`st` set('on','off') DEFAULT NULL) "
   "ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"Create test: keys and indices", "grtdiff_alter_test.tr1", "drop table if exists grtdiff_alter_test.keys_check;", "",
   "CREATE  TABLE IF NOT EXISTS `grtdiff_alter_test`.`keys_check` (`id` INT(11) NOT NULL ,  `txt` VARCHAR(64) NULL "
   "DEFAULT NULL ,  `cnt` INT(11) NOT NULL AUTO_INCREMENT ,  `chk` INT(11) NOT NULL ,  PRIMARY KEY (`id`) ,  UNIQUE "
   "INDEX txt_2 (`txt` ASC) ,  INDEX cnt USING HASH (`cnt` ASC) ,  FULLTEXT INDEX txt (`txt` ASC) ) ENGINE = MyISAM "
   "DEFAULT CHARACTER SET = latin1;"},
  {"Create test: foreign keys", "grtdiff_alter_test.tr1",
   "drop table if exists grtdiff_alter_test.frn_keys_check_t2;drop table if exists "
   "grtdiff_alter_test.frn_keys_check_t1;",
   "",
   "CREATE TABLE grtdiff_alter_test.frn_keys_check_t1( `id` int(11) NOT NULL,  PRIMARY KEY (`id`) USING BTREE) "
   "ENGINE=InnoDB DEFAULT CHARSET=latin1; CREATE TABLE `grtdiff_alter_test`.`frn_keys_check_t2` (`id` int(11) NOT "
   "NULL,  KEY `fid` (`id`),  CONSTRAINT `fid` FOREIGN KEY (`id`) REFERENCES `grtdiff_alter_test`.`frn_keys_check_t1` "
   "(`id`) ON DELETE CASCADE ON UPDATE CASCADE) ENGINE=InnoDB DEFAULT CHARSET=latin1;"},
#endif
  {"Change CHARSET attribute to server default", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin2",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT"},

  {"Change COLLATE attribute to server default", "grtdiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1 "
   "COLLATE=latin1_german1_ci",
   "CREATE TABLE grtdiff_alter_test.t1 (`id` int(11) NOT NULL DEFAULT '0') ENGINE=MyISAM DEFAULT CHARSET=latin1"},
  {NULL, NULL, NULL, NULL, NULL}};

TEST_FUNCTION(10) {
  std::shared_ptr<DiffChange> alter_change;
  std::shared_ptr<DiffChange> empty_change;

  // column insertion

  ensure("connection is NULL", connection.get() != NULL);

  {
    std::auto_ptr<sql::Statement> stmt(connection->createStatement());
    execute_script(stmt.get(),
                   "DROP DATABASE IF EXISTS grtdiff_alter_test;"
                   "DROP DATABASE IF EXISTS grtdiff_alter_test2");
    execute_script(stmt.get(),
                   "CREATE DATABASE IF NOT EXISTS grtdiff_alter_test /*!40100 DEFAULT CHARACTER SET latin1 */");
  }

  MySQLParserServices::Ref services = MySQLParserServices::get();
  parser::MySQLParserContext::Ref context =
    services->createParserContext(tester->get_rdbms()->characterSets(), tester->get_rdbms()->version(), false);
  grt::DictRef options(true);
  for (int i = 0; data[i].description != NULL; i++) {
    std::cout << ".";
    if ((i + 1) % 30 == 0)
      std::cout << std::endl;

    db_mysql_CatalogRef org_cat = create_empty_catalog_for_import();
    db_mysql_CatalogRef mod_cat = create_empty_catalog_for_import();

    {
      std::string org_script;
      org_script.append("CREATE DATABASE IF NOT EXISTS grtdiff_alter_test /*!40100 DEFAULT CHARACTER SET latin1 */;\n")
        .append(data[i].org)
        .append(";");
      services->parseSQLIntoCatalog(context, org_cat, org_script, options);
    }
    {
      std::string mod_script;
      mod_script.append("CREATE DATABASE IF NOT EXISTS grtdiff_alter_test /*!40100 DEFAULT CHARACTER SET latin1 */;\n")
        .append(data[i].mod)
        .append(";");
      services->parseSQLIntoCatalog(context, mod_cat, mod_script, options);
    }

    if (strcmp(data[i].description, "C CR C") == 0)
      mod_cat->schemata().get(0)->tables().get(0)->columns().get(1)->oldName("t");
    else if (strcmp(data[i].description, "C CR> C") == 0)
      mod_cat->schemata().get(0)->tables().get(0)->columns().get(2)->oldName("t");
    else if (strcmp(data[i].description, "C CR< C") == 0)
      mod_cat->schemata().get(0)->tables().get(0)->columns().get(0)->oldName("t");
    else if (strcmp(data[i].description, "C CR*> C") == 0)
      mod_cat->schemata().get(0)->tables().get(0)->columns().get(2)->oldName("t");
    else if (strcmp(data[i].description, "C CR*< C") == 0)
      mod_cat->schemata().get(0)->tables().get(0)->columns().get(0)->oldName("t");
    else if (strcmp(data[i].description, "Rename table") == 0)
      mod_cat->schemata().get(0)->tables().get(0)->oldName("t1");
    else if (strcmp(data[i].description, "Rename view") == 0)
      mod_cat->schemata().get(0)->views().get(0)->oldName("v1");

    grt::NormalizedComparer normalizer;
    normalizer.init_omf(&omf);
    grt::ValueRef default_engine = bec::GRTManager::get()->get_app_option("db.mysql.Table:tableEngine");
    std::string default_engine_name;
    if (grt::StringRef::can_wrap(default_engine))
      default_engine_name = grt::StringRef::cast_from(default_engine);

    bec::CatalogHelper::apply_defaults(mod_cat, default_engine_name);
    bec::CatalogHelper::apply_defaults(org_cat, default_engine_name);

    alter_change = diff_make(org_cat, mod_cat, &omf);
    ensure("Empty alter:", (bool)alter_change);

#if VERBOSE_TESTING
    alter_change->dump_log(0);
#endif

    // 1. generate alter
    grt::StringListRef alter_map(grt::Initialized);
    grt::ListRef<GrtNamedObject> alter_object_list(true);
    grt::DictRef options(true);
    options.set("UseFilteredLists", grt::IntegerRef(0));
    options.set("OutputContainer", alter_map);
    options.set("OutputObjectContainer", alter_object_list);
    options.set("CaseSensitive", grt::IntegerRef(omf.case_sensitive));

    diffsql_module->generateSQL(mod_cat, options, alter_change);
    diffsql_module->makeSQLSyncScript(mod_cat, options, alter_map, alter_object_list);
    std::string export_sql_script = options.get_string("OutputScript");

    // 2. apply it to server
    std::auto_ptr<sql::Statement> stmt(connection->createStatement());

    try {
      execute_script(stmt.get(), data[i].cleanup);
      execute_script(stmt.get(), data[i].org);
      execute_script(stmt.get(), export_sql_script);
    } catch (sql::SQLException &ex) {
      std::cout << "EXCEPTION: " << ex.what() << std::endl;
      std::cout << export_sql_script << std::endl;
      fail("Server returned error:" + std::string(data[i].description));
    }

    // 3. reveng the new catalog
    std::list<std::string> schemata;
    schemata.push_back("grtdiff_alter_test");
    schemata.push_back("grtdiff_alter_test2");
    db_mysql_CatalogRef cat = tester->db_rev_eng_schema(schemata);
    if ((cat->schemata().get(0).is_valid()) && (cat->schemata().get(0)->name() == "mydb"))
      cat->schemata().remove(0);
    mod_cat->oldName("");

    // 3a. cleanup the server
    execute_script(stmt.get(), data[i].cleanup);

    // 4. diff to mod - TEST - must be empty diff
    if (strcmp(data[i].description, "C CR C") == 0)
      mod_cat->schemata().get(0)->tables().get(0)->columns().get(1)->oldName("t2");
    else if (strcmp(data[i].description, "C CR> C") == 0)
      mod_cat->schemata().get(0)->tables().get(0)->columns().get(2)->oldName("t2");
    else if (strcmp(data[i].description, "C CR< C") == 0)
      mod_cat->schemata().get(0)->tables().get(0)->columns().get(0)->oldName("t2");
    else if (strcmp(data[i].description, "C CR*> C") == 0)
      mod_cat->schemata().get(0)->tables().get(0)->columns().get(2)->oldName("t2");
    else if (strcmp(data[i].description, "C CR*< C") == 0)
      mod_cat->schemata().get(0)->tables().get(0)->columns().get(0)->oldName("t2");
    else if (strcmp(data[i].description, "Rename table") == 0)
      mod_cat->schemata().get(0)->tables().get(0)->oldName("t2");
    else if (strcmp(data[i].description, "Rename view") == 0)
      mod_cat->schemata().get(0)->views().get(0)->oldName("v2");

    normalizer.init_omf(&omf);
    empty_change = diff_make(cat, mod_cat, &omf);

    if (empty_change) {
      alter_map.clear();
      alter_object_list.clear();
      diffsql_module->generateSQL(mod_cat, options, empty_change);
      diffsql_module->makeSQLSyncScript(mod_cat, options, alter_map, alter_object_list);

      // we can't check for the changeset to make sure there are not changes, because some changes from the diff
      // don't cause a script to be generated (like foreign keys being reordered)
      // so, it's better to check whether there's any actual alterations
      if (alter_map.count() > 0 || alter_object_list.count() > 0) {
        empty_change->dump_log(0);
        std::cout << "Iteration:" << i << std::endl;
        std::cout << export_sql_script << std::endl;
        std::string export_sql_script = options.get_string("OutputScript");
        std::cout << "Output:\n" << export_sql_script;
        fail(data[i].description);
      }
    }
    tester->wb->close_document();
    tester->wb->close_document_finish();
  }
  std::cout << std::endl;
}

TEST_FUNCTION(20) {
  std::shared_ptr<DiffChange> alter_change;
  std::shared_ptr<DiffChange> empty_change;

  // column insertion
  ensure("connection is NULL", connection.get() != NULL);

  {
    std::auto_ptr<sql::Statement> stmt(connection->createStatement());
    execute_script(stmt.get(),
                   "DROP DATABASE IF EXISTS grtdiff_alter_test;"
                   "DROP DATABASE IF EXISTS grtdiff_alter_test2");
    execute_script(stmt.get(),
                   "CREATE DATABASE IF NOT EXISTS grtdiff_alter_test /*!40100 DEFAULT CHARACTER SET latin1 */");
  }

  MySQLParserServices::Ref services = MySQLParserServices::get();
  MySQLParserContext::Ref context =
    services->createParserContext(tester->get_rdbms()->characterSets(), tester->get_rdbms()->version(), false);

  grt::DictRef options(true);
  for (int i = 0; data[i].description != NULL; i++) {
    std::cout << ".";
    if ((i + 1) % 30 == 0)
      std::cout << std::endl;

    for (int j = 0; j <= 1; ++j) {
      db_mysql_CatalogRef org_cat = create_empty_catalog_for_import();
      db_mysql_CatalogRef mod_cat = create_empty_catalog_for_import();

      {
        std::string org_script;
        org_script
          .append("CREATE DATABASE IF NOT EXISTS grtdiff_alter_test /*!40100 DEFAULT CHARACTER SET latin1 */;\n")
          .append(data[i].org)
          .append(";");
        services->parseSQLIntoCatalog(context, org_cat, org_script, options);
      }
      {
        std::string mod_script;
        mod_script
          .append("CREATE DATABASE IF NOT EXISTS grtdiff_alter_test /*!40100 DEFAULT CHARACTER SET latin1 */;\n")
          .append(data[i].mod)
          .append(";");
        services->parseSQLIntoCatalog(context, mod_cat, mod_script, options);
      }

      if (strcmp(data[i].description, "C CR C") == 0)
        mod_cat->schemata().get(0)->tables().get(0)->columns().get(1)->oldName("t");
      else if (strcmp(data[i].description, "C CR> C") == 0)
        mod_cat->schemata().get(0)->tables().get(0)->columns().get(2)->oldName("t");
      else if (strcmp(data[i].description, "C CR< C") == 0)
        mod_cat->schemata().get(0)->tables().get(0)->columns().get(0)->oldName("t");
      else if (strcmp(data[i].description, "C CR*> C") == 0)
        mod_cat->schemata().get(0)->tables().get(0)->columns().get(2)->oldName("t");
      else if (strcmp(data[i].description, "C CR*< C") == 0)
        mod_cat->schemata().get(0)->tables().get(0)->columns().get(0)->oldName("t");
      else if (strcmp(data[i].description, "Rename table") == 0)
        mod_cat->schemata().get(0)->tables().get(0)->oldName("t1");
      else if (strcmp(data[i].description, "Rename view") == 0)
        mod_cat->schemata().get(0)->views().get(0)->oldName("v1");

      grt::NormalizedComparer normalizer;
      normalizer.init_omf(&omf);
      grt::ValueRef default_engine = bec::GRTManager::get()->get_app_option("db.mysql.Table:tableEngine");
      std::string default_engine_name;
      if (grt::StringRef::can_wrap(default_engine))
        default_engine_name = grt::StringRef::cast_from(default_engine);

      bec::CatalogHelper::apply_defaults(mod_cat, default_engine_name);
      bec::CatalogHelper::apply_defaults(org_cat, default_engine_name);

      alter_change = diff_make(org_cat, mod_cat, &omf);
      ensure("Empty alter:", (bool)alter_change);

#if VERBOSE_TESTING
      alter_change->dump_log(0);
#endif

      const char TemplateFile[] = "data/reporting/Basic_Text.tpl/basic_text_report.txt.tpl";

      // 1. generate alter
      grt::StringListRef alter_map(grt::Initialized);
      grt::DictRef options(true);
      options.set("UseFilteredLists", grt::IntegerRef(0));
      options.set("OutputContainer", alter_map);
      options.set("TemplateFile", grt::StringRef(TemplateFile));
      options.set("UseShortNames", grt::IntegerRef(j));
      options.set("SeparateForeignKeys", grt::IntegerRef(0));

      std::string report = diffsql_module->generateReport(org_cat, options, alter_change);
      std::string reportFile =
        base::strfmt("data/reporting/Basic_Text.tpl/reports/testres%s%d.txt", (j != 0) ? "_longname" : "_shortname", i);

      std::ifstream rep;
      rep.open(reportFile.c_str());
      std::string expected((std::istreambuf_iterator<char>(rep)), std::istreambuf_iterator<char>());

      if (report != expected) {
        std::cout << std::endl << std::endl << "Reports differ, expected:" << std::endl;
        // alter_change->dump_log(0);
        std::cout << expected << std::endl << std::endl;
        std::cout << "==================================================" << std::endl
                  << std::endl
                  << "actual: " << std::endl;
        std::cout << report << std::endl << std::endl;
        fail(reportFile);
      }

      // Test Data generation
      /*
        sprintf(buf1, "testres%s%d.txt",j?"_longname":"_shortname", i);
        std::ofstream out;
        out.open(buf1);
        out<<(*report).c_str();
        out.close();
      */
    }
  }
  std::cout << std::endl;
}

static struct {
  const char *description;
  const char *object_name;
  const char *cleanup;
  const char *org;
  const char *mod;
} neg_data[] = {
  // Tests that should give no differences
  {"BINARY flag columns", "grtiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",
   "CREATE TABLE grtdiff_alter_test.t1 (`a` varchar(20) BINARY) ENGINE=InnoDB DEFAULT CHARSET=latin1",
   "CREATE TABLE grtdiff_alter_test.t1 (`a` varchar(20) CHARACTER SET latin1 COLLATE latin1_bin DEFAULT NULL) "
   "ENGINE=InnoDB DEFAULT CHARSET=latin1"},
  {"BINARY flag columns (utf8)", "grtiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",
   "CREATE TABLE grtdiff_alter_test.t1 (`a` varchar(20) BINARY) ENGINE=InnoDB DEFAULT CHARSET=utf8",
   "CREATE TABLE grtdiff_alter_test.t1 (`a` varchar(20) CHARACTER SET utf8 COLLATE utf8_bin DEFAULT NULL) "
   "ENGINE=InnoDB DEFAULT CHARSET=utf8"},
  {"ASCII flag columns", "grtiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",
   "CREATE TABLE grtdiff_alter_test.t1 (`a` varchar(20) ASCII) ENGINE=InnoDB DEFAULT CHARSET=utf8",
   "CREATE TABLE grtdiff_alter_test.t1 (`a` varchar(20) CHARACTER SET latin1 DEFAULT NULL) ENGINE=InnoDB DEFAULT "
   "CHARSET=utf8"},
  {"ZEROFILL flag columns", "grtiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",
   "CREATE TABLE grtdiff_alter_test.t1 (`a` int ZEROFILL) ENGINE=InnoDB",
   "CREATE TABLE grtdiff_alter_test.t1 (`a` int UNSIGNED ZEROFILL DEFAULT NULL) ENGINE=InnoDB"},
  {"reorder index", "grtiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",
   "CREATE TABLE grtdiff_alter_test.t1 (`a` int, `b` int, index aa (a), index bb (b))",
   "CREATE TABLE grtdiff_alter_test.t1 (`a` int, `b` int, index bb (b), index aa (a))"},
  {"enums", "grtiff_alter_test.t1", "DROP TABLE IF EXISTS grtdiff_alter_test.t1;",
   "CREATE TABLE grtdiff_alter_test.t1 (`a` enum('a','b','c'))",
   "CREATE TABLE grtdiff_alter_test.t1 (`a` ENUM('a', 'b',    'c'  ))"},
  {NULL, NULL, NULL, NULL, NULL}};

TEST_FUNCTION(30) {
  std::shared_ptr<DiffChange> empty_change;

  // column insertion

  ensure("connection is NULL", connection.get() != NULL);

  {
    std::auto_ptr<sql::Statement> stmt(connection->createStatement());
    execute_script(stmt.get(),
                   "DROP DATABASE IF EXISTS grtdiff_alter_test;"
                   "DROP DATABASE IF EXISTS grtdiff_alter_test2");
    execute_script(stmt.get(),
                   "CREATE DATABASE IF NOT EXISTS grtdiff_alter_test /*!40100 DEFAULT CHARACTER SET latin1 */");
  }

  MySQLParserServices::Ref services = MySQLParserServices::get();
  MySQLParserContext::Ref context =
    services->createParserContext(tester->get_rdbms()->characterSets(), tester->get_rdbms()->version(), false);

  grt::DictRef options(true);
  for (int i = 0; neg_data[i].description != NULL; i++) {
    std::cout << ".";
    if ((i + 1) % 30 == 0)
      std::cout << std::endl;

    db_mysql_CatalogRef org_cat = create_empty_catalog_for_import();
    db_mysql_CatalogRef mod_cat = create_empty_catalog_for_import();

    {
      std::string org_script;
      org_script.append("CREATE DATABASE IF NOT EXISTS grtdiff_alter_test /*!40100 DEFAULT CHARACTER SET latin1 */;\n")
        .append(neg_data[i].org)
        .append(";");
      services->parseSQLIntoCatalog(context, org_cat, org_script, options);
    }
    {
      std::string mod_script;
      mod_script.append("CREATE DATABASE IF NOT EXISTS grtdiff_alter_test /*!40100 DEFAULT CHARACTER SET latin1 */;\n")
        .append(neg_data[i].mod)
        .append(";");
      services->parseSQLIntoCatalog(context, mod_cat, mod_script, options);
    }

    if (strcmp(neg_data[i].description, "C CR C") == 0)
      mod_cat->schemata().get(0)->tables().get(0)->columns().get(1)->oldName("t");
    else if (strcmp(neg_data[i].description, "C CR> C") == 0)
      mod_cat->schemata().get(0)->tables().get(0)->columns().get(2)->oldName("t");
    else if (strcmp(neg_data[i].description, "C CR< C") == 0)
      mod_cat->schemata().get(0)->tables().get(0)->columns().get(0)->oldName("t");
    else if (strcmp(neg_data[i].description, "C CR*> C") == 0)
      mod_cat->schemata().get(0)->tables().get(0)->columns().get(2)->oldName("t");
    else if (strcmp(neg_data[i].description, "C CR*< C") == 0)
      mod_cat->schemata().get(0)->tables().get(0)->columns().get(0)->oldName("t");
    else if (strcmp(neg_data[i].description, "Rename table") == 0)
      mod_cat->schemata().get(0)->tables().get(0)->oldName("t1");
    else if (strcmp(neg_data[i].description, "Rename view") == 0)
      mod_cat->schemata().get(0)->views().get(0)->oldName("v1");

    grt::NormalizedComparer normalizer;
    normalizer.init_omf(&omf);
    grt::ValueRef default_engine = bec::GRTManager::get()->get_app_option("db.mysql.Table:tableEngine");
    std::string default_engine_name;
    if (grt::StringRef::can_wrap(default_engine))
      default_engine_name = grt::StringRef::cast_from(default_engine);

    bec::CatalogHelper::apply_defaults(mod_cat, default_engine_name);
    bec::CatalogHelper::apply_defaults(org_cat, default_engine_name);

    // this should yield no differences
    empty_change = diff_make(org_cat, mod_cat, &omf);

    if (empty_change) {
      grt::StringListRef alter_map(grt::Initialized);
      grt::ListRef<GrtNamedObject> alter_object_list(true);
      grt::DictRef options(true);
      options.set("UseFilteredLists", grt::IntegerRef(0));
      options.set("OutputContainer", alter_map);
      options.set("OutputObjectContainer", alter_object_list);
      options.set("CaseSensitive", grt::IntegerRef(omf.case_sensitive));

      alter_map.clear();
      alter_object_list.clear();
      diffsql_module->generateSQL(mod_cat, options, empty_change);
      diffsql_module->makeSQLSyncScript(mod_cat, options, alter_map, alter_object_list);

      // we can't check for the changeset to make sure there are not changes, because some changes from the diff
      // don't cause a script to be generated (like foreign keys being reordered)
      // so, it's better to check whether there's any actual alterations
      if (alter_map.count() > 0 || alter_object_list.count() > 0) {
        empty_change->dump_log(0);
        std::cout << "Iteration:" << i << std::endl;
        std::string export_sql_script = options.get_string("OutputScript");
        std::cout << "Output:\n" << export_sql_script;
        fail(neg_data[i].description);
      }
    }
  }
  std::cout << std::endl;
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  delete tester;
}

END_TESTS
