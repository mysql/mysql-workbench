/*
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates. All rights reserved.
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

// Test if opened model can be saved (we ran into an issue with libzip that
// didn't close the file properly, resulting in inability to save the model)
TEST_FUNCTION(10)
{
  grt::GRT*        grt  = tester.wb->get_grt();
  bec::GRTManager* grtm = bec::GRTManager::get_instance_for(grt);

  base::create_directory(TMP_DIR, 0666);
  std::string temp_dir = TMP_DIR;

  // read the file - the file should be properly closed after reading
  ModelFile mf(temp_dir);
  try
  {
    mf.open("data/workbench/test_model1.mwb", grtm);
  }
  catch (...)
  {
    fail("opening model failed");
  }

  // try to write to the file - if the file wasn't closed, it will fail
  try
  {
    mf.save_to("data/workbench/test_model1.mwb");
  }
  catch (...)
  {
    fail("saving file failed - perhaps the file is locked?");
  }
}

std::string test_loading_and_saving_a_model( const WBTester& tester, std::string& base_path )
{
  grt::GRT *grt= tester.wb->get_grt();
  bec::GRTManager* grtm = bec::GRTManager::get_instance_for(grt);

  #ifdef _WIN32
    base::create_directory(TMP_DIR, 0666);
  #endif
  std::string temp_dir = TMP_DIR;
  ModelFile mf(temp_dir);

  std::string src_path = base_path +     ".mwb";
  std::string dst_path = base_path + "_tmp.mwb";

  // remove copy file if it was left over from a previous unsuccessful run
  base::remove(dst_path);

  // open the file
  try
  {
    mf.open(src_path, grtm);
  }
  catch (...)
  {
    return "opening model failed";
  }

  // write a copy
  try
  {
    mf.save_to(dst_path);
    mf.cleanup();
  }
  catch (...)
  {
     return "saving model copy failed";
  }

  // verify the copy was created
  try
  {
    mf.open(dst_path, grtm);
    mf.cleanup();
  }
  catch (...)
  {
    // if the copy exists but somehow failed to open, it will be left on disk on purpose.
    // This is so a developer can see if the copy was created.  It will be erased on subsuquent runs (via base::remove() above)

    return "opening model copy failed - was the file created?";
  }

  // delete the copy
  try
  {
    if (!base::remove(dst_path))
    {
      return "model copy was not created";
    }
  }
  catch (base::file_error)
  {
    return "removing model copy failed";
  }

  // all went ok
  return "";
}

// Test model loading and saving with ASCII path + ASCII file
TEST_FUNCTION(15)
{
  std::string base_path = "data/workbench/test_model1";

  std::string result = test_loading_and_saving_a_model( tester, base_path );
  if ( !result.empty() )
    fail(result);
}

// Test model loading and saving with ASCII path + Unicode file
TEST_FUNCTION(16)
{
  std::string base_path = "data/workbench/file_with_unicode_\xC4\x85\xC4\x87\xC4\x99";

  std::string result = test_loading_and_saving_a_model(tester, base_path);
  if (!result.empty())
    fail(result);
}

// Test model loading and saving with Unicode path + ASCII file
TEST_FUNCTION(17)
{
  std::string base_path = "data/workbench/path_with_unicode_\xC4\x85\xC4\x87\xC4\x99/test_model1";

  std::string result = test_loading_and_saving_a_model(tester, base_path);
  if (!result.empty())
    fail(result);
}

// Test model loading and saving with Unicode path + Unicode file
TEST_FUNCTION(18)
{
  std::string base_path = "data/workbench/path_with_unicode_\xC4\x85\xC4\x87\xC4\x99/file_with_unicode_\xC4\x85\xC4\x87\xC4\x99";

  std::string result = test_loading_and_saving_a_model(tester, base_path);
  if (!result.empty())
    fail(result);
}

END_TESTS

