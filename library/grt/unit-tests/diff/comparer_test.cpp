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

#include "grt_test_utility.h"
#include "testgrt.h"
#include "diff/grtdiff.h"
#include "grt.h"
#include "diff/diffchange.h"
#include "diff/changeobjects.h"
#include "diff/changelistobjects.h"
#include "grtdb/diff_dbobjectmatch.h"
#include "wb_helpers.h"
#include "synthetic_mysql_model.h"
#include "module_db_mysql.h"
#include "backend/diff_tree.h"

using namespace grt;

static grt::DictRef get_traits(bool case_sensitive = false) {
  grt::DictRef traits(true);
  traits.set("CaseSensitive", grt::IntegerRef(case_sensitive));
  traits.set("maxTableCommentLength", grt::IntegerRef(60));
  traits.set("maxIndexCommentLength", grt::IntegerRef(0));
  traits.set("maxColumnCommentLength", grt::IntegerRef(255));
  return traits;
}

BEGIN_TEST_DATA_CLASS(comparer_test)
public:
WBTester* tester;
TEST_DATA_CONSTRUCTOR(comparer_test) {
  tester = new WBTester;
}
END_TEST_DATA_CLASS

TEST_MODULE(comparer_test, "comparer test");

TEST_FUNCTION(1) {
  grt::DbObjectMatchAlterOmf omf;
  db_TableRef table1 = db_mysql_TableRef(grt::Initialized);
  table1->name("Table");
  db_TableRef table2 = db_mysql_TableRef(grt::Initialized);
  table2->name("TABLE");
  grt::NormalizedComparer caseless_normalizer(get_traits(false));
  caseless_normalizer.init_omf(&omf);
  std::shared_ptr<DiffChange> change = diff_make(table1, table2, &omf);
  ensure("Case table name comparison:", change.get() == NULL);
}

TEST_FUNCTION(2) {
  grt::DbObjectMatchAlterOmf omf;
  db_TableRef table1 = db_mysql_TableRef(grt::Initialized);
  table1->name("Table");
  db_TableRef table2 = db_mysql_TableRef(grt::Initialized);
  table2->name("TABLE");
  grt::NormalizedComparer normalizer(get_traits(true));
  normalizer.init_omf(&omf);
  std::shared_ptr<DiffChange> change = diff_make(table1, table2, &omf);
  ensure("Caseless table name comparison:", change.get() != NULL);
}

struct test_params {
  test_params(const bool cr, const bool clr, const std::function<void(SynteticMySQLModel&, SynteticMySQLModel&)>& f,
              const std::string& c)
    : case_result(cr), caseless_result(clr), model_init(f), comment(c){};
  bool case_result;
  bool caseless_result;
  std::function<void(SynteticMySQLModel&, SynteticMySQLModel&)> model_init;
  std::string comment;
};

void table_name_case(SynteticMySQLModel& model1, SynteticMySQLModel& model2) {
  model1.table->name("table1");
  model2.table->name("TABLE1");
}

void cloumn_name_case(SynteticMySQLModel& model1, SynteticMySQLModel& model2) {
  model1.column->name("col");
  model2.column->name("COL");
}

void index_cloumn_name(SynteticMySQLModel& model1, SynteticMySQLModel& model2) {
  model1.primaryKey->columns().get(0)->name("Iname1");
  model2.primaryKey->columns().get(0)->name("Iname2");
}

void pack_keys_case(SynteticMySQLModel& model1, SynteticMySQLModel& model2) {
  model1.table->packKeys("Test keys");
  model2.table->packKeys("Test Keys");
}

void pack_keys_defaults(SynteticMySQLModel& model1, SynteticMySQLModel& model2) {
  model1.table->packKeys("");
  model2.table->packKeys("DEFAULT");
}

// Bug #11889204 60478: CASE CHANGES IN ENUM VALUES ARE NOT RECOGNIZED
void enum_case(SynteticMySQLModel& model1, SynteticMySQLModel& model2) {
  model1.columnEnum->datatypeExplicitParams("(E1,e2)");
  model2.columnEnum->datatypeExplicitParams("(e1,E2)");
}

TEST_FUNCTION(3) {
  SynteticMySQLModel model1;
  SynteticMySQLModel model2;
  grt::DbObjectMatchAlterOmf omf;
  grt::NormalizedComparer case_normalizer(get_traits(true));
  case_normalizer.init_omf(&omf);
  std::shared_ptr<DiffChange> change = diff_make(model1.catalog, model2.catalog, &omf);
  ensure("Syntetic model comparison:", change.get() == NULL);
  grt::DbObjectMatchAlterOmf caselsess_omf;
  grt::NormalizedComparer caseless_normalizer(get_traits(false));
  caseless_normalizer.init_omf(&caselsess_omf);
  change = diff_make(model1.catalog, model2.catalog, &caselsess_omf);
  ensure("Syntetic model caseless comparison:", change.get() == NULL);
}

TEST_FUNCTION(4) {
  std::vector<test_params> test_cases;
  test_cases.push_back(test_params(true, false, table_name_case, "Table Name Case"));
  test_cases.push_back(
    test_params(true, true, cloumn_name_case, "Column Name Case")); // columns are always case sesitive
  test_cases.push_back(test_params(false, false, index_cloumn_name,
                                   "Index Column")); // index columns doesn't get compared and thus always equal
  test_cases.push_back(test_params(false, false, pack_keys_case, "Pack keys Case"));
  test_cases.push_back(test_params(false, false, pack_keys_defaults, "Pack keys Defauls"));
  test_cases.push_back(test_params(true, true, enum_case, "ENUM case compare"));
  for (std::vector<test_params>::const_iterator It = test_cases.begin(); It != test_cases.end(); ++It) {
    std::shared_ptr<DiffChange> change;
    SynteticMySQLModel model1;
    SynteticMySQLModel model2;
    It->model_init(model1, model2);

    grt::DbObjectMatchAlterOmf omf;
    grt::NormalizedComparer case_normalizer(get_traits(true));
    case_normalizer.init_omf(&omf);

    change = diff_make(model1.catalog, model2.catalog, &omf);
    ensure(std::string("Case sensitive comparison: ") + It->comment, (change.get() != NULL) == It->case_result);

    grt::DbObjectMatchAlterOmf caselsess_omf;
    grt::NormalizedComparer caseless_normalizer(get_traits(false));
    caseless_normalizer.init_omf(&caselsess_omf);

    change = diff_make(model1.catalog, model2.catalog, &caselsess_omf);
    ensure(std::string("Case insensitive comparison: ") + It->comment, (change.get() != NULL) == It->caseless_result);
  }
}

TEST_FUNCTION(5) {
  SynteticMySQLModel model1;
  SynteticMySQLModel model2;
  grt::DictRef traits = get_traits();
  traits.set("maxTableCommentLength", grt::IntegerRef(5));
  model1.table->comment("123456");
  model2.table->comment("12345");
  grt::DbObjectMatchAlterOmf omf;
  grt::NormalizedComparer normalizer5(traits);
  normalizer5.init_omf(&omf);
  std::shared_ptr<DiffChange> change = diff_make(model1.catalog, model2.catalog, &omf);
  ensure("Table Comments comparison:", change.get() == NULL);

  traits.set("maxTableCommentLength", grt::IntegerRef(6));
  grt::NormalizedComparer normalizer6(traits);
  normalizer6.init_omf(&omf);
  change = diff_make(model1.catalog, model2.catalog, &omf);
  ensure("Table Comments comparison:", change.get() != NULL);
}

TEST_FUNCTION(6) {
  SynteticMySQLModel model1;
  SynteticMySQLModel model2;
  grt::DictRef traits = get_traits();
  traits.set("maxIndexCommentLength", grt::IntegerRef(5));
  model1.indexColumn->comment("123456");
  model2.indexColumn->comment("12345");
  grt::DbObjectMatchAlterOmf omf;
  grt::NormalizedComparer normalizer5(traits);
  normalizer5.init_omf(&omf);
  std::shared_ptr<DiffChange> change = diff_make(model1.catalog, model2.catalog, &omf);
  ensure("Index Comments comparison:", change.get() == NULL);

  traits.set("maxIndexCommentLength", grt::IntegerRef(6));
  grt::NormalizedComparer normalizer6(traits);
  normalizer6.init_omf(&omf);
  change = diff_make(model1.catalog, model2.catalog, &omf);
  ensure("Index Comments comparison:", change.get() != NULL);
}

TEST_FUNCTION(7) {
  SynteticMySQLModel model1;
  SynteticMySQLModel model2;
  grt::DictRef traits = get_traits();
  model1.indexColumn->comment("abcd");
  model2.indexColumn->comment("12345");
  grt::DbObjectMatchAlterOmf omf;
  grt::NormalizedComparer normalizer(traits);
  normalizer.init_omf(&omf);
  std::shared_ptr<DiffChange> change = diff_make(model1.catalog, model2.catalog, &omf);
  ensure("Index Comments omit comparison:", change.get() == NULL);
}

TEST_FUNCTION(8) {
  SynteticMySQLModel model1;
  SynteticMySQLModel model2;
  grt::DictRef traits = get_traits();
  traits.set("maxColumnCommentLength", grt::IntegerRef(5));
  model1.column->comment("123456");
  model2.column->comment("12345");
  grt::DbObjectMatchAlterOmf omf;
  grt::NormalizedComparer normalizer5(traits);
  normalizer5.init_omf(&omf);
  std::shared_ptr<DiffChange> change = diff_make(model1.catalog, model2.catalog, &omf);
  ensure("Column Comments comparison:", change.get() == NULL);

  traits.set("maxColumnCommentLength", grt::IntegerRef(6));
  grt::NormalizedComparer normalizer6(traits);
  normalizer6.init_omf(&omf);
  change = diff_make(model1.catalog, model2.catalog, &omf);
  ensure("Column Comments comparison:", change.get() != NULL);
}

void test_table_collation(std::string src, std::string dst, bool equal = false) {
  grt::DbObjectMatchAlterOmf omf3;
  db_TableRef table1 = db_mysql_TableRef(grt::Initialized);
  table1->set_member("defaultCollationName", grt::StringRef(src.c_str()));
  db_TableRef table2 = db_mysql_TableRef(grt::Initialized);
  table2->set_member("defaultCollationName", grt::StringRef(dst.c_str()));
  grt::NormalizedComparer caseless_normalizer3(get_traits(false));
  caseless_normalizer3.init_omf(&omf3);
  std::shared_ptr<DiffChange> change3 = diff_make(table1, table2, &omf3);

  /*
  if(change3)
    change3->dump_log(0);
    */

  std::string infomsg = "Table collation comparison for (" + src + ") and (" + dst + ")";
  if (equal)
    ensure(infomsg, change3 == NULL);
  else
    ensure(infomsg, change3 != NULL);
}

/*
 * test collation change, related to #16492371
 * there was a problem when we were changing collation inside character set
 * wb was not detecting that change
 */
TEST_FUNCTION(9) {
  grt::DbObjectMatchAlterOmf omf;
  db_SchemaRef schema1 = db_SchemaRef(grt::Initialized);
  db_SchemaRef schema2 = db_SchemaRef(grt::Initialized);
  grt::NormalizedComparer normalizer(get_traits(true));
  normalizer.init_omf(&omf);
  std::shared_ptr<DiffChange> change = diff_make(schema1, schema2, &omf);
  ensure("9.1 No changes:", change.get() == NULL);

  test_table_collation("latin1_general_ci", "latin1_spanish_ci");
  test_table_collation("", "latin1_spanish_ci");
  test_table_collation("", "latin1_swedish_ci");
  test_table_collation("latin1_swedish_ci", "latin1_swedish_ci", true);

  SynteticMySQLModel model1a;
  model1a.column->collationName("latin1_general_ci");
  SynteticMySQLModel model2a;
  model2a.column->collationName("latin1_polish_ci");
  grt::DbObjectMatchAlterOmf omf4a;
  grt::NormalizedComparer caseless_normalizer4a(get_traits(false));
  caseless_normalizer4a.init_omf(&omf4a);

  std::shared_ptr<DiffChange> change4a = diff_make(model1a.column, model2a.column, &omf4a);
  ensure("9.6 Column collation comparison:", change4a.get() != NULL);

  SynteticMySQLModel model1b;
  model1b.column->characterSetName("latin1");
  SynteticMySQLModel model2b;
  model2b.column->collationName("latin1_german_ci");
  grt::DbObjectMatchAlterOmf omf4b;
  grt::NormalizedComparer caseless_normalizer4b(get_traits(false));
  caseless_normalizer4b.init_omf(&omf4b);

  std::shared_ptr<DiffChange> change4b = diff_make(model1b.column, model2b.column, &omf4b);
  ensure("9.7 Column collation comparison:", change4b.get() != NULL);

  SynteticMySQLModel model1c;
  model1c.table->set_member("defaultCharacterSetName", grt::StringRef("utf8"));
  model1c.table->set_member("defaultCollationName", grt::StringRef(""));
  model1c.column->characterSetName("latin1");

  SynteticMySQLModel model2c;
  model2c.table->set_member("defaultCharacterSetName", grt::StringRef("utf8"));
  model2c.table->set_member("defaultCollationName", grt::StringRef(""));
  model2c.column->characterSetName("latin1");
  model2c.column->collationName("latin1_german_ci");

  grt::DbObjectMatchAlterOmf omf4c;
  grt::NormalizedComparer caseless_normalizer4c(get_traits(false));
  caseless_normalizer4c.init_omf(&omf4c);

  std::shared_ptr<DiffChange> change4c = diff_make(model1c.column, model2c.column, &omf4c);
  ensure("9.8 Column collation comparison:", change4c.get() != NULL);

  SynteticMySQLModel model1d;
  model1d.table->set_member("defaultCharacterSetName", grt::StringRef("utf8"));
  model1d.table->set_member("defaultCollationName", grt::StringRef("utf8_czech_ci"));
  model1d.column->characterSetName("latin1");

  SynteticMySQLModel model2d;
  model2d.table->set_member("defaultCharacterSetName", grt::StringRef("utf8"));
  model2d.table->set_member("defaultCollationName", grt::StringRef("utf8_czech_ci"));
  model2d.column->characterSetName("latin1");
  model2d.column->collationName("latin1_german_ci");

  grt::DbObjectMatchAlterOmf omf4d;
  grt::NormalizedComparer caseless_normalizer4d(get_traits(false));
  caseless_normalizer4d.init_omf(&omf4d);

  std::shared_ptr<DiffChange> change4d = diff_make(model1d.column, model2d.column, &omf4d);
  ensure("9.9 Column collation comparison:", change4d.get() != NULL);
}

TEST_FUNCTION(10) {
  grt::DbObjectMatchAlterOmf omf;
  db_RoutineRef routine1 = db_RoutineRef(grt::Initialized);
  db_RoutineRef routine2 = db_RoutineRef(grt::Initialized);
  routine1->name("routine1");
  routine2->name("routine1");
  routine1->definer("root@localhost");
  grt::DictRef opts = get_traits(false);
  opts.set("SkipRoutineDefiner", grt::IntegerRef(1));
  grt::NormalizedComparer normalizer(opts);
  normalizer.init_omf(&omf);
  std::shared_ptr<DiffChange> change = diff_make(routine1, routine2, &omf);
  ensure("10.1 Routine deffiner, wasn't skipped", change.get() == NULL);

  grt::DbObjectMatchAlterOmf omf2;
  opts.set("SkipRoutineDefiner", grt::IntegerRef(0));
  grt::NormalizedComparer normalizer2(opts);
  normalizer2.init_omf(&omf2);
  std::shared_ptr<DiffChange> change2 = diff_make(routine1, routine2, &omf2);
  ensure("10.2 Routine definer, wasn't different", change2.get() != NULL);
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(11) {
  delete tester;
}

END_TESTS
