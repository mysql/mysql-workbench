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
#include "wb_test_helpers.h"

#include "grt.h"
#include "mysql_table_editor.h"
#include "model_mockup.h"

using namespace grt;
using namespace bec;
using namespace casmine;

namespace {

$ModuleEnvironment() {};

$TestData {
  std::unique_ptr<WorkbenchTester> tester;
};

$describe("MySQL Table Editor") {
  $beforeAll([this]() {
    data->tester.reset(new WorkbenchTester());
    data->tester->initializeRuntime();
    data->tester->flushUntil(0.5);
    data->tester->createNewDocument();
  });

  $it("Valid RDBMS after renewing document", [this]() {
    data->tester->renewDocument();
    $expect(data->tester->getRdbms().is_valid()).toBeTrue("db_mgmt_RdbmsRef initialization");
  });

  $it("Trigger parsing", [this]() {
    // Note: this test relied on content of a code editor (which is checked when setting trigger sql).
    //       However in tests we only have a stub implementation, so this doesn't work.
    //       The test shouldn't be about parsing trigger sql, as this is a low level parser test.
    //       Instead test if trigger addition works (removal is a simple grt call).
    data->tester->renewDocument();
    SyntheticMySQLModel model(data->tester.get());

    model.schema->name("test_schema");
    model.table->name("film");
    MySQLTableEditorBE t(model.table);
    model.table->triggers().remove_all();

    t.add_trigger("after", "delete");
    t.add_trigger("before", "delete");
    t.add_trigger("after", "update");

    $expect(model.table->triggers().count()).toEqual(3U);
    std::vector<std::string> names = { "film_after_delete", "film_before_delete", "film_after_update" };

    for (size_t i = 0, size = model.table->triggers().count(); i < size; i++) {
      std::string name = model.table->triggers().get(i)->name();
      $expect(name).toEqual(names[i]);
    }
  });

  $it("Add columns/indices/foreign keys by setting name of placeholder items", [this]() {
    data->tester->renewDocument();
    SyntheticMySQLModel model(data->tester.get());

    db_mysql_TableRef table = model.table;
    table->name("table");
    table->columns().remove_all();
    table->indices().remove_all();
    table->foreignKeys().remove_all();

    MySQLTableEditorBE editor(table);

    $expect(table->columns().count()).toEqual(0U, "add column");
    ((bec::TableColumnsListBE *)editor.get_columns())->set_field(0, 0, "newcol");
    ((bec::TableColumnsListBE *)editor.get_columns())->set_field(0, 1, "int(11)");
    $expect(table->columns().count()).toEqual(1U, "add column");

    editor.get_indexes()->select_index(0);
    $expect(table->indices().count()).toEqual(0U, "add index");
    editor.get_indexes()->set_field(0, 0, "index");
    $expect(table->indices().count()).toEqual(1U, "add index");

    editor.get_fks()->select_fk(0);
    $expect(table->foreignKeys().count()).toEqual(0U, "add fk");
    editor.get_fks()->set_field(0, 0, "newfk");
    $expect(table->foreignKeys().count()).toEqual(1U, "add fk");
  });
};

}
