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

#include "testgrt.h"
#include "grt_test_utility.h"
#include "grt/grt_manager.h"
#include "grt.h"

#include "grts/structs.h"
#include "grts/structs.workbench.h"
#include "grts/structs.db.mgmt.h"
#include "grts/structs.db.mysql.h"
#include "grts/structs.db.mgmt.h"

#include "grtdb/db_object_helpers.h"

#include "cppdbc.h"
#include "backend/db_rev_eng_be.h"

#include "db_mysql_diffsqlgen.h"

#include "diff/diffchange.h"
#include "diff/grtdiff.h"
#include "diff/changeobjects.h"
#include "diff/changelistobjects.h"
#include "grtdb/db_helpers.h"
#include "grtdb/diff_dbobjectmatch.h"

#include "myx_statement_parser.h"
#include "backend/db_mysql_sql_script_sync.h"
#include "backend/db_mysql_sql_export.h"
#include "module_db_mysql.h"
#include "wb_helpers.h"

#include "module_db_mysql_shared_code.h"

/*
  We override get_model_catalog() method of the original plugin,
  to make testing setup easier, in real life the model catalog
  is taken from the GRT tree, and this is not tested here.
*/

using namespace parser;

class DbMySQLScriptSyncTest : public DbMySQLScriptSync {
protected:
  db_mysql_CatalogRef model_catalog;
  virtual db_mysql_CatalogRef get_model_catalog() {
    return model_catalog;
  }

public:
  void set_model_catalog(const db_mysql_CatalogRef &catalog) {
    model_catalog = catalog;
  }
  DbMySQLScriptSyncTest() : DbMySQLScriptSync() {
  }
};

class DbMySQLSQLExportTest : public DbMySQLSQLExport {
protected:
  db_mysql_CatalogRef model_catalog;
  grt::DictRef options;

  virtual db_mysql_CatalogRef get_model_catalog() {
    return model_catalog;
  }
  virtual grt::DictRef get_options_as_dict() {
    return options;
  }

public:
  DbMySQLSQLExportTest(db_mysql_CatalogRef cat) : DbMySQLSQLExport(cat) {
    set_model_catalog(cat);
  }

  void set_model_catalog(db_mysql_CatalogRef catalog) {
    model_catalog = catalog;
  }
  void set_options_as_dict(grt::DictRef options_dict) {
    options = options_dict;
  }
};

struct all_objects_mwb {
  db_SchemaRef schema;
  db_TableRef t1;
  db_TableRef t2;
  db_ViewRef view;
  db_RoutineRef routine;
  db_ForeignKeyRef FK;
  db_TriggerRef trigger;
};

BEGIN_TEST_DATA_CLASS(db_mysql_plugin_test)
protected:
WBTester *tester;
std::shared_ptr<DbMySQLScriptSync> sync_plugin;
std::shared_ptr<DbMySQLSQLExport> fwdeng_plugin;
sql::ConnectionWrapper connection;
grt::DbObjectMatchAlterOmf omf;

db_mysql_CatalogRef create_catalog_from_script(const std::string &sql);

std::string run_sync_plugin_generate_script(const std::vector<std::string> &, db_mysql_CatalogRef org_cat,
                                            db_mysql_CatalogRef mod_cat);

void run_sync_plugin_apply_to_model(const std::vector<std::string> &schemata, db_mysql_CatalogRef org_cat,
                                    db_mysql_CatalogRef mod_cat);

std::string run_fwdeng_plugin_generate_script(db_mysql_CatalogRef cat, DbMySQLSQLExportTest *plugin);
std::shared_ptr<DiffChange> compare_catalog_to_server_schema(db_mysql_CatalogRef org_cat,
                                                             const std::string &schema_name);
void apply_sql_to_model(const std::string &sql);
all_objects_mwb get_model_objects();

TEST_DATA_CONSTRUCTOR(db_mysql_plugin_test) {
  tester = new WBTester();
  // init datatypes
  populate_grt(*tester);

  omf.dontdiff_mask = 3;

  // init database connection
  connection = tester->create_connection_for_import();

  // Modeling uses a default server version, which is not related to any server it might have
  // reverse engineered content from, nor where it was sync'ed to. So we have to mimic this here.
  std::string target_version = bec::GRTManager::get()->get_app_option_string("DefaultTargetMySQLVersion");
  if (target_version.empty())
    target_version = "5.5.49";
  tester->get_rdbms()->version(parse_version(target_version));
}

END_TEST_DATA_CLASS

TEST_MODULE(db_mysql_plugin_test, "db.mysql plugin test");

db_mysql_CatalogRef tut::Test_object_base<db_mysql_plugin_test>::create_catalog_from_script(const std::string &sql) {
  db_mysql_CatalogRef cat = create_empty_catalog_for_import();
  MySQLParserServices::Ref services = MySQLParserServices::get();
  MySQLParserContext::Ref context =
    services->createParserContext(tester->get_rdbms()->characterSets(), tester->get_rdbms()->version(), false);

  grt::DictRef options(true);
  if (services->parseSQLIntoCatalog(context, cat, sql, options) != 0)
    fail("SQL failed to parse: " + sql);
  return cat;
}

std::string tut::Test_object_base<db_mysql_plugin_test>::run_sync_plugin_generate_script(
  const std::vector<std::string> &schemata, db_mysql_CatalogRef org_cat, db_mysql_CatalogRef mod_cat) {
  sync_plugin.reset(new DbMySQLScriptSyncTest());
  static_cast<DbMySQLScriptSyncTest *>(sync_plugin.get())->set_model_catalog(mod_cat);
  sync_plugin->init_diff_tree(std::vector<std::string>(), mod_cat, org_cat);
  return sync_plugin->generate_diff_tree_script();
}

void tut::Test_object_base<db_mysql_plugin_test>::run_sync_plugin_apply_to_model(
  const std::vector<std::string> &schemata, db_mysql_CatalogRef org_cat, db_mysql_CatalogRef mod_cat) {
  sync_plugin.reset(new DbMySQLScriptSyncTest());
  static_cast<DbMySQLScriptSyncTest *>(sync_plugin.get())->set_model_catalog(mod_cat);
  sync_plugin->init_diff_tree(std::vector<std::string>(), org_cat, ValueRef());
  sync_plugin->apply_changes_to_model();
}

std::string tut::Test_object_base<db_mysql_plugin_test>::run_fwdeng_plugin_generate_script(
  db_mysql_CatalogRef cat, DbMySQLSQLExportTest *plugin) {
  fwdeng_plugin.reset(plugin);
  ValueRef retval = fwdeng_plugin->export_task(grt::StringRef());
  return fwdeng_plugin->export_sql_script();
}

std::shared_ptr<DiffChange> tut::Test_object_base<db_mysql_plugin_test>::compare_catalog_to_server_schema(
  db_mysql_CatalogRef org_cat, const std::string &schema_name) {
  sync_plugin.reset(new DbMySQLScriptSyncTest());
  std::list<std::string> schemata;
  schemata.push_back("db_mysql_plugin_test");
  db_mysql_CatalogRef cat = tester->db_rev_eng_schema(schemata);
  if ((cat->schemata().get(0).is_valid()) && (cat->schemata().get(0)->name() == "mydb"))
    cat->schemata().remove(0);
  org_cat->oldName("");

  grt::ValueRef default_engine = bec::GRTManager::get()->get_app_option("db.mysql.Table:tableEngine");
  std::string default_engine_name;
  if (grt::StringRef::can_wrap(default_engine))
    default_engine_name = grt::StringRef::cast_from(default_engine);

  bec::CatalogHelper::apply_defaults(cat, default_engine_name);
  bec::CatalogHelper::apply_defaults(org_cat, default_engine_name);

  grt::NormalizedComparer comparer(grt::DictRef(true));
  comparer.init_omf(&omf);

  std::shared_ptr<DiffChange> result = diff_make(cat, org_cat, &omf);

  tester->wb->close_document();
  tester->wb->close_document_finish();

  return result;
}

void tut::Test_object_base<db_mysql_plugin_test>::apply_sql_to_model(const std::string &sql) {
  db_mysql_CatalogRef org_cat = create_catalog_from_script(sql);

  std::vector<std::string> schemata;
  schemata.push_back("mydb");

  db_mysql_CatalogRef mod_cat = db_mysql_CatalogRef::cast_from(tester->get_catalog());

  DbMySQLSQLExportTest *plugin = new DbMySQLSQLExportTest(mod_cat);

  grt::DictRef options(true);
  options.set("UseFilteredLists", grt::IntegerRef(0));
  plugin->set_options_as_dict(options);

  std::string value;

  DbMySQLScriptSyncTest p;
  p.set_model_catalog(mod_cat);
  std::shared_ptr<DiffTreeBE> tree = p.init_diff_tree(std::vector<std::string>(), mod_cat, org_cat);

  // apply everything back to model
  tree->set_apply_direction(tree->get_root(), DiffNode::ApplyToModel, true);
  bec::NodeId mydb_node = tree->get_child(NodeId(), 0);
  bec::NodeId table1_node = tree->get_child(mydb_node, 0);
  tree->get_field(table1_node, DiffTreeBE::ModelObjectName, value);

  p.apply_changes_to_model();
}

TEST_FUNCTION(5) {
  // Bug #32327
  static const char *sql1 =
    "DROP DATABASE IF EXISTS `db_mysql_plugin_test`;"
    "CREATE DATABASE `db_mysql_plugin_test` DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;"
    "CREATE  TABLE IF NOT EXISTS `db_mysql_plugin_test`.`table1`"
    " (`idtable1` INT(11) NOT NULL , PRIMARY KEY (`idtable1`) ) ENGINE=InnoDB DEFAULT CHARSET=latin1 DEFAULT COLLATE = "
    "latin1_swedish_ci;";

  db_mysql_CatalogRef mod_cat = create_catalog_from_script(sql1);
  db_mysql_CatalogRef org_cat = grt::copy_object(mod_cat);

  db_mysql_IndexRef pk = mod_cat->schemata().get(0)->tables().get(0)->indices().get(0);
  ensure("bug_32327 - invalid test input", pk->isPrimary() != 0);

  // rename PK
  pk->name("mypk");

  std::vector<std::string> schemata;
  schemata.push_back("db_mysql_plugin_test");

  std::string script = run_sync_plugin_generate_script(schemata, org_cat, mod_cat);

  std::auto_ptr<sql::Statement> stmt(connection->createStatement());
  execute_script(stmt.get(), sql1);
  execute_script(stmt.get(), script);

  std::shared_ptr<DiffChange> empty_change = compare_catalog_to_server_schema(org_cat, "db_mysql_plugin_test");

  if (empty_change)
    empty_change->dump_log(0);
  ensure("Unexpected changes", empty_change == NULL);
}

TEST_FUNCTION(10) {
  // Bug #32330
  static const char *sql1 =
    "DROP DATABASE IF EXISTS `db_mysql_plugin_test`;"
    "CREATE DATABASE IF NOT EXISTS `db_mysql_plugin_test` DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;"
    "CREATE  TABLE IF NOT EXISTS `db_mysql_plugin_test`.`table1` (`idtable1` INT NOT NULL PRIMARY KEY) "
    " ENGINE=InnoDB DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;";

  db_mysql_CatalogRef mod_cat = create_catalog_from_script(sql1);
  db_mysql_CatalogRef org_cat = grt::copy_object(mod_cat);

  ensure("bug_32330 - invalid test input", mod_cat->schemata().get(0)->tables().count() == 1);

  std::vector<std::string> schemata;
  schemata.push_back("db_mysql_plugin_test");

  // remove table
  mod_cat->schemata().get(0)->tables().remove(0);

  std::string script = run_sync_plugin_generate_script(schemata, org_cat, mod_cat);

  std::auto_ptr<sql::Statement> stmt(connection->createStatement());
  execute_script(stmt.get(), sql1);
  execute_script(stmt.get(), script);

  std::shared_ptr<DiffChange> empty_change = compare_catalog_to_server_schema(mod_cat, "db_mysql_plugin_test");

  if (empty_change)
    empty_change->dump_log(0);
  ensure("Unexpected changes", empty_change == NULL);
}

TEST_FUNCTION(15) {
  // Bug #32334
  static const char *sql1 =
    "DROP DATABASE IF EXISTS `db_mysql_plugin_test`;"
    "CREATE DATABASE IF NOT EXISTS `db_mysql_plugin_test` DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;"
    "CREATE  TABLE IF NOT EXISTS `db_mysql_plugin_test`.`table1` (`idtable1` INT NOT NULL, PRIMARY KEY (`idtable1`) ) "
    "ENGINE = MyISAM CHARSET = latin1 DEFAULT COLLATE = latin1_swedish_ci;"
    "CREATE  TABLE IF NOT EXISTS `db_mysql_plugin_test`.`table2` (`idtable1` INT NOT NULL, PRIMARY KEY (`idtable1`) ) "
    "ENGINE = MyISAM CHARSET = latin1 DEFAULT COLLATE = latin1_swedish_ci;"
    "CREATE  TABLE IF NOT EXISTS `db_mysql_plugin_test`.`table3` (`idtable1` INT NOT NULL, PRIMARY KEY (`idtable1`) ) "
    "ENGINE = MyISAM CHARSET = latin1 DEFAULT COLLATE = latin1_swedish_ci;";

  db_mysql_CatalogRef mod_cat = create_catalog_from_script(sql1);
  db_mysql_CatalogRef org_cat = grt::copy_object(mod_cat);

  ensure("bug_32334 - invalid test input", mod_cat->schemata().get(0)->tables().count() == 3);

  // set table options
  db_mysql_TableRef table = mod_cat->schemata().get(0)->tables().get(0);
  table->avgRowLength("100");
  table->checksum(1);
  table->delayKeyWrite(1);
  table->maxRows("100");
  table->mergeInsert("LAST");
  table->mergeUnion("db_mysql_plugin_test.t2,db_mysql_plugin_test.t3");
  table->minRows("10");
  table->nextAutoInc("2");
  table->packKeys("DEFAULT");
  table->rowFormat("COMPACT");

  std::vector<std::string> schemata;
  schemata.push_back("db_mysql_plugin_test");

  std::string script = run_sync_plugin_generate_script(schemata, org_cat, mod_cat);

  std::auto_ptr<sql::Statement> stmt(connection->createStatement());
  execute_script(stmt.get(), sql1);
  execute_script(stmt.get(), script);

  // TODO: this check doesnt work because of a rev-eng problem http://bugs.mysql.com/bug.php?id=32491

  // DiffChange *empty_change= compare_catalog_to_server_schema(mod_cat, "db_mysql_plugin_test");
  // if(empty_change)
  //  empty_change->dump_log(0);
  // ensure("bug_32334 - test failed", empty_change == NULL);
}

TEST_FUNCTION(20) {
  // Bug #32336
  static const char *sql1 =
    "DROP DATABASE IF EXISTS `db_mysql_plugin_test`;"
    "CREATE DATABASE IF NOT EXISTS `db_mysql_plugin_test` DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;"
    "CREATE  TABLE IF NOT EXISTS `db_mysql_plugin_test`.`table1` (`idtable1` INT NOT NULL PRIMARY KEY) "
    " ENGINE=InnoDB DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;";

  db_mysql_CatalogRef mod_cat = create_catalog_from_script(sql1);
  db_mysql_CatalogRef org_cat = grt::copy_object(mod_cat);

  ensure("bug_32336 - invalid test input", mod_cat->schemata().get(0)->tables().count() == 1);

  // insert an invalid column
  db_mysql_TableRef table = mod_cat->schemata().get(0)->tables().get(0);
  db_mysql_ColumnRef column(grt::Initialized);
  column->owner(table);
  column->name("col1");
  table->columns().insert(column);

  std::vector<std::string> schemata;
  schemata.push_back("db_mysql_plugin_test");

  std::string script = run_sync_plugin_generate_script(schemata, org_cat, mod_cat);

  std::auto_ptr<sql::Statement> stmt(connection->createStatement());
  execute_script(stmt.get(), sql1);

  try {
    execute_script(stmt.get(), script);
  } catch (sql::SQLException &) {
    // expected
  }
}

TEST_FUNCTION(25) {
  // Bug #32358
  static const char *sql1 =
    "DROP DATABASE IF EXISTS `db_mysql_plugin_test`;"
    "CREATE DATABASE IF NOT EXISTS `db_mysql_plugin_test` DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;"
    "CREATE  TABLE IF NOT EXISTS `db_mysql_plugin_test`.`table1` (`idtable1` INT NOT NULL PRIMARY KEY) "
    "ENGINE=InnoDB DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;"
    "CREATE  TABLE IF NOT EXISTS `db_mysql_plugin_test`.`table2` "
    "(`idtable2` INT NOT NULL DEFAULT 100 , `col1` VARCHAR(45) NULL , PRIMARY KEY (`idtable2`) ) "
    " ENGINE=InnoDB DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;";

  static const char *sql2 = "DROP DATABASE IF EXISTS `db_mysql_plugin_test`;";

  db_mysql_CatalogRef mod_cat = create_catalog_from_script(sql1);
  ensure("bug_32358 - invalid test input", mod_cat->schemata().get(0)->tables().count() == 2);

  // insert an self-referencing FK
  db_mysql_TableRef table = mod_cat->schemata().get(0)->tables().get(1);
  db_mysql_ForeignKeyRef fk(grt::Initialized);
  fk->owner(table);
  fk->name("fk1");
  fk->referencedTable(table);
  fk->columns().insert(table->columns().get(0));
  fk->columns().insert(table->columns().get(1));
  fk->referencedColumns().insert(table->columns().get(0));
  fk->referencedColumns().insert(table->columns().get(1));
  table->foreignKeys().insert(fk);

  std::vector<std::string> schemata;
  schemata.push_back("db_mysql_plugin_test");

  std::string script = run_sync_plugin_generate_script(schemata, db_mysql_CatalogRef(grt::Initialized), mod_cat);

  std::auto_ptr<sql::Statement> stmt(connection->createStatement());
  execute_script(stmt.get(), sql2);
  // Self referencing keys are no longer supported, but as test case itself checks for
  // crash it is better to this test case since there shouldn't be any crashes even
  // in case of invalid sql being produced
  //  execute_script(stmt.get(), script, tester->wb->get_grt_manager());
}

TEST_FUNCTION(30) {
  // Bug #32367
  static const char *sql1 =
    "DROP DATABASE IF EXISTS db_mysql_plugin_test;"
    "CREATE DATABASE db_mysql_plugin_test DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;"
    "USE db_mysql_plugin_test;"
    "CREATE TABLE t1(id INT NOT NULL PRIMARY KEY AUTO_INCREMENT, col_char CHAR(1));"
    "CREATE TABLE t2(id INT NOT NULL PRIMARY KEY AUTO_INCREMENT, col_char CHAR(1));"
    "CREATE TABLE t3(id INT NOT NULL PRIMARY KEY AUTO_INCREMENT, col_char CHAR(1));\n"
    "DELIMITER //\n"
    "CREATE PROCEDURE proc1(OUT param1 INT) "
    "BEGIN "
    "SELECT COUNT(*) FROM t1; "
    "END// "
    "create DEFINER=root@localhost trigger tr1 after insert on t1 for each row begin delete from t2; end //\n"
    "DELIMITER ;\n"
    "INSERT INTO t1(col_char) VALUES ('a'), ('b'), ('c');";

  std::auto_ptr<sql::Statement> stmt(connection->createStatement());
  execute_script(stmt.get(), sql1);

  std::list<std::string> schemata_list;
  schemata_list.push_back("db_mysql_plugin_test");
  db_mysql_CatalogRef mod_cat = grt::copy_object(tester->db_rev_eng_schema(schemata_list));
  db_mysql_CatalogRef org_cat = grt::copy_object(mod_cat);
  tester->wb->close_document();
  tester->wb->close_document_finish();

  ensure("bug_32367 - invalid test input wrong table count", (mod_cat->schemata().get(0)->tables().count() == 3));
  ensure("bug_32367 - invalid test input wrong trigger count",
         (mod_cat->schemata().get(0)->tables().get(0)->triggers().count() == 1));
  ensure("bug_32367 - invalid test input wrong routines count", (mod_cat->schemata().get(0)->routines().count() == 1));

  // delete a table, a routine and a trigger
  mod_cat->schemata().get(0)->tables().remove(2);
  mod_cat->schemata().get(0)->tables().get(0)->triggers().remove(0);
  mod_cat->schemata().get(0)->routines().remove(0);

  std::vector<std::string> schemata;
  schemata.push_back("db_mysql_plugin_test");

  std::string script = run_sync_plugin_generate_script(schemata, org_cat, mod_cat);
  execute_script(stmt.get(), script);

  db_mysql_CatalogRef new_cat = tester->db_rev_eng_schema(schemata_list);

  ensure_equals("Table count mismatch", new_cat->schemata().get(0)->tables().count(), 2U);
  ensure_equals("Trigger count mismatch", new_cat->schemata().get(0)->tables().get(0)->triggers().count(), 0U);
  ensure_equals("Routines count mismatch", new_cat->schemata().get(0)->routines().count(), 0U);

  tester->wb->close_document();
  tester->wb->close_document_finish();
}

TEST_FUNCTION(35) {
  // Bug #32371
  static const char *sql1 =
    "DROP DATABASE IF EXISTS db_mysql_plugin_test;\n"
    "CREATE DATABASE db_mysql_plugin_test DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;\n"
    "USE db_mysql_plugin_test;\n"
    "CREATE TABLE t1 (id INT NOT NULL PRIMARY KEY AUTO_INCREMENT, col_char CHAR(1)) ENGINE=InnoDB "
    "DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;\n"
    "DELIMITER //\n"
    "CREATE PROCEDURE proc1(OUT param1 INT) "
    "BEGIN "
    "SELECT COUNT(*) FROM t1; "
    "END//\n"
    "CREATE PROCEDURE proc2(OUT param1 INT) "
    "BEGIN "
    "SELECT COUNT(*) FROM t1; "
    "END//\n"
    "DELIMITER ;\n"
    "INSERT INTO t1(col_char) VALUES ('a'), ('b'), ('c');";

  // part1 - check that unmodified procedure is not updated

  std::auto_ptr<sql::Statement> stmt(connection->createStatement());
  execute_script(stmt.get(), sql1);

  std::list<std::string> schemata_list;
  schemata_list.push_back("db_mysql_plugin_test");
  db_mysql_CatalogRef mod_cat = grt::copy_object(tester->db_rev_eng_schema(schemata_list));
  db_mysql_CatalogRef org_cat = grt::copy_object(mod_cat);

  tester->wb->close_document();
  tester->wb->close_document_finish();

  ensure("bug_32371 - invalid test input",
         (mod_cat->schemata().get(0)->tables().count() == 1) && (mod_cat->schemata().get(0)->routines().count() == 2));

  std::shared_ptr<DiffChange> empty_change = compare_catalog_to_server_schema(mod_cat, "db_mysql_plugin_test");

  if (empty_change)
    empty_change->dump_log(0);

  ensure("bug_32371 - test failed", empty_change == NULL);

  // Part2 - check that delimiters for routines are generated.
  static const char *def1 =
    "CREATE PROCEDURE proc1(OUT param1 INT) "
    "BEGIN "
    "SELECT 1; "
    "END";

  static const char *def2 =
    "CREATE PROCEDURE proc2(OUT param1 INT) "
    "BEGIN "
    "SELECT 1; "
    "END";

  // modify routines
  mod_cat->schemata().get(0)->routines().get(0)->sqlDefinition(def1);
  mod_cat->schemata().get(0)->routines().get(1)->sqlDefinition(def2);

  std::vector<std::string> schemata;
  schemata.push_back("db_mysql_plugin_test");

  std::string script = run_sync_plugin_generate_script(schemata, org_cat, mod_cat);
  execute_script(stmt.get(), script);
}

TEST_FUNCTION(40) {
  // Bug #32329
  static const char *sql1 =
    "DROP DATABASE IF EXISTS `db_mysql_plugin_test`;"
    "CREATE DATABASE IF NOT EXISTS `db_mysql_plugin_test` DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;\n"
    "CREATE  TABLE IF NOT EXISTS `db_mysql_plugin_test`.`table1` "
    "(`idtable1` INT NOT NULL , `col1` VARCHAR(45) NULL , PRIMARY KEY (`idtable1`) , INDEX idx1 (`idtable1` ASC, "
    "`col1` ASC) ) engine = MyISAM;";

  db_mysql_CatalogRef mod_cat = create_catalog_from_script(sql1);
  db_mysql_CatalogRef org_cat = grt::copy_object(mod_cat);

  ensure("bug_32329 - invalid test input",
         mod_cat->schemata().get(0)->tables().get(0)->indices().get(1)->columns().count() == 2);

  // delete column `col1` from index idx1
  mod_cat->schemata().get(0)->tables().get(0)->indices().get(1)->columns().remove(1);

  std::vector<std::string> schemata;
  schemata.push_back("db_mysql_plugin_test");

  std::string script = run_sync_plugin_generate_script(schemata, org_cat, mod_cat);

  std::auto_ptr<sql::Statement> stmt(connection->createStatement());
  execute_script(stmt.get(), sql1);
  execute_script(stmt.get(), script);

  std::shared_ptr<DiffChange> empty_change = compare_catalog_to_server_schema(mod_cat, "db_mysql_plugin_test");

  if (empty_change)
    empty_change->dump_log(0);

  ensure("Unexpected changes", empty_change == NULL);
}

TEST_FUNCTION(45) {
  // Bug #32324
  static const char *sql1 =
    "DROP DATABASE IF EXISTS `db_mysql_plugin_test`;"
    "CREATE DATABASE IF NOT EXISTS `db_mysql_plugin_test` DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;\n"
    "CREATE  TABLE IF NOT EXISTS `db_mysql_plugin_test`.`table1` "
    "(`idtable1` INT NOT NULL , `col1` VARCHAR(45) NULL , `col2` VARCHAR(45) NULL , PRIMARY KEY (`idtable1`) ) engine "
    "= MyISAM;";

  db_mysql_CatalogRef mod_cat = create_catalog_from_script(sql1);
  db_mysql_CatalogRef org_cat = grt::copy_object(mod_cat);

  ensure("bug_32324 - invalid test input", mod_cat->schemata().get(0)->tables().get(0)->columns().count() == 3);

  // move `col1` after `col2`
  db_mysql_TableRef table = mod_cat->schemata().get(0)->tables().get(0);
  db_mysql_ColumnRef col1 = table->columns().get(1);
  table->columns().remove(1);
  table->columns().insert(col1);

  std::vector<std::string> schemata;
  schemata.push_back("db_mysql_plugin_test");

  std::string script = run_sync_plugin_generate_script(schemata, org_cat, mod_cat);

  std::auto_ptr<sql::Statement> stmt(connection->createStatement());
  execute_script(stmt.get(), sql1);
  execute_script(stmt.get(), script);

  std::shared_ptr<DiffChange> empty_change = compare_catalog_to_server_schema(mod_cat, "db_mysql_plugin_test");

  if (empty_change)
    empty_change->dump_log(0);

  ensure("Unexpected changes", empty_change == NULL);
}

TEST_FUNCTION(50) {
  // Bug #32331
  static const char *sql1 =
    "DROP DATABASE IF EXISTS `db_mysql_plugin_test`;"
    "CREATE DATABASE IF NOT EXISTS `db_mysql_plugin_test` DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;\n"
    "CREATE VIEW `db_mysql_plugin_test`.`view2` AS SELECT * FROM `db_mysql_plugin_test`.`view1`;"
    "CREATE VIEW `db_mysql_plugin_test`.`view1` AS SELECT 1;";

  db_mysql_CatalogRef mod_cat = create_catalog_from_script(sql1);

  ensure("bug_32331 - invalid test input", mod_cat->schemata().get(0)->views().count() == 2);

  // first test export

  DbMySQLSQLExportTest *plugin = new DbMySQLSQLExportTest(mod_cat);
  plugin->set_option("ViewsAreSelected", true);

  grt::DictRef options(true);
  grt::StringListRef views(grt::Initialized);
  views.insert(get_old_object_name_for_key(mod_cat->schemata().get(0)->views().get(0), false), false);
  views.insert(get_old_object_name_for_key(mod_cat->schemata().get(0)->views().get(1), false), false);
  options.set("ViewFilterList", views);
  plugin->set_options_as_dict(options);

  std::string script = run_fwdeng_plugin_generate_script(mod_cat, plugin);

  std::auto_ptr<sql::Statement> stmt(connection->createStatement());
  execute_script(stmt.get(), script);

  std::vector<std::string> schemata;
  schemata.push_back("db_mysql_plugin_test");

  // now the same test for sync
  script.assign(run_sync_plugin_generate_script(schemata, db_mysql_CatalogRef(grt::Initialized), mod_cat));

  std::auto_ptr<sql::Statement> stmt2(connection->createStatement());
  execute_script(stmt2.get(), "DROP DATABASE IF EXISTS `db_mysql_plugin_test`");
  execute_script(stmt2.get(), script);
}

// bug #37634
// http://bugs.mysql.com/bug.php?id=37634
// update model figures (and FKs) when a table is replaced
// during the db/script to model synchronization
TEST_FUNCTION(55) {
  static const char *sql1 =
    "CREATE SCHEMA IF NOT EXISTS `mydb` DEFAULT CHARACTER SET latin1 COLLATE latin1_swedish_ci;\n"
    "USE `mydb`;\n"
    "CREATE  TABLE IF NOT EXISTS `mydb`.`table1` (\n"
    "  `idtable1` INT NOT NULL ,\n"
    "  PRIMARY KEY (`idtable1`) )\n"
    "ENGINE = InnoDB;";

  tester->wb->open_document("data/workbench/diff_table_replace_test.mwb");

  db_mgmt_ManagementRef mgmt(db_mgmt_ManagementRef::cast_from(grt::GRT::get()->get("/wb/rdbmsMgmt")));

  ListRef<db_DatatypeGroup> grouplist = ListRef<db_DatatypeGroup>::cast_from(
    grt::GRT::get()->unserialize(tester->wboptions->basedir + "/data/db_datatype_groups.xml"));
  grt::replace_contents(mgmt->datatypeGroups(), grouplist);

  db_mgmt_RdbmsRef rdbms = db_mgmt_RdbmsRef::cast_from(
    grt::GRT::get()->unserialize(tester->wboptions->basedir + "/modules/data/mysql_rdbms_info.xml"));
  ensure("db_mgmt_Rdbms initialization", rdbms.is_valid());
  grt::GRT::get()->set("/rdbms", rdbms);

  mgmt->rdbms().insert(rdbms);
  rdbms->owner(mgmt);

  db_TableRef t1 = tester->get_catalog()->schemata().get(0)->tables().get(0);

  ensure("before update table is referenced from figure 0",
         grt::GRT::get()->get("/wb/doc/physicalModels/0/diagrams/0/figures/0/table") == t1);

  ensure("before update table is referenced from figure 1",
         grt::GRT::get()->get("/wb/doc/physicalModels/0/diagrams/1/figures/0/table") == t1);

  db_mysql_CatalogRef org_cat = create_catalog_from_script(sql1);

  std::vector<std::string> schemata;
  schemata.push_back("mydb");

  db_mysql_CatalogRef mod_cat = db_mysql_CatalogRef::cast_from(tester->get_catalog());

  DbMySQLSQLExportTest *plugin = new DbMySQLSQLExportTest(mod_cat);

  grt::DictRef options(true);
  options.set("UseFilteredLists", grt::IntegerRef(0));
  plugin->set_options_as_dict(options);

  std::string value;

  DbMySQLScriptSyncTest p;
  p.set_model_catalog(mod_cat);
  std::shared_ptr<DiffTreeBE> tree = p.init_diff_tree(std::vector<std::string>(), org_cat, ValueRef());

  // change apply direction for table table1
  bec::NodeId mydb_node = tree->get_child(NodeId(), 0);
  bec::NodeId table1_node = tree->get_child(mydb_node, 0);
  tree->get_field(table1_node, DiffTreeBE::ModelObjectName, value);

  p.set_next_apply_direction(table1_node);
  p.set_next_apply_direction(table1_node);

  p.apply_changes_to_model();

  db_TableRef t2 = tester->get_catalog()->schemata().get(0)->tables().get(0);

  ensure("before update table is referenced from figure 0",
         grt::GRT::get()->get("/wb/doc/physicalModels/0/diagrams/0/figures/0/table") == t2);

  ensure("before update table is referenced from figure 1",
         grt::GRT::get()->get("/wb/doc/physicalModels/0/diagrams/1/figures/0/table") == t2);

  tester->wb->close_document();
  tester->wb->close_document_finish();
}

TEST_FUNCTION(60) {
  static const char *sql1 =
    "CREATE SCHEMA IF NOT EXISTS `mydb` DEFAULT CHARACTER SET latin1 COLLATE latin1_swedish_ci;\n"
    "USE `mydb`;\n"
    "CREATE  TABLE IF NOT EXISTS `mydb`.`table1` (\n"
    "  `idtable1` TINYINT NOT NULL ,\n"
    "  PRIMARY KEY (`idtable1`) )\n"
    "ENGINE = InnoDB;";
  tester->wb->open_document("data/workbench/diff_table_replace_test.mwb");
  apply_sql_to_model(sql1);

  db_TableRef t2 = tester->get_catalog()->schemata().get(0)->tables().get(0);
  db_ColumnRef col = t2->columns().get(0);
  db_SimpleDatatypeRef dtype = col->simpleType();
  ensure_equals("Column type not changed", dtype->name().c_str(), "TINYINT");

  tester->wb->close_document();
  tester->wb->close_document_finish();
}

TEST_FUNCTION(65) {
  static const char *sql1 =
    "CREATE SCHEMA IF NOT EXISTS `mydb` DEFAULT CHARACTER SET latin1 COLLATE latin1_swedish_ci;";
  tester->wb->open_document("data/workbench/diff_table_replace_test.mwb");
  apply_sql_to_model(sql1);
  ensure("drop table in model", tester->get_catalog()->schemata().get(0)->tables().count() == 0);

  tester->wb->close_document();
  tester->wb->close_document_finish();
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  sync_plugin.reset();
  fwdeng_plugin.reset();
  delete tester;
}

END_TESTS
