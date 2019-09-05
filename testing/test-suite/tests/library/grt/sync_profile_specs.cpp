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

#include "grt/grt_manager.h"
#include "grt.h"
#include "grtdb/diff_dbobjectmatch.h"
#include "grtdb/sync_profile.h"
#include "db_mysql_public_interface.h"

#include "db_mysql_diffsqlgen.h"
#include "module_db_mysql.h"
#include "diff/changeobjects.h"
#include "diff/changelistobjects.h"

#include "model_mockup.h"
#include "wb_connection_helpers.h"

#include "casmine.h"

using namespace grt;
using namespace casmine;

namespace {

$ModuleEnvironment() {};

$TestData {
  std::unique_ptr<WorkbenchTester> tester;
  SqlFacade::Ref sqlParser;
  DbMySQLImpl* diffsqlModule;
  DbObjectMatchAlterOmf omf;
  sql::ConnectionWrapper connection;
};

$describe("Synchronize profiles") {

  $beforeAll([this]() {
    data->tester.reset(new WorkbenchTester());
    data->tester->initializeRuntime();

    data->omf.dontdiff_mask = 3;
    data->diffsqlModule = GRT::get()->get_native_module<DbMySQLImpl>();
    $expect(data->diffsqlModule).Not.toBeNull("DiffSQLGen module initialization");

    data->connection = createConnectionForImport();

    data->sqlParser = SqlFacade::instance_for_rdbms_name("Mysql");
    $expect(data->sqlParser).Not.toBeNull("failed to get sqlparser module");
  });

  $beforeEach([this]() {
    data->tester->wb->new_document();
  });

  $afterEach([this]() {
    data->tester->wb->close_document();
    data->tester->wb->close_document_finish();
  });

  $it("Validation of mockup data", [this]() {
    ValueRef e;
    NormalizedComparer cmp;

    SyntheticMySQLModel model1(data->tester.get());
    StringRef tablename = model1.table->name();
    // unaltered model to test diffs

    const SyntheticMySQLModel model2(data->tester.get());

    // Save unaltered names
    db_mgmt_SyncProfileRef initial_old_names = create_sync_profile(model1.model, "test_profile", "");
    update_sync_profile_from_schema(initial_old_names, model1.catalog->schemata()[0], false);
    $expect(initial_old_names->lastKnownDBNames().count()).toBeGreaterThan(0U, "Invalid initial old names count");

    // Rename table
    model1.table->name("new_name");

    // save updated names
    db_mgmt_SyncProfileRef updated_old_names = create_sync_profile(model1.model, "test_profile", "");
    update_sync_profile_from_schema(updated_old_names, model1.catalog->schemata()[0], false);
    $expect(updated_old_names->lastKnownDBNames().count()).toBeGreaterThan(0U, "Invalid updated old names count");

    // Check that table rename is seen by diff module
    std::shared_ptr<DiffChange> diff = diff_make(model1.catalog, model2.catalog, &data->omf);
    $expect(diff).toBeValid();

    model1.table->name(tablename);
    // Now model1.table will have its initial name.
    diff = diff_make(model1.catalog, model2.catalog, &data->omf);
    $expect(diff).Not.toBeValid();

    // the only difference is oldName which should lead to drop/create of table
    update_schema_from_sync_profile(model1.catalog->schemata()[0], updated_old_names);

    diff = diff_make(model1.catalog, model2.catalog, &data->omf);
    $expect(diff).toBeValid();
  });

  $it("Create + reverse engineer a mockup model", [this]() {
    ValueRef e;
    std::unique_ptr<sql::Statement> stmt(data->connection->createStatement());
    NormalizedComparer cmp;

    // Kind of a hack. At the moment we don't properly handle server representations of procedures and views.
    cmp.add_comparison_rule("sqlDefinition", std::bind([]() { return true; }));

    SyntheticMySQLModel model(data->tester.get());
    model.trigger->modelOnly(1);
    model.view->modelOnly(1);

    db_mysql_CatalogRef catalog = model.catalog;

    cmp.init_omf(&data->omf);

    std::shared_ptr<DiffChange> create_change = diff_make(e, catalog, &data->omf);
    std::shared_ptr<DiffChange> drop_change = diff_make(catalog, e, &data->omf);

    DictRef create_map(true);
    DictRef drop_map(true);
    DictRef options(true);
    options.set("UseFilteredLists", IntegerRef(0));
    options.set("OutputContainer", create_map);
    options.set("CaseSensitive", IntegerRef(data->omf.case_sensitive));
    options.set("GenerateSchemaDrops", IntegerRef(1));
    data->diffsqlModule->generateSQL(catalog, options, create_change);

    options.set("OutputContainer", drop_map);
    data->diffsqlModule->generateSQL(catalog, options, drop_change);

    data->diffsqlModule->makeSQLExportScript(catalog, options, create_map, drop_map);
    std::string export_sql_script = options.get_string("OutputScript");
    data->tester->executeScript(stmt.get(), export_sql_script);

    std::list<std::string> schemas;
    schemas.push_back(model.schema->name());
    GRT::get()->get_undo_manager()->disable();
    db_mysql_CatalogRef cat1 = data->tester->reverseEngineerSchemas(schemas);
    if ((cat1->schemata().get(0).is_valid()) && (cat1->schemata().get(0)->name() == "mydb"))
      cat1->schemata().remove(0);

    db_mysql_CatalogRef cat2 = copy_object(cat1);

    // Diff identical catalogs, no changes expected.
    std::shared_ptr<DiffChange> diff = diff_make(cat1, cat2, &data->omf);
    $expect(diff).Not.toBeValid();
  });

}
}
