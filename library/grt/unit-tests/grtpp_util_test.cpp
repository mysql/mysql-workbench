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
#include "grt_test_utility.h"
#include "structs.test.h"
#include "grtpp_util.h"

using namespace grt;

BEGIN_TEST_DATA_CLASS(grtpp_util_test)
public:
END_TEST_DATA_CLASS

TEST_MODULE(grtpp_util_test, "GRT: util functions");

TEST_FUNCTION(1) {
  // Load test data.
  grt::GRT::get()->load_metaclasses("data/structs.test.xml");
  grt::GRT::get()->end_loading_metaclasses();
  ensure_equals("load structs", grt::GRT::get()->get_metaclasses().size(), 6U);
}

TEST_FUNCTION(2) { // set_value_by_path
  test_BookRef book(grt::Initialized);
  bool flag;

  flag = set_value_by_path(book, "/title", StringRef("TITLE"));
  ensure("set_value_by_path", flag);
  ensure_equals("set_value_by_path", *book->title(), "TITLE");

  flag = set_value_by_path(book, "/", StringRef("TITLE"));
  ensure("set_value_by_path", !flag);

  try {
    set_value_by_path(book, "/xxx", StringRef("TITLE"));
    ensure("set_value_by_path with bad path", false);
  } catch (grt::bad_item &) {
  }

  flag = set_value_by_path(book, "/title/x", StringRef("TITLE"));
  ensure("set_value_by_path with bad path 2", !flag);

  try {
    set_value_by_path(book, "/title", IntegerRef(1234));
    ensure("set_value_by_path with bad type", false);
  } catch (grt::type_error &) {
  }
}

// regression test for Bug #17324160    MySQL workbench loses connections list
TEST_FUNCTION(10) {
  test_PublisherRef publisher(grt::Initialized);
  test_BookRef book(grt::Initialized);

  book->title("testbook");
  publisher->name("testpub");
  publisher->books().insert(book);
  book->publisher(publisher);

  test_PublisherRef publisher_copy(grt::shallow_copy_object(publisher));

  ensure("copy", publisher_copy.id() != publisher.id());
  ensure_equals("copy name", *publisher_copy->name(), *publisher->name());
  ensure_equals("copy book list", publisher_copy->books().count(), 1U);
  ensure_equals("copy book list contents", publisher_copy->books()[0].id(), book.id());
  // The bug was that a shallow_copy would modify the referenced objects that would back-reference the copied object
  ensure_equals("don't modify owned objects Bug #17324160", book->publisher().id(), publisher.id());
}

END_TESTS
