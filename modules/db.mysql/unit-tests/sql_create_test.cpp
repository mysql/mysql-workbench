/*
 * Copyright (c) 2011, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "testgrt.h"
#include "grt_test_utility.h"
#include "grt/grt_manager.h"
#include "grt.h"
#include "synthetic_mysql_model.h"
#include "grtdb/diff_dbobjectmatch.h"
#include "interfaces/sqlgenerator.h"
#include "backend/db_mysql_sql_export.h"

#include "db_mysql_diffsqlgen.h"

#include "grtsqlparser/mysql_parser_services.h"

BEGIN_TEST_DATA_CLASS(sql_create_test)
protected:
WBTester *tester;
SQLGeneratorInterfaceImpl *diffsql_module;
sql::ConnectionWrapper connection;

TEST_DATA_CONSTRUCTOR(sql_create_test) {
  tester = new WBTester;
  diffsql_module = NULL;

  // load modules
  diffsql_module = dynamic_cast<SQLGeneratorInterfaceImpl *>(grt::GRT::get()->get_module("DbMySQL"));
  ensure("DiffSQLGen module initialization", NULL != diffsql_module);

  // init datatypes
  populate_grt(*tester);

  // init database connection
  connection = tester->create_connection_for_import();
}

TEST_DATA_DESTRUCTOR(sql_create_test) {
  std::auto_ptr<sql::Statement> stmt(connection->createStatement());
  stmt->execute("DROP SCHEMA IF EXISTS `A`;");
  stmt->execute("DROP SCHEMA IF EXISTS `B`;");
}

END_TEST_DATA_CLASS

TEST_MODULE(sql_create_test, "sql create");

// Test if sql generated for synthetic model is valid.
TEST_FUNCTION(10) {
  grt::ValueRef e;
  std::auto_ptr<sql::Statement> stmt(connection->createStatement());
  NormalizedComparer cmp;
  grt::DbObjectMatchAlterOmf omf;

  SynteticMySQLModel model;
  db_mysql_ViewRef view(grt::Initialized);
  model.schema->views().insert(view);
  view->owner(model.schema);
  view->name("v2");
  view->sqlDefinition(
    "create view v2 as SELECT "
    "if(t1.id > 2, 'active, very active', 'inactive, very very very inactive'), "
    "if(t1.id > 4, 'active, very active', 'inactive, very very very inactive') "
    "FROM t1");

  db_mysql_CatalogRef catalog = model.catalog;

  cmp.init_omf(&omf);

  std::shared_ptr<DiffChange> create_change = diff_make(e, catalog, &omf);
  std::shared_ptr<DiffChange> drop_change = diff_make(catalog, e, &omf);

  DictRef create_map(true);
  DictRef drop_map(true);
  grt::DictRef options(true);
  options.set("UseFilteredLists", grt::IntegerRef(0));
  options.set("OutputContainer", create_map);
  options.set("CaseSensitive", grt::IntegerRef(omf.case_sensitive));
  options.set("GenerateSchemaDrops", grt::IntegerRef(1));
  options.set("GenerateDrops", grt::IntegerRef(1));
  diffsql_module->generateSQL(catalog, options, create_change);

  options.set("OutputContainer", drop_map);
  diffsql_module->generateSQL(catalog, options, drop_change);

  diffsql_module->makeSQLExportScript(catalog, options, create_map, drop_map);
  std::string export_sql_script = options.get_string("OutputScript");
  ensure("DROP TABLE missing in generated sql",
         export_sql_script.find("DROP TABLE IF EXISTS `test_schema`.`t1`") != std::string::npos);

  execute_script(stmt.get(), export_sql_script);
}

// Forward engineer synthetic model without qualifying schema, but inserting USE statements instead.
TEST_FUNCTION(20) {
  grt::ValueRef e;
  std::auto_ptr<sql::Statement> stmt(connection->createStatement());
  NormalizedComparer cmp;
  grt::DbObjectMatchAlterOmf omf;

  SynteticMySQLModel model;

  db_mysql_CatalogRef catalog = model.catalog;

  cmp.init_omf(&omf);

  std::shared_ptr<DiffChange> create_change = diff_make(e, catalog, &omf);
  std::shared_ptr<DiffChange> drop_change = diff_make(catalog, e, &omf);

  DictRef create_map(true);
  DictRef drop_map(true);
  grt::DictRef options(true);
  options.set("UseFilteredLists", grt::IntegerRef(0));
  options.set("OutputContainer", create_map);
  options.set("CaseSensitive", grt::IntegerRef(omf.case_sensitive));
  options.set("GenerateSchemaDrops", grt::IntegerRef(1));
  options.set("OmitSchemas", grt::IntegerRef(1));
  options.set("GenerateUse", grt::IntegerRef(1));
  options.set("GenerateDrops", grt::IntegerRef(1));
  diffsql_module->generateSQL(catalog, options, create_change);

  options.set("OutputContainer", drop_map);
  diffsql_module->generateSQL(catalog, options, drop_change);

  diffsql_module->makeSQLExportScript(catalog, options, create_map, drop_map);
  std::string export_sql_script = options.get_string("OutputScript");
  execute_script(stmt.get(), export_sql_script);
}

// Test case for Bug #11926862 NO WAY TO SORT SCHEMAS ON EXPORT
TEST_FUNCTION(30) {
  grt::ValueRef e;
  std::auto_ptr<sql::Statement> stmt(connection->createStatement());
  NormalizedComparer cmp;
  grt::DbObjectMatchAlterOmf omf;

  tester->wb->open_document("data/workbench/11926862.mwb");
  db_mysql_CatalogRef catalog =
    db_mysql_CatalogRef::cast_from(tester->wb->get_document()->physicalModels().get(0)->catalog());

  cmp.init_omf(&omf);

  std::shared_ptr<DiffChange> create_change = diff_make(e, catalog, &omf);
  std::shared_ptr<DiffChange> drop_change = diff_make(catalog, e, &omf);

  DictRef create_map(true);
  DictRef drop_map(true);
  grt::DictRef options(true);
  options.set("UseFilteredLists", grt::IntegerRef(0));
  options.set("OutputContainer", create_map);
  options.set("CaseSensitive", grt::IntegerRef(omf.case_sensitive));
  options.set("GenerateSchemaDrops", grt::IntegerRef(1));
  options.set("OmitSchemas", grt::IntegerRef(1));
  options.set("GenerateUse", grt::IntegerRef(1));
  options.set("GenerateDrops", grt::IntegerRef(1));
  diffsql_module->generateSQL(catalog, options, create_change);

  options.set("OutputContainer", drop_map);
  diffsql_module->generateSQL(catalog, options, drop_change);

  diffsql_module->makeSQLExportScript(catalog, options, create_map, drop_map);
  std::string export_sql_script = options.get_string("OutputScript");
  execute_script(stmt.get(), export_sql_script);
}

// Test case for Bug #14278043 DB SYNCRONIZE MODEL GENERATES INCORRECT COLLATION
// If somehow the collation doesn't correspond to the charset it should be skipped during
// sql generation to avoid creating invalid DDL.
TEST_FUNCTION(40) {
  grt::ValueRef e;
  std::auto_ptr<sql::Statement> stmt(connection->createStatement());
  NormalizedComparer cmp;
  grt::DbObjectMatchAlterOmf omf;

  SynteticMySQLModel model;

  model.schema->defaultCharacterSetName("utf8");
  model.schema->defaultCollationName("latin_1_swedish_ci");
  model.table->defaultCharacterSetName("utf8");
  model.table->defaultCollationName("latin_1_swedish_ci");
  model.columnText->characterSetName("utf8");
  model.columnText->collationName("latin_1_swedish_ci");

  db_mysql_CatalogRef catalog = model.catalog;

  cmp.init_omf(&omf);

  std::shared_ptr<DiffChange> create_change = diff_make(e, catalog, &omf);
  std::shared_ptr<DiffChange> drop_change = diff_make(catalog, e, &omf);

  DictRef create_map(true);
  DictRef drop_map(true);
  grt::DictRef options(true);
  options.set("UseFilteredLists", grt::IntegerRef(0));
  options.set("OutputContainer", create_map);
  options.set("CaseSensitive", grt::IntegerRef(omf.case_sensitive));
  options.set("GenerateSchemaDrops", grt::IntegerRef(1));
  options.set("GenerateDrops", grt::IntegerRef(1));
  diffsql_module->generateSQL(catalog, options, create_change);

  options.set("OutputContainer", drop_map);
  diffsql_module->generateSQL(catalog, options, drop_change);

  diffsql_module->makeSQLExportScript(catalog, options, create_map, drop_map);
  std::string export_sql_script = options.get_string("OutputScript");
  execute_script(stmt.get(), export_sql_script);
}

static std::string strrange(const std::string &s, const std::string &start, const std::string &end) {
  try {
    std::string res = s.substr(s.find(start));
    if (end.empty())
      return res;
    return res.substr(0, res.find(end));
  } catch (std::exception &e) {
    throw std::runtime_error(s + " does not contain " + start + " or " + end + ":" + e.what());
  }
}

// Test generation of comments (with support for truncation)
// Bug #17455899
TEST_FUNCTION(50) {
  std::string comment_60 = "012345678901234567890123456789012345678901234567890123456789";
  std::string comment_255 = comment_60 + comment_60 + comment_60 + comment_60 + "012345678912345";

  grt::ValueRef e;
  NormalizedComparer cmp;
  grt::DbObjectMatchAlterOmf omf;

  SynteticMySQLModel model;

  db_mysql_CatalogRef catalog = model.catalog;

  db_mysql_TableRef table = catalog->schemata()[0]->tables()[0];

  cmp.init_omf(&omf);

  // before 5.5.3, column comments <= 255, table <= 60
  {
    grt::DictRef dbsett(diffsql_module->getTraitsForServerVersion(5, 5, 2));
    dbsett.set("CaseSensitive", grt::IntegerRef(omf.case_sensitive));
    table->comment(comment_60);
    table->columns()[1]->comment(comment_255);
    std::shared_ptr<DiffChange> create_change = diff_make(e, catalog, &omf);

    DictRef create_map(true);
    DictRef drop_map(true);
    grt::DictRef options(true);
    options.set("UseFilteredLists", grt::IntegerRef(0));
    options.set("OutputContainer", create_map);
    options.set("CaseSensitive", grt::IntegerRef(omf.case_sensitive));
    options.set("GenerateSchemaDrops", grt::IntegerRef(1));
    options.set("GenerateDrops", grt::IntegerRef(1));
    options.set("DBSettings", dbsett);
    diffsql_module->generateSQL(catalog, options, create_change);

    std::string s = create_map.get_string("db.mysql.Table::`test_schema`.`t1`::t1");

    ensure_equals("column comment", strrange(s, "`parent_id`", ","),
                  "`parent_id` TINYINT NULL COMMENT '" + comment_255 + "'");
    ensure_equals("table comment", strrange(s, "COMMENT =", ""), "COMMENT = '" + comment_60 + "'");
  }

  {
    grt::DictRef dbsett(diffsql_module->getTraitsForServerVersion(5, 5, 2));
    dbsett.set("CaseSensitive", grt::IntegerRef(omf.case_sensitive));
    table->comment(comment_60 + "###");
    table->columns()[1]->comment(comment_255 + "###");
    std::shared_ptr<DiffChange> create_change = diff_make(e, catalog, &omf);

    DictRef create_map(true);
    DictRef drop_map(true);
    grt::DictRef options(true);
    options.set("UseFilteredLists", grt::IntegerRef(0));
    options.set("OutputContainer", create_map);
    options.set("CaseSensitive", grt::IntegerRef(omf.case_sensitive));
    options.set("GenerateSchemaDrops", grt::IntegerRef(1));
    options.set("GenerateDrops", grt::IntegerRef(1));
    options.set("DBSettings", dbsett);
    diffsql_module->generateSQL(catalog, options, create_change);

    std::string s = create_map.get_string("db.mysql.Table::`test_schema`.`t1`::t1");

    ensure_equals("column comment", strrange(s, "`parent_id`", ","),
                  "`parent_id` TINYINT NULL COMMENT '" + comment_255 + "' /* comment truncated */ /*###*/");
    ensure_equals("table comment", strrange(s, "COMMENT =", ""),
                  "COMMENT = '" + comment_60 + "' /* comment truncated */ /*###*/");
  }

  // after 5.5.3
  std::string comment_2048(std::string(2048, 'X'));
  std::string comment_1024(std::string(1024, 'X'));

  grt::DictRef dbsett(diffsql_module->getTraitsForServerVersion(5, 5, 4));
  dbsett.set("CaseSensitive", grt::IntegerRef(omf.case_sensitive));

  {
    table->comment(comment_2048);
    table->columns()[1]->comment(comment_1024);
    std::shared_ptr<DiffChange> create_change = diff_make(e, catalog, &omf);

    DictRef create_map(true);
    DictRef drop_map(true);
    grt::DictRef options(true);
    options.set("UseFilteredLists", grt::IntegerRef(0));
    options.set("OutputContainer", create_map);
    options.set("CaseSensitive", grt::IntegerRef(omf.case_sensitive));
    options.set("GenerateSchemaDrops", grt::IntegerRef(1));
    options.set("GenerateDrops", grt::IntegerRef(1));
    options.set("DBSettings", dbsett);
    diffsql_module->generateSQL(catalog, options, create_change);

    std::string s = create_map.get_string("db.mysql.Table::`test_schema`.`t1`::t1", "ERROR");
    ensure_equals("column comment", strrange(s, "`parent_id`", ","),
                  "`parent_id` TINYINT NULL COMMENT '" + comment_1024 + "'");
    ensure_equals("table comment", strrange(s, "COMMENT =", ""), "COMMENT = '" + comment_2048 + "'");
  }

  {
    table->comment(comment_2048 + "###");
    table->columns()[1]->comment(comment_1024 + "###");
    std::shared_ptr<DiffChange> create_change = diff_make(e, catalog, &omf);

    DictRef create_map(true);
    DictRef drop_map(true);
    grt::DictRef options(true);
    options.set("UseFilteredLists", grt::IntegerRef(0));
    options.set("OutputContainer", create_map);
    options.set("CaseSensitive", grt::IntegerRef(omf.case_sensitive));
    options.set("GenerateSchemaDrops", grt::IntegerRef(1));
    options.set("GenerateDrops", grt::IntegerRef(1));
    options.set("DBSettings", dbsett);
    diffsql_module->generateSQL(catalog, options, create_change);

    std::string s = create_map.get_string("db.mysql.Table::`test_schema`.`t1`::t1");
    ensure_equals("column comment", strrange(s, "`parent_id`", ","),
                  "`parent_id` TINYINT NULL COMMENT '" + comment_1024 + "' /* comment truncated */ /*###*/");
    ensure_equals("table comment", strrange(s, "COMMENT =", ""),
                  "COMMENT = '" + comment_2048 + "' /* comment truncated */ /*###*/");
  }
}

////Test for bug: 11765994, fwd view can be sometimes problematic with case sensitivity
TEST_FUNCTION(60) {
  {
    grt::ValueRef e;
    std::auto_ptr<sql::Statement> stmt(connection->createStatement());
    NormalizedComparer cmp;
    grt::DbObjectMatchAlterOmf omf;

    std::string modelfile = "data/forward_engineer/view_mixed_case.mwb";
    tester->wb->open_document(modelfile);
    db_mysql_CatalogRef catalog =
      db_mysql_CatalogRef::cast_from(tester->wb->get_document()->physicalModels().get(0)->catalog());

    cmp.init_omf(&omf);

    std::shared_ptr<DiffChange> create_change = diff_make(e, catalog, &omf);
    std::shared_ptr<DiffChange> drop_change = diff_make(catalog, e, &omf);

    DictRef create_map(true);
    DictRef drop_map(true);
    DictRef db_opts(true);
    grt::DictRef options(true);
    options.set("UseFilteredLists", grt::IntegerRef(0));
    options.set("OutputContainer", create_map);
    options.set("CaseSensitive", grt::IntegerRef(1));
    options.set("GenerateSchemaDrops", grt::IntegerRef(1));
    options.set("OmitSchemas", grt::IntegerRef(1));
    options.set("GenerateUse", grt::IntegerRef(1));
    options.set("GenerateDrops", grt::IntegerRef(1));
    options.set("GenerateDocumentProperties", grt::IntegerRef(0));
    diffsql_module->generateSQL(catalog, options, create_change);

    // Case sensitive in db set to true
    db_opts.set("CaseSensitive", grt::IntegerRef(1));
    options.set("DBSettings", db_opts);

    options.set("OutputContainer", drop_map);
    diffsql_module->generateSQL(catalog, options, drop_change);

    diffsql_module->makeSQLExportScript(catalog, options, create_map, drop_map);
    std::string export_sql_script = options.get_string("OutputScript");

    std::string expected_sql = "data/forward_engineer/view_mixed_case.expected.broken.sql";

    std::ifstream ref(expected_sql.c_str());
    std::stringstream ss(export_sql_script);

    std::string line, refline;

    tut::ensure(expected_sql, ref.is_open());

    while (ref.good() && ss.good()) {
      getline(ref, refline);
      getline(ss, line);
      tut::ensure_equals("Different lines", line, refline);
    }

    if (ref.good() || ss.good())
      fail("Generated and reference line count differ.");

    tester->wb->close_document();
    tester->wb->close_document_finish();
  }

  {
    grt::ValueRef e;
    std::auto_ptr<sql::Statement> stmt(connection->createStatement());
    NormalizedComparer cmp;
    grt::DbObjectMatchAlterOmf omf;

    std::string modelfile = "data/forward_engineer/view_mixed_case.mwb";
    tester->wb->open_document(modelfile);
    db_mysql_CatalogRef catalog =
      db_mysql_CatalogRef::cast_from(tester->wb->get_document()->physicalModels().get(0)->catalog());

    cmp.init_omf(&omf);

    std::shared_ptr<DiffChange> create_change = diff_make(e, catalog, &omf);
    std::shared_ptr<DiffChange> drop_change = diff_make(catalog, e, &omf);

    DictRef create_map(true);
    DictRef drop_map(true);
    DictRef db_opts(true);
    grt::DictRef options(true);
    options.set("UseFilteredLists", grt::IntegerRef(0));
    options.set("OutputContainer", create_map);
    options.set("CaseSensitive", grt::IntegerRef(1));
    options.set("GenerateSchemaDrops", grt::IntegerRef(1));
    options.set("OmitSchemas", grt::IntegerRef(1));
    options.set("GenerateUse", grt::IntegerRef(1));
    options.set("GenerateDrops", grt::IntegerRef(1));
    options.set("GenerateDocumentProperties", grt::IntegerRef(0));
    diffsql_module->generateSQL(catalog, options, create_change);

    // Case sensitive in db set to false
    db_opts.set("CaseSensitive", grt::IntegerRef(0));
    options.set("DBSettings", db_opts);

    options.set("OutputContainer", drop_map);
    diffsql_module->generateSQL(catalog, options, drop_change);

    diffsql_module->makeSQLExportScript(catalog, options, create_map, drop_map);
    std::string export_sql_script = options.get_string("OutputScript");

    std::string expected_sql = "data/forward_engineer/view_mixed_case.expected.good.sql";

    std::ifstream ref(expected_sql.c_str());
    std::stringstream ss(export_sql_script);

    std::string line, refline;

    tut::ensure(expected_sql, ref.is_open());

    std::string error_msg("Forward engineer of:");
    error_msg += modelfile;
    error_msg += " and ";
    error_msg += expected_sql;
    error_msg += " failed";

    while (ref.good() && ss.good()) {
      getline(ref, refline);
      getline(ss, line);
      tut::ensure_equals(error_msg, line, refline);
    }
  }
}

// Test forward engineering after renaming a schema, if it generates proper sql.
TEST_FUNCTION(70) {
  grt::ValueRef e;
  std::auto_ptr<sql::Statement> stmt(connection->createStatement());
  NormalizedComparer cmp;
  grt::DbObjectMatchAlterOmf omf;

  std::string modelfile = "data/forward_engineer/sakila_full.mwb";
  tester->wb->open_document(modelfile);
  db_mysql_CatalogRef catalog = db_mysql_CatalogRef::cast_from(tester->get_catalog());

  catalog->schemata()[0]->name("sakila_test");

  parsers::MySQLParserServices::Ref services = parsers::MySQLParserServices::get();
  GrtVersionRef version = bec::parse_version("5.6.0");
  parsers::MySQLParserContext::Ref context =
    services->createParserContext(tester->get_rdbms()->characterSets(), version, "", false);
  services->renameSchemaReferences(context, catalog, "sakila", "sakila_test");

  cmp.init_omf(&omf);

  std::shared_ptr<DiffChange> create_change = diff_make(e, catalog, &omf);
  std::shared_ptr<DiffChange> drop_change = diff_make(catalog, e, &omf);

  DictRef create_map(true);
  DictRef drop_map(true);

  grt::DictRef options = DictRef::cast_from(grt::GRT::get()->unserialize("data/forward_engineer/rename_opts.dict"));
  options.set("GenerateDocumentProperties", grt::IntegerRef(0));

  create_map = diffsql_module->generateSQLForDifferences(GrtNamedObjectRef(), catalog, options);

  grt::DictRef dbsett(diffsql_module->getTraitsForServerVersion(5, 6, 0));
  dbsett.set("CaseSensitive", grt::IntegerRef(omf.case_sensitive));
  options.set("DBSettings", dbsett);
  diffsql_module->makeSQLExportScript(catalog, options, create_map, drop_map);

  std::string export_sql_script = options.get_string("OutputScriptHeader") + options.get_string("OutputScript");

  std::string expected_sql = "data/forward_engineer/sakila_name_changed.expected.sql";

  std::ifstream ref(expected_sql.c_str());
  std::stringstream ss(export_sql_script);

  std::string line, refline;

  tut::ensure(expected_sql, ref.is_open());

  std::string error_msg("Forward engineer of: ");
  error_msg += modelfile;
  error_msg += " and ";
  error_msg += expected_sql;
  error_msg += " failed in line ";

  size_t i = 0;
  while (ref.good() && ss.good()) {
    getline(ref, refline);
    getline(ss, line);
    tut::ensure_equals(error_msg + std::to_string(i++), line, refline);
  }

  tester->wb->close_document();
  tester->wb->close_document_finish();
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  delete tester;
}

END_TESTS 
