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

#include "structs.test.h"
#include "grtpp_util.h"

#include "casmine.h"
#include "wb_test_helpers.h"

namespace {

using namespace grt;

$ModuleEnvironment() {};

$describe("GRT: util functions") {

  $beforeAll([&]() {
    WorkbenchTester::reinitGRT();
    register_structs_test_xml();
    grt::GRT::get()->load_metaclasses(casmine::CasmineContext::get()->tmpDataDir() + "/structs.test.xml");
    grt::GRT::get()->end_loading_metaclasses();
    $expect(grt::GRT::get()->get_metaclasses().size()).toBe(6U);
  });

  $afterAll([&]() {
    WorkbenchTester::reinitGRT();
  });

  $it("Set value by path", []() {
    test_BookRef book(grt::Initialized);
    bool flag;

    flag = set_value_by_path(book, "/title", StringRef("TITLE"));
    $expect(flag).toBeTrue();
    $expect(*book->title()).toBe("TITLE");

    flag = set_value_by_path(book, "/", StringRef("TITLE"));
    $expect(!flag).toBeTrue();

    try {
      set_value_by_path(book, "/xxx", StringRef("TITLE"));
      $expect(false).toBeTrue();
    } catch (grt::bad_item &) {
    }

    flag = set_value_by_path(book, "/title/x", StringRef("TITLE"));
    $expect(!flag).toBeTrue();

    try {
      set_value_by_path(book, "/title", IntegerRef(1234));
      $expect(false).toBeTrue();
    } catch (grt::type_error &) {
    }
  });

  $it("Regression test for Bug #17324160 MySQL workbench loses connections list", []() {
    test_PublisherRef publisher(grt::Initialized);
    test_BookRef book(grt::Initialized);

    book->title("testbook");
    publisher->name("testpub");
    publisher->books().insert(book);
    book->publisher(publisher);

    test_PublisherRef publisher_copy(grt::shallow_copy_object(publisher));

    $expect(publisher_copy.id() != publisher.id()).toBeTrue();
    $expect(*publisher_copy->name()).toBe(*publisher->name());
    $expect(publisher_copy->books().count()).toBe(1U);
    $expect(publisher_copy->books()[0].id()).toBe(book.id());
    // The bug was that a shallow_copy would modify the referenced objects that would back-reference the copied object
    $expect(book->publisher().id()).toBe(publisher.id());
  });

}
}
