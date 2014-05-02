/* 
 * Copyright (c) 2011, 2013 Oracle and/or its affiliates. All rights reserved.
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

// test model file 

#include "tut_stdafx.h"

#include "workbench/wb_model_file.h"

#include "wb_helpers.h"
#include "grt_test_utility.h"

#include "base/file_utilities.h"

#ifdef _WIN32
#define TMP_DIR "temp"
#else
#define TMP_DIR "/tmp"
#endif

using namespace wb;



BEGIN_TEST_DATA_CLASS(wb_model_file)
public:
  WBTester tester;
END_TEST_DATA_CLASS;





TEST_MODULE(wb_model_file, "tests for WB model file");


TEST_FUNCTION(1)
{
  bec::GRTManager *grtm;
  grt::GRT *grt= tester.wb->get_grt();

#ifdef _WIN32
  base::create_directory(TMP_DIR, 0666);
#endif
  grtm= bec::GRTManager::get_instance_for(grt);
  std::string tmpDir = TMP_DIR;

  {
    ModelFile mf(tmpDir);

    workbench_DocumentRef doc(grt);

    // create a test file, change it and then save_as
    
    mf.create(grtm);

    doc->name("t1");

    workbench_physical_ModelRef pmodel(grt);
    pmodel->owner(doc);
    db_Catalog catalog(grt);
    pmodel->catalog(&catalog);
    doc->physicalModels().insert(pmodel);

    mf.store_document(grt, doc);
    mf.save_to("t1.mwb");

    doc->name("t2");
    mf.store_document(grt, doc);
    mf.save_to("t2.mwb");
  }

  {
    ModelFile mf1(tmpDir);
    ModelFile mf2(tmpDir);

    mf1.open("t1.mwb", grtm);
    mf2.open("t2.mwb", grtm);

    workbench_DocumentRef d1, d2;

    d1= mf1.retrieve_document(grt);
    d2= mf2.retrieve_document(grt);

    ensure_equals("document 1 content", *d1->name(), "t1");
    ensure_equals("document 2 content", *d2->name(), "t2");
  }
}


TEST_FUNCTION(2)
{
  //WBTester tester;
  bec::GRTManager *grtm;
  grt::GRT *grt= tester.wb->get_grt();
  grtm= bec::GRTManager::get_instance_for(grt);

  // load sakile a bunch of times
  std::string tmpDir = TMP_DIR;

  ModelFile m(tmpDir);

  m.open("data/workbench/sakila.mwb", grtm);

  try
  {
    m.open("data/workbench/sakila.mwb", grtm);
    fail("open model file locking didnt work");
  }
  catch (...)
  {
    ensure("locking works", true);
  }
}

TEST_FUNCTION(3)
{
 // WBTester wb;
  tester.wb->new_document();
  tester.wb->open_document("data/workbench/sakila_full.mwb");
  tester.wb->close_document();
}


TEST_FUNCTION(4)
{
  std::string tmpDir = TMP_DIR;
  ModelFile mf(tmpDir);
  std::string comment = mf.read_comment("data/workbench/empty_file.sql");
  ensure("4.1 Comment is empty, ", comment.empty());
  comment = mf.read_comment("data/workbench/empty_model_with_comment.mwb");
  ensure("4.2 Comment is mydb, ", comment == "mydb");
}


END_TESTS
