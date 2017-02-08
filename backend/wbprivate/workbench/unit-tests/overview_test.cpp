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

#include "grt.h"
#include "grt_test_utility.h"
#include "wb_helpers.h"

#include "workbench/wb_overview.h"
#include "grts/structs.workbench.h"
#include "grts/structs.workbench.logical.h"
#include "grts/structs.workbench.physical.h"

using namespace grt;
using namespace wb;
using namespace bec;

BEGIN_TEST_DATA_CLASS(wb_overview)
public:
WBTester *tester;

TEST_DATA_CONSTRUCTOR(wb_overview) {
  tester = new WBTester;
}

END_TEST_DATA_CLASS

TEST_MODULE(wb_overview, "wb overview");

TEST_FUNCTION(1) {
  bool flag = tester->wb->open_document("data/workbench/test_model1.mwb");
  ensure("open_document", flag);
}

TEST_FUNCTION(2) {
  std::vector<ssize_t> columns;

  columns.push_back(wb::OverviewBE::Label);
  columns.push_back(wb::OverviewBE::NodeType);
  columns.push_back(wb::OverviewBE::Expanded);
  columns.push_back(wb::OverviewBE::Height);
  columns.push_back(wb::OverviewBE::DisplayMode);
  dump_tree_model("output/overview_test2.txt", wb::WBContextUI::get()->get_physical_overview(), columns);

  ensure_files_equal("initial overview state ", "output/overview_test2.txt", "data/be/overview_test2.txt");
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  delete tester;
}

END_TESTS
