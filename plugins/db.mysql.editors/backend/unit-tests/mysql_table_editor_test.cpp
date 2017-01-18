/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "grt.h"
#include "../../plugins/db.mysql.editors/backend/mysql_table_editor.h"
#include "synthetic_mysql_model.h"
#include "wb_helpers.h"

using namespace grt;
using namespace bec;
using namespace tut;

BEGIN_TEST_DATA_CLASS(mysql_table_editor)
public:
WBTester *tester;

TEST_DATA_CONSTRUCTOR(mysql_table_editor) {
  tester = new WBTester();
  populate_grt(*tester);

  tester->flush_until(0.5);
  tester->create_new_document();
}

END_TEST_DATA_CLASS

TEST_MODULE(mysql_table_editor, "mysql_table_editor");

TEST_FUNCTION(10) {
  tester->renew_document();
  ensure("db_mgmt_RdbmsRef initialization", tester->get_rdbms().is_valid());
}

TEST_FUNCTION(20) {
  // Note: this test relied on content of a code editor (which is checked when setting trigger sql).
  //       However in tests we only have a stub implementation, so this doesn't work.
  //       The test shouldn't be about parsing trigger sql, as this is a low level parser test.
  //       Instead test if trigger addition works (removal is a simple grt call).

  tester->renew_document();
  SynteticMySQLModel model(tester);

  model.schema->name("test_schema");
  model.table->name("film");
  MySQLTableEditorBE t(model.table);
  model.table->triggers().remove_all();

  t.add_trigger("after", "delete");
  t.add_trigger("before", "delete");
  t.add_trigger("after", "update");

  assure_equal(model.table->triggers().count(), 3U);
  std::string names[] = {"film_after_delete", "film_before_delete", "film_after_update"};

  for (size_t i = 0, size = model.table->triggers().count(); i < size; i++) {
    std::string name = model.table->triggers().get(i)->name();
    assure_equal(name, names[i]);
  }
}

TEST_FUNCTION(30) {
  // check if adding columns/indices/foreign keys by setting name of placeholder item works

  tester->renew_document();
  SynteticMySQLModel model(tester);

  db_mysql_TableRef table = model.table;
  table->name("table");
  table->columns().remove_all();
  table->indices().remove_all();
  table->foreignKeys().remove_all();

  MySQLTableEditorBE editor(table);

  ensure_equals("add column", table->columns().count(), 0U);
  ((bec::TableColumnsListBE *)editor.get_columns())->set_field(0, 0, "newcol");
  ((bec::TableColumnsListBE *)editor.get_columns())->set_field(0, 1, "int(11)");
  ensure_equals("add column", table->columns().count(), 1U);

  editor.get_indexes()->select_index(0);
  ensure_equals("add index", table->indices().count(), 0U);
  editor.get_indexes()->set_field(0, 0, "index");
  ensure_equals("add index", table->indices().count(), 1U);

  editor.get_fks()->select_fk(0);
  ensure_equals("add fk", table->foreignKeys().count(), 0U);
  editor.get_fks()->set_field(0, 0, "newfk");
  ensure_equals("add fk", table->foreignKeys().count(), 1U);
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  delete tester;
}

END_TESTS
