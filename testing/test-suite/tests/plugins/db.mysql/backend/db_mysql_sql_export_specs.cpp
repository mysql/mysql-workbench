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

#include "base/file_utilities.h"

#include "grtsqlparser/sql_facade.h"

#include "backend/db_mysql_sql_export.h"

#include "wb_test_helpers.h"
#include "casmine.h"

using namespace grt;

namespace {

$ModuleEnvironment() {};

$TestData {
  std::unique_ptr<WorkbenchTester> tester;
  SqlFacade::Ref sql_facade;
  db_mgmt_RdbmsRef rdbms;
  DictRef options;

  std::string dataDir;

  void doForwardEngineering(std::string &modelfile, std::string &expectedFileName, std::map<std::string, bool> &fwd_opts) {
    $expect(base::file_exists(modelfile)).toBeTrue("Model file not found");

    tester->wb->open_document(modelfile);
    tester->openAllDiagrams();
    tester->activateOverview();

    DbMySQLSQLExport exp(db_mysql_CatalogRef::cast_from(tester->getCatalog()));

    ValueRef valRef = grt::GRT::get()->get("/wb/doc/physicalModels/0/catalog/schemata/0");

    db_mysql_SchemaRef schemaRef = db_mysql_SchemaRef::cast_from(valRef);
    $expect(schemaRef.is_valid()).toBeTrue();

    bec::GrtStringListModel *users_model;
    bec::GrtStringListModel *users_imodel;
    bec::GrtStringListModel *tables_model;
    bec::GrtStringListModel *tables_imodel;
    bec::GrtStringListModel *views_model;
    bec::GrtStringListModel *views_imodel;
    bec::GrtStringListModel *routines_model;
    bec::GrtStringListModel *routines_imodel;
    bec::GrtStringListModel *triggers_model;
    bec::GrtStringListModel *triggers_imodel;

    exp.setup_grt_string_list_models_from_catalog(&users_model, &users_imodel, &tables_model, &tables_imodel,
                                                  &views_model, &views_imodel, &routines_model, &routines_imodel,
                                                  &triggers_model, &triggers_imodel);

    std::map<std::string, bool>::iterator it;
    for (it = fwd_opts.begin(); it != fwd_opts.end(); ++it)
      exp.set_option(it->first, it->second);

    exp.start_export(true);

    std::string output = exp.export_sql_script();
    $expect(output).toEqualContentOfFile(expectedFileName);

    tester->wb->close_document();
    tester->wb->close_document_finish();
  }
};

$describe("Forward Engineer") {

  $beforeAll([this]() {
    data->dataDir = casmine::CasmineContext::get()->tmpDataDir();
    data->tester.reset(new WorkbenchTester());
  });

  $it("General test for forward engineer of sakila database", [this]() {
    std::map<std::string, bool> opts;
    std::string modelfile = data->dataDir + "/forward_engineer/sakila.mwb";
    std::string expectedFileName = data->dataDir + "/forward_engineer/sakila.expected.sql";

    opts["GenerateDrops"] = true;
    opts["GenerateSchemaDrops"] = true;
    opts["SkipForeignKeys"] = true;
    opts["SkipFKIndexes"] = true;
    opts["GenerateWarnings"] = true;
    opts["GenerateCreateIndex"] = true;
    opts["NoUsersJustPrivileges"] = false;
    opts["NoViewPlaceholders"] = false;
    opts["GenerateInserts"] = false;
    opts["NoFKForInserts"] = false;
    opts["TriggersAfterInserts"] = true;
    opts["OmitSchemata"] = false;
    opts["GenerateUse"] = true;

    opts["TablesAreSelected"] = true;
    opts["TriggersAreSelected"] = true;
    opts["RoutinesAreSelected"] = true;
    opts["ViewsAreSelected"] = true;
    opts["UsersAreSelected"] = true;
    opts["GenerateDocumentProperties"] = false;

    data->doForwardEngineering(modelfile, expectedFileName, opts);
  });

  $it("Forward engineering of routines with ommitSchemata enabled", [this]() {
    std::map<std::string, bool> opts;
    std::string modelfile = data->dataDir + "/forward_engineer/omit_schema_routine.mwb";
    std::string expectedFileName = data->dataDir + "/forward_engineer/omit_schema_routine.expected.sql";

    opts["GenerateDrops"] = true;
    opts["GenerateSchemaDrops"] = false;
    opts["SkipForeignKeys"] = true;
    opts["SkipFKIndexes"] = false;
    opts["GenerateWarnings"] = false;
    opts["GenerateCreateIndex"] = false;
    opts["NoUsersJustPrivileges"] = false;
    opts["NoViewPlaceholders"] = false;
    opts["GenerateInserts"] = false;
    opts["NoFKForInserts"] = false;
    opts["TriggersAfterInserts"] = false;
    opts["OmitSchemata"] = true;
    opts["GenerateUse"] = false;

    opts["TablesAreSelected"] = true;
    opts["TriggersAreSelected"] = false;
    opts["RoutinesAreSelected"] = true;
    opts["ViewsAreSelected"] = false;
    opts["UsersAreSelected"] = true;
    opts["GenerateDocumentProperties"] = false;

    data->doForwardEngineering(modelfile, expectedFileName, opts);
  });

  $it("Forward engineering of routines with ommitSchemata enabled", [this]() {
    std::map<std::string, bool> opts;
    std::string modelfile = data->dataDir + "/forward_engineer/schema_rename.mwb";
    std::string expectedFileName = data->dataDir + "/forward_engineer/schema_rename.expected.sql";

    opts["GenerateDrops"] = true;
    opts["GenerateSchemaDrops"] = true;
    opts["SkipForeignKeys"] = true;
    opts["SkipFKIndexes"] = true;
    opts["GenerateWarnings"] = true;
    opts["GenerateCreateIndex"] = true;
    opts["NoUsersJustPrivileges"] = true;
    opts["NoViewPlaceholders"] = true;
    opts["GenerateInserts"] = true;
    opts["NoFKForInserts"] = true;
    opts["TriggersAfterInserts"] = true;
    opts["OmitSchemata"] = true;
    opts["GenerateUse"] = true;

    opts["TablesAreSelected"] = true;
    opts["TriggersAreSelected"] = true;
    opts["RoutinesAreSelected"] = true;
    opts["ViewsAreSelected"] = true;
    opts["UsersAreSelected"] = true;
    opts["GenerateDocumentProperties"] = false;

    data->doForwardEngineering(modelfile, expectedFileName, opts);
  });

}

}
