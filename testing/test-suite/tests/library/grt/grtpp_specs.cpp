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

#include "grt.h"

#include "casmine.h"

// test class outside any namespace
class Foo {
public:
  int member1;
  double member2;
};

using namespace std;
using namespace grt;

namespace {

$ModuleEnvironment() {};

$describe("GRT wrapper base tests") {

  $it("get_full_type_name + get_type_name", [this]() {
    string name = get_full_type_name(typeid(Foo));
    $expect(name).toEqual("Foo");

    name = get_full_type_name(typeid(Foo().member1));
    $expect(name).toEqual("int");

    name = get_type_name(typeid(Foo));
    $expect(name).toEqual("Foo");

    name = get_type_name(typeid(*this));
    $expect(name).toEqual("DescribeImpl");

    name = get_type_name(typeid(int));
    $expect(name).toEqual("int");
  });

  $it("os_error exception", []() {
    os_error* error = new os_error("dummy");

    $expect(error->what()).toEqual("dummy");
    delete error;

    error = new os_error(5);
    $expect(error->what()).toEqual(g_strerror(5));
    delete error;
  });

  $it("type_error exception", []() {
    Type expected = StringType;
    Type actual = DoubleType;
    Type container = ListType;

    type_error* error = new type_error("dummy");
    $expect(error->what()).toEqual("dummy");
    delete error;

    error = new type_error("foo", "bar");
    $expect(error->what()).toEqual("Type mismatch: expected object of type foo, but got bar");
    delete error;

    error = new type_error("foo", "bar", container);
    $expect(error->what()).toEqual("Type mismatch: expected content object of type foo, but got bar");
    delete error;

    error = new type_error(expected, actual);
    $expect(error->what()).toEqual("Type mismatch: expected type string, but got real");
    delete error;

    error = new type_error(expected, actual, container);
    $expect(error->what()).toEqual("Type mismatch: expected content-type string, but got real");
    delete error;
  });

  $it("null_value exception", []() {
    $expect(null_value("dummy").what()).toEqual("dummy");
    $expect(null_value().what()).toEqual("Attempt to operate on a NULL GRT value.");
  });

  $it("bad_item exception", []() {
    $expect(bad_item("dummy").what()).toEqual("Invalid item name 'dummy'");
    $expect(bad_item(10, 100).what()).toEqual("Index out of range");
  });

  $it("grt_runtime_error exception", []() {
    grt_runtime_error error("dummy", "details");
    $expect(error.what()).toEqual("dummy");
    $expect(error.detail).toEqual("details");
    $expect(error.fatal).toBeFalse();

    error = grt_runtime_error("foo", "bar", false);
    $expect(error.what()).toEqual("foo");
    $expect(error.detail).toEqual("bar");
    $expect(error.fatal).toBeFalse();

    error = grt_runtime_error("foo", "bar", true);
    $expect(error.what()).toEqual("foo");
    $expect(error.detail).toEqual("bar");
    $expect(error.fatal).toBeTrue();;
  });

  // Notes:
  // - Tests for Struct are in struct_specs.cpp.
  // - Tests for GRT values are in value_specs.cpp.

}

}
