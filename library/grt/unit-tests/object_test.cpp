/*
 * Copyright (c) 2011, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "testgrt.h"
#include "structs.test.h"

using namespace grt;

BEGIN_TEST_DATA_CLASS(grt_object_value)
public:
TEST_DATA_CONSTRUCTOR(grt_object_value){};

END_TEST_DATA_CLASS

TEST_MODULE(grt_object_value, "GRT: object values");

/*
 * - load sample structs.test.xml
 * - create objects
 * - check getting/setting values
 * - check duplication
 * - check if all members exist
 * - check adding invalid member
 *
 * - for bridged object
 * - register bridge
 * - allocate object
 * - getting/setting values
 * - getting/setting bad values
 * - duplication
 * - destroy
 *
 */

TEST_FUNCTION(1) {
  // load structs
  grt::GRT::get()->load_metaclasses("data/structs.test.xml");
  grt::GRT::get()->end_loading_metaclasses();

  ensure_equals("load structs", grt::GRT::get()->get_metaclasses().size(), 6U);
}

TEST_FUNCTION(5) {
  test_BookRef book(grt::Initialized);

  ensure("has_member", book.has_member("title"));
  ensure("has_member bad", !book.has_member("Title"));

  ensure("default title is not NULL", book.get_member("title").is_valid());

  book.set_member("title", StringRef("Harry Potter"));
  ensure_equals("get_member", book.get_string_member("title"), "Harry Potter");

  book.set_member("price", DoubleRef(123.45));
  ensure_equals("get_dbl_member", book.get_double_member("price"), DoubleRef(123.45));

  test_AuthorRef author(grt::Initialized);
  author.set_member("name", StringRef("Some One"));

  ensure_equals("constructor", author.get_string_member("name"), "Some One");

  try {
    book->authors().insert(author);
  } catch (...) {
    fail("add value to list");
  };

  /* compiler wont allow this
  try {
    book->authors().insert(book);
    fail("add bad value to list");
  } catch (...) {
  };
*/
}

TEST_FUNCTION(6) {
  test_BookRef obj(grt::Initialized);
  bool flag = false;
  try {
    obj.set_member("invalid", StringRef("XXX"));
  } catch (grt::bad_item &) {
    flag = true;
  };

  ensure("set invalid member", flag == true);

  flag = false;
  try {
    obj.get_integer_member("invalid");
  } catch (grt::bad_item &) {
    flag = true;
  };
  ensure("get invalid member", flag == true);

  flag = false;
  try {
    obj.set_member("title", IntegerRef(1234));
  } catch (type_error &) {
    flag = true;
  };
  ensure("set bad type1", flag == true);
  flag = false;
  try {
    obj.set_member("title", DoubleRef(1234.123));
  } catch (type_error &) {
    flag = true;
  };
  ensure("set bad type2", flag == true);
  flag = false;
  try {
    obj.set_member("price", StringRef("hello"));
  } catch (type_error &) {
    flag = true;
  };
  ensure("set bad type3", flag == true);
  flag = false;
  try {
    obj.set_member("authors", StringRef("joe"));
  } catch (read_only_item &) {
    flag = true;
  };
  ensure("set read-only", flag == true);
  flag = false;
  try {
    obj.set_member("pages", DoubleRef(1234.456));
  } catch (type_error &) {
    flag = true;
  };
  ensure("set bad type6", flag == true);
}

// generated object tests

TEST_FUNCTION(10) {
  test_BookRef book(grt::Initialized);

  book->title("Harry Potter");
  book->title(*book->title() + " XXV");
  book->price(500.23);
  ensure("bad title", !(*book->title() == "Harry Potter"));

  ensure("title", *book->title() == "Harry Potter XXV");

  test_AuthorRef author(grt::Initialized);

  book->authors().insert(author);
  ensure("author add", book->authors().count() == 1);

  book->authors().get(0)->name("J.K.Bowling");
  ensure("indirect author name", *author->name() == "J.K.Bowling");

  book->authors()[0]->name("ABC");
  ensure("indirect author name", *author->name() == "ABC");

  book->authors().remove(0);
  assure_equal(0U, book->authors().count());
}

static bool count_member(const grt::MetaClass::Member *member, int *count) {
  (*count)++;
  return true;
}

TEST_FUNCTION(11) {
  // Check if inherited values are properly initialized.
  test_BookRef book(grt::Initialized);

  int count = 0;
  book->get_metaclass()->foreach_member(std::bind(&count_member, std::placeholders::_1, &count));
  ensure_equals("book item count", count, 6);
}

/*
// bridged object test

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


TEST_FUNCTION(20)
{
  bool ret;

  ret= ObjectBridgeBase::register_bridge<TestBridge>;
  ensure_equals("bridge registration", ret, true);
}


TEST_FUNCTION(21)
{
  bool bridge_destroyed= false;

  {
    test_Bridged bridged;
    test_Book book;

    ensure_equals("bridge name", bridged.get_metaclass().get_metaclass()->bridge, "tut::TestBridge");

    ensure("bridge binding", bridged.get_bridge_private() != 0);

    book.title("Harry Potter");
    book.title(*book.title()+ " XXV");
    book.price(500.23);

    TestBridge *bridge_data;

    ensure("get_value", *bridged->name() == "hello");

    bridge_data= (TestBridge*)bridged.get_bridge_private();

    ensure("get private", bridge_data!=0);

    bridge_data->flag= &bridge_destroyed;

    ensure("check private", bridge_data->myname == bridged->name());

    bridged.name("xyz");
    ensure("check change", *bridge_data->myname == "xyz");

    bridged.x(1234);
    ensure_equals("change x", bridged.x(), 1234);

    ensure_equals("list count", (int)bridged.books().count(), 0);

    bridged.books().insert(book);

    ensure_equals("list count after add", (int)bridged.books().count(), 1);

    ensure_equals("book title", *bridged.books().get(0).title(),
                  "Harry Potter XXV");

    bridged.books().remove(0);

    ensure_equals("list count after del", (int)bridged.books().count(), 0);

    try
    {
      bridged.books().remove(0);
      ensure("del invalid item", false);
    }
    catch (grt::bad_item &)
    {
    }

    ensure_equals("bridged item count", bridged.count_members(), 4U);

//    ensure("stub", false);

    // XXX test bridged.get_member(), bridged.get_member_by_index()



  }
  // leaving the context should destroy the objects

  ensure_equals("bridge destroy callback", bridge_destroyed, true);
}


TEST_FUNCTION(22)
{
  //XXX test all myx_grt_obj_* functions with a bridged object
//  ensure("stub", false);
}
*/

END_TESTS
