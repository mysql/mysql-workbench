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

#include "wb_helpers.h"
#include "grtdb/db_object_helpers.h"
#include "grt_test_utility.h"
#include "grtpp_util.h"
#include "grt/clipboard.h"
#include "base/string_utilities.h"

using namespace base;
using namespace wb;

BEGIN_TEST_DATA_CLASS(wb_copy_paste)
public:
WBTester *tester;

TEST_DATA_CONSTRUCTOR(wb_copy_paste) {
  tester = new WBTester;
  populate_grt(*tester);
}

END_TEST_DATA_CLASS;

TEST_MODULE(wb_copy_paste, "copy/paste related tests");

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

  ensure_equals(member->name, value1.toString(), value2.toString());

  return true;
}

static void ensure_simple_contents_match(const grt::ObjectRef &copy, const grt::ObjectRef &source) {
  grt::MetaClass *mc = copy.get_metaclass();

  mc->foreach_member(std::bind(&match_member, std::placeholders::_1, copy, source));
}

static void ensure_list_contents_copy(const grt::BaseListRef &copy, const grt::BaseListRef &source) {
  ensure("table columns list are not same", copy.valueptr() != source.valueptr());

  ensure_equals("list size", copy.count(), source.count());

  for (size_t c = copy.count(), i = 0; i < c; i++) {
    ensure("list content not same", copy[i].valueptr() != source[i].valueptr());

    ObjectRef copyRef = ObjectRef::cast_from(copy[i]);
    ObjectRef sourceRef = ObjectRef::cast_from(source[i]);
    ensure_simple_contents_match(grt::ObjectRef(copyRef), grt::ObjectRef(sourceRef));
  }
}

TEST_FUNCTION(1) {
  tester->wb->open_document("data/workbench/all_objects.mwb");

  ensure_equals("figure count", tester->get_pview()->figures().count(), 6U);

  workbench_physical_TableFigureRef source, copy;
  source = workbench_physical_TableFigureRef::cast_from(
    grt::find_named_object_in_list(tester->get_pview()->figures(), "table1"));

  ensure("table1", source.is_valid());

  WBComponent *compo = tester->wb->get_component_handling(source);
  ensure("table is copiable", compo != 0);

  grt::CopyContext context;

  compo->copy_object_to_clipboard(source, context);

  ensure("clipboard has data", bec::GRTManager::get()->get_clipboard()->get_data().empty() == false);
  copy = workbench_physical_TableFigureRef::cast_from(bec::GRTManager::get()->get_clipboard()->get_data().front());

  ensure("copy is valid", copy.is_valid());
  ensure("copy worked", copy.id() != source.id());

  ensure("both in same view", copy->owner() == source->owner());
  ensure("both in same layer", copy->layer() == source->layer());

  ensure("copy is not identical", copy.valueptr() != source.valueptr());

  ensure("table obj is same", copy->table() == source->table());
  // we're just copying the figure, so the db table obj shouldn't be duplicated
  // ensure("table is not same", copy->table() != source->table());

  // ensure_list_contents_copy(copy->table()->columns(), source->table()->columns());

  // ensure_list_contents_copy(copy->table()->indices(), source->table()->indices());

  tester->wb->close_document();
  tester->wb->close_document_finish();
}

TEST_FUNCTION(2) {
  // create a table with PK and make sure that a copy will contain
  // proper refs to the copied objects
  tester->create_new_document();

  db_mysql_TableRef table(grt::Initialized);
  table->name("person");

  for (int i = 0; i < 5; i++) {
    db_mysql_ColumnRef column(grt::Initialized);

    column->owner(table);
    column->name(strfmt("col%i", i));
    if (i > 2)
      column->setParseType("VARCHAR(32)", tester->get_rdbms()->simpleDatatypes());
    else
      column->setParseType("INT", tester->get_rdbms()->simpleDatatypes());
    table->columns().insert(column);

    if (i == 0)
      table->addPrimaryKeyColumn(column);
  }

  db_mysql_TableRef copy = db_mysql_TableRef::cast_from(grt::copy_object(table));

  ensure("copy", copy.is_valid());
  ensure("copy != orig", copy.valueptr() != table.valueptr());

  ensure_list_contents_copy(table->columns(), copy->columns());

  ensure("pk exists", copy->primaryKey().is_valid());
  ensure("primary key copied", copy->primaryKey().valueptr() != table->primaryKey().valueptr());
  ensure("pk copy points to column copy",
         copy->primaryKey()->columns()[0].valueptr() != table->primaryKey()->columns()[0].valueptr());
  ensure("pk points to correct index", copy->indices().get(0).valueptr() == copy->primaryKey().valueptr());

  ensure_equals("column0", *copy->columns().get(0)->name(), "col0");
  ensure("column owner", copy->columns().get(0)->owner() == copy);

  ensure_equals("pk correct", copy->columns().get(0).valueptr(),
                copy->primaryKey()->columns().get(0)->referencedColumn().valueptr());

  tester->wb->close_document();
  tester->wb->close_document_finish();
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  delete tester;
}

END_TESTS
