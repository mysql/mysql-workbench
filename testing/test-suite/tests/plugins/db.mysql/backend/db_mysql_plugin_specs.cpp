/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#include "casmine.h"

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

#include "module_db_mysql_shared_code.h"

#include "wb_test_helpers.h"
#include "wb_connection_helpers.h"


/*
  We override get_model_catalog() method of the original plugin,
  to make testing setup easier, in real life the model catalog
  is taken from the GRT tree, and this is not tested here.
*/

using namespace parsers;
using namespace grt;

class DbMySQLScriptSyncTest : public DbMySQLScriptSync {
protected:
  db_mysql_CatalogRef model_catalog;
  virtual db_mysql_CatalogRef get_model_catalog() override {
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
  DictRef options;

  virtual db_mysql_CatalogRef get_model_catalog() override {
    return model_catalog;
  }
  virtual DictRef get_options_as_dict() override {
    return options;
  }

public:
  DbMySQLSQLExportTest(db_mysql_CatalogRef cat) : DbMySQLSQLExport(cat) {
    set_model_catalog(cat);
  }

  void set_model_catalog(db_mysql_CatalogRef catalog) {
    model_catalog = catalog;
  }
  void set_options_as_dict(DictRef options_dict) {
    options = options_dict;
  }
};

namespace {

$ModuleEnvironment() {};

$TestData {
  std::unique_ptr<WorkbenchTester> tester;
  std::shared_ptr<DbMySQLScriptSync> syncPlugin;
  std::shared_ptr<DbMySQLSQLExport> fwePlugin;
  sql::ConnectionWrapper connection;
  DbObjectMatchAlterOmf omf;

  std::string dataDir;

  //--------------------------------------------------------------------------------------------------------------------

  db_mysql_CatalogRef createCatalogFromScript(const std::string &sql) {
    GrtVersionRef version = tester->getRdbms()->version();
    db_mysql_CatalogRef cat = createEmptyCatalog();
    MySQLParserServices::Ref services = MySQLParserServices::get();
    MySQLParserContext::Ref context = services->createParserContext(tester->getRdbms()->characterSets(),
                                                                    tester->getRdbms()->version(), "", false);

    DictRef options(true);
    $expect(services->parseSQLIntoCatalog(context, cat, sql, options)).toEqual(0U, "SQL failed to parse: " + sql);

    return cat;
  }

  //--------------------------------------------------------------------------------------------------------------------

  std::string generateScript(const std::vector<std::string> &, db_mysql_CatalogRef org_cat, db_mysql_CatalogRef mod_cat) {
    syncPlugin.reset(new DbMySQLScriptSyncTest());
    static_cast<DbMySQLScriptSyncTest *>(syncPlugin.get())->set_model_catalog(mod_cat);
    syncPlugin->init_diff_tree(std::vector<std::string>(), mod_cat, org_cat);
    return syncPlugin->generate_diff_tree_script();
  }

  //--------------------------------------------------------------------------------------------------------------------

  void applyToModel(const std::vector<std::string> &schemata, db_mysql_CatalogRef org_cat, db_mysql_CatalogRef mod_cat) {
    syncPlugin.reset(new DbMySQLScriptSyncTest());
    static_cast<DbMySQLScriptSyncTest *>(syncPlugin.get())->set_model_catalog(mod_cat);
    syncPlugin->init_diff_tree(std::vector<std::string>(), org_cat, ValueRef());
    syncPlugin->apply_changes_to_model();
  }

  //--------------------------------------------------------------------------------------------------------------------

  std::string runFwGenerateScript(db_mysql_CatalogRef cat, DbMySQLSQLExportTest *plugin) {
    fwePlugin.reset(plugin);
    ValueRef retval = fwePlugin->export_task(StringRef());
    return fwePlugin->export_sql_script();
  }

  //--------------------------------------------------------------------------------------------------------------------

  std::shared_ptr<DiffChange> compareCatalogToServer(db_mysql_CatalogRef org_cat, const std::string &schema_name) {
    syncPlugin.reset(new DbMySQLScriptSyncTest());
    std::list<std::string> schemas = { "db_mysql_plugin_test" };
    db_mysql_CatalogRef cat = tester->reverseEngineerSchemas(schemas);
    if ((cat->schemata().get(0).is_valid()) && (cat->schemata().get(0)->name() == "mydb"))
      cat->schemata().remove(0);
    org_cat->oldName("");

    ValueRef default_engine = bec::GRTManager::get()->get_app_option("db.mysql.Table:tableEngine");
    std::string default_engine_name;
    if (StringRef::can_wrap(default_engine))
      default_engine_name = StringRef::cast_from(default_engine);

    bec::CatalogHelper::apply_defaults(cat, default_engine_name);
    bec::CatalogHelper::apply_defaults(org_cat, default_engine_name);

    NormalizedComparer comparer(DictRef(true));
    comparer.init_omf(&omf);

    std::shared_ptr<DiffChange> result = diff_make(cat, org_cat, &omf);

    tester->wb->close_document();
    tester->wb->close_document_finish();

    return result;
  }

  //--------------------------------------------------------------------------------------------------------------------

  void applySqlToModel(const std::string &sql) {
    db_mysql_CatalogRef org_cat = createCatalogFromScript(sql);

    std::vector<std::string> schemata;
    schemata.push_back("mydb");

    db_mysql_CatalogRef mod_cat = db_mysql_CatalogRef::cast_from(tester->getCatalog());

    std::string value;

    DbMySQLScriptSyncTest p;
    p.set_model_catalog(mod_cat);
    std::shared_ptr<DiffTreeBE> tree = p.init_diff_tree(std::vector<std::string>(), mod_cat, org_cat);

    // Apply everything back to model.
    tree->set_apply_direction(tree->get_root(), DiffNode::ApplyToModel, true);
    bec::NodeId mydb_node = tree->get_child(NodeId(), 0);
    bec::NodeId table1_node = tree->get_child(mydb_node, 0);
    tree->get_field(table1_node, DiffTreeBE::ModelObjectName, value);

    p.apply_changes_to_model();
  }

};

$describe("db.mysql plugin test") {

  $beforeAll([this] () {
    data->dataDir = casmine::CasmineContext::get()->tmpDataDir();

    data->tester.reset(new WorkbenchTester());
    data->tester->initializeRuntime();

    data->omf.dontdiff_mask = 3;

    data->connection = createConnectionForImport();

    // Modeling uses a default server version, which is not related to any server it might have
    // reverse engineered content from, nor where it was sync'ed to. So we have to mimic this here.
    std::string target_version = bec::GRTManager::get()->get_app_option_string("DefaultTargetMySQLVersion");
    if (target_version.empty())
      target_version = "8.0.16";
    data->tester->getRdbms()->version(parse_version(target_version));
  });

  $afterAll([this]() {
    data->syncPlugin.reset();
    data->fwePlugin.reset();
  });

  $it("Bug #32327", [this] () {
    std::string sql1 =
      "DROP DATABASE IF EXISTS `db_mysql_plugin_test`;"
      "CREATE DATABASE `db_mysql_plugin_test` DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;"
      "CREATE  TABLE IF NOT EXISTS `db_mysql_plugin_test`.`table1`"
      " (`idtable1` INT(11) NOT NULL , PRIMARY KEY (`idtable1`) ) ENGINE=InnoDB DEFAULT CHARSET=latin1 DEFAULT COLLATE = "
      "latin1_swedish_ci;";

    db_mysql_CatalogRef mod_cat = data->createCatalogFromScript(sql1);
    db_mysql_CatalogRef org_cat = copy_object(mod_cat);

    db_mysql_IndexRef pk = mod_cat->schemata().get(0)->tables().get(0)->indices().get(0);
    $expect(*pk->isPrimary()).Not.toEqual(0);

    // Rename PK.
    pk->name("mypk");

    std::vector<std::string> schemata;
    schemata.push_back("db_mysql_plugin_test");

    std::string script = data->generateScript(schemata, org_cat, mod_cat);

    std::unique_ptr<sql::Statement> stmt(data->connection->createStatement());
    data->tester->executeScript(stmt.get(), sql1);
    data->tester->executeScript(stmt.get(), script);

    std::shared_ptr<DiffChange> empty_change = data->compareCatalogToServer(org_cat, "db_mysql_plugin_test");

    $expect(empty_change).Not.toBeValid();
  });

  $it("Bug #32330", [this] () {
    std::string sql1 =
      "DROP DATABASE IF EXISTS `db_mysql_plugin_test`;"
      "CREATE DATABASE IF NOT EXISTS `db_mysql_plugin_test` DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;"
      "CREATE  TABLE IF NOT EXISTS `db_mysql_plugin_test`.`table1` (`idtable1` INT NOT NULL PRIMARY KEY) "
      " ENGINE=InnoDB DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;";

    db_mysql_CatalogRef mod_cat = data->createCatalogFromScript(sql1);
    db_mysql_CatalogRef org_cat = copy_object(mod_cat);

    $expect(mod_cat->schemata().get(0)->tables().count()).toEqual(1U);

    std::vector<std::string> schemata;
    schemata.push_back("db_mysql_plugin_test");

    // Remove table.
    mod_cat->schemata().get(0)->tables().remove(0);

    std::string script = data->generateScript(schemata, org_cat, mod_cat);

    std::unique_ptr<sql::Statement> stmt(data->connection->createStatement());
    data->tester->executeScript(stmt.get(), sql1);
    data->tester->executeScript(stmt.get(), script);

    std::shared_ptr<DiffChange> empty_change = data->compareCatalogToServer(mod_cat, "db_mysql_plugin_test");

    $expect(empty_change).Not.toBeValid();
  });

  $it("Bug #32334", [this] () {
    std::string sql1 =
      "DROP DATABASE IF EXISTS `db_mysql_plugin_test`;"
      "CREATE DATABASE IF NOT EXISTS `db_mysql_plugin_test` DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;"
      "CREATE  TABLE IF NOT EXISTS `db_mysql_plugin_test`.`table1` (`idtable1` INT NOT NULL, PRIMARY KEY (`idtable1`) ) "
      "ENGINE = MyISAM CHARSET = latin1 DEFAULT COLLATE = latin1_swedish_ci;"
      "CREATE  TABLE IF NOT EXISTS `db_mysql_plugin_test`.`table2` (`idtable1` INT NOT NULL, PRIMARY KEY (`idtable1`) ) "
      "ENGINE = MyISAM CHARSET = latin1 DEFAULT COLLATE = latin1_swedish_ci;"
      "CREATE  TABLE IF NOT EXISTS `db_mysql_plugin_test`.`table3` (`idtable1` INT NOT NULL, PRIMARY KEY (`idtable1`) ) "
      "ENGINE = MyISAM CHARSET = latin1 DEFAULT COLLATE = latin1_swedish_ci;";

    db_mysql_CatalogRef mod_cat = data->createCatalogFromScript(sql1);
    db_mysql_CatalogRef org_cat = copy_object(mod_cat);

    $expect(mod_cat->schemata().get(0)->tables().count()).toEqual(3U);

    // Set table options.
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

    std::string script = data->generateScript(schemata, org_cat, mod_cat);

    std::unique_ptr<sql::Statement> stmt(data->connection->createStatement());
    data->tester->executeScript(stmt.get(), sql1);
    data->tester->executeScript(stmt.get(), script);
  });

  $it("Bug #32336", [this] () {
    std::string sql1 =
      "DROP DATABASE IF EXISTS `db_mysql_plugin_test`;"
      "CREATE DATABASE IF NOT EXISTS `db_mysql_plugin_test` DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;"
      "CREATE  TABLE IF NOT EXISTS `db_mysql_plugin_test`.`table1` (`idtable1` INT NOT NULL PRIMARY KEY) "
      " ENGINE=InnoDB DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;";

    db_mysql_CatalogRef mod_cat = data->createCatalogFromScript(sql1);
    db_mysql_CatalogRef org_cat = copy_object(mod_cat);

    $expect(mod_cat->schemata().get(0)->tables().count()).toEqual(1U);

    // Insert an invalid column.
    db_mysql_TableRef table = mod_cat->schemata().get(0)->tables().get(0);
    db_mysql_ColumnRef column(Initialized);
    column->owner(table);
    column->name("col1");
    table->columns().insert(column);

    std::vector<std::string> schemata;
    schemata.push_back("db_mysql_plugin_test");

    std::string script = data->generateScript(schemata, org_cat, mod_cat);

    std::unique_ptr<sql::Statement> stmt(data->connection->createStatement());
    data->tester->executeScript(stmt.get(), sql1);

    $expect([&] () { data->tester->executeScript(stmt.get(), script); }).toThrow();
  });

  $it("Bug #32358", [this] () {
    std::string sql1 =
      "DROP DATABASE IF EXISTS `db_mysql_plugin_test`;"
      "CREATE DATABASE IF NOT EXISTS `db_mysql_plugin_test` DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;"
      "CREATE  TABLE IF NOT EXISTS `db_mysql_plugin_test`.`table1` (`idtable1` INT NOT NULL PRIMARY KEY) "
      "ENGINE=InnoDB DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;"
      "CREATE  TABLE IF NOT EXISTS `db_mysql_plugin_test`.`table2` "
      "(`idtable2` INT NOT NULL DEFAULT 100 , `col1` VARCHAR(45) NULL , PRIMARY KEY (`idtable2`) ) "
      " ENGINE=InnoDB DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;";

    std::string sql2 = "DROP DATABASE IF EXISTS `db_mysql_plugin_test`;";

    db_mysql_CatalogRef mod_cat = data->createCatalogFromScript(sql1);
    $expect(mod_cat->schemata().get(0)->tables().count()).toEqual(2U);

    // Insert an self-referencing FK.
    db_mysql_TableRef table = mod_cat->schemata().get(0)->tables().get(1);
    db_mysql_ForeignKeyRef fk(Initialized);
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

    std::string script = data->generateScript(schemata, db_mysql_CatalogRef(Initialized), mod_cat);

    std::unique_ptr<sql::Statement> stmt(data->connection->createStatement());
    data->tester->executeScript(stmt.get(), sql2);
  });

  $it("Bug #32367", [this] () {
    std::string sql1 =
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

    std::unique_ptr<sql::Statement> stmt(data->connection->createStatement());
    data->tester->executeScript(stmt.get(), sql1);

    std::list<std::string> schemata_list;
    schemata_list.push_back("db_mysql_plugin_test");
    db_mysql_CatalogRef mod_cat = copy_object(data->tester->reverseEngineerSchemas(schemata_list));
    db_mysql_CatalogRef org_cat = copy_object(mod_cat);
    data->tester->wb->close_document();
    data->tester->wb->close_document_finish();

    $expect(mod_cat->schemata().get(0)->tables().count()).toEqual(3U);
    $expect(mod_cat->schemata().get(0)->tables().get(0)->triggers().count()).toEqual(1U);
    $expect(mod_cat->schemata().get(0)->routines().count()).toEqual(1U);

    // Delete a table, a routine and a trigger.
    mod_cat->schemata().get(0)->tables().remove(2);
    mod_cat->schemata().get(0)->tables().get(0)->triggers().remove(0);
    mod_cat->schemata().get(0)->routines().remove(0);

    std::vector<std::string> schemata;
    schemata.push_back("db_mysql_plugin_test");

    std::string script = data->generateScript(schemata, org_cat, mod_cat);
    data->tester->executeScript(stmt.get(), script);

    db_mysql_CatalogRef new_cat = data->tester->reverseEngineerSchemas(schemata_list);

    $expect(new_cat->schemata().get(0)->tables().count()).toEqual(2U);
    $expect(new_cat->schemata().get(0)->tables().get(0)->triggers().count()).toEqual(0U);
    $expect(new_cat->schemata().get(0)->routines().count()).toEqual(0U);

    data->tester->wb->close_document();
    data->tester->wb->close_document_finish();
  });

  $it("Bug #32371", [this] () {
    std::string sql1 =
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

    // Part1 - check that unmodified procedure is not updated.
    std::unique_ptr<sql::Statement> stmt(data->connection->createStatement());
    data->tester->executeScript(stmt.get(), sql1);

    std::list<std::string> schemata_list;
    schemata_list.push_back("db_mysql_plugin_test");
    db_mysql_CatalogRef mod_cat = copy_object(data->tester->reverseEngineerSchemas(schemata_list));
    db_mysql_CatalogRef org_cat = copy_object(mod_cat);

    data->tester->wb->close_document();
    data->tester->wb->close_document_finish();

    $expect(mod_cat->schemata().get(0)->tables().count()).toEqual(1U);
    $expect(mod_cat->schemata().get(0)->routines().count()).toEqual(2U);

    std::shared_ptr<DiffChange> empty_change = data->compareCatalogToServer(mod_cat, "db_mysql_plugin_test");

    $expect(empty_change).Not.toBeValid();

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

    // Modify routines.
    mod_cat->schemata().get(0)->routines().get(0)->sqlDefinition(def1);
    mod_cat->schemata().get(0)->routines().get(1)->sqlDefinition(def2);

    std::vector<std::string> schemata;
    schemata.push_back("db_mysql_plugin_test");

    std::string script = data->generateScript(schemata, org_cat, mod_cat);
    data->tester->executeScript(stmt.get(), script);
  });

  $it("Bug #32329", [this] () {
    std::string sql1 =
      "DROP DATABASE IF EXISTS `db_mysql_plugin_test`;"
      "CREATE DATABASE IF NOT EXISTS `db_mysql_plugin_test` DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;\n"
      "CREATE  TABLE IF NOT EXISTS `db_mysql_plugin_test`.`table1` "
      "(`idtable1` INT NOT NULL , `col1` VARCHAR(45) NULL , PRIMARY KEY (`idtable1`) , INDEX idx1 (`idtable1` ASC, "
      "`col1` ASC) ) engine = MyISAM;";

    db_mysql_CatalogRef mod_cat = data->createCatalogFromScript(sql1);
    db_mysql_CatalogRef org_cat = copy_object(mod_cat);

    $expect(mod_cat->schemata().get(0)->tables().get(0)->indices().get(1)->columns().count()).toEqual(2U);

    // Delete column `col1` from index idx1.
    mod_cat->schemata().get(0)->tables().get(0)->indices().get(1)->columns().remove(1);

    std::vector<std::string> schemata;
    schemata.push_back("db_mysql_plugin_test");

    std::string script = data->generateScript(schemata, org_cat, mod_cat);

    std::unique_ptr<sql::Statement> stmt(data->connection->createStatement());
    data->tester->executeScript(stmt.get(), sql1);
    data->tester->executeScript(stmt.get(), script);

    std::shared_ptr<DiffChange> empty_change = data->compareCatalogToServer(mod_cat, "db_mysql_plugin_test");

    $expect(empty_change).Not.toBeValid();
  });

  $it("Bug #32324", [this] () {
    std::string sql1 =
      "DROP DATABASE IF EXISTS `db_mysql_plugin_test`;"
      "CREATE DATABASE IF NOT EXISTS `db_mysql_plugin_test` DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;\n"
      "CREATE  TABLE IF NOT EXISTS `db_mysql_plugin_test`.`table1` "
      "(`idtable1` INT NOT NULL , `col1` VARCHAR(45) NULL , `col2` VARCHAR(45) NULL , PRIMARY KEY (`idtable1`) ) engine "
      "= MyISAM;";

    db_mysql_CatalogRef mod_cat = data->createCatalogFromScript(sql1);
    db_mysql_CatalogRef org_cat = copy_object(mod_cat);

    $expect(mod_cat->schemata().get(0)->tables().get(0)->columns().count()).toEqual(3U);

    // Move `col1` after `col2`.
    db_mysql_TableRef table = mod_cat->schemata().get(0)->tables().get(0);
    db_mysql_ColumnRef col1 = table->columns().get(1);
    table->columns().remove(1);
    table->columns().insert(col1);

    std::vector<std::string> schemata;
    schemata.push_back("db_mysql_plugin_test");

    std::string script = data->generateScript(schemata, org_cat, mod_cat);

    std::unique_ptr<sql::Statement> stmt(data->connection->createStatement());
    data->tester->executeScript(stmt.get(), sql1);
    data->tester->executeScript(stmt.get(), script);

    std::shared_ptr<DiffChange> empty_change = data->compareCatalogToServer(mod_cat, "db_mysql_plugin_test");

    $expect(empty_change).Not.toBeValid();
  });

  $it("Bug #32331", [this] () {
    std::string sql1 =
      "DROP DATABASE IF EXISTS `db_mysql_plugin_test`;"
      "CREATE DATABASE IF NOT EXISTS `db_mysql_plugin_test` DEFAULT CHARSET=latin1 DEFAULT COLLATE = latin1_swedish_ci;\n"
      "CREATE VIEW `db_mysql_plugin_test`.`view2` AS SELECT * FROM `db_mysql_plugin_test`.`view1`;"
      "CREATE VIEW `db_mysql_plugin_test`.`view1` AS SELECT 1;";

    db_mysql_CatalogRef mod_cat = data->createCatalogFromScript(sql1);

    $expect(mod_cat->schemata().get(0)->views().count()).toEqual(2U);

    // First test export.
    DbMySQLSQLExportTest *plugin = new DbMySQLSQLExportTest(mod_cat);
    plugin->set_option("ViewsAreSelected", true);

    DictRef options(true);
    StringListRef views(Initialized);
    views.insert(get_old_object_name_for_key(mod_cat->schemata().get(0)->views().get(0), false), false);
    views.insert(get_old_object_name_for_key(mod_cat->schemata().get(0)->views().get(1), false), false);
    options.set("ViewFilterList", views);
    plugin->set_options_as_dict(options);

    std::string script = data->runFwGenerateScript(mod_cat, plugin);

    std::unique_ptr<sql::Statement> stmt(data->connection->createStatement());
    data->tester->executeScript(stmt.get(), script);

    std::vector<std::string> schemata;
    schemata.push_back("db_mysql_plugin_test");

    // Now the same test for sync.
    script.assign(data->generateScript(schemata, db_mysql_CatalogRef(Initialized), mod_cat));

    std::unique_ptr<sql::Statement> stmt2(data->connection->createStatement());
    data->tester->executeScript(stmt2.get(), "DROP DATABASE IF EXISTS `db_mysql_plugin_test`");
    data->tester->executeScript(stmt2.get(), script);

    // XXX: what is being tested here?
  });

  $it("Bug #37634", [this] () {
    // Update model figures (and FKs) when a table is replaced
    // during the db/script to model synchronization.

    std::string sql1 =
      "CREATE SCHEMA IF NOT EXISTS `mydb` DEFAULT CHARACTER SET latin1 COLLATE latin1_swedish_ci;\n"
      "USE `mydb`;\n"
      "CREATE  TABLE IF NOT EXISTS `mydb`.`table1` (\n"
      "  `idtable1` INT NOT NULL ,\n"
      "  PRIMARY KEY (`idtable1`) )\n"
      "ENGINE = InnoDB;";

    data->tester->wb->open_document("data/workbench/diff_table_replace_test.mwb");

    db_mgmt_ManagementRef mgmt(db_mgmt_ManagementRef::cast_from(GRT::get()->get("/wb/rdbmsMgmt")));

    ListRef<db_DatatypeGroup> grouplist =
      ListRef<db_DatatypeGroup>::cast_from(GRT::get()->unserialize(data->tester->wboptions->basedir + "/data/db_datatype_groups.xml"));
    replace_contents(mgmt->datatypeGroups(), grouplist);

    db_TableRef t1 = data->tester->getCatalog()->schemata().get(0)->tables().get(0);
    $expect(GRT::get()->get("/wb/doc/physicalModels/0/diagrams/0/figures/0/table"))
      .toEqual(t1, "before update table is referenced from figure 0");

    $expect(GRT::get()->get("/wb/doc/physicalModels/0/diagrams/1/figures/0/table"))
    .toEqual(t1, "before update table is referenced from figure 1");

    db_mysql_CatalogRef org_cat = data->createCatalogFromScript(sql1);

    std::vector<std::string> schemata;
    schemata.push_back("mydb");

    db_mysql_CatalogRef mod_cat = db_mysql_CatalogRef::cast_from(data->tester->getCatalog());

    std::string value;

    DbMySQLScriptSyncTest p;
    p.set_model_catalog(mod_cat);
    std::shared_ptr<DiffTreeBE> tree = p.init_diff_tree(std::vector<std::string>(), org_cat, ValueRef());

    // Change apply direction for table table1.
    bec::NodeId mydb_node = tree->get_child(NodeId(), 0);
    bec::NodeId table1_node = tree->get_child(mydb_node, 0);
    tree->get_field(table1_node, DiffTreeBE::ModelObjectName, value);

    p.set_next_apply_direction(table1_node);
    p.set_next_apply_direction(table1_node);

    p.apply_changes_to_model();

    db_TableRef t2 = data->tester->getCatalog()->schemata().get(0)->tables().get(0);
    $expect(GRT::get()->get("/wb/doc/physicalModels/0/diagrams/0/figures/0/table"))
      .toEqual(t2, "before update table is referenced from figure 0");

    $expect(GRT::get()->get("/wb/doc/physicalModels/0/diagrams/1/figures/0/table"))
      .toEqual(t2, "before update table is referenced from figure 1");

    data->tester->wb->close_document();
    data->tester->wb->close_document_finish();
  });

  $it("Column type change", [this] () {
    std::string sql1 =
      "CREATE SCHEMA IF NOT EXISTS `mydb` DEFAULT CHARACTER SET latin1 COLLATE latin1_swedish_ci;\n"
      "USE `mydb`;\n"
      "CREATE  TABLE IF NOT EXISTS `mydb`.`table1` (\n"
      "  `idtable1` TINYINT NOT NULL ,\n"
      "  PRIMARY KEY (`idtable1`) )\n"
      "ENGINE = InnoDB;";
    data->tester->wb->open_document(data->dataDir + "/workbench/diff_table_replace_test.mwb");
    data->applySqlToModel(sql1);

    db_TableRef t2 = data->tester->getCatalog()->schemata().get(0)->tables().get(0);
    db_ColumnRef col = t2->columns().get(0);
    db_SimpleDatatypeRef dtype = col->simpleType();
    $expect(*dtype->name()).toEqual("TINYINT", "Column type not changed");

    data->tester->wb->close_document();
    data->tester->wb->close_document_finish();
  });

  $it("Schema collation/charset change", [this] () {
    std::string sql1 = "CREATE schema IF NOT EXISTS `mydb` DEFAULT CHARACTER SET latin1 COLLATE latin1_swedish_ci;";
    data->tester->wb->open_document(data->dataDir + "/workbench/diff_table_replace_test.mwb");
    data->applySqlToModel(sql1);
    $expect(data->tester->getCatalog()->schemata().get(0)->tables().count()).toEqual(0U);

    data->tester->wb->close_document();
    data->tester->wb->close_document_finish();
  });

}

}
