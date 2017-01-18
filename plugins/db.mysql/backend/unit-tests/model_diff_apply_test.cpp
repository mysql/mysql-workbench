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

#include "grtsqlparser/sql_facade.h"
#include "db_mysql_diffsqlgen.h"

#include "diff/diffchange.h"
#include "diff/grtdiff.h"
#include "diff/changeobjects.h"
#include "diff/changelistobjects.h"
#include "grtdb/diff_dbobjectmatch.h"

#include "myx_statement_parser.h"
#include "backend/db_mysql_sql_script_sync.h"
#include "backend/db_mysql_sql_export.h"
#include "module_db_mysql.h"
#include "wb_helpers.h"

/*
  We override get_model_catalog() method of the original plugin,
  to make testing setup easier, in real life the model catalog
  is taken from the GRT tree, and this is not tested here
*/

class DbMySQLScriptSyncTest : public DbMySQLScriptSync {
protected:
  db_mysql_CatalogRef model_catalog;
  virtual db_mysql_CatalogRef get_model_catalog() {
    return model_catalog;
  }

public:
  void set_model_catalog(const db_mysql_CatalogRef& catalog) {
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

BEGIN_TEST_DATA_CLASS(model_diff_apply)
protected:
WBTester* tester;
std::auto_ptr<DbMySQLScriptSync> sync_plugin;
std::auto_ptr<DbMySQLSQLExport> fwdeng_plugin;
SqlFacade::Ref sql_parser;
sql::ConnectionWrapper connection;
grt::DbObjectMatchAlterOmf omf;

db_mysql_CatalogRef create_catalog_from_script(const std::string& sql);

std::string run_sync_plugin_generate_script(const std::vector<std::string>&, db_mysql_CatalogRef org_cat,
                                            db_mysql_CatalogRef mod_cat);

void run_sync_plugin_apply_to_model(const std::vector<std::string>& schemata, db_mysql_CatalogRef org_cat,
                                    db_mysql_CatalogRef mod_cat);

std::string run_fwdeng_plugin_generate_script(db_mysql_CatalogRef cat, DbMySQLSQLExportTest* plugin);
std::shared_ptr<DiffChange> compare_catalog_to_server_schema(db_mysql_CatalogRef org_cat,
                                                             const std::string& schema_name);
void apply_sql_to_model(const std::string& sql);
all_objects_mwb get_model_objects();
TEST_DATA_CONSTRUCTOR(model_diff_apply) {
  tester = new WBTester();
  // init datatypes
  populate_grt(*tester);

  omf.dontdiff_mask = 3;

  // init database connection
  connection = tester->create_connection_for_import();

  sql_parser = SqlFacade::instance_for_rdbms_name("Mysql");
  ensure("failed to get sqlparser module", (NULL != sql_parser));
}

END_TEST_DATA_CLASS

TEST_MODULE(model_diff_apply, "db.mysql plugin test");

db_mysql_CatalogRef tut::Test_object_base<model_diff_apply>::create_catalog_from_script(const std::string& sql) {
  db_mysql_CatalogRef cat = create_empty_catalog_for_import();
  sql_parser->parseSqlScriptString(cat, sql);
  return cat;
}

std::string tut::Test_object_base<model_diff_apply>::run_sync_plugin_generate_script(
  const std::vector<std::string>& schemata, db_mysql_CatalogRef org_cat, db_mysql_CatalogRef mod_cat) {
  sync_plugin.reset(new DbMySQLScriptSyncTest());
  static_cast<DbMySQLScriptSyncTest*>(sync_plugin.get())->set_model_catalog(mod_cat);
  sync_plugin->init_diff_tree(std::vector<std::string>(), mod_cat, org_cat, grt::StringListRef());
  return sync_plugin->generate_diff_tree_script();
}

void tut::Test_object_base<model_diff_apply>::run_sync_plugin_apply_to_model(const std::vector<std::string>& schemata,
                                                                             db_mysql_CatalogRef org_cat,
                                                                             db_mysql_CatalogRef mod_cat) {
  sync_plugin.reset(new DbMySQLScriptSyncTest());
  static_cast<DbMySQLScriptSyncTest*>(sync_plugin.get())->set_model_catalog(mod_cat);
  sync_plugin->init_diff_tree(std::vector<std::string>(), mod_cat, org_cat, grt::StringListRef());
  sync_plugin->apply_changes_to_model();
}

std::string tut::Test_object_base<model_diff_apply>::run_fwdeng_plugin_generate_script(db_mysql_CatalogRef cat,
                                                                                       DbMySQLSQLExportTest* plugin) {
  fwdeng_plugin.reset(plugin);
  ValueRef retval = fwdeng_plugin->export_task(grt::StringRef());
  return fwdeng_plugin->export_sql_script();
}

std::shared_ptr<DiffChange> tut::Test_object_base<model_diff_apply>::compare_catalog_to_server_schema(
  db_mysql_CatalogRef org_cat, const std::string& schema_name) {
  sync_plugin.reset(new DbMySQLScriptSyncTest());
  std::list<std::string> schemata;
  schemata.push_back("model_diff_apply");
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

  return diff_make(cat, org_cat, &omf);
}

void tut::Test_object_base<model_diff_apply>::apply_sql_to_model(const std::string& sql) {
  db_mysql_CatalogRef org_cat = create_catalog_from_script(sql);

  std::vector<std::string> schemata;
  schemata.push_back("mydb");

  db_mysql_CatalogRef mod_cat = db_mysql_CatalogRef::cast_from(tester->get_catalog());

  DbMySQLSQLExportTest* plugin = new DbMySQLSQLExportTest(mod_cat);

  grt::DictRef options(true);
  options.set("UseFilteredLists", grt::IntegerRef(0));
  plugin->set_options_as_dict(options);

  std::string value;

  DbMySQLScriptSyncTest p;
  p.set_model_catalog(mod_cat);
  std::shared_ptr<DiffTreeBE> tree =
    p.init_diff_tree(std::vector<std::string>(), mod_cat, org_cat, grt::StringListRef());

  // apply everything back to model
  tree->set_apply_direction(tree->get_root(), DiffNode::ApplyToModel, true);
  bec::NodeId mydb_node = tree->get_child(NodeId(), 0);
  bec::NodeId table1_node = tree->get_child(mydb_node, 0);
  tree->get_field(table1_node, DiffTreeBE::ModelObjectName, value);

  p.apply_changes_to_model();
}

static const char* create_schema =
  "CREATE SCHEMA IF NOT EXISTS `mydb` DEFAULT CHARACTER SET latin1 COLLATE latin1_swedish_ci ;\n"
  "USE `mydb` ;";

static const char* create_table1 =
  ""
  "CREATE  TABLE IF NOT EXISTS `mydb`.`table1` (\n"
  "  `idtable1` INT NOT NULL AUTO_INCREMENT ,\n"
  "  `name` VARCHAR(45) NULL DEFAULT 'noname' ,\n"
  "  `email` VARCHAR(45) NULL ,\n"
  "  PRIMARY KEY (`idtable1`) );\n";

static const char* create_table1_trigger_delim =
  "DELIMITER $$\n"
  "USE `mydb`$$";

static const char* create_table1_trigger =
  "CREATE\n"
  "DEFINER=`root`@`localhost`\n"
  "TRIGGER `mydb`.`tr1`\n"
  "BEFORE INSERT ON `mydb`.`table1`\n"
  "FOR EACH ROW\n"
  "set new.idtable1 = 1";

static const char* create_table1_trigger_end_delim = "$$\nDELIMITER ;\n";

static const char* create_table2 =
  "CREATE  TABLE IF NOT EXISTS `mydb`.`table2` (\n"
  "  `table1_idtable1` INT NULL ,\n"
  "  INDEX `fktable1_idx` (`table1_idtable1` ASC) ,\n"
  "  CONSTRAINT `fktable1`\n"
  "    FOREIGN KEY (`table1_idtable1` )\n"
  "    REFERENCES `mydb`.`table1` (`idtable1` )\n"
  "    ON DELETE NO ACTION\n"
  "    ON UPDATE NO ACTION);\n";

static const char* create_procedure_delim =
  "DELIMITER $$\n"
  "USE `mydb`$$\n";

static const char* create_procedure =
  "CREATE DEFINER=`root`@`localhost` PROCEDURE `routine1`(OUT p INT)\r\n"
  "BEGIN\r\n"
  "select 1 into p;\r\n"
  "END";

static const char* create_procedure_end_delim =
  "$$\n"
  "DELIMITER ;\n";

static const char* create_view_use = "USE `mydb`;\n";

static const char* create_view =
  "CREATE  OR REPLACE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL SECURITY DEFINER VIEW `mydb`.`view1` AS "
  "select 1 AS `1`";

struct all_objects_sql {
  std::string schema_sql;
  std::string table1_sql;
  std::string table2_sql;
  std::string trigger_sql;
  std::string procedure_sql;
  std::string view_sql;
  all_objects_sql()
    : schema_sql(create_schema),
      table1_sql(create_table1),
      table2_sql(create_table2),
      trigger_sql(create_table1_trigger),
      procedure_sql(create_procedure),
      view_sql(create_view) {
  }
  std::string get_sql() {
    return schema_sql.append(table1_sql)
      .append(table2_sql)
      .append(create_table1_trigger_delim)
      .append(trigger_sql)
      .append(create_table1_trigger_end_delim)
      .append(create_procedure_delim)
      .append(procedure_sql)
      .append(create_procedure_end_delim)
      .append(create_view_use)
      .append(view_sql)
      .append(";\n");
  };
};

class validate_property {
  bool enabled;

public:
  validate_property(const bool enabled_flag = true) : enabled(enabled_flag){};
  virtual ~validate_property(){};
  void enable() {
    enabled = true;
  };
  void disable() {
    enabled = false;
  };
  virtual void validate(const all_objects_mwb& objects) = 0;
  void operator()(const all_objects_mwb& objects) {
    if (enabled)
      validate(objects);
  }
};

class t1_validator : public validate_property {
public:
  virtual void validate(const all_objects_mwb& objects) {
    ensure("t1 is invalid", objects.t1.is_valid());
    ensure("unexpected column count for t1", objects.t1->columns().count() == 3);
  };
};

class t2_validator : public validate_property {
public:
  virtual void validate(const all_objects_mwb& objects) {
    ensure("t2 is invalid", objects.t2.is_valid());
    ensure("unexpected column count for t2", objects.t2->columns().count() == 1);
  };
};

class FK_validator : public validate_property {
public:
  virtual void validate(const all_objects_mwb& objects) {
    ensure("FK is invalid", objects.FK.is_valid());
    ensure("Wrong referenced column count", objects.FK->referencedColumns().count() == 1);
    db_ColumnRef refcol = objects.FK->referencedColumns().get(0);
    db_ColumnRef t1col = find_named_object_in_list(objects.t1->columns(), "idtable1");
    ensure("Wrong column reference in t2 FK", refcol == t1col);
  };
};

class view_validator : public validate_property {
public:
  virtual void validate(const all_objects_mwb& objects) {
    ensure("View is invalid", objects.view.is_valid());
    std::string viewdef = objects.view->sqlDefinition();
    ensure("View definition doesn't match", viewdef == create_view);
  };
};

class routine_validator : public validate_property {
public:
  virtual void validate(const all_objects_mwb& objects) {
    ensure("Routine is invalid", objects.routine.is_valid());
    std::string sqldef = objects.routine->sqlDefinition();
    if (sqldef.find("\n\n") == 0)
      sqldef = sqldef.substr(2);
    ensure("Routine definition doesn't match", sqldef == create_procedure);
  };
};

class trigger_validator : public validate_property {
public:
  virtual void validate(const all_objects_mwb& objects) {
    ensure("Trigger is invalid", objects.trigger.is_valid());
    std::string sqldef = objects.trigger->sqlDefinition();
    if (sqldef.find("\n\n") == 0)
      sqldef = sqldef.substr(2);
    ensure("Trigger definition doesn't match", sqldef == create_table1_trigger);
  };
};

struct all_objects_mwb_validator {
  t1_validator validate_t1;
  t2_validator validate_t2;
  FK_validator validate_FK;
  view_validator validate_view;
  routine_validator validate_routine;
  trigger_validator validate_trigger;
  void validate(const all_objects_mwb& objects) {
    validate_t1(objects);
    validate_t2(objects);
    validate_FK(objects);
    validate_view(objects);
    validate_routine(objects);
    validate_trigger(objects);
  };
};

all_objects_mwb tut::Test_object_base<model_diff_apply>::get_model_objects() {
  all_objects_mwb objects;
  if (tester->get_catalog()->schemata().count() == 0)
    return objects;
  objects.schema = tester->get_catalog()->schemata().get(0);

  objects.t1 = find_named_object_in_list(objects.schema->tables(), "table1");
  objects.t2 = find_named_object_in_list(objects.schema->tables(), "table2");
  if (objects.t1.is_valid())
    objects.trigger = find_named_object_in_list(objects.t1->triggers(), "tr1");
  if (objects.t2.is_valid())
    objects.FK = find_named_object_in_list(objects.t2->foreignKeys(), "fktable1");

  objects.view = find_named_object_in_list(objects.schema->views(), "view1");

  objects.routine = find_named_object_in_list(objects.schema->routines(), "routine1");
  return objects;
}

// Nothing changed, checking unaltered model
TEST_FUNCTION(2) {
  all_objects_sql sql;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();
  all_objects_mwb_validator validator;
  validator.validate(objects);
}

TEST_FUNCTION(3) {
  tester->wb->open_document("data/workbench/all_objects.mwb");
  apply_sql_to_model("");

  all_objects_mwb objects = get_model_objects();
  ensure("Schema wasn't dropped", !objects.schema.is_valid());
}

// 2x are Table tests, 3x Column, 4x FK, 5x View, 6x Procedure, 7x Trigger

// Tables
TEST_FUNCTION(20) {
  static const char* create_table1_altered =
    ""
    "CREATE  TABLE IF NOT EXISTS `mydb`.`table1` (\n"
    "  `idtable1` INT NOT NULL AUTO_INCREMENT ,\n"
    "  `email` VARCHAR(45) NULL ,\n"
    "  PRIMARY KEY (`idtable1`) );\n";

  all_objects_sql sql;
  sql.table1_sql = create_table1_altered;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();
  ensure("remove table column", objects.t1->columns().count() == 2);
  all_objects_mwb_validator validator;
  validator.validate_t1.disable();
  validator.validate(objects);
}

TEST_FUNCTION(21) {
  all_objects_sql sql;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  all_objects_mwb initial_objects = get_model_objects();
  // rename t1
  initial_objects.t1->name("table1_renamed");
  // now t1 should get it's initial name back
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();
  all_objects_mwb_validator validator;
  validator.validate(objects);
}

TEST_FUNCTION(22) {
  all_objects_sql sql;
  all_objects_mwb_validator validator;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  all_objects_mwb initial_objects = get_model_objects();
  validator.validate(initial_objects);
  // rename t2
  initial_objects.t2->name("table2_renamed");
  // now t2 should get it's initial name back
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();
  validator.validate(objects);
}

TEST_FUNCTION(23) {
  all_objects_sql sql;
  all_objects_mwb_validator validator;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  all_objects_mwb initial_objects = get_model_objects();
  validator.validate(initial_objects);
  // rename t1 and t2
  initial_objects.t1->name("table1_renamed");
  initial_objects.t2->name("table2_renamed");
  // now t1 and t2 should get it's initial names back
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();
  validator.validate(objects);
}

// column
TEST_FUNCTION(30) {
  static const char* create_table1_altered =
    ""
    "CREATE  TABLE IF NOT EXISTS `mydb`.`table1` (\n"
    "  `idtable1` INT NOT NULL AUTO_INCREMENT ,\n"
    "  PRIMARY KEY (`idtable1`) );\n";

  all_objects_sql sql;
  sql.table1_sql = create_table1_altered;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();
  ensure("remove table column", objects.t1->columns().count() == 1);
}

TEST_FUNCTION(31) {
  static const char* create_table1_altered =
    ""
    "CREATE  TABLE IF NOT EXISTS `mydb`.`table1` (\n"
    "  `idtable1` INT NOT NULL AUTO_INCREMENT ,\n"
    "  `email` VARCHAR(45) NULL ,\n"
    "  `name` VARCHAR(45) NULL DEFAULT 'noname' ,\n"
    "  PRIMARY KEY (`idtable1`) );\n";

  all_objects_sql sql;
  sql.table1_sql = create_table1_altered;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();
  all_objects_mwb_validator validator;
  validator.validate(objects);
  ensure("Column order wasn't changed", objects.t1->columns().get(2)->name() == "name");
}

TEST_FUNCTION(32) {
  static const char* create_table3 =
    ""
    "CREATE  TABLE IF NOT EXISTS `mydb`.`table3` (\n"
    "  `idtable1` INT NOT NULL AUTO_INCREMENT ,\n"
    "  `email` VARCHAR(45) NULL ,\n"
    "  PRIMARY KEY (`idtable1`) );\n";

  all_objects_sql sql;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  apply_sql_to_model(sql.get_sql().append(create_table3));

  all_objects_mwb objects = get_model_objects();
  ensure("Add Table failed", objects.schema->tables()->count() == 3);
  all_objects_mwb_validator validator;
  validator.validate(objects);
}

TEST_FUNCTION(33) {
  static const char* create_table1_added_col =
    ""
    "CREATE  TABLE IF NOT EXISTS `mydb`.`table1` (\n"
    "  `idtable1` INT NOT NULL AUTO_INCREMENT ,\n"
    "  `name` VARCHAR(45) NULL DEFAULT 'noname' ,\n"
    "  `newcol` VARCHAR(45) NULL DEFAULT 'newcol' ,\n"
    "  `email` VARCHAR(45) NULL ,\n"
    "  PRIMARY KEY (`idtable1`) );\n";

  all_objects_sql sql;
  sql.table1_sql = create_table1_added_col;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  all_objects_mwb initial_objects = get_model_objects();
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();
  all_objects_mwb_validator validator;
  validator.validate_t1.disable();
  validator.validate(objects);
  ensure("Error adding column", objects.t1->columns()->count() == 4);
  ensure("Column added on wrong pos", objects.t1->columns().get(2)->name() == "newcol");
}

TEST_FUNCTION(34) {
  static const char* create_table1_altered =
    ""
    "CREATE  TABLE IF NOT EXISTS `mydb`.`table1` (\n"
    "  `idtable1` INT NOT NULL AUTO_INCREMENT ,\n"
    "  `email` VARCHAR(45) NULL ,\n"
    "  `name` TINYINT,\n"
    "  PRIMARY KEY (`idtable1`) );\n";

  all_objects_sql sql;
  sql.table1_sql = create_table1_altered;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();
  all_objects_mwb_validator validator;
  validator.validate(objects);
  ensure("Column order wasn't changed", objects.t1->columns().get(2)->name() == "name");
  db_ColumnRef col = objects.t1->columns().get(2);
  db_SimpleDatatypeRef dtype = col->simpleType();
  ensure("Column type not changed", dtype->name() == "TINYINT");
}

TEST_FUNCTION(35) {
  static const char* create_table1_added_col =
    ""
    "CREATE  TABLE IF NOT EXISTS `mydb`.`table1` (\n"
    "  `idtable1` INT NOT NULL AUTO_INCREMENT ,\n"
    "  `name` VARCHAR(45) NULL DEFAULT 'noname' ,\n"
    "  `newcol` VARCHAR(45) NULL DEFAULT 'newcol' ,\n"
    "  `email` VARCHAR(45) NULL ,\n"
    "  `newcol2` VARCHAR(45) NULL DEFAULT 'newcol' ,\n"
    "  PRIMARY KEY (`idtable1`) );\n";

  all_objects_sql sql;
  sql.table1_sql = create_table1_added_col;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();
  all_objects_mwb_validator validator;
  validator.validate_t1.disable();
  validator.validate(objects);
  ensure("Error adding column", objects.t1->columns()->count() == 5);
  ensure("Column added on wrong pos", objects.t1->columns().get(2)->name() == "newcol");
  ensure("Column added on wrong pos", objects.t1->columns().get(4)->name() == "newcol2");
}

TEST_FUNCTION(36) {
  static const char* create_table1_added_col =
    ""
    "CREATE  TABLE IF NOT EXISTS `mydb`.`table1` (\n"
    "  `idtable1` INT NOT NULL AUTO_INCREMENT ,\n"
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
    "`st` set('on','off') DEFAULT NULL, "
    "  PRIMARY KEY (`idtable1`) );\n";

  all_objects_sql sql;
  sql.table1_sql = create_table1_added_col;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();
  all_objects_mwb_validator validator;
  validator.validate_t1.disable();
  validator.validate(objects);
  ensure("Error adding columns", objects.t1->columns()->count() == 30);
}

// FK
TEST_FUNCTION(40) {
  static const char* create_table1_new_PK =
    ""
    "CREATE  TABLE IF NOT EXISTS `mydb`.`table1` (\n"
    "  `idtable1_renamed` INT NOT NULL AUTO_INCREMENT ,\n"
    "  `name` VARCHAR(45) NULL DEFAULT 'noname' ,\n"
    "  `email` VARCHAR(45) NULL ,\n"
    "  PRIMARY KEY (`idtable1_renamed`) );\n";

  all_objects_sql sql;
  sql.table1_sql = create_table1_new_PK;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  all_objects_mwb initial_objects = get_model_objects();
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();
  all_objects_mwb_validator validator;
  validator.validate_FK.disable();
  validator.validate(objects);
  ensure("FK not removed after referenced col is gone", objects.t2->foreignKeys().count() == 0);
}

// Check that FK is renamed back
TEST_FUNCTION(41) {
  all_objects_sql sql;
  all_objects_mwb_validator validator;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  all_objects_mwb initial_objects = get_model_objects();
  validator.validate(initial_objects);
  initial_objects.t2->foreignKeys().get(0)->name("fk1_newname");
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();
  validator.validate(objects);
}

TEST_FUNCTION(42) {
  all_objects_sql sql;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  all_objects_mwb initial_objects = get_model_objects();
  initial_objects.t1->oldName("newname");
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();
  all_objects_mwb_validator validator;
  validator.validate_FK.disable();
  validator.validate(objects);
  ensure("FK not removed", objects.t2->foreignKeys().count() == 0);
}

/*
As there is no t1 definition in sql a stub will be created when creating t2
FK referring it, and as iSStub cols aren't diffed original t1 will be left
untouched.
TODO check if such behavior is ok
*/
TEST_FUNCTION(43) {
  static const char* no_t1 = "";

  all_objects_sql sql;
  sql.table1_sql = no_t1;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  all_objects_mwb initial_objects = get_model_objects();
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();
  all_objects_mwb_validator validator;
  validator.validate(objects);
}

// Index to a new column
TEST_FUNCTION(45) {
  static const char* create_table1_altered =
    ""
    "CREATE  TABLE IF NOT EXISTS `mydb`.`table1` (\n"
    "  `idtable1` INT NOT NULL AUTO_INCREMENT ,\n"
    "  `name` VARCHAR(45) NULL ,\n"
    "  `email` VARCHAR(45) NULL ,\n"
    "  `newcol` INT ,\n"
    "  KEY newindex (newcol),\n"
    "  PRIMARY KEY (`idtable1`) );\n";

  all_objects_sql sql;
  sql.table1_sql = create_table1_altered;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();

  ensure_equals("New column added", objects.t1->columns().count(), 4U);
  ensure_equals("New column name", *objects.t1->columns()[3]->name(), "newcol");
  ensure_equals("New index added", objects.t1->indices().count(), 2U);
  ensure_equals("New index name", *objects.t1->indices()[0]->name(), "newindex");

  // BUG #14588524 - MYSQL WORKBENCH SEGFAULTS WHEN UPDATING MODEL FROM A DATABASE
  ensure_equals("refcolumn from new index", objects.t1->indices()[0]->columns()[0]->referencedColumn().id(),
                objects.t1->columns()[3]->id());
}

TEST_FUNCTION(46) {
  static const char* create_table2 =
    "CREATE  TABLE IF NOT EXISTS `mydb`.`table2` (\n"
    "  `table1_idtable1` INT NULL ,\n"
    "  INDEX `fktable1_idx` (`table1_idtable1` ASC) ,\n"
    "  CONSTRAINT `fktable1`\n"
    "    FOREIGN KEY (`table1_idtable1` )\n"
    "    REFERENCES `mydb`.`table1` (`idtable1` )\n"
    "    ON DELETE RESTRICT\n"
    "    ON UPDATE RESTRICT);\n";

  all_objects_sql sql;
  sql.table2_sql = create_table2;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  all_objects_mwb initial_objects = get_model_objects();
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();
  all_objects_mwb_validator validator;
  validator.validate_FK.disable();
  validator.validate(objects);

  ensure("FK ON UPDATE wasn't changed", objects.t2->foreignKeys().get(0)->updateRule() == "RESTRICT");
  ensure("FK ON DELETE wasn't changed", objects.t2->foreignKeys().get(0)->deleteRule() == "RESTRICT");
}

// View
TEST_FUNCTION(51) {
  static const char* create_renamed_view =
    "CREATE  OR REPLACE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL SECURITY DEFINER VIEW "
    "`mydb`.`view1_renamed` AS select 1 AS `1`;";

  all_objects_sql sql;
  sql.view_sql = create_renamed_view;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();
  all_objects_mwb_validator validator;
  validator.validate_view.disable();
  ensure("View removed instead of replace", objects.schema->views().count() != 0);
  ensure("View added instead of replace", objects.schema->views().count() == 1);
  ensure("View wasn't replaced", !objects.view.is_valid());
  validator.validate(objects);
}

TEST_FUNCTION(52) {
  static const char* create_altered_view =
    "CREATE  OR REPLACE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL SECURITY DEFINER VIEW `mydb`.`view1` AS "
    "select 1 AS `2`";

  all_objects_sql sql;
  sql.view_sql = create_altered_view;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();
  all_objects_mwb_validator validator;
  validator.validate_view.disable();
  ensure("View removed instead of alter", objects.schema->views().count() != 0);
  ensure("View added instead of alter", objects.schema->views().count() == 1);
  std::string view_sql = objects.schema->views().get(0)->sqlDefinition();
  ensure("View sql doesn't updated", view_sql == create_altered_view);

  validator.validate(objects);
}

TEST_FUNCTION(53) {
  static const char* dont_create = "";

  all_objects_sql sql;
  sql.view_sql = dont_create;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();
  all_objects_mwb_validator validator;
  validator.validate_view.disable();
  ensure("View wasn't removed", objects.schema->views().count() == 0);
  validator.validate(objects);
}

TEST_FUNCTION(54) {
  static const char* create_2views =
    ""
    "CREATE  OR REPLACE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL SECURITY DEFINER VIEW `mydb`.`view1` AS "
    "select 1 AS `1`;\n"
    "USE `mydb`;\nCREATE  OR REPLACE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL SECURITY DEFINER VIEW "
    "`mydb`.`view2` AS select 1 AS `1`;\n";

  all_objects_sql sql;
  sql.view_sql = create_2views;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();
  all_objects_mwb_validator validator;
  ensure("View not added", objects.schema->views().count() == 2);
  validator.validate(objects);
}

// Procedure
TEST_FUNCTION(60) {
  static const char* create_renamed_procedure =
    "CREATE DEFINER=`root`@`localhost` PROCEDURE `routine2`(OUT p INT)\n"
    "BEGIN\n"
    "select 1 into p;\n"
    "END";

  all_objects_sql sql;
  sql.procedure_sql = create_renamed_procedure;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();
  all_objects_mwb_validator validator;
  ensure("Routine removed insead of replace", objects.schema->routines().count() != 0);
  ensure("Routine added insead of replace", objects.schema->routines().count() == 1);
  ensure("Routine wasn't replaced", !objects.routine.is_valid());
  validator.validate_routine.disable();
  validator.validate(objects);
}

TEST_FUNCTION(61) {
  static const char* create_altered_procedure =
    "CREATE DEFINER=`root`@`localhost` PROCEDURE `routine1`(OUT p INT)\n"
    "BEGIN\n"
    "select 2 into p;\n"
    "END";

  all_objects_sql sql;
  sql.procedure_sql = create_altered_procedure;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();
  all_objects_mwb_validator validator;
  ensure("Routine removed insead of alter", objects.schema->routines().count() != 0);
  ensure("Routine added insead of alter", objects.schema->routines().count() == 1);
  std::string proc_sql = objects.schema->routines().get(0)->sqlDefinition();
  ensure("Routine sql doesn't updated", proc_sql == create_altered_procedure);

  validator.validate_routine.disable();
  validator.validate(objects);
}

// Trigger
TEST_FUNCTION(70) {
  static const char* create_table1_trigger_renamed =
    "CREATE\n"
    "DEFINER=`root`@`localhost`\n"
    "TRIGGER `mydb`.`tr1_renamed`\n"
    "BEFORE INSERT ON `mydb`.`table1`\n"
    "FOR EACH ROW\n"
    "set new.idtable1 = 1";

  all_objects_sql sql;
  sql.trigger_sql = create_table1_trigger_renamed;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();
  ensure("Trigger removed insead of replace", objects.t1->triggers().count() != 0);
  ensure("Trigger added insead of replace", objects.t1->triggers().count() == 1);
  ensure("Trigger wasn't replaced", !objects.trigger.is_valid());
  all_objects_mwb_validator validator;
  validator.validate_trigger.disable();
  validator.validate(objects);
}

TEST_FUNCTION(71) {
  static const char* create_table1_trigger_altered =
    "CREATE\n"
    "DEFINER=`root`@`localhost`\n"
    "TRIGGER `mydb`.`tr1`\n"
    "BEFORE INSERT ON `mydb`.`table1`\n"
    "FOR EACH ROW\n"
    "set new.idtable1 = 2";

  all_objects_sql sql;
  sql.trigger_sql = create_table1_trigger_altered;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();

  ensure("Trigger removed insead of alter", objects.t1->triggers().count() != 0);
  ensure("Trigger added insead of alter", objects.t1->triggers().count() == 1);
  std::string trigger_sql = objects.trigger->sqlDefinition();
  ensure("Trigger sql doesn't updated", trigger_sql == create_table1_trigger_altered);

  all_objects_mwb_validator validator;
  validator.validate_trigger.disable();
  validator.validate(objects);
}

TEST_FUNCTION(72) {
  all_objects_sql sql;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  all_objects_mwb initial_objects = get_model_objects();
  // rename t1
  initial_objects.trigger->name("tr1_renamed");
  // now t1 should get it's initial name back
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();
  all_objects_mwb_validator validator;
  validator.validate(objects);
}

TEST_FUNCTION(73) {
  all_objects_sql sql;
  tester->wb->open_document("data/workbench/all_objects.mwb");
  all_objects_mwb initial_objects = get_model_objects();
  // rename t1
  initial_objects.trigger->oldName("tr1_renamed");
  // now t1 should get it's initial name back
  apply_sql_to_model(sql.get_sql());

  all_objects_mwb objects = get_model_objects();
  all_objects_mwb_validator validator;
  validator.validate(objects);
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  delete tester;
}

END_TESTS
