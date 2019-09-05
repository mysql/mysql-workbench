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

#include "diff/grtdiff.h"
#include "grt.h"
#include "diff/diffchange.h"
#include "diff/changeobjects.h"
#include "diff/changelistobjects.h"
#include "grtdb/diff_dbobjectmatch.h"
#include "wb_test_helpers.h"
#include "grtdb/db_helpers.h"
#include "module_db_mysql.h"
#include "backend/diff_tree.h"
#include "base/util_functions.h"

#include "casmine.h"

using namespace grt;

namespace {

$ModuleEnvironment() {};

static grt::DictRef get_traits(bool case_sensitive = false) {
  grt::DictRef traits(true);
  traits.set("CaseSensitive", grt::IntegerRef(case_sensitive));
  traits.set("maxTableCommentLength", grt::IntegerRef(60));
  traits.set("maxIndexCommentLength", grt::IntegerRef(0));
  traits.set("maxColumnCommentLength", grt::IntegerRef(255));
  return traits;
}

$TestData {
  std::unique_ptr<WorkbenchTester> tester;
  std::string dataDir;
};

$describe("Sync diff") {
  $beforeAll([this]() {
    data->tester.reset(new WorkbenchTester());
    data->tester->initializeRuntime();

    data->dataDir = casmine::CasmineContext::get()->tmpDataDir();
  });

  $it("Bug #16492371: sync problem when model has difference in schema collation and table collation", [this]() {
    ValueRef source_val(grt::GRT::get()->unserialize(data->dataDir + "/diff/sync-catalogs-collations/source_catalog.xml"));
    ValueRef target_val(grt::GRT::get()->unserialize(data->dataDir + "/diff/sync-catalogs-collations/target_catalog.xml"));

    db_mysql_CatalogRef mod_cat = db_mysql_CatalogRef::cast_from(source_val);
    db_mysql_CatalogRef org_cat = db_mysql_CatalogRef::cast_from(target_val);

    grt::NormalizedComparer normalizer(get_traits(true));
    grt::DbObjectMatchAlterOmf omf;
    omf.dontdiff_mask = 3;
    normalizer.init_omf(&omf);
    std::shared_ptr<DiffChange> diff_change = diff_make(org_cat, mod_cat, &omf);

    DbMySQLImpl *diffsql_module = grt::GRT::get()->get_native_module<DbMySQLImpl>();

    grt::StringListRef alter_map(grt::Initialized);
    grt::ListRef<GrtNamedObject> alter_object_list(true);
    grt::DictRef options(true);
    options.set("UseFilteredLists", grt::IntegerRef(0));
    options.set("OutputContainer", alter_map);
    options.set("OutputObjectContainer", alter_object_list);
    options.set("CaseSensitive", grt::IntegerRef(omf.case_sensitive));

    diffsql_module->generateSQL(org_cat, options, diff_change);
    std::shared_ptr<DiffTreeBE> diff_tree(new ::DiffTreeBE(std::vector<std::string>(), mod_cat, org_cat, diff_change));

    bool foundSchemaDiff = false;
    bool foundTableDiff = false;
    for (std::size_t c = diff_tree->count(), i = 0; i < c; i++) {
      bec::NodeId schema((int)i);
      for (size_t j = 0; j < diff_tree->count_children(schema); j++) {
        bec::NodeId object(diff_tree->get_child(schema, j));
        std::string name;

        diff_tree->get_field(schema, DiffTreeBE::ModelObjectName, name);
        if (name == "chartest" && (diff_tree->get_apply_direction(schema) == DiffNode::ApplyToDb)) {
          foundSchemaDiff = true;
        }
        diff_tree->get_field(object, DiffTreeBE::ModelObjectName, name);
        if (name == "chartable" && (diff_tree->get_apply_direction(object) == DiffNode::ApplyToDb)) {
          foundTableDiff = true;
        }
      }
    }

    $expect(foundTableDiff && foundSchemaDiff).toBeTrue();
  });

  $it("Regression test for bug #17454626", [this]() {
    ValueRef source_val(grt::GRT::get()->unserialize(data->dataDir + "/diff/sync-catalogs-rowformat/source_catalog.xml"));
    ValueRef target_val(grt::GRT::get()->unserialize(data->dataDir + "/diff/sync-catalogs-rowformat/target_catalog.xml"));

    db_mysql_CatalogRef source_cat = db_mysql_CatalogRef::cast_from(source_val);
    db_mysql_CatalogRef target_cat = db_mysql_CatalogRef::cast_from(target_val);

    DbMySQLImpl *diffsql_module = grt::GRT::get()->get_native_module<DbMySQLImpl>();

    grt::DictRef options(true);
    options.set("CaseSensitive", grt::IntegerRef(true));
    options.set("GenerateDocumentProperties", grt::IntegerRef(0));

    std::string script = diffsql_module->makeAlterScript(target_cat, source_cat, options);

    std::string expected_sql = data->dataDir + "/diff/sync-catalogs-rowformat/good.sql";
    std::ifstream ref(expected_sql.c_str());
    std::stringstream ss(script);

    std::string line, refline;

    $expect(ref.is_open()).toBeTrue();

    while (ref.good() && ss.good()) {
      getline(ref, refline);
      getline(ss, line);
      $expect(line).toBe(refline);
    }
  });

  $it("Test for bug column rename no #19500938", [this]() {
    ValueRef source_val(grt::GRT::get()->unserialize(data->dataDir + "/diff/column_rename/1_src.xml"));
    ValueRef target_val(grt::GRT::get()->unserialize(data->dataDir + "/diff/column_rename/1_dst.xml"));

    db_mysql_CatalogRef source_cat = db_mysql_CatalogRef::cast_from(source_val);
    db_mysql_CatalogRef target_cat = db_mysql_CatalogRef::cast_from(target_val);
    DbMySQLImpl *diffsql_module = grt::GRT::get()->get_native_module<DbMySQLImpl>();

    grt::DictRef options(true);
    options.set("CaseSensitive", grt::IntegerRef(true));
    options.set("GenerateDocumentProperties", grt::IntegerRef(0));

    std::string script = diffsql_module->makeAlterScript(target_cat, source_cat, options);

    std::string expected_sql = data->dataDir + "/diff/column_rename/1_expected.sql";
    std::ifstream ref(expected_sql.c_str());
    std::stringstream ss(script);

    std::string line, refline;

    $expect(ref.is_open()).toBeTrue();

    while (ref.good() && ss.good()) {
      getline(ref, refline);
      getline(ss, line);
      $expect(line).toBe(refline);
    }
  });

  $it("Test for bug column rename and reorder no #20128561", [this]() {
    ValueRef source_val(grt::GRT::get()->unserialize(data->dataDir + "/diff/column_rename/2_src.xml"));
    ValueRef target_val(grt::GRT::get()->unserialize(data->dataDir + "/diff/column_rename/2_dst.xml"));

    db_mysql_CatalogRef source_cat = db_mysql_CatalogRef::cast_from(source_val);
    db_mysql_CatalogRef target_cat = db_mysql_CatalogRef::cast_from(target_val);
    DbMySQLImpl *diffsql_module = grt::GRT::get()->get_native_module<DbMySQLImpl>();

    grt::DictRef options(true);
    options.set("CaseSensitive", grt::IntegerRef(true));
    options.set("GenerateDocumentProperties", grt::IntegerRef(0));

    std::string script = diffsql_module->makeAlterScript(target_cat, source_cat, options);

    std::string expected_sql = data->dataDir + "/diff/column_rename/2_expected.sql";
    std::ifstream ref(expected_sql.c_str());
    std::stringstream ss(script);

    std::string line, refline;

    $expect(ref.is_open()).toBeTrue();

    while (ref.good() && ss.good()) {
      getline(ref, refline);
      getline(ss, line);
      $expect(line).toBe(refline);
    }
  });

  $it("Test for bug index change no #27868813", [this]() {
    ValueRef source_val(grt::GRT::get()->unserialize(data->dataDir + "/diff/index_change/1_src.xml"));
    ValueRef target_val(grt::GRT::get()->unserialize(data->dataDir + "/diff/index_change/1_dst.xml"));

    db_mysql_CatalogRef source_cat = db_mysql_CatalogRef::cast_from(source_val);
    db_mysql_CatalogRef target_cat = db_mysql_CatalogRef::cast_from(target_val);
    DbMySQLImpl *diffsql_module = grt::GRT::get()->get_native_module<DbMySQLImpl>();

    grt::DictRef options(true);
    options.set("CaseSensitive", grt::IntegerRef(true));
    options.set("GenerateDocumentProperties", grt::IntegerRef(0));

    std::string script = diffsql_module->makeAlterScript(target_cat, source_cat, options);

    std::string expected_sql = data->dataDir + "/diff/index_change/1_expected.sql";
    std::ifstream ref(expected_sql.c_str());
    std::stringstream ss(script);

    std::string line, refline;
    $expect(ref.is_open()).toBeTrue();

    while (ref.good() && ss.good()) {
      getline(ref, refline);
      getline(ss, line);
      $expect(line).toBe(refline);
    }
  });

  $it("Diff test with CaseSensitive option 1", [this]() {
    ValueRef source_val(grt::GRT::get()->unserialize(data->dataDir + "/diff/index_change/2_src.xml"));
    ValueRef target_val(grt::GRT::get()->unserialize(data->dataDir + "/diff/index_change/2_dst.xml"));

    db_mysql_CatalogRef source_cat = db_mysql_CatalogRef::cast_from(source_val);
    db_mysql_CatalogRef target_cat = db_mysql_CatalogRef::cast_from(target_val);
    DbMySQLImpl *diffsql_module = grt::GRT::get()->get_native_module<DbMySQLImpl>();

    grt::DictRef options(true);
    options.set("CaseSensitive", grt::IntegerRef(true));
    options.set("GenerateDocumentProperties", grt::IntegerRef(0));

    std::string script = diffsql_module->makeAlterScript(target_cat, source_cat, options);

    std::string expected_sql = data->dataDir + "/diff/index_change/2_expected.sql";
    std::ifstream ref(expected_sql.c_str());
    std::stringstream ss(script);

    std::string line, refline;
    $expect(ref.is_open()).toBeTrue();

    while (ref.good() && ss.good()) {
      getline(ref, refline);
      getline(ss, line);
      $expect(line).toBe(refline);
    }
  });

  $it("Diff test with CaseSensitive option 2", [this]() {
    ValueRef source_val(grt::GRT::get()->unserialize(data->dataDir + "/diff/index_change/3_src.xml"));
    ValueRef target_val(grt::GRT::get()->unserialize(data->dataDir + "/diff/index_change/3_dst.xml"));
    db_mysql_CatalogRef source_cat = db_mysql_CatalogRef::cast_from(source_val);
    db_mysql_CatalogRef target_cat = db_mysql_CatalogRef::cast_from(target_val);
    source_cat->version(bec::parse_version(base::getVersion()));
    target_cat->version(bec::parse_version(base::getVersion()));
    DbMySQLImpl *diffsql_module = grt::GRT::get()->get_native_module<DbMySQLImpl>();

    grt::DictRef options(true);
    options.set("CaseSensitive", grt::IntegerRef(true));
    options.set("GenerateDocumentProperties", grt::IntegerRef(0));

    std::string script = diffsql_module->makeAlterScript(source_cat, target_cat, options);

    std::string expected_sql = data->dataDir + "/diff/index_change/3_expected.sql";
    std::ifstream ref(expected_sql.c_str());
    std::stringstream ss(script);

    std::string line, refline;
    $expect(ref.is_open()).toBeTrue();

    while (ref.good() && ss.good()) {
      getline(ref, refline);
      getline(ss, line);
      $expect(line).toBe(refline);
    }
  });

}
}
