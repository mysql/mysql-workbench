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

// test model file

#include "workbench/wb_model_file.h"

#include "grt_test_utility.h"
#include "wb_helpers.h"

#include "base/file_utilities.h"
#include "base/utf8string.h"

#ifdef _WIN32
#define TMP_DIR "temp"
#else
#define TMP_DIR "/tmp"
#endif

using namespace wb;

BEGIN_TEST_DATA_CLASS(wb_model_file)
public:
WBTester *tester;
TEST_DATA_CONSTRUCTOR(wb_model_file) { tester = new WBTester; }
END_TEST_DATA_CLASS;

TEST_MODULE(wb_model_file, "tests for WB model file");

// TODO we need a test to compare between load / save / duplicate / save / load

void copyFile(const base::utf8string &filename1,
              const base::utf8string &filename2) {
#ifdef _WIN32
  std::wifstream file1(base::string_to_wstring(filename1), std::ios::binary);
  std::wofstream file2(base::string_to_wstring(filename2), std::ios::binary);
#else
  std::ifstream file1(filename1, std::ios::binary);
  std::ofstream file2(filename2, std::ios::binary);
#endif

  file2 << file1.rdbuf();
}

const base::utf8string BaseModelFile = "data/workbench/test_model1_temp.mwb";
const base::utf8string UnicodeBaseModelFile =
    "data/workbench/file_with_unicode_\xC4\x85\xC4\x87\xC4\x99.mwb";
const base::utf8string UnicodeDirectory =
    "data/workbench/path_with_unicode_\xC4\x85\xC4\x87\xC4\x99";
const base::utf8string UnicodeDiractoryFile1 =
    "data/workbench/path_with_unicode_\xC4\x85\xC4\x87\xC4\x99/test_model1.mwb";
const base::utf8string UnicodeDiractoryFile2 =
    "data/workbench/path_with_unicode_\xC4\x85\xC4\x87\xC4\x99/"
    "file_with_unicode_\xC4\x85\xC4\x87\xC4\x99.mwb";

TEST_FUNCTION(1) {

  {
    // Copy file with unicode
    if (file_exists(UnicodeBaseModelFile)) {
      base::remove(UnicodeBaseModelFile);
    }

    copyFile("data/workbench/file_with_unicode.mwb", UnicodeBaseModelFile);
  }

  {
    // Create the unicode directory
    if (is_directory(UnicodeDirectory)) {
      base::remove_recursive(UnicodeDirectory);
    }

    base::create_directory(UnicodeDirectory, 0777);

    copyFile("data/workbench/path_with_unicode/test_model1.mwb",
             UnicodeDiractoryFile1);
    copyFile("data/workbench/path_with_unicode/file_with_unicode.mwb",
             UnicodeDiractoryFile2);
  }

  {
    // Copy model so that we don't bother source control
    base::utf8string filename = "data/workbench/test_model1.mwb";

    if (base::file_exists(BaseModelFile)) {
      base::remove(BaseModelFile);
    }

    copyFile(filename, BaseModelFile);
  }

#ifdef _WIN32
  base::create_directory(TMP_DIR, 0666);
#endif
}

TEST_FUNCTION(10) {
  std::string tmpDir = TMP_DIR;

  {
    ModelFile mf(tmpDir);

    workbench_DocumentRef doc(grt::Initialized);

    // create a test file, change it and then save_as

    mf.create();

    doc->name("t1");

    workbench_physical_ModelRef pmodel(grt::Initialized);
    pmodel->owner(doc);
    db_Catalog catalog;
    pmodel->catalog(&catalog);
    doc->physicalModels().insert(pmodel);

    mf.store_document(doc);
    mf.save_to("t1.mwb");

    doc->name("t2");
    mf.store_document(doc);
    mf.save_to("t2.mwb");
  }

  {
    ModelFile mf1(tmpDir);
    ModelFile mf2(tmpDir);

    mf1.open("t1.mwb");
    mf2.open("t2.mwb");

    workbench_DocumentRef d1, d2;

    d1 = mf1.retrieve_document();
    d2 = mf2.retrieve_document();

    ensure_equals("document 1 content", *d1->name(), "t1");
    ensure_equals("document 2 content", *d2->name(), "t2");
  }
}

TEST_FUNCTION(11) {
  // WBTester tester;
  // load sakile a bunch of times
  std::string tmpDir = TMP_DIR;

  ModelFile m(tmpDir);

  m.open("data/workbench/sakila.mwb");

  try {
    m.open("data/workbench/sakila.mwb");
    fail("open model file locking didnt work");
  } catch (...) {
    ensure("locking works", true);
  }
}

TEST_FUNCTION(12) {
  tester->wb->new_document();
  tester->wb->open_document("data/workbench/sakila_full.mwb");
  tester->wb->close_document();
}

TEST_FUNCTION(13) {
  std::string tmpDir = TMP_DIR;
  ModelFile mf(tmpDir);
  std::string comment = mf.read_comment("data/workbench/empty_file.sql");
  ensure("4.1 Comment is empty, ", comment.empty());
  comment = mf.read_comment("data/workbench/empty_model_with_comment.mwb");
  ensure("4.2 Comment is mydb, ", comment == "mydb");
}

// Test if opened model can be saved (we ran into an issue with libzip that
// didn't close the file properly, resulting in inability to save the model)
TEST_FUNCTION(20) {
  base::create_directory(TMP_DIR, 0666);
  std::string temp_dir = TMP_DIR;

  // read the file - the file should be properly closed after reading
  ModelFile mf(temp_dir);

  try {
    mf.open(BaseModelFile);
  } catch (...) {
    fail("opening model failed");
  }

  // try to write to the file - if the file wasn't closed, it will fail
  try {
    mf.save_to(BaseModelFile);
  } catch (...) {
    fail("saving file failed - perhaps the file is locked?");
  }
}

std::string test_loading_and_saving_a_model(const base::utf8string &base_path) {
#ifdef _WIN32
  base::create_directory(TMP_DIR, 0666);
#endif
  std::string temp_dir = TMP_DIR;
  ModelFile mf(temp_dir);

  base::utf8string src_path = base_path;
  base::utf8string dst_path = base::appendExtensionIfNeeded(
      base::strip_extension(base_path) + "_tmp", base::extension(base_path));

  // remove copy file if it was left over from a previous unsuccessful run
  base::remove(dst_path);

  // open the file
  try {
    mf.open(src_path);
  } catch (...) {
    return "opening model failed";
  }

  // write a copy
  try {
    mf.save_to(dst_path);
    mf.cleanup();
  } catch (...) {
    return "saving model copy failed";
  }

  // verify the copy was created
  try {
    mf.open(dst_path);
    mf.cleanup();
  } catch (...) {
    // if the copy exists but somehow failed to open, it will be left on disk on
    // purpose.
    // This is so a developer can see if the copy was created.  It will be
    // erased on subsuquent runs (via base::remove() above)

    return "opening model copy failed - was the file created?";
  }

  // delete the copy
  try {
    if (!base::remove(dst_path)) {
      return "model copy was not created";
    }
  } catch (base::file_error) {
    return "removing model copy failed";
  }

  // all went ok
  return "";
}

// Test model loading and saving with ASCII path + ASCII file
TEST_FUNCTION(25) {
  std::string result = test_loading_and_saving_a_model(BaseModelFile);

  if (!result.empty()) {
    fail(result);
  }
}

#ifndef _WIN32
// Test model loading and saving with ASCII path + Unicode file
TEST_FUNCTION(26) {
  std::string result = test_loading_and_saving_a_model(UnicodeBaseModelFile);

  if (!result.empty()) {
    fail(result);
  }
}

// Test model loading and saving with Unicode path + ASCII file
TEST_FUNCTION(27) {
  std::string result = test_loading_and_saving_a_model(UnicodeDiractoryFile1);

  if (!result.empty()) {
    fail(result);
  }
}

// Test model loading and saving with Unicode path + Unicode file
TEST_FUNCTION(28) {
  std::string result = test_loading_and_saving_a_model(UnicodeDiractoryFile2);

  if (!result.empty()) {
    fail(result);
  }
}
#endif

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  delete tester;

  if (file_exists(BaseModelFile)) {
    base::remove(BaseModelFile);
  }

  if (file_exists(UnicodeBaseModelFile)) {
    base::remove(UnicodeBaseModelFile);
  }

  if (is_directory(UnicodeDirectory)) {
    base::remove_recursive(UnicodeDirectory);
  }
}

END_TESTS
