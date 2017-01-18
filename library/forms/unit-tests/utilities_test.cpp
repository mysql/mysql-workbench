/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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
#ifndef _WIN32
#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 8)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-source-encoding"
#endif
#endif
#include "test.h"
#include "mforms/utilities.h"
#include "stub/stub_mforms.h"

using namespace mforms;

BEGIN_TEST_DATA_CLASS(mforms_utilities_test)
END_TEST_DATA_CLASS;

TEST_MODULE(mforms_utilities_test, "mforms utilities testing");

TEST_FUNCTION(1) {
  mforms::stub::init(NULL);

  std::string service = "A quick brown fox jumps over the lazy dog. ÄÖÜ";
  std::string user_name = "father abraham";
  std::string the_password = "!§$%&dingeling ß123@";

  std::string result;
  ensure("Reading non-existing password",
         !Utilities::find_password("out of service", "don't call me, I call you", result));
  Utilities::store_password(
    service, user_name,
    the_password); // ensure("Writing a password", Utilities::store_password(service, user_name, the_password));
  ensure("Reading password back", Utilities::find_password(service, user_name, result));
  ensure_equals("Comparing returned to stored password", the_password, result);
}

END_TESTS
#ifndef _WIN32
#if __GNUC__ < 5
#pragma GCC diagnostic pop
#endif
#endif
