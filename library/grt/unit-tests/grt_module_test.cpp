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

#include "testgrt.h"
#include "structs.test.h"

#include "grtpp_module_cpp.h"

BEGIN_TEST_DATA_CLASS(grt_module_test)
public:
END_TEST_DATA_CLASS

TEST_MODULE(grt_module_test, "GRT: module functionality");

TEST_FUNCTION(5) {
  grt::GRT::get()->load_metaclasses("data/structs.test.xml");
  grt::GRT::get()->end_loading_metaclasses();
}

// ml: No tests defined currently, as the few previously defined tests are no longer valid and
// have been removed. Remove this notice as soon as you start writing new tests.

END_TESTS
