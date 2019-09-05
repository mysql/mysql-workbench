/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "data_types.h"

#include "casmine.h"

namespace {

$ModuleEnvironment() {};

$TestData {
  bool callbackTriggered = false;
};

$describe("command line parser") {
  $it("General argument handling", []() {
    std::vector<std::string> args({"--test-argument-value-space", "argument value", "--test-argument-value-equals",
      "=sample", "some/file/path", "--test-boolean"});
    dataTypes::OptionsList opts;
    opts.addEntry(dataTypes::OptionEntry(dataTypes::OptionArgumentType::OptionArgumentFilename, 0, "test-argument-value-space",
                                         "Test passing argument for the value with space", nullptr, "name"));
    opts.addEntry(dataTypes::OptionEntry(dataTypes::OptionArgumentType::OptionArgumentFilename, 0, "test-argument-value-equals",
                                         "Test passing argument for the value with equals", nullptr, "name"));
    opts.addEntry(
      dataTypes::OptionEntry(dataTypes::OptionArgumentType::OptionArgumentLogical, 0, "test-boolean", "Test boolean value")
    );

    int retVal = 0;
    $expect(opts.parse(args, retVal)).toBeTrue();
    $expect(opts.getEntry("test-boolean")->value.logicalValue).toBeTrue();
    $expect(opts.getEntry("test-argument-value-equals")->value.textValue).toBe("=sample");
    $expect(opts.getEntry("test-argument-value-space")->value.textValue).toBe("argument value");
    $expect(opts.pathArgs.size()).toBe(1U);
    $expect(opts.pathArgs[0]).toBe("some/file/path");
  });

  $it("Unknown argument handling", []() {
    std::vector<std::string> args({"--test-return-value", "--test-callback", "--test-argument-value-space",
      "argument value", "--test-argument-value-equals", "=sample", "some/file/path",
      "--test-boolean"});
    dataTypes::OptionsList opts;
    opts.addEntry(
      dataTypes::OptionEntry(dataTypes::OptionArgumentType::OptionArgumentLogical, 0, "test-boolean", "Test boolean value")
    );

    $expect([&]() {
      int retVal = 0;
      opts.parse(args, retVal);
    }).toThrowError<std::runtime_error>("Unknown argument");
  });

  $it("Argument callback trigger", [this]() {
    std::vector<std::string> args({"--test-callback"});
    dataTypes::OptionsList opts;
    opts.addEntry(dataTypes::OptionEntry(dataTypes::OptionArgumentType::OptionArgumentLogical, 0, "test-callback",
                                         "Test callback trigger", [&](const dataTypes::OptionEntry &entry, int *retval) {
                                           data->callbackTriggered = true;
                                           *retval = 10;
                                           return false;
                                         }));

    int retVal = 0;
    $expect(opts.parse(args, retVal)).toBeFalse();
    $expect(data->callbackTriggered).toBeTrue();
    $expect(retVal).toEqual(10);
  });
}

}
