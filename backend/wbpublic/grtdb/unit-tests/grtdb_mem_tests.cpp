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

#include <stdio.h>
#include "grtpp.h"

#include "grtdb/editor_table.h"
#include "grtdb/db_object_helpers.h"
#include "wb_helpers.h"

using namespace grt;
using namespace bec;
using namespace std;

BEGIN_TEST_DATA_CLASS(bedb_mem_tests)
public:
  db_mgmt_RdbmsRef rdbms;
  GRTManager grtm;

TEST_DATA_CONSTRUCTOR(bedb_mem_tests)
  : grtm(false)
{
}
END_TEST_DATA_CLASS


TEST_MODULE(bedb_mem_tests, "DB stuff memory tests");


TEST_FUNCTION(10)
{
  // make sure everything is registered
  register_structs_app_xml();
  
  int i= grtm.get_grt()->scan_metaclasses_in("../../res/grt/");
  ensure("load structs", i>0);
  
  grtm.get_grt()->end_loading_metaclasses();

  // load datatype groups so that it can be found during load of types
  grtm.get_grt()->set_root(grtm.get_grt()->unserialize("../../res/grtdata/db_datatype_groups.xml"));


  rdbms= db_mgmt_RdbmsRef::cast_from(grtm.get_grt()->unserialize("../../modules/db.mysql/res/mysql_rdbms_info.xml"));

  ensure("rdbms", rdbms.is_valid());
}

// XXX: what does this test case actually test?
TEST_FUNCTION(20)
{
  // test primary key
  enum {N=100};
  char buf[64];
  db_mysql_SchemaRef scm(grtm.get_grt());
  WBTester tester;
  db_mysql_TableRef prev_table(grtm.get_grt());

  for ( int i = 0; i < N; i++ )
  {
    db_mysql_TableRef table(grtm.get_grt());
    sprintf(buf, "Table_%i", i);
    table->name("tbl");

    enum {NCOLS = 8};
    for ( int j = 0; j < 8; j++ )
    { 
      db_mysql_ColumnRef column(grtm.get_grt());

      sprintf(buf, "col_%i_%i", i, j);
      column->name(buf);
      column->owner(table);

      column->simpleType(rdbms->simpleDatatypes().get(3));

      table->columns().ginsert(column);
    }

    sprintf(buf, "col_%i_%i", i, NCOLS+1);
    db_mysql_ColumnRef column(grtm.get_grt());
    column->name(buf);
    column->owner(table);
    table->columns().ginsert(column);

    column->autoIncrement(1);
    table->addPrimaryKeyColumn(column);

    table->owner(scm);
    scm->tables().insert(table);    

    if ( prev_table.is_valid() )
    {
      try {
        bec::TableHelper::create_foreign_key_to_table(table, prev_table, true, true, true, true,
          rdbms, grt::DictRef(tester.grt), grt::DictRef(tester.grt));
      }
      catch (...)
      {}
    }
    //fprintf(stderr, ".");
    prev_table = table;
  }
}

END_TESTS


