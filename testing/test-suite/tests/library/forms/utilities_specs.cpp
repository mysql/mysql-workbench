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

#ifndef _MSC_VER
#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 8)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-source-encoding"
#endif
#endif

#include "mforms/utilities.h"
#include "stub/stub_mforms.h"

#include "casmine.h"

using namespace mforms;

namespace {

$ModuleEnvironment() {};

$describe("mforms utilities testing") {

  $it("Finding password", [&]() {
    mforms::stub::init(NULL);

    std::string service = "A quick brown fox jumps over the lazy dog. ÄÖÜ";
    std::string user_name = "father abraham";
    std::string the_password = "!§$%&dingeling ß123@";

    std::string result;
    $expect(!Utilities::find_password("out of service", "don't call me, I call you", result)).toBeTrue();
    Utilities::store_password(service, user_name, the_password);
    $expect(Utilities::find_password(service, user_name, result)).toBeTrue();
    $expect(the_password).toBe(result);
  });
}
}

#ifndef _MSC_VER
#if __GNUC__ < 5
#pragma GCC diagnostic pop
#endif
#endif
