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
#include "structs.test.h"

BEGIN_TEST_DATA_CLASS(grt_struct)
public:
END_TEST_DATA_CLASS

TEST_MODULE(grt_struct, "GRT: structs/metaclasses");

TEST_FUNCTION(1) {
  try {
    // try to alloc something before loading metaclasses
    test_Book book;
    ensure("allocate obj before loading metaclasses", false);
  } catch (...) {
  }

  // Load test data.
  grt::GRT::get()->load_metaclasses("data/structs.test.xml");
  grt::GRT::get()->end_loading_metaclasses();
  ensure_equals("load structs", grt::GRT::get()->get_metaclasses().size(), 6U);
}

TEST_FUNCTION(2) {
  // Test valid struct creation and comparison to another struct.
  MetaClass *book(grt::GRT::get()->get_metaclass("test.Book"));

  ensure("get test.Book", book != 0);
  ensure_equals("name", book->name(), "test.Book");

  ensure("is_a test.Publishing", book->is_a(grt::GRT::get()->get_metaclass("test.Publication")));
  ensure("is_a test.Base", book->is_a(grt::GRT::get()->get_metaclass("test.Base")));
  ensure("is_a test.Base", book->is_a("test.Base"));
  ensure("is_a invalid", !book->is_a("XXXX"));

  ensure_equals("struct attribute", book->get_attribute("caption"), "Book");
  ensure_equals("bad struct attribute", book->get_attribute("xxx"), "");

  ensure_equals("parent", book->parent()->name(), "test.Publication");
}

TEST_FUNCTION(4) {
  // check has_member
}

TEST_FUNCTION(5) {
  // check get_member

  MetaClass *book = grt::GRT::get()->get_metaclass("test.Book");
  const MetaClass::Member *mem;

  mem = book->get_member_info("pages");
  ensure("member_info pages", mem != 0);

  mem = book->get_member_info("title");
  ensure("member_info title (inherited)", mem != 0);

  test_BookRef book_obj(grt::Initialized);

  book_obj->pages(1234);

  ensure_equals("get_member", *IntegerRef::cast_from(book->get_member_value(&book_obj.content(), "pages")), 1234);
  ensure_equals("pages", *book_obj->pages(), 1234);
}

TEST_FUNCTION(6) {
  // check set_member

  // from parent class

  // with override
}

TEST_FUNCTION(7) {
  // check allocation
}

TEST_FUNCTION(8) {
  // check method call
}

TEST_FUNCTION(9) {
  // check foreach_member
}

TEST_FUNCTION(20) {
  // Test struct members and their attributes.
  MetaClass *book(grt::GRT::get()->get_metaclass("test.Book"));
  const MetaClass::Member *m;
  grt::TypeSpec t;
  std::string a;

  m = book->get_member_info("authors");
  ensure("get_member authors", m != 0);

  m = book->get_member_info("title");
  ensure("get_member title", m != 0);

  t = book->get_member_type("authors");
  ensure_equals("get_member_type authors", t.base.type, ListType);

  t = book->get_member_type("title");
  ensure_equals("get_member_type title", t.base.type, StringType);

  // Member attributes.
  a = book->get_member_attribute("authors", "caption");
  ensure_equals("get_attr authors caption", a, "Authors");

  a = book->get_member_attribute("authors", "desc");
  ensure_equals("get_attr authors desc", a, "the list of authors");

  a = book->get_member_attribute("authors", "group");
  ensure_equals("get_attr authors group", a, "group1");

  a = book->get_member_attribute("title", "caption");
  ensure_equals("get_attr title caption", a, "Title");

  a = book->get_member_attribute("title", "desc");
  ensure_equals("get_attr title desc", a, "title of the book");

  a = book->get_member_attribute("title", "group");
  ensure_equals("get_attr title group", a, "");
}

TEST_FUNCTION(21) {
  // Check member completeness returned by Struct.
  // This is actually a generalization of test case 20.
  /*QQQ
  const MetaClass::Member *m;
  size_t count;
  MetaClass *book(grt::GRT::get()->get_metaclass("test.Book"));

  ensure_equals("test.Book member count", count, 6);
  MYX_GRT_STRUCT_MEMBER** member= members;
  for (size_t i= 0; i < count; i++, member++)
  {
    const MYX_GRT_STRUCT_MEMBER* book_member= book->get_member((*member)->name);
    ensure_equals("All struct members", book_member, *member);
  }

   */
}

END_TESTS
