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

#include "structs.test.h"

#include "casmine.h"
#include "wb_test_helpers.h"

namespace {

$ModuleEnvironment() {};

$describe("GRT: structs/metaclasses") {
  $beforeAll([]() {
    $expect([]() { test_Book book; }).toThrow();
    register_structs_test_xml();
    grt::GRT::get()->load_metaclasses(casmine::CasmineContext::get()->tmpDataDir() + "/structs.test.xml");
    grt::GRT::get()->end_loading_metaclasses();
  });

  $afterAll([]() {
    WorkbenchTester::reinitGRT();
  });

  $it("Load structures", [&]() {
     $expect(grt::GRT::get()->get_metaclasses().size()).toBe(6U);
   });

  $it("Test valid struct creation and comparison to another struct", [&](){
    grt::MetaClass *book(grt::GRT::get()->get_metaclass("test.Book"));

    $expect(book).Not.toBeNull();
    $expect(book->name()).toBe("test.Book");

    $expect(book->is_a(grt::GRT::get()->get_metaclass("test.Publication"))).toBeTrue();
    $expect(book->is_a(grt::GRT::get()->get_metaclass("test.Base"))).toBeTrue();
    $expect(book->is_a("test.Base")).toBeTrue();
    $expect(book->is_a("XXXX")).toBeFalse();

    $expect(book->get_attribute("caption")).toBe("Book");
    $expect(book->get_attribute("xxx")).toBe("");

    $expect(book->parent()->name()).toBe("test.Publication");
  });

  $it("check get_member", [&](){
    grt::MetaClass *book = grt::GRT::get()->get_metaclass("test.Book");
    const grt::MetaClass::Member *mem;

    mem = book->get_member_info("pages");
    $expect(mem).Not.toBe(nullptr);

    mem = book->get_member_info("title");
    $expect(mem).Not.toBe(nullptr);

    test_BookRef book_obj(grt::Initialized);

    book_obj->pages(1234);

    $expect(*grt::IntegerRef::cast_from(book->get_member_value(&book_obj.content(), "pages"))).toBe(1234);
    $expect(*book_obj->pages()).toBe(1234);
  });

  $it("check has_member", []() {
    $pending("it needs an implementation");
  });

  $it("check get_member", []() {
    $pending("it needs an implementation");
  });

  $it("check set_member", []() {
    $pending("it needs an implementation");
    // check set_member

    // from parent class

    // with override
  });

  $it("check allocation", []() {
    $pending("it needs an implementation");
  });

  $it("check method call", []() {
    $pending("it needs an implementation");
  });

  $it("check foreach_member", []() {
    $pending("it needs an implementation");
  });

  $it("Test struct members and their attributes", [&](){
    grt::MetaClass *book(grt::GRT::get()->get_metaclass("test.Book"));
    const grt::MetaClass::Member *m;
    grt::TypeSpec t;
    std::string a;

    m = book->get_member_info("authors");
    $expect(m).Not.toBe(nullptr);

    m = book->get_member_info("title");
    $expect(m).Not.toBe(nullptr);

    t = book->get_member_type("authors");
    $expect((int)t.base.type).toBe(grt::ListType);

    t = book->get_member_type("title");
    $expect((int)t.base.type).toBe(grt::StringType);

    // Member attributes.
    a = book->get_member_attribute("authors", "caption");
    $expect(a).toBe("Authors");

    a = book->get_member_attribute("authors", "desc");
    $expect(a).toBe("the list of authors");

    a = book->get_member_attribute("authors", "group");
    $expect(a).toBe("group1");

    a = book->get_member_attribute("title", "caption");
    $expect(a).toBe("Title");

    a = book->get_member_attribute("title", "desc");
    $expect(a).toBe("title of the book");

    a = book->get_member_attribute("title", "group");
    $expect(a).toBe("");
  });
};

}
