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

  $it("Is os supported test", []() {
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

    $expect(isOsSupportedProxy("Ubuntu 18.04 x86_64")).toBeTrue();
    $expect(isOsSupportedProxy("..... Ubuntu 18.04 ..... x86_64 .....")).toBeTrue();
    $expect(isOsSupportedProxy("Ubuntu 18.04 x86_64")).toBeTrue();
    $expect(isOsSupportedProxy("..... Ubuntu 18.04 ..... x86_64 .....")).toBeTrue();
    $expect(isOsSupportedProxy("Ubuntu 19.04 x86_64")).toBeTrue();
    $expect(isOsSupportedProxy("Ubuntu 19.10 x86_64")).toBeTrue();

    // red-hat based
    $expect(isOsSupportedProxy("Red Hat Enterprise Linux Server release")).toBeFalse();
    $expect(isOsSupportedProxy("Red Hat Enterprise Linux Server release 6")).toBeFalse();
    $expect(isOsSupportedProxy("..... Red Hat Enterprise Linux Server release 6 .....")).toBeFalse();
    $expect(isOsSupportedProxy("Red Hat Enterprise Linux Server release 7")).toBeFalse();
    $expect(isOsSupportedProxy("Red Hat Enterprise Linux Server release 7 i386")).toBeFalse();
    $expect(isOsSupportedProxy("..... Red Hat Enterprise Linux Server release 7 i386 .....")).toBeFalse();
    $expect(isOsSupportedProxy("Red Hat Enterprise Linux Server release 7 x86_64"));
    $expect(isOsSupportedProxy("..... Red Hat Enterprise Linux Server release 7 ..... x86_64 .....")).toBeTrue();
    $expect(isOsSupportedProxy("Red Hat Enterprise Linux Server release 7.1 x86_64")).toBeTrue();
    $expect(isOsSupportedProxy("..... Red Hat Enterprise Linux Server release 7.1 ..... x86_64 .....")).toBeTrue();
    $expect(isOsSupportedProxy("..... Red Hat Enterprise Linux Server release ..... 7.1 ..... x86_64 .....")).toBeFalse();
    $expect(isOsSupportedProxy("..... Red Hat Enterprise Linux release 8 ..... x86_64 .....")).toBeTrue();
    $expect(isOsSupportedProxy("Red Hat Enterprise Linux release 8.0 x86_64")).toBeTrue();
    $expect(isOsSupportedProxy("..... Red Hat Enterprise Linux release 8.0 ..... x86_64 .....")).toBeTrue();

    // mac
    $expect(isOsSupportedProxy("Mac OS")).toBeFalse();
    $expect(isOsSupportedProxy("OS X 10.1")).toBeFalse();
    $expect(isOsSupportedProxy("..... OS X 10.1 .....")).toBeFalse();
    $expect(isOsSupportedProxy("OS X 10.10")).toBeFalse();
    $expect(isOsSupportedProxy("OS X 10.10 i386")).toBeFalse();
    $expect(isOsSupportedProxy("..... macOS 10.13 i386 .....")).toBeFalse();
    $expect(isOsSupportedProxy("macOS 10.13 x86_64")).toBeTrue();
    $expect(isOsSupportedProxy("..... macOS 10.13 ..... x86_64 .....")).toBeTrue();
    $expect(isOsSupportedProxy("..... macOS ..... 10.13 ..... x86_64 .....")).toBeFalse();

    // other debian-based
    $expect(isOsSupportedProxy("Debian 5 x86_64")).toBeFalse();
    $expect(isOsSupportedProxy("Debian 9 x86_64")).toBeTrue();

    // other red-hat-based
    $expect(isOsSupportedProxy("Fedora release 26 x86_64")).toBeFalse();
    $expect(isOsSupportedProxy("Fedora release 27 x86_64")).toBeFalse();
    $expect(isOsSupportedProxy("Fedora release 28 x86_64")).toBeTrue();
    $expect(isOsSupportedProxy("Fedora release 29 x86_64")).toBeTrue();
  });
}
}
