/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "base/data_types.h"
#include "wb_helpers.h"

BEGIN_TEST_DATA_CLASS(command_line_parser_test)
protected:
bool callbackTriggered;

TEST_DATA_CONSTRUCTOR(command_line_parser_test) {
  callbackTriggered = false;
}
END_TEST_DATA_CLASS;

TEST_MODULE(command_line_parser_test, "Base library command line handling");

using namespace base;
using dataTypes::OptionsList;
using dataTypes::OptionEntry;

TEST_FUNCTION(10) {
  std::vector<std::string> args({"--test-argument-value-space", "argument value", "--test-argument-value-equals",
                                 "=sample", "some/file/path", "--test-boolean"});
  OptionsList opts;
  opts.addEntry(OptionEntry(dataTypes::OptionArgumentType::OptionArgumentFilename, 0, "test-argument-value-space",
                            "Test passing argument for the value with space", nullptr, "name"));
  opts.addEntry(OptionEntry(dataTypes::OptionArgumentType::OptionArgumentFilename, 0, "test-argument-value-equals",
                            "Test passing argument for the value with equals", nullptr, "name"));
  opts.addEntry(
    OptionEntry(dataTypes::OptionArgumentType::OptionArgumentLogical, 0, "test-boolean", "Test boolean value"));

  int retVal = 0;
  ensure("Unable to parse command line", opts.parse(args, retVal));
  ensure("incorrect test-boolean", opts.getEntry("test-boolean")->value.logicalValue);
  ensure_equals("incorrect test-argument-value-equals", opts.getEntry("test-argument-value-equals")->value.textValue,
                "=sample");
  ensure_equals("incorrect test-argument-value-space", opts.getEntry("test-argument-value-space")->value.textValue,
                "argument value");
  ensure("incorrect number of leftover values", opts.pathArgs.size() == 1);
  ensure_equals("incorrect leftover value", opts.pathArgs[0], "some/file/path");
}

TEST_FUNCTION(20) {
  std::vector<std::string> args({"--test-return-value", "--test-callback", "--test-argument-value-space",
                                 "argument value", "--test-argument-value-equals", "=sample", "some/file/path",
                                 "--test-boolean"});
  OptionsList opts;
  opts.addEntry(
    OptionEntry(dataTypes::OptionArgumentType::OptionArgumentLogical, 0, "test-boolean", "Test boolean value"));

  try {
    int retVal = 0;
    opts.parse(args, retVal);
    fail("Unknown argument not thrown");
  } catch (std::exception &exc) {
    ensure_equals("Callback not called", exc.what(), "Unknown argument");
  }
}

TEST_FUNCTION(30) {
  std::vector<std::string> args({"--test-callback"});
  OptionsList opts;
  opts.addEntry(OptionEntry(dataTypes::OptionArgumentType::OptionArgumentLogical, 0, "test-callback",
                            "Test callback trigger", [&](const OptionEntry &entry, int *retval) {
                              callbackTriggered = true;
                              *retval = 10;
                              return false;
                            }));

  int retVal = 0;
  ensure("Unable to parse command line", !opts.parse(args, retVal));
  ensure("Callback not called", callbackTriggered);
  ensure_equals("Incorrect return value", retVal, 10);
}

END_TESTS;

//----------------------------------------------------------------------------------------------------------------------
