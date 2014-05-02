/* 
 * Copyright (c) 2011, 2012, Oracle and/or its affiliates. All rights reserved.
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

#include "tut_stdafx.h"

#include "grtpp.h"
#include "../../plugins/db.mysql.editors/backend/mysql_routinegroup_editor.h"
#include "synthetic_mysql_model.h"
#include "wb_helpers.h"
#include <vector>
#include "stub/stub_mforms.h"
#include "../../library/base/base/string_utilities.h"

using namespace grt;
using namespace bec;
using namespace tut;

BEGIN_TEST_DATA_CLASS(mysql_routinegroup_editor_test)
public:
  WBTester wbt;
  //GRTManagerTest grtm;
  GRT* grt;
  db_mgmt_RdbmsRef rdbms;

TEST_DATA_CONSTRUCTOR(mysql_routinegroup_editor_test)
//  :grtm(false)
{
  grt = wbt.grt;
 // std::string path = "../../Bin/Debug";
 // grtm.set_search_paths(path, path, path);
 // grtm.initialize(path);

 // grt= grtm.get_grt();
//  grt->scan_metaclasses_in("../../res/grt/");
//  grt->end_loading_metaclasses();
}

END_TEST_DATA_CLASS


TEST_MODULE(mysql_routinegroup_editor_test, "mysql_routinegroup_editor_test");

TEST_FUNCTION(1) 
{
const char* routine_sql= 
  "DELIMITER //"
  "\n\n-- --------------------------------\n";

  SynteticMySQLModel model(grt);
  int count= model.routineGroup->routines().count();
  ensure("Invalid number of routines", count == 1);

  model.schema->name("test_schema");
  model.routineGroup->name("rg");
  MySQLRoutineGroupEditorBE rg(wbt.wb->get_grt_manager(), model.routineGroup, model.model->rdbms());

  // Parse SQL without any routine definition.
  rg.parse_sql(grt, routine_sql);

  count= model.routineGroup->routines().count();
  ensure("Previous routine definition still there", count == 0);
}


TEST_FUNCTION(20) 
{
#ifndef NL
#define NL "\n"
#endif

const char* routine_sql= 
"DELIMITER //"NL
"CREATE FUNCTION get_count(less_than INT, greather_than INT) RETURNS INT"NL
"    DETERMINISTIC"NL
"    READS SQL DATA"NL
"BEGIN"NL
"       #OK, here some comment"NL
"  DECLARE res INTEGER; #FEES PAID TO RENT THE VIDEOS INITIALLY"NL
"  SELECT count(*) INTO res"NL
"    FROM t1"NL
"    WHERE id > less_than AND id < greather_than;"NL
"  RETURN res;"NL
"END //"NL
"CREATE FUNCTION get_count1(less_than INT, greather_than INT) RETURNS INT"NL
"    DETERMINISTIC"NL
"    READS SQL DATA"NL
"BEGIN"NL
"       #OK, here some comment"NL
"  DECLARE res INTEGER; #FEES PAID TO RENT THE VIDEOS INITIALLY"NL
"  SELECT count(*) INTO res"NL
"    FROM t1"NL
"    WHERE id > less_than AND id < greather_than;"NL
"  RETURN res;"NL
"END //"NL
"CREATE FUNCTION get_count2(less_than INT, greather_than INT) RETURNS INT"NL
"    DETERMINISTIC"NL
"    READS SQL DATA"NL
"BEGIN"NL
"       #OK, here some comment"NL
"  DECLARE res INTEGER; #FEES PAID TO RENT THE VIDEOS INITIALLY"NL
"  SELECT count(*) INTO res"NL
"    FROM t1"NL
"    WHERE id > less_than AND id < greather_than;"NL
"  RETURN res;"NL
"END //"NL
"DELIMITER ;";

  SynteticMySQLModel model(grt);
  model.schema->name("test_schema");
  model.routineGroup->name("rg");
  MySQLRoutineGroupEditorBE rg(wbt.wb->get_grt_manager(), model.routineGroup, model.model->rdbms());

  rg.parse_sql(grt, routine_sql);

  std::string names[]= {"get_count", "get_count1", "get_count2"};
  assure_equal(model.routineGroup->routines().count(), sizeof(names)/sizeof(names[0]));
  for (size_t i= 0, size= model.routineGroup->routines().count(); i < size; i++)
  {
    db_RoutineRef r= model.routineGroup->routines().get(i);
    std::string name= r->name();
    assure_equal(names[i], name);
  }

  // it would be wise requirement that it stays unchanged in case nothing changed in build sql
  // otherwise it will grow with every processing
  std::string processed_sql= rg.get_routines_sql();

  std::vector<std::string> processed_routines = base::split(processed_sql, "\n\n");

  // Gets rid of the header
  processed_routines.erase(processed_routines.begin());

  ensure("New line insertion failed", 3 == processed_routines.size());


  rg.parse_sql(grt, processed_sql);

  int routines_count= model.routineGroup->routines().count();

  assure_equal(routines_count, sizeof(names) / sizeof(names[0]));
  for (size_t i= 0, size= model.routineGroup->routines().count(); i < size; i++)
  {
    db_RoutineRef r= model.routineGroup->routines().get(i);
    std::string name= r->name();
    assure_equal(names[i], name);
  }

  std::string twice_processed_sql= rg.get_routines_sql();

  std::vector<std::string> twice_processed_routines = base::split(twice_processed_sql, "\n\n");
  
  twice_processed_routines.erase(twice_processed_routines.begin());

  ensure("New line insertion failed", 3 == twice_processed_routines.size());

  // Now compares each routine to discard any difference
  for(size_t index = 0; index < processed_routines.size(); index++)
  {
    ensure("Processed routine is not stable", processed_routines[index] == twice_processed_routines[index]);
  }

}

TEST_FUNCTION(21) 
{
const char* routine_sql= 
"DELIMITER //"NL
"CR!!! FUNCTION get_count(less_than INT, greather_than INT) RETURNS INT"NL
"    DETERMINISTIC"NL
"    READS SQL DATA"NL
"BEGIN"NL
"       #OK, here some comment"NL
"  DECLARE res INTEGER; #FEES PAID TO RENT THE VIDEOS INITIALLY"NL
"  SELECT count(*) INTO res"NL
"    FROM t1"NL
"    WHERE id > less_than AND id < greather_than;"NL
"  RETURN res;"NL
"END //"NL
"CREATE FUNCTION get_count1(less_than INT, greather_than INT) RETURNS INT"NL
"    DETERMINISTIC"NL
"    READS SQL DATA"NL
"BEGIN"NL
"       #OK, here some comment"NL
"  DECLARE res INTEGER; #FEES PAID TO RENT THE VIDEOS INITIALLY"NL
"  SELECT count(*) INTO res"NL
"    FROM t1"NL
"    WHERE id > less_than AND id < greather_than;"NL
"  RETURN res;"NL
"END //"NL
"CREATE FUNCTION get_count2(less_than INT, greather_than INT) RETURNS INT"NL
"    DETERMINISTIC"NL
"    READS SQL DATA"NL
"-- BEGIN"NL
"       #OK, here some comment"NL
"  DECLARE res INTEGER; #FEES PAID TO RENT THE VIDEOS INITIALLY"NL
"  SELECT count(*) INTO res"NL
"    FROM t1"NL
"    WHERE id > less_than AND id < greather_than;"NL
"  RETURN res;"NL
"END //"NL
"DELIMITER ;";

  SynteticMySQLModel model(grt);
  model.schema->name("test_schema");
  model.routineGroup->name("rg");
  MySQLRoutineGroupEditorBE rg(wbt.wb->get_grt_manager(), model.routineGroup, model.model->rdbms());

  rg.parse_sql(grt, routine_sql);

  std::string names[]= {"rg_SYNTAX_ERROR_1", "get_count1", "rg_SYNTAX_ERROR_2"};
  assure_equal(model.routineGroup->routines().count(), sizeof(names)/sizeof(names[0]));
  for (size_t i= 0, size= model.routineGroup->routines().count(); i < size; i++)
  {
    db_RoutineRef r= model.routineGroup->routines().get(i);
    std::string name= r->name();
    assure_equal(names[i], name);
  }

  // it would be wise requirement that it stays unchanged in case nothing changed in build sql
  // otherwise it will grow with every processing
  std::string processed_sql= rg.get_routines_sql();

  std::vector<std::string> processed_routines = base::split(processed_sql, "\n\n");

  processed_routines.erase(processed_routines.begin());

  ensure("New line insertion failed", 3 == processed_routines.size());


  rg.parse_sql(grt, processed_sql);

  int routines_count= model.routineGroup->routines().count();

  assure_equal(routines_count, sizeof(names)/sizeof(names[0]));
  for (size_t i= 0, size= model.routineGroup->routines().count(); i < size; i++)
  {
    db_RoutineRef r= model.routineGroup->routines().get(i);
    std::string name= r->name();
    assure_equal(names[i], name);
  }

  std::string twice_processed_sql= rg.get_routines_sql();
  
  std::vector<std::string> twice_processed_routines = base::split(twice_processed_sql, "\n\n");

  twice_processed_routines.erase(twice_processed_routines.begin());

  ensure("New line insertion failed", 3 == twice_processed_routines.size());


  // Now compares each routine to discard any difference
  for(size_t index = 0; index < processed_routines.size(); index++)
  {
    ensure("Processed routine is not stable", processed_routines[index] == twice_processed_routines[index]);
  }
}

END_TESTS
