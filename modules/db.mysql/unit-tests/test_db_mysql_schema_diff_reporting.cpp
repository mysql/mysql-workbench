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

#include <pcre.h>

#include "testgrt.h"
#include "grt_test_utility.h"
#include "grts/structs.db.mysql.h"
#include "grts/structs.workbench.physical.h"
#include "grts/structs.workbench.model.reporting.h"

#include "db_rev_eng_be.h"

// #include <diffsqlgen.h>

#include "grt_manager.h"
#include "wb_helpers.h"

#include "diffchange.h"
#include "grtdiff.h"
#include "changeobjects.h"
#include "changelistobjects.h"
// #include "dbobjectmatch.h"

using namespace std;

// these functions are defined in grtdiff_alter_test.cpp
void populate_grt(GRT *grt, const char *);
// db_mysql_Catalog create_empty_catalog_for_import();
db_mysql_Catalog db_rev_eng_schema(bec::GRTManager *grtm, const std::list<std::string> &schema_names);

//=============================================================================
//
//=============================================================================
BEGIN_TEST_DATA_CLASS(module_db_mysql_schema_diff_reporting)
public:
END_TEST_DATA_CLASS

TEST_MODULE(module_db_mysql_schema_diff_reporting, "DB MySQL: schema reporting");

//=============================================================================
// Test cases type for TEST_FUNCTION(5)
//=============================================================================
struct TestCase {
  const char *name;
  const char *originalSQL;
  const char *modifiedSQL;
  const char *testExpression;
};

//-----------------------------------------------------------------------------
#if 0 // warning: unused variable 'testCases' [-Wunused-const-variable]
static const TestCase testCases[] =
{
  {
      "Drop test 1"
     ,"CREATE TABLE grtdiff_alter_test.to_be_dropped (id INT);"
     ,""
     ,"`to_be_dropped`\\s+was\\s+dropped"
  }
 ,{
      "Drop test 2"
     ,"CREATE TABLE grtdiff_alter_test.to_be_dropped (id INT);"
     ,""
     ,"`to_be_dropped`\\s+was\\s+dropped"
  }
 ,{
      "Alter engine"
     ,"CREATE TABLE grtdiff_alter_test.alter_table_engine (id INT) engine=myisam;"
     ,"CREATE TABLE grtdiff_alter_test.alter_table_engine (id INT) engine=innodb;"
     ,"alter_table_engine.*modified.*engine.*myisam.*innodb"
  }
 ,{
      "Alter table delay key write"
     ,"CREATE TABLE grtdiff_alter_test.alter_table_delay_key_write (id INT) delay_key_write=0;"
     ,""
     ,"alter_table_delay_key_write.*was dropped"
  }
 ,{
      "Create charset"
     ,"CREATE TABLE grtdiff_alter_test.alter_table_charset (id INT) default charset utf8;"
     ,"CREATE TABLE grtdiff_alter_test.alter_table_charset (id INT) default charset ascii;"
     ,"alter_table_charset.*was modified.*default character set.*utf8.*ascii"
  }
 ,{
      "Create collate"
     ,"CREATE TABLE grtdiff_alter_test.alter_table_collate (id INT) collate utf8_general_ci;"
     ,"CREATE TABLE grtdiff_alter_test.alter_table_collate (id INT) collate ascii_general_ci;"
     ,"alter_table_collate.*was modified.*default character set.*utf8.*ascii"
  }
 ,{
      "Create avg_row_length"
     ,"CREATE TABLE grtdiff_alter_test.alter_table_avg_row_length (id INT) avg_row_length=1;"
     ,"CREATE TABLE grtdiff_alter_test.alter_table_avg_row_length (id INT) avg_row_length=2;"
     ,"alter_table_avg_row_length.*was modified.*average row length: 1 --> 2"
  }
 ,{
      "Create table test 1"    
     ,""
     ,"CREATE TABLE grtdiff_alter_test.t1 (id INT);"
     ,"t1.? was created.*id of type INT"
  }
 ,{
     "Create table test 2"
    ,""
    ,"CREATE TABLE grtdiff_alter_test.t2 (id INT, id2 INT(11), val VARCHAR(255), t TEXT) ENGINE=MyISAM;"
    ,"t2.? was created.*id of type INT.*id2 of type INT.*val of type VARCHAR.*t of type TEXT"
  }
 ,{
     "Create table index test"
    ,""
    ,"CREATE TABLE grtdiff_alter_test.t3 (id INT, id2 INT(11), INDEX ix1 (id, id2 DESC));"
    ,"t3.? was created.*id of type INT.*id2 of type INT.*indices.*ix1 with columns:\\s+id,\\s+id2\\s+.?desc.?" // \(desc\)
  }
 ,{
    "Create table test foreign keys"
   ,""
   ,"CREATE TABLE grtdiff_alter_test.t3 (id INT, id2 INT(11), INDEX ix1 (id, id2 DESC)); CREATE TABLE grtdiff_alter_test.t4 (id INT, id2 INT(11), INDEX ix1 (id, id2 DESC), CONSTRAINT fk1 FOREIGN KEY(id, id2) REFERENCES t3(id, id2));"
   ,"t4.? was created.*id of type INT.*id2 of type INT.*indices.*ix1 with columns:\\s+id,\\s+id2\\s+.?desc.?.*foreign keys.*fk1 with columns.*id.*id2.*referred table.*t3.*id.*id2"
  }
 ,{
    "Create table test auto_increment and pass"
   ,"" 
   ,"CREATE TABLE grtdiff_alter_test.t1 (id int) auto_increment=3;"
   ,"next auto increment.*3"
  }
 ,{
   "Alter table test: column modified"
   ,"CREATE TABLE grtdiff_alter_test.t1 (id int);"
   ,"CREATE TABLE grtdiff_alter_test.t1 (id float);"
   ,"FIX ME"
  }
 ,{
   "Alter table test: columnt added, column deleted"
   ,"CREATE TABLE grtdiff_alter_test.t1 (id int);"
   ,"CREATE TABLE grtdiff_alter_test.t1 (id2 int);"
   ,"added column id2 of type INT.*removed column id"
  }
 ,{
   "Alter table test: index added"
   ,"CREATE TABLE grtdiff_alter_test.t1 (id int);"
   ,"CREATE TABLE grtdiff_alter_test.t1 (id int primary key);"
   ,"added index PRIMARY with columns: id"
  }
 ,{
   "Alter table test: index removed"
   ,"CREATE TABLE grtdiff_alter_test.t1 (id int primary key);"
   ,"CREATE TABLE grtdiff_alter_test.t1 (id int);"
   ,"removed index PRIMARY !FIELD NAME!"
  }
 ,{
   "Alter table test: added foreign key"
   ,"CREATE TABLE grtdiff_alter_test.t2 (id int); CREATE TABLE grtdiff_alter_test.t1 (id int);"
   ,"CREATE TABLE grtdiff_alter_test.t2 (id int); CREATE TABLE grtdiff_alter_test.t1 (id int references grtdiff_alter_test.t2(id));"
   ,"added foreign key  with columns: id, referred table: t2 with columns: id"
  }
 ,{
   "Alter table test: removed foreign key"
   ,"CREATE TABLE grtdiff_alter_test.t2 (id int); CREATE TABLE grtdiff_alter_test.t1 (id int references grtdiff_alter_test.t2(id));"
   ,"CREATE TABLE grtdiff_alter_test.t2 (id int); CREATE TABLE grtdiff_alter_test.t1 (id int);"
   ,"removed index KEY NAME"
  }
 ,{
   "Create table test: attributes auto_inrement, avg_row_length, delay_key_write"
   ,""
   ,"CREATE TABLE grtdiff_alter_test.t1(id int primary key) auto_increment=2 avg_row_length=8 delay_key_write=1;"
   ,"next auto increment.*2.*delay key writes.*1.*average row length.*8"
  }
 ,{
   "Create table test: attributes checksum, min_rows, max_rows, comment, pack_keys, row_format"
   ,""
   ,"CREATE TABLE grtdiff_alter_test.t1(id int primary key) checksum=1 comment='cmt' max_rows=5 min_rows=1 pack_keys=1 row_format=fixed;"
   ,"pack keys.*1.*checksum.*1.*row format.*fixed.*min rows.*1.*max rows.*5.*comment.*cmt"
  }
 ,{
   "Create table test: attributes insert_method"
   ,""
   ,"CREATE TABLE grtdiff_alter_test.t1(id int primary key) insert_method=last;"
   ,"merge insert method.*last"
  }
 ,{
   "Create table test: attributes"
   ,""
   ,"CREATE TABLE grtdiff_alter_test.t1(id int primary key) engine=myisam key_block_size=8 union=t2 data_directory=/var index_directory=/index connection='host 192.168.0.2';"
   ,"FIX ME"
  }
 ,{
    "Create trigger"
   ,""
   ,"create trigger grtdiff_alter_test.t1_trig after update on grtdiff_alter_test.t1 for each row insert into grtdiff_alter_test.t2 select max(grtdiff_alter_test.t1.id) from grtdiff_alter_test.t1;"
   ,"[Tt]rigger.*grtdiff_alter_test.*t1_trig.*created"
  }
 ,{
    "Drop trigger"
   ,"create trigger grtdiff_alter_test.t1_trig after update on grtdiff_alter_test.t1 for each row insert into grtdiff_alter_test.t2 select max(grtdiff_alter_test.t1.id) from grtdiff_alter_test.t1;"
   ,""
   ,"[Tt]rigger.*grtdiff_alter_test.*t1_trig.*dropped"
  }
 ,{
    "Create view"
   ,""
   ,"CREATE VIEW grtdiff_alter_test.t1v as select * from grtdiff_alter_test.t1;"
   ,"[Vv]iew.*t1v.*created"
  }
 ,{
    "Drop view"
   ,"CREATE VIEW grtdiff_alter_test.t1v as select * from grtdiff_alter_test.t1;"
   ,""
   ,"[Vv]iew.*t1v.*dropped"
  }
 ,{
    "Drop procedure"
   ,"CREATE PROCEDURE simpleproc (OUT param1 INT) SELECT COUNT(*) INTO param1 FROM t;"
   ,""
   ,"[Rr]outine.*simpleproc.*dropped"
  }
 ,{
    "Create procedure"
   ,""
   ,"CREATE PROCEDURE simpleproc (OUT param1 INT) SELECT COUNT(*) INTO param1 FROM t;"
   ,"[Rr]outine.*simpleproc.*created"
  }
 ,{
    "Drop user"
   ,"CREATE USER U1;"
   ,""
   ,"FIX ME"
  }
 ,{
    "Create user"
   ,""
   ,"CREATE USER U1;"
   ,"FIX ME"
  }
 ,{
    0,
    0,
    0,
    0
  }
};
#endif

#if 0

// TODO: to use function from db_helpers.
//------------------------------------------------------------------------------
static int pcre_compile_exec(const char *pattern, const char *str, int *patres, int patresnum)
{
  const char *errptr;
  int erroffs;
  int c;
  pcre *patre= pcre_compile(pattern, PCRE_DOTALL, &errptr, &erroffs, NULL);
  if (!patre)
    throw std::logic_error("error compiling regex "+std::string(errptr));

  c= pcre_exec(patre, NULL, str, strlen(str), 0, 0, patres, patresnum);
  pcre_free(patre);

  return c;
}

//------------------------------------------------------------------------------
TEST_FUNCTION(-4)
{
  int ovector[32];
  
  std::cout << pcre_compile_exec("", "test 1", ovector, sizeof(ovector) / sizeof(int));
}

//------------------------------------------------------------------------------
TEST_FUNCTION(5)
{
  // TODO: add test for table rename
  // NOTE: collate test doesn't produce the proper diff 
  grt::DbObjectMatchAlterOmf omf;

  populate_grt(grtm.get_grt(), NULL);

  DiffSQLGenInterfaceModule *diffsql_module= 
    static_cast<DiffSQLGenInterfaceModule *>(grtm.get_grt()->get_module("MySQLModuleDbMySQL"));
  ensure("DiffSQLGen module initialization", NULL != diffsql_module);

  Mysql_sql_parser mysql_sql_parser;

  std::vector<const TestCase*> testsFailed;

  for (  const TestCase *currentTest = testCases
        ; currentTest && currentTest->originalSQL
        ; ++currentTest 
      )
  {
    const char* org_sql   = currentTest->originalSQL;
    const char* mod_sql   = currentTest->modifiedSQL;
    const char* test_expr = currentTest->testExpression;

    db_mysql_Catalog org_cat= create_empty_catalog_for_import(grtm.get_grt());
    db_mysql_Catalog mod_cat= create_empty_catalog_for_import(grtm.get_grt());

    {
      std::string org_script;
      org_script
        .append("CREATE DATABASE IF NOT EXISTS grtdiff_alter_test /*!40100 DEFAULT CHARACTER SET latin1 */;\n")
        .append(org_sql);
      mysql_sql_parser.parseSqlScriptString(org_cat, org_script);
    }
    {
      std::string mod_script;
      mod_script
        .append("CREATE DATABASE IF NOT EXISTS grtdiff_alter_test /*!40100 DEFAULT CHARACTER SET latin1 */;\n")
        .append(mod_sql);
      mysql_sql_parser.parseSqlScriptString(mod_cat, mod_script);
    }


    DiffChange* alter_change= diff_make(org_cat, mod_cat, &omf);
    //alter_change->dump_log(0);

    // 1. generate alter
    grt::DictValue options(grtm.get_grt());
    options.set("UseFilteredLists", grt::IntValue(0));
    options.set("TemplateFile", 
      grt::StringValue("../../modules/db.mysql/res/reporting/Basic_Text.tpl/basic_text_report.txt.tpl"));

    char buf1[128];
    sprintf(buf1, "%p", alter_change);
    grt::StringValue out_str(
      diffsql_module->generateReport(org_cat, options, std::string(buf1)));
    
    std::cout << std::endl << std::endl;
    std::cout << (currentTest ? currentTest->name : "<unnamed>") << std::endl << out_str.c_str() << std::endl;

    int ovector[32];
    const int checkResult = pcre_compile_exec(test_expr
                                             ,out_str.c_str()
                                             ,ovector
                                             ,sizeof(ovector) / sizeof(int)
                                             );
    
    //std::cout << "CHECK (" << test_expr << "): '" << checkResult << "'\n";
    
    if ( checkResult != 1 )
    {
      if ( currentTest )
      {
         testsFailed.push_back(currentTest);
         std::cout << "Test '" << currentTest->name << "' failed. code=" << checkResult << std::endl;
      }
    }
    else
      std::cout << "Test '" << (currentTest ? currentTest->name : "") << "' passed." << std::endl;
    //ensure("report test check", checkResult == 1);
  } //for (TestCase *test = testCases; test && test->originalSQL; ++test )
  
  std::cout << std::endl << std::endl << "======= Summary =======" << std::endl;
  if ( testsFailed.size() > 0 )
  {
    std::cout << "There were failed tests\n";
    struct Print
    {
      void operator()(const TestCase* tc)
      {
        std::cout << "Test failed: " << tc->name << std::endl;
      }
    };
    std::for_each(testsFailed.begin(), testsFailed.end(), Print());
    //TODO: print failed tests report
  }
  
  ensure("Diff report tests", testsFailed.size() == 0);
}

#endif

END_TESTS
