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

#include "mysql_routinegroup_editor.h"
#include "model_mockup.h"


using namespace grt;
using namespace bec;
using namespace casmine;

namespace {

$ModuleEnvironment() {};

$TestData {
  std::unique_ptr<WorkbenchTester> tester;
};

$describe("MySQL Routine Group Editor") {

  $beforeAll([this]() {
    data->tester.reset(new WorkbenchTester());
  });

  $it("Editor with no routines", []() {
    const char* routine_sql = "";

    SyntheticMySQLModel model;
    size_t count = model.routineGroup->routines().count();
    $expect(count).toEqual(1U, "Invalid number of routines");

    model.schema->name("test_schema");
    model.routineGroup->name("rg");
    MySQLRoutineGroupEditorBE rg(model.routineGroup);

    // Parse SQL without any routine definition. That should not affect the routine's existence.
    rg.set_sql(routine_sql);

    count = model.routineGroup->routines().count();
    $expect(count).toEqual(1U, "Routine disappeard");
  });

  $it("Editor with routines", []() {
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

    SyntheticMySQLModel model;
    model.schema->name("test_schema");
    model.routineGroup->name("rg");
    MySQLRoutineGroupEditorBE rg(model.routineGroup);

    // Note: use_sql is a special function only for tests like this. The normal access function
    //       set_sql() is interacting with the associated code editor. We don't have a working editor
    //       in unit tests, however.
    rg.use_sql(routine_sql);

    std::vector<std::string> names = { "get_count", "get_count1", "get_count2" };
    $expect(model.routineGroup->routines().count()).toEqual(names.size());
    for (size_t i = 0, size = model.routineGroup->routines().count(); i < size; i++) {
      db_RoutineRef r = model.routineGroup->routines().get(i);
      std::string name = r->name();
      $expect(name).toEqual(names[i]);
    }

    // Read back the sql from the group to see how it changed.
    std::string processed_sql = rg.get_sql();

    std::vector<std::string> processed_routines = base::split(processed_sql, "\n\n");
    $expect(processed_routines.size()).toEqual(5U, "Lines unintentionally removed");

    // Do the same steps from above again with the processed sql.
    // There shouldn't be any change.
    rg.use_sql(processed_sql);

    $expect(model.routineGroup->routines().count()).toEqual(names.size());
    for (size_t i = 0, size = model.routineGroup->routines().count(); i < size; i++) {
      db_RoutineRef r = model.routineGroup->routines().get(i);
      std::string name = r->name();
      $expect(name).toEqual(names[i]);
    }

    std::string twice_processed_sql = rg.get_sql();
    std::vector<std::string> twice_processed_routines = base::split(twice_processed_sql, "\n\n");
    $expect(twice_processed_routines.size()).toEqual(5U, "Lines unintentionally removed");

    // Now compares each routine to discard any difference
    for (size_t index = 0; index < processed_routines.size(); index++) {
      $expect(processed_routines[index]).toEqual(twice_processed_routines[index], "Routine unintentionally changed");
    }
  });

  /**
   *	Same test as case 20, but this time with syntax errors.
   */
  $it("Editor with invalid routines", []() {
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

    SyntheticMySQLModel model;
    model.schema->name("test_schema");
    model.routineGroup->name("rg");
    MySQLRoutineGroupEditorBE rg(model.routineGroup);

    rg.use_sql(routine_sql);

    std::vector<std::string> names = { "rg_SYNTAX_ERROR_1", "get_count1", "get_count2_SYNTAX_ERROR" };
    $expect(model.routineGroup->routines().count()).toEqual(names.size());

    size_t i = 0;
    for (db_RoutineRef routine : model.routineGroup->routines()) {
      std::string name = routine->name();
      $expect(name).toEqual(names[i++]);
    }

    // Read back the sql from the group to see how it changed.
    std::string processed_sql = rg.get_sql();

    std::vector<std::string> processed_routines = base::split(processed_sql, "\n\n");
    $expect(processed_routines.size()).toEqual(5U, "Lines unintentionally removed");

    // Do the same steps from above again with the processed sql.
    // There shouldn't be any change.
    rg.use_sql(processed_sql);

    i = 0;
    $expect(model.routineGroup->routines().count()).toEqual(names.size());
    for (db_RoutineRef routine : model.routineGroup->routines()) {
      std::string name = routine->name();
      $expect(name).toEqual(names[i++]);
    }

    std::string double_processed_sql = rg.get_sql();
    std::vector<std::string> double_processed_routines = base::split(double_processed_sql, "\n\n");
    $expect(double_processed_routines.size()).toEqual(5U, "Lines unintentionally removed");

    // Now compares each routine to discard any difference
    for (size_t index = 0; index < processed_routines.size(); index++) {
      $expect(processed_routines[index]).toEqual(double_processed_routines[index], "Routine unintentionally changed");
    }
  });
}

}
