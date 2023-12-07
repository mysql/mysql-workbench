/*
 * Copyright (c) 2019, 2022, Oracle and/or its affiliates.
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

#include "wb_test_helpers.h"
#include "workbench/wb_module.h"

#include "casmine.h"

namespace {

$ModuleEnvironment() {};

$TestData {
  std::unique_ptr<WorkbenchTester> tester;
};

$describe("wb_module tests for Workbench") {
  $beforeAll([&]() {
    data->tester.reset(new WorkbenchTester());
    // data->tester->initializeRuntime();
  });

  $afterAll([&]() {
  });

  $it("Supported OS test", []() {
    // As we move out of supporting old operating systems, we will need to update both this test and isOsSupported()
    // So if it's failing and it wasn't before, that's probably why - just update them.

    // proxy function for a module call
    grt::Module* module = grt::GRT::get()->get_module("Workbench");
    auto isOsSupportedProxy = [module](std::string const& os) -> bool {
      grt::StringListRef arguments(grt::Initialized);
      arguments.ginsert(grt::StringRef(os));

      grt::ValueRef result = module->call_function("isOsSupported", arguments);
      return *grt::IntegerRef::cast_from(result) != 0;
    };

    // unrecognised OS
    $expect(isOsSupportedProxy("")).toBeFalse();
    $expect(isOsSupportedProxy("Some OS")).toBeFalse();
    $expect(isOsSupportedProxy("unknown")).toBeTrue(); // special flag returned by get_local_os_name() when it was unable to get OS info

    // windows
    $expect(isOsSupportedProxy("Windows")).toBeFalse();
    $expect(isOsSupportedProxy("Windows 98")).toBeFalse();
    $expect(isOsSupportedProxy("..... Windows 98 .....")).toBeFalse();
    $expect(isOsSupportedProxy("Windows 10")).toBeTrue();
    $expect(isOsSupportedProxy("..... Windows 10 .....")).toBeTrue();
    $expect(isOsSupportedProxy("..... Windows ..... 10 .....")).toBeFalse();
    $expect(isOsSupportedProxy("..... Windows 11 .....")).toBeTrue();
    $expect(isOsSupportedProxy("..... Windows ..... 11 .....")).toBeFalse();

    // debian-based
    $expect(isOsSupportedProxy("Ubuntu")).toBeFalse();
    $expect(isOsSupportedProxy("Ubuntu 12.04")).toBeFalse();
    $expect(isOsSupportedProxy("..... Ubuntu 12.04 .....")).toBeFalse();
    $expect(isOsSupportedProxy("Ubuntu 15.04")).toBeFalse();
    $expect(isOsSupportedProxy("Ubuntu 15.04 i386")).toBeFalse();
    $expect(isOsSupportedProxy("..... Ubuntu 15.04 i386 .....")).toBeFalse();
    $expect(isOsSupportedProxy("Ubuntu 16.04 x86_64")).toBeFalse();
    $expect(isOsSupportedProxy("..... Ubuntu ..... 16.04 ..... x86_64 .....")).toBeFalse();
    $expect(isOsSupportedProxy("..... Ubuntu 16.04 ..... x86_64 .....")).toBeFalse();
    $expect(isOsSupportedProxy("Ubuntu 16.04.2 x86_64")).toBeFalse();
    $expect(isOsSupportedProxy("Ubuntu 18.10 x86_64")).toBeFalse();
    $expect(isOsSupportedProxy("..... Ubuntu 18.10 ..... x86_64 .....")).toBeFalse();

    $expect(isOsSupportedProxy("Ubuntu 18.04")).toBeFalse();
    $expect(isOsSupportedProxy("..... Ubuntu 18.04 ..... x86_64 .....")).toBeFalse();
    $expect(isOsSupportedProxy("Ubuntu 19.04")).toBeFalse();
    $expect(isOsSupportedProxy("Ubuntu 19.10")).toBeFalse();
    $expect(isOsSupportedProxy("Ubuntu 20.04")).toBeFalse();
    $expect(isOsSupportedProxy("Ubuntu 20.10")).toBeFalse();
    $expect(isOsSupportedProxy("Ubuntu 21.04")).toBeFalse();
    $expect(isOsSupportedProxy("Ubuntu 21.10")).toBeFalse();
    $expect(isOsSupportedProxy("Ubuntu 22.04")).toBeTrue();
    $expect(isOsSupportedProxy("Ubuntu 22.10")).toBeTrue();
    $expect(isOsSupportedProxy("Ubuntu 23.04")).toBeTrue();

    // red-hat based
    $expect(isOsSupportedProxy("Red Hat Enterprise Linux Server release")).toBeFalse();
    $expect(isOsSupportedProxy("Red Hat Enterprise Linux Server release 6")).toBeFalse();
    $expect(isOsSupportedProxy("..... Red Hat Enterprise Linux Server release 6 .....")).toBeFalse();
    $expect(isOsSupportedProxy("Red Hat Enterprise Linux Server release 7")).toBeFalse();
    $expect(isOsSupportedProxy("..... Red Hat Enterprise Linux Server release 7 i386 .....")).toBeFalse();
    $expect(isOsSupportedProxy("Red Hat Enterprise Linux Server release 7.1 x86_64")).toBeFalse();
    $expect(isOsSupportedProxy("..... Red Hat Enterprise Linux Server release 7.1 ..... x86_64 .....")).toBeFalse();
    $expect(isOsSupportedProxy("..... Red Hat Enterprise Linux Server release ..... 7.1 ..... x86_64 .....")).toBeFalse();
    $expect(isOsSupportedProxy("..... Red Hat Enterprise Linux release 8 .....")).toBeFalse();
    $expect(isOsSupportedProxy("Red Hat Enterprise Linux release 8.0")).toBeFalse();
    $expect(isOsSupportedProxy("..... Red Hat Enterprise Linux release 8.0")).toBeFalse();
    $expect(isOsSupportedProxy("..... Red Hat Enterprise Linux release 9 .....")).toBeTrue();
    $expect(isOsSupportedProxy("Red Hat Enterprise Linux release 9.0")).toBeTrue();
    $expect(isOsSupportedProxy("..... Red Hat Enterprise Linux release 9.0")).toBeTrue();

    // mac
    $expect(isOsSupportedProxy("Mac OS")).toBeFalse();
    $expect(isOsSupportedProxy("OS X 10.1")).toBeFalse();
    $expect(isOsSupportedProxy("..... OS X 10.1 .....")).toBeFalse();
    $expect(isOsSupportedProxy("OS X 10.10")).toBeFalse();
    $expect(isOsSupportedProxy("OS X 10.10 i386")).toBeFalse();
    $expect(isOsSupportedProxy("macOS 10.14")).toBeFalse();
    $expect(isOsSupportedProxy("..... macOS 10.14 i386 .....")).toBeFalse();
    $expect(isOsSupportedProxy("macOS 10.15 x86_64")).toBeFalse();
    $expect(isOsSupportedProxy("..... macOS 10.15 ..... x86_64 .....")).toBeFalse();
    $expect(isOsSupportedProxy("..... macOS ..... 10.15 ..... x86_64 .....")).toBeFalse();
    $expect(isOsSupportedProxy("macOS 11.2")).toBeFalse();
    $expect(isOsSupportedProxy("..... macOS 11.5 ..... x86_64 .....")).toBeFalse();
    $expect(isOsSupportedProxy("..... macOS ..... 11.2 ..... x86_64 .....")).toBeFalse();
    $expect(isOsSupportedProxy("..... macOS 12 ..... x86_64 .....")).toBeFalse();
    $expect(isOsSupportedProxy("macOS 12")).toBeFalse();
    $expect(isOsSupportedProxy("..... macOS 13 ..... x86_64 .....")).toBeTrue();
    $expect(isOsSupportedProxy("macOS 13")).toBeTrue();
    $expect(isOsSupportedProxy("..... macOS 14 ..... x86_64 .....")).toBeTrue();
    $expect(isOsSupportedProxy("macOS 14")).toBeTrue();

    // other debian-based
    $expect(isOsSupportedProxy("Debian 5 x86_64")).toBeFalse();
    $expect(isOsSupportedProxy("Debian 9 x86_64")).toBeFalse();
    $expect(isOsSupportedProxy("Debian 10 x86_64")).toBeTrue();

    // other red-hat-based
    $expect(isOsSupportedProxy("Fedora release 26 x86_64")).toBeFalse();
    $expect(isOsSupportedProxy("Fedora release 27 x86_64")).toBeFalse();
    $expect(isOsSupportedProxy("Fedora release 28 x86_64")).toBeFalse();
    $expect(isOsSupportedProxy("Fedora release 29 x86_64")).toBeFalse();
    $expect(isOsSupportedProxy("Fedora release 30 x86_64")).toBeFalse();
    $expect(isOsSupportedProxy("Fedora release 31 x86_64")).toBeFalse();
    $expect(isOsSupportedProxy("Fedora release 32 x86_64")).toBeFalse();
    $expect(isOsSupportedProxy("Fedora release 33 x86_64")).toBeFalse();
    $expect(isOsSupportedProxy("Fedora release 34 x86_64")).toBeFalse();
    $expect(isOsSupportedProxy("Fedora release 35 x86_64")).toBeFalse();
    $expect(isOsSupportedProxy("Fedora release 36 x86_64")).toBeFalse();
    $expect(isOsSupportedProxy("Fedora release 37 x86_64")).toBeTrue();
    $expect(isOsSupportedProxy("Fedora release 38 x86_64")).toBeTrue();
  });
}
}
