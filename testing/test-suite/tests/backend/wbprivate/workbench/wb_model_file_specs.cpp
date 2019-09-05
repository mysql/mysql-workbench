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

#include "workbench/wb_model_file.h"

#include "wb_test_helpers.h"

#include "base/file_utilities.h"
#include "base/utf8string.h"

#include "casmine.h"

namespace {

$ModuleEnvironment() {};

using namespace wb;

$TestData {
  std::unique_ptr<WorkbenchTester> tester;
  std::string tmpDataDir;
  std::string outputDir;

  const base::utf8string BaseModelFile = "/workbench/test_model1.mwb";
  const base::utf8string UnicodeDirectory = "/workbench/pqŃńдфصض◒◓";
  const base::utf8string UnicodeBaseModelFile = "/workbench/pqŃńдфصض◒◓/☀☁☂☘_model.mwb";

 void testModelSavingAndLoading(const base::utf8string &modelFile) {
    wb::ModelFile mf(outputDir);

    base::utf8string tempPath = base::strip_extension(modelFile) + "_tmp" + base::extension(modelFile);
    $expect(base::file_exists(tempPath)).toBeFalse("Model file left-over found");

    // Open, save copy, reopen from copy.
    $expect([&]() { mf.open(modelFile); }).Not.toThrow();
    $expect([&]() { mf.save_to(tempPath); mf.cleanup(); }).Not.toThrow();
    $expect([&]() { mf.open(tempPath); mf.cleanup(); }).Not.toThrow();
  }
};

$describe("Tests for WB model file") {
  $beforeAll([&]() {
    data->tmpDataDir = casmine::CasmineContext::get()->tmpDataDir();
    data->outputDir = casmine::CasmineContext::get()->outputDir();
    data->tester.reset(new WorkbenchTester());
  });

  $it("Model file creation + rename", [this]() {
    ModelFile mf(data->outputDir);
    workbench_DocumentRef doc(grt::Initialized);

    // Create a test file, change it and then save_as.
    mf.create();
    doc->name("t1");

    workbench_physical_ModelRef pmodel(grt::Initialized);
    pmodel->owner(doc);
    db_Catalog catalog;
    pmodel->catalog(&catalog);
    doc->physicalModels().insert(pmodel);

    mf.store_document(doc);
    mf.save_to(data->outputDir + "/t1.mwb");

    doc->name("t2");
    mf.store_document(doc);
    mf.save_to(data->outputDir + "/t2.mwb");

    ModelFile mf1(data->outputDir);
    ModelFile mf2(data->outputDir);

    mf1.open(data->outputDir + "/t1.mwb");
    mf2.open(data->outputDir + "/t2.mwb");

    workbench_DocumentRef d1, d2;

    d1 = mf1.retrieve_document();
    d2 = mf2.retrieve_document();

    $expect(*d1->name()).toBe("t1");
    $expect(*d2->name()).toBe("t2");
  });

  $it("Open file locking test", [this]() {
    $pending("test needs rework as accessing a locked model file no longer throws an exception");
    ModelFile mf(data->outputDir);

    mf.open(data->tmpDataDir + "/workbench/sakila.mwb");
    $expect([&]() { mf.open(data->tmpDataDir + "/workbench/sakila.mwb"); }).toThrow();
  });

  $it("Reading comment test", [this]() {
    ModelFile mf(data->outputDir);
    std::string comment = mf.read_comment(data->tmpDataDir + "/workbench/empty_file.sql");
    $expect(comment.empty()).toBeTrue();
    comment = mf.read_comment(data->tmpDataDir + "/workbench/empty_model_with_comment.mwb");
    $expect(comment == "mydb").toBeTrue();
  });

  $it("Test if opened model can be saved", [this]() {
    // read the file - the file should be properly closed after reading
    ModelFile mf(data->outputDir);
    $expect([&]() { mf.open(data->tmpDataDir + data->BaseModelFile); }).Not.toThrow();

    // Try to write to the file - if the file wasn't closed this will fail.
    $expect([&]() { mf.save_to(data->tmpDataDir + data->BaseModelFile); }).Not.toThrow();
  });

  $it("Test model loading and saving with ANSI + full Unicode paths/names", [this]() {
    data->testModelSavingAndLoading(data->tmpDataDir + data->BaseModelFile);

    // We have to prepare the directory
    {
      base::create_directory(data->tmpDataDir + data->UnicodeDirectory, 0777);

      copyFile(data->tmpDataDir + data->BaseModelFile,
               data->tmpDataDir + data->UnicodeBaseModelFile);
    }
    data->testModelSavingAndLoading(data->tmpDataDir + data->UnicodeBaseModelFile);
  });

}

}
