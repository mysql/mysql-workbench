/* 
 * Copyright (c) 2011, 2014, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _WIN32
#include <sstream>
#include <fstream>
#endif

#include "testgrt.h"
#include "grt_test_utility.h"
#include "grts/structs.db.mgmt.h"
#include "grts/structs.db.mysql.h"
#include "grts/structs.workbench.physical.h"
#include "wb_mysql_import.h"
#include "grt/grt_manager.h"
#include "wb_mysql_import.h"
#include "wb_helpers.h"

using namespace std;


BEGIN_TEST_DATA_CLASS(wb_mysql_import_dbd4)
public:
  GRTManagerTest grtm;
  WbMysqlImportImpl *module;
  db_mgmt_RdbmsRef rdbms;
  DictRef options;
  GRT *grt;
  workbench_physical_ModelRef test_import_dbd4(int test_no);
END_TEST_DATA_CLASS


TEST_MODULE(wb_mysql_import_dbd4, "WB module: import from DBD4");


TEST_FUNCTION(1)
{
  grt= grtm.get_grt();

  module= grt->get_native_module<WbMysqlImportImpl>();
  ensure("WbMysqlImport module initialization", NULL != module);

  options= DictRef(grt);
  options.set("gen_fk_names_when_empty", IntegerRef(0));
}


#if 0
workbench_physical_ModelRef Test_object_base<wb_mysql_import_dbd4>::test_import_dbd4(int test_no)
{
  ensure("WbMysqlImport module initialization", NULL != module);

  static const char* TEST_DATA_DIR= "data/modules_grt/wb_mysql_import/dbd4/";

  /* set filenames & messages based on test no. */
  ostringstream oss;
  oss << test_no;
  string test_message=              "Model (" + oss.str() + ")";
  string test_model_filename=       TEST_DATA_DIR + oss.str() + ".xml";
  string test_model_state_filename= TEST_DATA_DIR + oss.str() + "_test.xml";
  string model_state_filename=      TEST_DATA_DIR + oss.str() + "_res.xml";

  /* import model */
  WBTester wbt;
  wbt.create_new_document();
  wbt.flush_until(2);

  ensure_equals("loaded phys model count",
                wbt.wb->get_document()->physicalModels().count(), 1U);

  workbench_physical_ModelRef res_model= wbt.wb->get_document()->physicalModels().get(0);
  module->importDBD4Ex(res_model, test_model_filename, options);

  db_ColumnRef column(res_model->catalog()->schemata().get(0)->tables().get(0)->columns().get(0));

  /* serialization */
  grt->serialize(res_model, model_state_filename);

  /* unserialization */
  res_model= workbench_physical_ModelRef::cast_from(grt->unserialize(model_state_filename));
  workbench_physical_ModelRef test_model= workbench_physical_ModelRef::cast_from(grt->unserialize(test_model_state_filename));

  /* comparison */
  grt_ensure_equals(test_message.c_str(), res_model, test_model);

  return test_model;
}


TEST_FUNCTION(10)
{ 
  test_import_dbd4(0); 
}

TEST_FUNCTION(11)
{
  test_import_dbd4(1); 
}

TEST_FUNCTION(12)
{
  test_import_dbd4(2);
}

TEST_FUNCTION(13)
{
  // this model contains a table with an accented column. should be in UTF8 after imported
  workbench_physical_ModelRef model= test_import_dbd4(3);
  
  ensure("model imported", model.is_valid());

  db_ColumnRef column(model->catalog()->schemata().get(0)->tables().get(0)->columns().get(0));

  ensure("imported column name", g_utf8_validate(column->name().c_str(), strlen(column->name().c_str()), NULL) != 0);

  ensure_equals("imported column name", *column->name(), "reuni\xc3\xb3n");
}
#endif

END_TESTS
