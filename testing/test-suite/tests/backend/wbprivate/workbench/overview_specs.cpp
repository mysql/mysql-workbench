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

#include "grt.h"
#include "wb_test_helpers.h"
#include "grt_test_helpers.h"

#include "workbench/wb_overview.h"
#include "grts/structs.workbench.h"
#include "grts/structs.workbench.logical.h"
#include "grts/structs.workbench.physical.h"

#include "casmine.h"

namespace {

using namespace grt;
using namespace wb;
using namespace bec;

$ModuleEnvironment() {};

static void ensure_files_equal(const std::string &test, const char *file, const char *reffile) {
  std::string line, refline;
  std::ifstream ref(reffile);
  std::ifstream f(file);

  $expect(ref.is_open()).toBeTrue();
  $expect(f.is_open()).toBeTrue();

  while (!ref.eof() && !f.eof()) {
    getline(ref, refline);
    getline(f, line);

    $expect(line).toBe(refline);
  }

  $expect(f.eof() && ref.eof()).toBeTrue();
}

$TestData {
  std::unique_ptr<WorkbenchTester> tester;
};

$describe("WB overview") {
  $beforeAll([&]() {
    data->tester.reset(new WorkbenchTester());
    data->tester->initializeRuntime();
  });

  $afterAll([&]() {
  });

  $it("Open document", [this]() {
    bool flag = data->tester->wb->open_document("data/workbench/test_model1.mwb");
    $expect(flag).toBeTrue();
  });

  $it("Dump tree model", [&]() {
    std::vector<ssize_t> columns;

    columns.push_back(wb::OverviewBE::Label);
    columns.push_back(wb::OverviewBE::NodeType);
    columns.push_back(wb::OverviewBE::Expanded);
    columns.push_back(wb::OverviewBE::Height);
    columns.push_back(wb::OverviewBE::DisplayMode);
    casmine::dumpTreeModel("output/overview_test2.txt", (TreeModel*)wb::WBContextUI::get()->get_physical_overview(), columns);

    ensure_files_equal("initial overview state ", "output/overview_test2.txt", "data/be/overview_test2.txt");
  });
}

}
