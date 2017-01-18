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
#include "synthetic_mysql_model.h"
#include "grtdb/diff_dbobjectmatch.h"
#include "grtdb/sync_profile.h"
#include "db_mysql_public_interface.h"

#include "db_mysql_diffsqlgen.h"
#include "module_db_mysql.h"
#include "diff/changeobjects.h"
#include "diff/changelistobjects.h"

BEGIN_TEST_DATA_CLASS(sync_profile_test)
protected:
WBTester* tester;
SqlFacade::Ref sql_parser;
DbMySQLImpl* diffsql_module;
grt::DbObjectMatchAlterOmf omf;
sql::ConnectionWrapper connection;

TEST_DATA_CONSTRUCTOR(sync_profile_test) {
  tester = new WBTester;
  omf.dontdiff_mask = 3;
  diffsql_module = grt::GRT::get()->get_native_module<DbMySQLImpl>();
  ensure("DiffSQLGen module initialization", NULL != diffsql_module);

  // init datatypes
  populate_grt(*tester);

  // init database connection
  connection = tester->create_connection_for_import();

  sql_parser = SqlFacade::instance_for_rdbms_name("Mysql");
  ensure("failed to get sqlparser module", (NULL != sql_parser));
}

END_TEST_DATA_CLASS

TEST_MODULE(sync_profile_test, "Syncronize profiles tests");

// Test if sql generated for syntetic model is valid
TEST_FUNCTION(1) {
  grt::ValueRef e;
  NormalizedComparer cmp;

  tester->wb->new_document();
  SynteticMySQLModel model1(tester);
  grt::StringRef tablename = model1.table->name();
  // unaltered model to test diffs

  const SynteticMySQLModel model2(tester);

  // Save unaltered names
  db_mgmt_SyncProfileRef initial_old_names = create_sync_profile(model1.model, "test_profile", "");
  update_sync_profile_from_schema(initial_old_names, model1.catalog->schemata()[0], false);
  ensure("Not Valid Initial Old Names", initial_old_names->lastKnownDBNames().count() > 0);

  // Rename table
  model1.table->name("new_name");

  // save updated names
  db_mgmt_SyncProfileRef updated_old_names = create_sync_profile(model1.model, "test_profile", "");
  update_sync_profile_from_schema(updated_old_names, model1.catalog->schemata()[0], false);
  ensure("Not Valid Updated Old Names", updated_old_names->lastKnownDBNames().count() > 0);

  // Check that table rename is seen by diff module
  std::shared_ptr<DiffChange> diff = diff_make(model1.catalog, model2.catalog, &omf);
  ensure("Diff module broken", diff.get() != NULL);

  model1.table->name(tablename);
  // now model1.table will have it's initiall name
  diff = diff_make(model1.catalog, model2.catalog, &omf);
  ensure("Rename Failure", diff.get() == NULL);

  // the only difference is oldName which should lead to drop/create of table
  update_schema_from_sync_profile(model1.catalog->schemata()[0], updated_old_names);

  diff = diff_make(model1.catalog, model2.catalog, &omf);
  ensure("OldName only Diff", diff.get() != NULL);
  tester->wb->close_document();
  tester->wb->close_document_finish();
}

TEST_FUNCTION(2) {
  grt::ValueRef e;
  std::auto_ptr<sql::Statement> stmt(connection->createStatement());
  NormalizedComparer cmp;
  // Kind of hack, atm we doesn't propertly cut server representation of procedures and vews, so just skip it
  cmp.add_comparison_rule("sqlDefinition", std::bind([]() { return true; }));

  tester->wb->new_document();

  SynteticMySQLModel model(tester);
  model.trigger->modelOnly(1);
  model.view->modelOnly(1);

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
  diffsql_module->generateSQL(catalog, options, create_change);

  options.set("OutputContainer", drop_map);
  diffsql_module->generateSQL(catalog, options, drop_change);

  diffsql_module->makeSQLExportScript(catalog, options, create_map, drop_map);
  std::string export_sql_script = options.get_string("OutputScript");
  execute_script(stmt.get(), export_sql_script);

  std::list<std::string> schemata;
  schemata.push_back(model.schema->name());
  grt::GRT::get()->get_undo_manager()->disable();
  db_mysql_CatalogRef cat1 = tester->db_rev_eng_schema(schemata);
  if ((cat1->schemata().get(0).is_valid()) && (cat1->schemata().get(0)->name() == "mydb"))
    cat1->schemata().remove(0);

  db_mysql_CatalogRef cat2 = grt::copy_object(cat1);

  // Diff identical catalogs, no chages expected
  std::shared_ptr<DiffChange> diff = diff_make(cat1, cat2, &omf);
  ensure("Diffs in copyies of same catalog", diff == NULL);

  // This doesn't make sense anymore, we don't support renaming schemas, schemas with different
  // names will be compared as being the same
  // Rename schema to new_name, without new_name existing on server
  // old schema should be dropped, new one created instead
  // cat1->schemata().get(0)->name("new_name");
  // diff = diff_make(cat2, cat1, &omf);
  /*
  {
      // 1. generate alter
   grt::StringListRef alter_map(grt::Initialized);
   grt::ListRef<GrtNamedObject> alter_object_list;
   grt::DictRef options(true);
   options.set("UseFilteredLists", grt::IntegerRef(0));
   options.set("OutputContainer", alter_map);
   options.set("OutputObjectContainer", alter_object_list);
   options.set("CaseSensitive", grt::IntegerRef(omf.case_sensitive));

   diffsql_module->generateSQL(cat2, options, diff);
   diffsql_module->makeSQLSyncScript(options, alter_map, alter_object_list);
   std::string export_sql_script= options.get_string("OutputScript");
   std::string drop_old_schema("DROP SCHEMA IF EXISTS `");
   drop_old_schema.append(cat1->schemata().get(0)->name()).append("` ;\n");
   execute_script(stmt.get(), drop_old_schema, tester->wb->get_grt_manager());
   execute_script(stmt.get(), export_sql_script,tester->wb->get_grt_manager());
  }

  schemata.clear();
  schemata.push_back(cat1->schemata().get(0)->name());
  schemata.push_back(cat2->schemata().get(0)->name());
  db_mysql_CatalogRef testcat = tester->db_rev_eng_schema(schemata);
  if((testcat->schemata().get(0).is_valid()) && (testcat->schemata().get(0)->name() == "mydb"))
     testcat->schemata().remove(0);
  ensure("Initial stchema wasn't dropped on rename", testcat->schemata().count() == 1);
  ensure("Renamed schema not found", testcat->schemata().get(0)->name() == cat1->schemata().get(0)->name());

  //Now sync renamed schema with 'new_name' schema already on server
  //This should lead to drop of old schema and altering new one
  //Recreate initial schema
  execute_script(stmt.get(), export_sql_script,tester->wb->get_grt_manager());
  std::cout<<export_sql_script<<"\n\n\n";
  //rev eng both schemas
  db_mysql_CatalogRef cat1_and_cat2 = tester->db_rev_eng_schema(schemata);
  //rename a table to check that alter did worked
  cat1->schemata().get(0)->signal_refreshDisplay()->disconnect_all_slots();
  cat1->schemata().get(0)->tables().get(0)->name("new_table_name");
  diff = diff_make(cat1_and_cat2, cat1, &omf);
  {
      // 1. generate alter
      grt::StringListRef alter_map(grt::Initialized);
      grt::ListRef<GrtNamedObject> alter_object_list;
      grt::DictRef options(true);
      options.set("UseFilteredLists", grt::IntegerRef(0));
      options.set("OutputContainer", alter_map);
      options.set("OutputObjectContainer", alter_object_list);
      options.set("CaseSensitive", grt::IntegerRef(omf.case_sensitive));

      diffsql_module->generateSQL(cat1_and_cat2, options, diff);
      diffsql_module->makeSQLSyncScript(options, alter_map, alter_object_list);
      std::string export_sql_script= options.get_string("OutputScript");
      std::cout << export_sql_script.c_str() << "\r\n";
      execute_script(stmt.get(), export_sql_script,tester->wb->get_grt_manager());
  }

  db_mysql_CatalogRef altered_cat2 = tester->db_rev_eng_schema(schemata);

  if((altered_cat2->schemata().get(0).is_valid()) && (altered_cat2->schemata().get(0)->name() == "mydb"))
     altered_cat2->schemata().remove(0);
  ensure("Initial stchema wasn't dropped on rename", altered_cat2->schemata().count() == 1);
  ensure("Renamed schema not found", altered_cat2->schemata().get(0)->name() == cat1->schemata().get(0)->name());
  ensure("Alter schema failed", altered_cat2->schemata().get(0)->tables().count() == 1);
  ensure("Alter schema failed", altered_cat2->schemata().get(0)->tables().get(0)->name() ==
  cat1->schemata().get(0)->tables().get(0)->name());

  tester->wb->close_document();
  tester->wb->close_document_finish();
  */
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  delete tester;
}

END_TESTS
