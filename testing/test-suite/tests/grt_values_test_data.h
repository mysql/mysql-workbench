/*
 * Copyright (c) 2005, 2019, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include "grt.h"
#include "structs.test.h"

inline grt::BaseListRef create_list_with_varied_data() {
  grt::BaseListRef l(true);

  // 1
  l.ginsert(grt::IntegerRef(111));

  // 2
  l.ginsert(grt::IntegerRef(222));

  // 3
  l.ginsert(grt::StringRef("test"));

  // 4
  l.ginsert(grt::DoubleRef(123.456));

  // 5
  l.ginsert(grt::StringRef("test"));

  // 6
  grt::IntegerListRef il(grt::Initialized);

  for (int i = 0; i < 5; i++)
    il.insert(i);
  l.ginsert(il);

  // 7
  grt::DictRef d(true);

  d.set("item1", grt::IntegerRef(1));
  d.set("item2", grt::DoubleRef(2.2));
  d.set("item3", grt::StringRef("test"));
  l.ginsert(d);

  // 8
  l.ginsert(grt::StringRef("test"));

  // 9
  l.ginsert(grt::StringRef("test"));

  // 10
  l.ginsert(grt::StringRef("test"));

  return l;
}

inline grt::StringListRef create_string_list(int size) {
  grt::StringListRef list(grt::Initialized);

  for (int i = 0; i < size; i++) {
    char buffer[10];
    sprintf(buffer, "test%i", i + 1);
    list.insert(grt::StringRef(buffer));
  }

  return list;
}

inline grt::IntegerListRef create_int_list(int size) {
  grt::IntegerListRef list(grt::Initialized);

  for (int i = 0; i < size; i++) {
    list.insert(i);
  }
  return list;
}

inline grt::DictRef create_dict_with_varied_data() {
  grt::DictRef dict(true);

  // out of alphabetical order on purpose
  dict.set("k4", grt::IntegerRef(444));
  dict.set("k1", grt::IntegerRef(111));
  dict.set("k5", grt::StringRef("test5"));
  dict.set("k3", grt::StringRef("test3"));
  dict.set("k2", grt::DoubleRef(123.456));

  grt::IntegerListRef list(grt::Initialized);

  for (int i = 1; i <= 5; i++)
    list.insert(grt::IntegerRef(i));

  dict.set("k6", list);

  return dict;
}

inline grt::DictRef create_dict_with_int_data() {
  grt::DictRef dict(grt::IntegerType);

  // out of alphabetical order on purpose
  for (int i = 0; i < 9; i++) {
    char buffer[10];

    sprintf(buffer, "k%i", i + 1);

    dict.set(buffer, grt::IntegerRef(i + 1));
  }

  return dict;
}

inline grt::ValueRef create_grt_tree1() {
  grt::DictRef root(true);

  grt::ListRef<test_Book> book_list(true);

  test_BookRef book1(grt::Initialized);
  book1->title("Some Book");
  book_list.insert(book1);

  test_BookRef book2(grt::Initialized);
  book2->title("The Illiad");
  book_list.insert(book2);

  root.set("books", book_list);

  root.set("somelist", create_list_with_varied_data());

  root.set("somedict", create_dict_with_varied_data());

  return root;
}
