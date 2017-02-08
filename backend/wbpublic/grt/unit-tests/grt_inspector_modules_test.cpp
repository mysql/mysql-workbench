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

#include "grtpp_module_cpp.h"
#include "wb_helpers.h"

// namespace modinsp {
#include "test_modules.h"
//};

// using namespace modinsp;
using namespace grt;
using namespace bec;

#include "grt_values_test_data.h"

BEGIN_TEST_DATA_CLASS(be_inspector_modules)
public:
END_TEST_DATA_CLASS;

TEST_MODULE(be_inspector_modules, "grt modules inspector backends");

TEST_FUNCTION(1) { // load modules
}

END_TESTS
