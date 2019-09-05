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

#include "wb_test_helpers.h"
#include "grtdb/db_object_helpers.h"
#include "grt_test_helpers.h"
#include "grtpp_util.h"
#include "grt/clipboard.h"
#include "base/string_utilities.h"

#include "casmine.h"

namespace {

$ModuleEnvironment(casmine::GrtEnvironment) {};

using namespace base;
using namespace wb;

static bool match_member(const grt::MetaClass::Member *member, const grt::ObjectRef &copy,
                         const grt::ObjectRef &source) {
  if (!grt::is_simple_type(member->type.base.type))
    return true;

  if (member->name == "guid") // it's always true, as it's unique per GrtObject
    return true;
  grt::ValueRef value1;
  grt::ValueRef value2;

  value1 = source.get_member(member->name);
  value2 = copy.get_member(member->name);

  $expect(value1.toString()).toBe(value2.toString());

  return true;
}

static void ensure_simple_contents_match(const grt::ObjectRef &copy, const grt::ObjectRef &source) {
  grt::MetaClass *mc = copy.get_metaclass();

  mc->foreach_member(std::bind(&match_member, std::placeholders::_1, copy, source));
}

static void ensure_list_contents_copy(const grt::BaseListRef &copy, const grt::BaseListRef &source) {
  $expect(copy.valueptr() != source.valueptr()).toBeTrue();

  $expect(copy.count()).toBe(source.count());

  for (size_t c = copy.count(), i = 0; i < c; i++) {
    $expect(copy[i].valueptr() != source[i].valueptr()).toBeTrue();

    grt::ObjectRef copyRef = grt::ObjectRef::cast_from(copy[i]);
    grt::ObjectRef sourceRef = grt::ObjectRef::cast_from(source[i]);
    ensure_simple_contents_match(grt::ObjectRef(copyRef), grt::ObjectRef(sourceRef));
  }
}

$TestData {
  std::unique_ptr<WorkbenchTester> tester;
};

$describe("Copy/paste related tests") {
  $beforeAll([&]() {
    data->tester.reset(new WorkbenchTester());
    data->tester->initializeRuntime();
  });

  $afterAll([&]() {
  });

  $it("Copy to clipboard", [this]() {
    data->tester->wb->open_document("data/workbench/all_objects.mwb");

    $expect(data->tester->getPview()->figures().count()).toBe(6U);

    workbench_physical_TableFigureRef source, copy;
    source = workbench_physical_TableFigureRef::cast_from(
      grt::find_named_object_in_list(data->tester->getPview()->figures(), "table1"));

    $expect(source.is_valid()).toBeTrue();

    wb::WBComponent *compo = data->tester->wb->get_component_handling(source);
    $expect(compo != 0).toBeTrue();

    grt::CopyContext context;

    compo->copy_object_to_clipboard(source, context);

    $expect(bec::GRTManager::get()->get_clipboard()->get_data().empty() == false).toBeTrue();
    copy = workbench_physical_TableFigureRef::cast_from(bec::GRTManager::get()->get_clipboard()->get_data().front());

    $expect(copy.is_valid()).toBeTrue();
    $expect(copy.id() != source.id()).toBeTrue();

    $expect(copy->owner() == source->owner()).toBeTrue();
    $expect(copy->layer() == source->layer()).toBeTrue();

    $expect(copy.valueptr() != source.valueptr()).toBeTrue();

    $expect(copy->table() == source->table()).toBeTrue();

    data->tester->wb->close_document();
    data->tester->wb->close_document_finish();
  });

  $it("Make copy of table", [this]() {
    // create a table with PK and make sure that a copy will contain
    // proper refs to the copied objects
    // data->tester->create_new_document();
    data->tester->wb->open_document("data/workbench/all_objects.mwb");

    db_mysql_TableRef table(grt::Initialized);
    table->name("person");

    for (int i = 0; i < 5; i++) {
      db_mysql_ColumnRef column(grt::Initialized);

      column->owner(table);
      column->name(strfmt("col%i", i));
      if (i > 2)
        column->setParseType("VARCHAR(32)", data->tester->getRdbms()->simpleDatatypes());
      else
        column->setParseType("INT", data->tester->getRdbms()->simpleDatatypes());
      table->columns().insert(column);

      if (i == 0)
        table->addPrimaryKeyColumn(column);
    }

    db_mysql_TableRef copy = db_mysql_TableRef::cast_from(grt::copy_object(table));

    $expect(copy.is_valid()).toBeTrue();
    $expect(copy.valueptr() != table.valueptr()).toBeTrue();

    ensure_list_contents_copy(table->columns(), copy->columns());

    $expect(copy->primaryKey().is_valid()).toBeTrue();
    $expect(copy->primaryKey().valueptr() != table->primaryKey().valueptr()).toBeTrue();
    $expect(copy->primaryKey()->columns()[0].valueptr() != table->primaryKey()->columns()[0].valueptr()).toBeTrue();
    $expect(copy->indices().get(0).valueptr() == copy->primaryKey().valueptr()).toBeTrue();

    $expect(*copy->columns().get(0)->name()).toBe("col0");
    $expect( copy->columns().get(0)->owner() == copy).toBeTrue();

    $expect(copy->columns().get(0).valueptr()).toBe(copy->primaryKey()->columns().get(0)->referencedColumn().valueptr());

    data->tester->wb->close_document();
    data->tester->wb->close_document_finish();
  });
}

}
