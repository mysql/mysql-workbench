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

$ModuleEnvironment() {};

static bool count_member(const grt::MetaClass::Member *member, int *count) {
  (*count)++;
  return true;
}
/*
class TestBridge : public ObjectBridgeBase {
public:
  grt::IntegerRef x;
  grt::IntegerRef y;
  grt::StringRef myname;
  grt::ListRef<test_Book> books;
  bool *flag;

protected:
  virtual void initialize(const DictRef &args)
  {
    x= grt::IntegerRef(0);
    y= grt::IntegerRef(0);
    myname= grt::StringRef("hello");
    books.init();
  }
  virtual void destroy()
  {
    *flag= true;
  }

  virtual ValueRef get_item(const std::string &name) const
  {
    if (name == "x")
      return x;
    if (name == "y")
      return y;
    if (name == "name")
      return myname;
    if (name == "books")
      return books;
    return ValueRef();
  }

  virtual void set_item(const std::string &name, const ValueRef &value)
  {
    if (name == "x")
      assign(x, value);
    if (name == "y")
      assign(y, value);
    if (name == "name")
      assign(myname, value);
    if (name == "books")
      throw std::logic_error(name+" is read-only");
  }

  virtual void serialize(xmlNodePtr node)
  {
  }

  virtual void unserialize(xmlNodePtr node)
  {
  }

  virtual void copy(ObjectBridgeBase *orig)
  {
  }


public:
  TestBridge(grt::ValueRef self, void *data) : ObjectBridgeBase(self, data) {};
};
*/

namespace {

$describe("GRT: object values") {
  $beforeAll([](){
      register_structs_test_xml();
      grt::GRT::get()->load_metaclasses(casmine::CasmineContext::get()->tmpDataDir() + "/structs.test.xml");
      grt::GRT::get()->end_loading_metaclasses();
    });

    $afterAll([]() {
      WorkbenchTester::reinitGRT();
    });

  $it("load structures", [&](){
    $expect(grt::GRT::get()->get_metaclasses().size()).toBe(6U);
  });

  $it("Meta class support", [&](){
    test_BookRef book(grt::Initialized);

    $expect(book.has_member("title")).toBeTrue();
    $expect(book.has_member("Title")).toBeFalse();

    $expect(book.get_member("title").is_valid()).toBeTrue();

    book.set_member("title", grt::StringRef("Harry Potter"));
    $expect(book.get_string_member("title")).toBe("Harry Potter");

    book.set_member("price", grt::DoubleRef(123.45));
    $expect(book.get_double_member("price")).toBe(grt::DoubleRef(123.45));

    test_AuthorRef author(grt::Initialized);
    author.set_member("name", grt::StringRef("Some One"));

    $expect(author.get_string_member("name")).toBe("Some One");

    $expect([&](){ book->authors().insert(author); }).Not.toThrow();
  });

  $it("Exceptions for invalid member access", [&](){
    test_BookRef obj(grt::Initialized);

    $expect([&]() { obj.set_member("invalid", grt::StringRef("XXX")); }).toThrow();
    $expect([&]() { obj.get_integer_member("invalid"); }).toThrow();
    $expect([&]() { obj.set_member("title", grt::IntegerRef(1234)); }).toThrow();
    $expect([&]() { obj.set_member("title", grt::DoubleRef(1234.123)); }).toThrow();
    $expect([&]() { obj.set_member("price", grt::StringRef("hello")); }).toThrow();
    $expect([&]() { obj.set_member("authors", grt::StringRef("joe")); }).toThrow();
    $expect([&]() { obj.set_member("pages", grt::DoubleRef(1234.456)); }).toThrow();
  });

  $it("Value member access", [&](){
    test_BookRef book(grt::Initialized);

    book->title("Harry Potter");
    book->title(*book->title() + " XXV");
    book->price(500.23);
    $expect(*book->title()).Not.toBe("Harry Potter");

    $expect(*book->title()).toBe("Harry Potter XXV");

    test_AuthorRef author(grt::Initialized);

    book->authors().insert(author);
    $expect(book->authors().count()).toBe(1U);

    book->authors().get(0)->name("J.K.Bowling");
    $expect(*author->name()).toBe("J.K.Bowling");

    book->authors()[0]->name("ABC");
    $expect(*author->name()).toBe("ABC");

    book->authors().remove(0);
    $expect(book->authors().count()).toBe(0U);
  });

  $it("Check if inherited values are properly initialized", [&](){
    test_BookRef book(grt::Initialized);

    int count = 0;
    book->get_metaclass()->foreach_member(std::bind(&count_member, std::placeholders::_1, &count));
    $expect(count).toEqual(6);
  });
/*
  $it("", [&](){
    bool ret;

    ret= ObjectBridgeBase::register_bridge<TestBridge>;
    $expect(ret).toBeTrue();
  });

  $it("", [&](){
    bool bridge_destroyed= false;

    {
      test_Bridged bridged;
      test_Book book;

      $expect(bridged.get_metaclass().get_metaclass()->bridge).toBe("tut::TestBridge");
      $expect(bridged.get_bridge_private()).Not.toEqual(0U);

      book.title("Harry Potter");
      book.title(*book.title()+ " XXV");
      book.price(500.23);

      TestBridge *bridge_data;
      $expect(*bridged->name()).toBe("hello");

      bridge_data= (TestBridge*)bridged.get_bridge_private();
      $expect(bridge_data).Not.toEqual(0U);

      bridge_data->flag= &bridge_destroyed;
      $expect(bridge_data->myname).toBe(bridged->name());

      bridged.name("xyz");
      $expect(*bridge_data->myname).toBe("xyz");

      bridged.x(1234);
      $expect(bridged.x()).toBe(1234);
      $expect(bridged.books().count()).toEqual(0U);

      bridged.books().insert(book);
      $expect(bridged.books().count()).toEqual(1U);
      $expect(*bridged.books().get(0).title()).toBe("Harry Potter XXV");

      bridged.books().remove(0);
      $expect(bridged.books().count()).toEqual(0U);
      $expect([&](){ bridged.books().remove(0); }).toThrow();
      $expect(bridged.count_members()).toEqual(4U);
    }
    // leaving the context should destroy the objects

    $expect(bridge_destroyed).toBeTrue();
  });
*/
};

}
