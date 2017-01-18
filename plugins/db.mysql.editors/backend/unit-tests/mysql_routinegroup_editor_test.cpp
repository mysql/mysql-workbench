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

#include "../../plugins/db.mysql.editors/backend/mysql_routinegroup_editor.h"
#include "synthetic_mysql_model.h"

using namespace grt;
using namespace bec;
using namespace tut;

BEGIN_TEST_DATA_CLASS(mysql_routinegroup_editor_test)
public:
WBTester* wbt;

TEST_DATA_CONSTRUCTOR(mysql_routinegroup_editor_test) {
  wbt = new WBTester();
}

END_TEST_DATA_CLASS

TEST_MODULE(mysql_routinegroup_editor_test, "mysql_routinegroup_editor_test");

TEST_FUNCTION(10) {
  const char* routine_sql = "";

  SynteticMySQLModel model;
  size_t count = model.routineGroup->routines().count();
  ensure("Invalid number of routines", count == 1);

  model.schema->name("test_schema");
  model.routineGroup->name("rg");
  MySQLRoutineGroupEditorBE rg(model.routineGroup);

  // Parse SQL without any routine definition. That should not affect the routine's existence.
  rg.set_sql(routine_sql);

  count = model.routineGroup->routines().count();
  ensure("Routine disappeard", count == 1);
}

TEST_FUNCTION(20) {
#ifndef NL
#define NL "\n"
#endif

  const char* routine_sql =
    "DELIMITER //" NL "CREATE FUNCTION get_count(less_than INT, greather_than INT) RETURNS INT" NL
    "    DETERMINISTIC" NL "    READS SQL DATA" NL "BEGIN" NL "       #OK, here some comment" NL
    "  DECLARE res INTEGER; #FEES PAID TO RENT THE VIDEOS INITIALLY" NL "  SELECT count(*) INTO res" NL "    FROM t1" NL
    "    WHERE id > less_than AND id < greather_than;" NL "  RETURN res;" NL "END //" NL
    "CREATE FUNCTION get_count1(less_than INT, greather_than INT) RETURNS INT" NL "    DETERMINISTIC" NL
    "    READS SQL DATA" NL "BEGIN" NL "       #OK, here some comment" NL
    "  DECLARE res INTEGER; #FEES PAID TO RENT THE VIDEOS INITIALLY" NL "  SELECT count(*) INTO res" NL "    FROM t1" NL
    "    WHERE id > less_than AND id < greather_than;" NL "  RETURN res;" NL "END //" NL
    "CREATE FUNCTION get_count2(less_than INT, greather_than INT) RETURNS INT" NL "    DETERMINISTIC" NL
    "    READS SQL DATA" NL "BEGIN" NL "       #OK, here some comment" NL
    "  DECLARE res INTEGER; #FEES PAID TO RENT THE VIDEOS INITIALLY" NL "  SELECT count(*) INTO res" NL "    FROM t1" NL
    "    WHERE id > less_than AND id < greather_than;" NL "  RETURN res;" NL "END //" NL "DELIMITER ;";

  SynteticMySQLModel model;
  model.schema->name("test_schema");
  model.routineGroup->name("rg");
  MySQLRoutineGroupEditorBE rg(model.routineGroup);

  // Note: use_sql is a special function only for tests like this. The normal access function
  //       set_sql() is interacting with the associated code editor. We don't have a working editor
  //       in unit tests, however.
  rg.use_sql(routine_sql);

  std::string names[] = {"get_count", "get_count1", "get_count2"};
  assure_equal(model.routineGroup->routines().count(), sizeof(names) / sizeof(names[0]));
  for (size_t i = 0, size = model.routineGroup->routines().count(); i < size; i++) {
    db_RoutineRef r = model.routineGroup->routines().get(i);
    std::string name = r->name();
    assure_equal(names[i], name);
  }

  // Read back the sql from the group to see how it changed.
  std::string processed_sql = rg.get_sql();

  std::vector<std::string> processed_routines = base::split(processed_sql, "\n\n");
  ensure_equals("Lines unintentionally removed", 5U, processed_routines.size());

  // Do the same steps from above again with the processed sql.
  // There shouldn't be any change.
  rg.use_sql(processed_sql);

  assure_equal(model.routineGroup->routines().count(), sizeof(names) / sizeof(names[0]));
  for (size_t i = 0, size = model.routineGroup->routines().count(); i < size; i++) {
    db_RoutineRef r = model.routineGroup->routines().get(i);
    std::string name = r->name();
    assure_equal(names[i], name);
  }

  std::string twice_processed_sql = rg.get_sql();
  std::vector<std::string> twice_processed_routines = base::split(twice_processed_sql, "\n\n");
  ensure_equals("Lines unintentionally removed", 5U, twice_processed_routines.size());

  // Now compares each routine to discard any difference
  for (size_t index = 0; index < processed_routines.size(); index++)
    ensure_equals("Routine unintentionally changed", processed_routines[index], twice_processed_routines[index]);
}

/**
 *	Same test as case 20, but this time with syntax errors.
 */
TEST_FUNCTION(30) {
  const char* routine_sql =
    "DELIMITER //" NL "CR!!! FUNCTION get_count(less_than INT, greather_than INT) RETURNS ..." NL "    DETERMINISTIC" NL
    "    READS SQL DATA" NL "BEGIN" NL "       #OK, here some comment" NL
    "  DECLARE res INTEGER; #FEES PAID TO RENT THE VIDEOS INITIALLY" NL "  SELECT count(*) INTO res" NL "    FROM t1" NL
    "    WHERE id > less_than AND id < greather_than;" NL "  RETURN res;" NL "END //" NL
    "CREATE FUNCTION get_count1(less_than INT, greather_than INT) RETURNS INT" NL "    DETERMINISTIC" NL
    "    READS SQL DATA" NL "BEGIN" NL "       #OK, here some comment" NL
    "  DECLARE res INTEGER; #FEES PAID TO RENT THE VIDEOS INITIALLY" NL "  SELECT count(*) INTO res" NL "    FROM t1" NL
    "    WHERE id > less_than AND id < greather_than;" NL "  RETURN res;" NL "END //" NL
    "CREATE FUNCTION get_count2(less_than INT, greather_than INT) RETURNS INT" NL "    DETERMINISTIC" NL
    "    READS SQL DATA" NL "-- BEGIN" NL "       #OK, here some comment" NL
    "  DECLARE res INTEGER; #FEES PAID TO RENT THE VIDEOS INITIALLY" NL "  SELECT count(*) INTO res" NL "    FROM t1" NL
    "    WHERE id > less_than AND id < greather_than;" NL "  RETURN res;" NL "END //" NL "DELIMITER ;";

  SynteticMySQLModel model;
  model.schema->name("test_schema");
  model.routineGroup->name("rg");
  MySQLRoutineGroupEditorBE rg(model.routineGroup);

  rg.use_sql(routine_sql);

  std::string names[] = {"rg_SYNTAX_ERROR_1", "get_count1", "get_count2_SYNTAX_ERROR"};
  assure_equal(model.routineGroup->routines().count(), sizeof(names) / sizeof(names[0]));

  size_t i = 0;
  for (db_RoutineRef routine : model.routineGroup->routines()) {
    std::string name = routine->name();
    assure_equal(name, names[i++]);
  }

  // Read back the sql from the group to see how it changed.
  std::string processed_sql = rg.get_sql();

  std::vector<std::string> processed_routines = base::split(processed_sql, "\n\n");
  ensure_equals("Lines unintentionally removed", 5U, processed_routines.size());

  // Do the same steps from above again with the processed sql.
  // There shouldn't be any change.
  rg.use_sql(processed_sql);

  assure_equal(model.routineGroup->routines().count(), sizeof(names) / sizeof(names[0]));
  i = 0;
  for (db_RoutineRef routine : model.routineGroup->routines()) {
    std::string name = routine->name();
    assure_equal(name, names[i++]);
  }

  std::string double_processed_sql = rg.get_sql();
  std::vector<std::string> double_processed_routines = base::split(double_processed_sql, "\n\n");
  ensure_equals("Lines unintentionally removed", 5U, double_processed_routines.size());

  // Now compares each routine to discard any difference
  for (size_t index = 0; index < processed_routines.size(); index++)
    ensure_equals("Routine unintentionally changed", processed_routines[index], double_processed_routines[index]);
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  delete wbt;
}

END_TESTS
