/*
 * Copyright(c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "wb_helpers.h"
#include "workbench/wb_module.h"

BEGIN_TEST_DATA_CLASS(wb_module_test)

protected:
WBTester* tester;

TEST_DATA_CONSTRUCTOR(wb_module_test) {
  tester = new WBTester;
}

END_TEST_DATA_CLASS;

TEST_MODULE(wb_module_test, "wb_module tests for Workbench");

TEST_FUNCTION(5) // test WorkbenchImpl::isOsSupported()
{
  // As we move out of supporting old operating systems, we will need to update both this test and isOsSupported()
  // So if it's failing and it wasn't before, that's probably why - just update them.

  // proxy function for a module call
  grt::Module* module = grt::GRT::get()->get_module("Workbench");
  auto isOsSupportedProxy = [module](const char* os) -> bool {
    grt::StringListRef arguments(grt::Initialized);
    arguments.ginsert(grt::StringRef(std::string(os)));

    grt::ValueRef result = module->call_function("isOsSupported", arguments);
    return *grt::IntegerRef::cast_from(result) != 0;
  };

  // unrecognised OS
  ensure_false("empty string", isOsSupportedProxy(""));
  ensure_false("some unknown OS", isOsSupportedProxy("Some OS"));
  ensure_true(
    "unknown",
    isOsSupportedProxy("unknown")); // special flag returned by get_local_os_name() when it was unable to get OS info

  // windows
  ensure_false("Windows, no version", isOsSupportedProxy("Windows"));
  ensure_false("Windows, old", isOsSupportedProxy("Windows 98"));
  ensure_false("Windows, old, extra chars", isOsSupportedProxy("..... Windows 98 ....."));
  ensure_true("Windows, supported", isOsSupportedProxy("Windows 8"));
  ensure_true("Windows, supported, extra chars", isOsSupportedProxy("..... Windows 8 ....."));
  ensure_true("Windows, subversion, supported", isOsSupportedProxy("Windows 8.1"));
  ensure_true("Windows, subversion, supported, extra chars", isOsSupportedProxy("..... Windows 8.1 ....."));
  ensure_false("Windows, chars between name and version", isOsSupportedProxy("..... Windows ..... 8 ....."));

  // debian-based
  ensure_false("Ubuntu, no version", isOsSupportedProxy("Ubuntu"));
  ensure_false("Ubuntu, old, no 64-bit", isOsSupportedProxy("Ubuntu 12.04"));
  ensure_false("Ubuntu, old, no 64-bit, extra chars", isOsSupportedProxy("..... Ubuntu 12.04 ....."));
  ensure_false("Ubuntu, supported, no 64-bit", isOsSupportedProxy("Ubuntu 15.04"));
  ensure_false("Ubuntu, supported, 32-bit", isOsSupportedProxy("Ubuntu 15.04 i386"));
  ensure_false("Ubuntu, supported, 32-bit, extra chars", isOsSupportedProxy("..... Ubuntu 15.04 i386 ....."));
  ensure_true("Ubuntu, supported", isOsSupportedProxy("Ubuntu 16.04 x86_64"));
  ensure_true("Ubuntu, supported, extra chars", isOsSupportedProxy("..... Ubuntu 16.04 ..... x86_64 ....."));
  ensure_true("Ubuntu, subversion, supported", isOsSupportedProxy("Ubuntu 16.04.2 x86_64"));
  ensure_true("Ubuntu, subversion, supported, extra chars",
              isOsSupportedProxy("..... Ubuntu 16.04.2 ..... x86_64 ....."));
  ensure_false("Ubuntu, chars between name and version",
               isOsSupportedProxy("..... Ubuntu ..... 16.04 ..... x86_64 ....."));

  // red-hat based
  ensure_false("Oracle, no version", isOsSupportedProxy("Red Hat Enterprise Linux Server release"));
  ensure_false("Oracle, old, no 64-bit", isOsSupportedProxy("Red Hat Enterprise Linux Server release 6"));
  ensure_false("Oracle, old, no 64-bit, extra chars",
               isOsSupportedProxy("..... Red Hat Enterprise Linux Server release 6 ....."));
  ensure_false("Oracle, supported, no 64-bit", isOsSupportedProxy("Red Hat Enterprise Linux Server release 7"));
  ensure_false("Oracle, supported, 32-bit", isOsSupportedProxy("Red Hat Enterprise Linux Server release 7 i386"));
  ensure_false("Oracle, supported, 32-bit, extra chars",
               isOsSupportedProxy("..... Red Hat Enterprise Linux Server release 7 i386 ....."));
  ensure_true("Oracle, supported", isOsSupportedProxy("Red Hat Enterprise Linux Server release 7 x86_64"));
  ensure_true("Oracle, supported, extra chars",
              isOsSupportedProxy("..... Red Hat Enterprise Linux Server release 7 ..... x86_64 ....."));
  ensure_true("Oracle, subversion, supported",
              isOsSupportedProxy("Red Hat Enterprise Linux Server release 7.1 x86_64"));
  ensure_true("Oracle, subversion, supported, extra chars",
              isOsSupportedProxy("..... Red Hat Enterprise Linux Server release 7.1 ..... x86_64 ....."));
  ensure_false("Oracle, chars between name and version",
               isOsSupportedProxy("..... Red Hat Enterprise Linux Server release ..... 7.1 ..... x86_64 ....."));

  // mac
  ensure_false("OSX, no version", isOsSupportedProxy("OS X"));
  ensure_false("OSX, old, no 64-bit", isOsSupportedProxy("OS X 10.1"));
  ensure_false("OSX, old, no 64-bit, extra chars", isOsSupportedProxy("..... OS X 10.1 ....."));
  ensure_false("OSX, supported, no 64-bit", isOsSupportedProxy("OS X 10.10"));
  ensure_false("OSX, supported, 32-bit", isOsSupportedProxy("OS X 10.10 i386"));
  ensure_false("OSX, supported, 32-bit, extra chars", isOsSupportedProxy("..... OS X 10.10 i386 ....."));
  ensure_true("OSX, supported", isOsSupportedProxy("OS X 10.10 x86_64"));
  ensure_true("OSX, supported, extra chars", isOsSupportedProxy("..... OS X 10.10 ..... x86_64 ....."));
  ensure_false("OSX, chars between name and version", isOsSupportedProxy("..... OS X ..... 10.10 ..... x86_64 ....."));

  // other debian-based
  ensure_false("Debian, old", isOsSupportedProxy("Debian 5 x86_64"));
  ensure_true("Debian, supported", isOsSupportedProxy("Debian 8 x86_64"));

  // other red-hat-based
  ensure_false("Fedora, old", isOsSupportedProxy("Fedora release 20 x86_64"));
  ensure_true("Fedora, supported", isOsSupportedProxy("Fedora release 24 x86_64"));
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  delete tester;
}

END_TESTS
