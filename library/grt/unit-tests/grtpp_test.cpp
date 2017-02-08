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

// test class outside any namespace
class Foo {
public:
  int member1;
  double member2;
};

using namespace std;

BEGIN_TEST_DATA_CLASS(grt_wrapper_base_tests)
public:
END_TEST_DATA_CLASS

TEST_MODULE(grt_wrapper_base_tests, "GRT wrapper base tests");

TEST_FUNCTION(1) {
  // Test std::string get_full_type_name
  string name = get_full_type_name(typeid(Foo));
  ensure_equals("Full name of Foo class differs.", name, "Foo");

  name = get_full_type_name(typeid(Foo().member1));
  ensure_equals("Full name of Foo class differs.", name, "int");

  // Test std::string get_type_name
  name = get_type_name(typeid(Foo));
  ensure_equals("Type name of Foo class differs.", name, "Foo");

  name = get_type_name(typeid(*this));
  ensure_equals("Type name of Foo class differs.", name, "grt_wrapper_base_tests> >");

  name = get_type_name(typeid(int));
  ensure_equals("Type name of Foo class differs.", name, "int");
}

TEST_FUNCTION(2) {
  // Tests for os_error class.
  os_error* error = new os_error("dummy");

  ensure_equals("os_error string c-tor", error->what(), "dummy");
  delete error;

  error = new os_error(5);
  ensure_equals("os_error code c-tor", error->what(), g_strerror(5));
  delete error;
}

TEST_FUNCTION(3) {
  // Tests for type_error class.
  Type expected = StringType;
  Type actual = DoubleType;
  Type container = ListType;

  type_error* error = new type_error("dummy");
  ensure_equals("type_error string c-tor", error->what(), "dummy");
  delete error;

  error = new type_error("foo", "bar");
  ensure_equals("type_error expected/actual strings c-tor", error->what(),
                "Type mismatch: expected object of type foo,"
                " but got bar");
  delete error;

  error = new type_error("foo", "bar", container);
  ensure_equals("type_error expected/actual strings in container c-tor", error->what(),
                "Type mismatch: expected "
                "content object of type foo, but got bar");
  delete error;

  error = new type_error(expected, actual);
  ensure_equals("type_error expected/actual GRT values c-tor", error->what(),
                "Type mismatch: expected "
                "type string, but got real");
  delete error;

  error = new type_error(expected, actual, container);
  ensure_equals("type_error expected/actual GRT values in container c-tor", error->what(),
                "Type mismatch: expected "
                "content-type string, but got real");
  delete error;
}

TEST_FUNCTION(5) {
  // Tests for null_value class.
  ensure_equals("null_value string c-tor", null_value("dummy").what(), "dummy");
  ensure_equals("null_value empty c-tor", null_value().what(), "Attempt to operate on a NULL GRT value.");
}

TEST_FUNCTION(6) {
  // Tests for bad_item class.
  ensure_equals("bad_item string c-tor", bad_item("dummy").what(), "Invalid item name 'dummy'.");
  ensure_equals("bad_item size + count c-tor", bad_item(10, 100).what(), "Index out of range.");
}

TEST_FUNCTION(7) {
  /* deprecated
  // Tests for module_error class.
  ensure_equals("module_error c-tor", module_error("module", "function", "description").what(),
    "description (module.function)");
  module_error error= FunctionCallError("module", "function", MYX_GRT_VALIDATION_ERROR);
  ensure_equals("module_error inline creation", error.what(), "Validation error (module.function)");

  error= FunctionCallError("module", "function", MYX_GRT_MODULE_INIT_ERROR, "something went wrong");
  ensure_equals("module_error inline creation", error.what(), "Error initializing module: "
    "something went wrong (module.function)");
   */
}

TEST_FUNCTION(8) {
  // Tests for grt_runtime_error class.
  grt_runtime_error error = grt_runtime_error("dummy", "details");
  ensure_equals("grt_runtime_error error + detail c-tor", error.what(), "dummy");
  ensure_equals("grt_runtime_error details", error.detail, "details");
  ensure("grt_runtime_error fatal check", !error.fatal);

  error = grt_runtime_error("foo", "bar", false);
  ensure_equals("grt_runtime_error error + detail c-tor", error.what(), "foo");
  ensure_equals("grt_runtime_error details", error.detail, "bar");
  ensure("grt_runtime_error fatal check", !error.fatal);

  error = grt_runtime_error("foo", "bar", true);
  ensure_equals("grt_runtime_error error + detail c-tor", error.what(), "foo");
  ensure_equals("grt_runtime_error details", error.detail, "bar");
  ensure("grt_runtime_error fatal check", error.fatal);
}

// Notes:
// - Tests for Struct are in struct_test.cpp.
// - Tests for GRT values are in value_test.cpp.

END_TESTS
