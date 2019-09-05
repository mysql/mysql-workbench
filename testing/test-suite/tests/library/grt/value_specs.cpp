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

#include "base/log.h"

#include "grtpp_util.h"
#include "grt.h"
#include "structs.test.h"

#include "grt_values_test_data.h"
#include "grt_test_helpers.h"
#include "wb_test_helpers.h"

#include "casmine.h"

using namespace casmine;

namespace {

$ModuleEnvironment(GrtEnvironment) {};

base::Logger test_logger(".", getenv("WB_LOG_STDERR") != 0);

template<typename ItemType>
void test_list_value(grt::ListRef<ItemType>& lv, grt::Ref<ItemType> v[]) {
  lv.retain();

  $expect(lv.refcount()).toEqual(2);

  $expect(lv.count()).toEqual(0U);
  $expect(v[0].refcount()).toEqual(1);

  lv.insert(v[0]);

  $expect(lv.count()).toEqual(1U);
  $expect(v[0].refcount()).toEqual(2);

  lv.remove(0);

  $expect(lv.count()).toEqual(0U);
  $expect(v[0].refcount()).toEqual(1);

  lv.insert(v[0]);

  $expect(lv.count()).toEqual(1U);

  lv.insert(v[1]);

  $expect(lv.count()).toEqual(2U);

  lv.insert(v[2], 1); // insert after first index

  $expect(lv.count()).toEqual(3U);
  $expect(lv.get(1)).toEqual(v[2]);

  lv.insert(v[3], 0); // insert by first index

  $expect(lv.count()).toEqual(4U);
  $expect(lv.get(0)).toEqual(v[3]);

  lv.insert(v[4], 3); // insert before last index

  $expect(lv.count()).toEqual(5U);
  $expect(lv.get(3)).toEqual(v[4]);

  lv.insert(v[5], 5); // insert by last index

  $expect(lv.count()).toEqual(6U);
  $expect(lv.get(5)).toEqual(v[5]);

  // Odd syntax here as we are in a template function.
  $expect([&]() { lv.insert(v[6], 7); }).template toThrowError<grt::bad_item>("Index out of range");

  $expect(lv.count()).toEqual(6U);

  $expect(v[0].refcount()).toEqual(2);

  for (int n = (int)lv.count() - 1; n >= 0; n--) {
    lv.remove(n);
  }

  $expect(v[5].refcount()).toEqual(1);
  $expect(lv.count()).toEqual(0U);

  for (int n = 0; n < 6; n++) {
    lv.insert(v[n], n);
  }

  $expect(lv.count()).toEqual(6U);

  for (int n = 0; n < 6; n++) {
    $expect(v[n].refcount()).toEqual(2);
  }

  lv.set(3, v[5]);

  $expect(lv.get(3)).toEqual(v[5]);

  for (int i = 0; i < 8; i++) {
    int refcount = 0;
    switch (i) {
      case 3: {
        refcount = 1;
        break;
      } // was replaced in list by v[5]
      case 5: {
        refcount = 3;
        break;
      } // was also assigned to v[3]
      default: {
        refcount = (i > 5 ? 1 : 2);
        break;
      } // Everything else is not used.
    }
    $expect(v[i].refcount()).toBe(refcount);
  }

  $expect([&]() { lv.set(6, v[6]); }).template toThrowError<grt::bad_item>("Index out of range");
  $expect(lv.count()).toEqual(6U);

  while (lv.count())
    lv.remove(0);

  $expect(lv.refcount()).toEqual(2);
  $expect(lv.count()).toEqual(0U);

  for (int i = 0; i < 8; i++) {
    $expect(v[i].refcount()).toEqual(1);
  }
};

$describe("GRT values") {
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

  $it("base tests", []() {
    grt::IntegerRef iv(6666);

    $expect(iv.refcount()).toEqual(1);

    {
      grt::ValueRef tmp = iv;
      $expect(iv.refcount()).toEqual(2);
    }
    $expect(iv.refcount()).toEqual(1);

    iv.retain();
    iv.retain();
    iv.retain();
    $expect(iv.refcount()).toEqual(4);

    grt::internal::Value* value = iv.valueptr();
    iv.clear();

    $expect(iv.valueptr()).toBe(nullptr);
    $expect(value->refcount()).toEqual(3);

    value->release();
    value->release();
    value->release();

  });

  $it("integer value", [&]() {
    grt::IntegerRef iv(1234);
    ssize_t i;

    $expect(grt::IntegerRef(static_cast<grt::internal::Integer *>(nullptr)).is_valid()).toBe(false);
    $expect(grt::IntegerRef(0).is_valid()).toBe(true);
    $expect(grt::IntegerRef().is_valid()).toBe(false);

    i = iv;
    $expect(i).toBe(1234);

    $expect(iv.refcount()).toEqual(1);

    grt::IntegerRef iv2(iv);
    $expect(iv2.valueptr()).toBe(iv.valueptr());
    $expect(iv.refcount()).toEqual(2);

    grt::IntegerRef iv3;
    $expect(iv3.valueptr()).toBe(nullptr);

    $expect(grt::IntegerRef::cast_from(iv3).is_valid()).toBe(false);

    iv3 = iv;
    $expect(iv3.valueptr()).toBe(iv.valueptr());

    iv3 = 5;
    $expect(iv3).toEqual(5);

    iv3 = iv3 + 10;
    $expect(iv3).toEqual(15);

    iv2 = 0;
    $expect(iv.refcount()).toEqual(1);

    grt::StringRef s("hi");
    $expect(iv.can_wrap(s)).toBeFalse();

    $expect([&]() { iv.cast_from(s); }).toThrow();
  });

  $it("double value", [&]() {
    grt::DoubleRef iv(1234.5678);
    double i;

    // will not compile $expect("DoubleRef(0)", DoubleRef(0).is_valid());
    $expect(grt::DoubleRef(static_cast<grt::internal::Double *>(nullptr)).is_valid()).toBe(false);
    $expect(grt::DoubleRef(0.0).is_valid()).toBe(true);
    $expect(grt::DoubleRef().is_valid()).toBe(false);

    i = iv;
    $expect(i).toBe(1234.5678);

    grt::DoubleRef iv2(iv);
    $expect(iv2.valueptr()).toBe(iv.valueptr());
    $expect(iv.refcount()).toEqual(2);

    grt::DoubleRef iv3;
    $expect(iv3.valueptr()).toBe(nullptr);

    iv3 = iv;
    $expect(iv3.valueptr()).toBe(iv.valueptr());

    iv3 = 1.5;
    $expect((double)iv3).toBe(1.5);

    iv3 = iv3 + 10;
    $expect((double)iv3).toBe(11.5);

    grt::IntegerRef v(1234);
    $expect(!grt::DoubleRef::can_wrap(v)).toBe(true);

    $expect([&]() { iv.cast_from(v); }).toThrowError<grt::type_error>("Type mismatch: expected type real, but got int");
  });

  $it("string value", [&]() {
    grt::StringRef iv("hello");
    std::string s;

    $expect(grt::StringRef(static_cast<grt::internal::String *>(nullptr)).is_valid()).toBe(false);
    $expect(grt::StringRef().is_valid()).toBe(false);
    $expect(grt::StringRef("").is_valid()).toBe(true);

    s = iv;
    $expect(s).toBe("hello");

    grt::StringRef iv2(iv);
    $expect(iv2.valueptr()).toBe(iv.valueptr());
    $expect(iv.refcount()).toEqual(2);

    grt::StringRef iv3;
    $expect(iv3.valueptr()).toBe(nullptr);

    iv3 = iv;
    $expect(iv3.valueptr()).toBe(iv.valueptr());

    iv3 = std::string("test");
    $expect((std::string)iv3).toBe("test");

    iv3 = (std::string)iv + " world";
    $expect((std::string)iv3).toBe("hello world");

    grt::IntegerRef v(1234);
    $expect(!grt::StringRef::can_wrap(v)).toBe(true);

    $expect([&]() { iv.cast_from(v); }).toThrowError<grt::type_error>("Type mismatch: expected type string, but got int");
  });


  $it("operator == ()", [&]() {
    grt::ValueRef tmp1, tmp2;

    $expect(tmp1).toEqual(tmp2);

    grt::IntegerRef i1234(1234);
    grt::IntegerRef i1235(1235);

    $expect(i1234 == grt::IntegerRef()).toBe(false);
    $expect(i1234).Not.toEqual(grt::IntegerRef());

    $expect(i1234 == i1234).toBe(true);
    $expect(i1234 != i1234).toBeFalse();
    $expect(i1234).toEqual(grt::IntegerRef(1234));
    $expect(i1234 != grt::IntegerRef(1234)).toBeFalse();
    $expect(i1234).toEqual(1234);
    $expect(i1234 != 1234).toBeFalse();
    $expect(i1234).Not.toEqual(0);
    $expect(i1234 == 0).toBeFalse();
    $expect(i1234 == i1235).toBeFalse();
    $expect(i1234).Not.toEqual(i1235);
    tmp1 = i1234;
    tmp2 = grt::IntegerRef(1234);
    $expect(tmp1).toEqual(tmp2);
    $expect(tmp1 != tmp2).toBeFalse();
    tmp2 = grt::IntegerRef(111);
    $expect(tmp1 == tmp2).toBeFalse();
    $expect(tmp1).Not.toEqual(tmp2);

    grt::DoubleRef d1234(123.4);
    grt::DoubleRef d1235(123.5);

    $expect(d1234).Not.toEqual(grt::DoubleRef());
    $expect(d1234).toEqual(d1234);
    $expect(d1234 != d1234).toBeFalse();
    $expect(d1234).toEqual(grt::DoubleRef(123.4));
    $expect(d1234 != grt::DoubleRef(123.4)).toBeFalse();
    $expect(d1234).toEqual(123.4);
    $expect(d1234 != 123.4).toBeFalse();
    $expect(d1234).Not.toEqual(0.0);
    $expect(d1234 == 0.0).toBeFalse();
    $expect(d1234 == d1235).toBeFalse();
    $expect(d1234).Not.toEqual(d1235);
    tmp1 = d1234;
    tmp2 = grt::DoubleRef(123.4);
    $expect(tmp1).toEqual(tmp2);
    $expect(tmp1 != tmp2).toBeFalse();
    tmp2 = grt::DoubleRef(111);
    $expect(tmp1 == tmp2).toBeFalse();
    $expect(tmp1).Not.toEqual(tmp2);

    grt::StringRef sbar("bar");
    grt::StringRef sbaz("baz");

    $expect(grt::StringRef()).toEqual(grt::StringRef(), "Unexpected difference for invalid StringRef instances");
    $expect(sbar == grt::StringRef()).toBeFalse();
    $expect(sbar).Not.toEqual(grt::StringRef());
    $expect(sbar).toEqual(sbar);
    $expect(sbar != sbar).toBeFalse();
    $expect(sbar).toEqual(grt::StringRef("bar"));
    $expect(sbar != grt::StringRef("bar")).toBeFalse();
    $expect(sbar).toEqual("bar");
    $expect(sbar).Not.toEqual("baz");
    $expect(sbar == sbaz).toBeFalse();
    $expect(sbar).Not.toEqual(sbaz);
    tmp1 = sbar;
    tmp2 = grt::StringRef("bar");
    $expect(tmp1).toEqual(tmp2);
    $expect(tmp1 != tmp2).toBeFalse();
    tmp2 = grt::StringRef("BAR");
    $expect(tmp1 == tmp2).toBeFalse();
    $expect(tmp1).Not.toEqual(tmp2);

    grt::BaseListRef l1(true);
    grt::BaseListRef l2(true);

    grt::BaseListRef l3;

    $expect(l1 == l3).toBeFalse();
    $expect(l1).Not.toEqual(l2);
    l3 = l1;
    $expect(l1).toEqual(l3);
    $expect(l1 != l3).toBeFalse();

    tmp1 = l1;
    tmp2 = l1;
    $expect(tmp1).toEqual(tmp2);
    $expect(tmp1 != tmp2).toBeFalse();
    tmp2 = l2;
    $expect(tmp1 == tmp2).toBeFalse();
    $expect(tmp1).Not.toEqual(tmp2);

    grt::DictRef d1(true);
    grt::DictRef d2(true);
    grt::DictRef d3;

    $expect(d1 == d3).toBeFalse();
    $expect(d1).Not.toEqual(d2);
    d3 = d1;
    $expect(d1).toEqual(d3);
    $expect(d1 != d3).toBeFalse();

    tmp1 = d1;
    tmp2 = d1;
    $expect(tmp1).toEqual(tmp2);
    $expect(tmp1 != tmp2).toBeFalse();
    tmp2 = d2;
    $expect(tmp1 == tmp2).toBeFalse();
    $expect(tmp1).Not.toEqual(tmp2);

    test_BookRef o1(grt::Initialized);
    test_BookRef o2(grt::Initialized);
    test_BookRef o3;

    $expect(o1 == o3).toBeFalse();
    //    $expect(o1).Not.toBe(o2);    TODO: Fix this...it's throwing an exception
    o3 = o1;
    //    $expect(o1).toBe(o3);          TODO: Fix this...it's throwing an exception
    $expect(o1 != o3).toBeFalse();

    tmp1 = o1;
    tmp2 = o1;
    //    $expect(tmp1).toBe(tmp2);      TODO: Fix this...it's throwing an exception
    $expect(tmp1 != tmp2).toBeFalse();
    tmp2 = o2;
    $expect(tmp1 == tmp2).toBeFalse();
    //    $expect(tmp1).Not.toBe(tmp2);    TODO: Fix this...it's throwing an exception

    // compare different types
    grt::ValueRef x[6] = {i1234, d1234, sbar, l1, d1, o1};

    for (int i = 0; i < 6; i++) {
      for (int j = 0; j < 6; j++) {
        if (i == j)
          continue;

        //        $expect(x[i]).Not.toBe(x[j]);    TODO: Fix this...it's throwing an exception
        $expect(x[i] == x[j]).toBeFalse();
      }
    }
  });

  $it("operator < ()", [&]() {
    grt::ValueRef tmp1, tmp2;
    grt::IntegerRef i1(10), i2(11);
    grt::DoubleRef d1(10.1), d2(10.2);
    grt::StringRef s1("aaa"), s2("bbb");
    grt::BaseListRef l1(true), l2(true);
    grt::DictRef t1(true), t2(true);
    test_BookRef o1(grt::Initialized), o2(grt::Initialized);

    $expect(i1).toBeLessThan(i2);
    $expect(i2 < i1).toBeFalse();
    tmp1 = i1;
    tmp2 = i2;
    $expect(tmp1).toBeLessThan(tmp2);
    $expect(tmp2 < tmp1).toBeFalse();

    $expect(d1).toBeLessThan(d2);
    $expect(d2 < d1).toBeFalse();
    tmp1 = d1;
    tmp2 = d2;
    $expect(tmp1).toBeLessThan(tmp2);
    $expect(tmp2 < tmp1).toBeFalse();

    $expect(s1).toBeLessThan(s2);
    $expect(s2 < s1).toBeFalse();
    tmp1 = s1;
    tmp2 = s2;
    $expect(tmp1).toBeLessThan(tmp2);
    $expect(tmp2 < tmp1).toBeFalse();

    $expect((l1 < l2 || l2 < l1) && !(l1 < l2 && l2 < l1));
    tmp1 = s1;
    tmp2 = s2;
    $expect((tmp1 < tmp2 || tmp2 < tmp1) && !(tmp1 < tmp2 && tmp2 < tmp1)).toBeTrue();

    $expect((t1 < t2 || t2 < t1) && !(t1 < t2 && t2 < t1)).toBeTrue();
    tmp1 = t1;
    tmp2 = t2;
    $expect((tmp1 < tmp2 || tmp2 < tmp1) && !(tmp1 < tmp2 && tmp2 < tmp1)).toBeTrue();

    $expect((o1 < o2 || o2 < o1) && !(o1 < o2 && o2 < o1)).toBeTrue();
    tmp1 = o1;
    tmp2 = o2;
    $expect((tmp1 < tmp2 || tmp2 < tmp1) && !(tmp1 < tmp2 && tmp2 < tmp1)).toBeTrue();

    grt::ValueRef x[6] = {i1, d1, s1, l1, t1, o1};

    for (int i = 0; i < 6; i++) {
      for (int j = 0; j < 6; j++) {
        if (i < j) {
          $expect(x[i]).toBeLessThan(x[j]);
          $expect(x[j] < x[i]).toBeFalse();
        } else if (i > j) {
          $expect(x[j]).toBeLessThan(x[i]);
          $expect(x[i] < x[j]).toBeFalse();
        } else {
          $expect(x[j] < x[i] || x[i] < x[j]).toBeFalse();
        }
      }
    }
  });

  $it("set/get value by path", [&]() {
    grt::DictRef root(grt::DictRef::cast_from(create_grt_tree1()));
    grt::ValueRef value;

    value = get_value_by_path(root, "/");

    $expect(value.is_valid()).toBeTrue();
    $expect(value.valueptr()).toBe(root.valueptr());

    value = get_value_by_path(root, "/books");
    $expect(value.is_valid()).toBeTrue();
    $expect(value.valueptr()).toBe(root.get("books").valueptr());

    value = get_value_by_path(root, "/books/0");
    $expect(value.is_valid()).toBeTrue();
    $expect(test_BookRef::can_wrap(value)).toBeTrue();
  });

  // For list test:
  //
  // - create untyped list
  // - add values to list end
  // - insert value in random order
  // - remove values from list
  // - get back inserted values
  // - set new item
  // - check refcount of inserted values
  // - count items
  // - destroy value
  // - create typed list
  // - test duplication
  $it("BaseListRef", [&]() {
    grt::BaseListRef lv(grt::AnyType);
    lv.retain();
    grt::IntegerRef iv[10] = {100, 101, 102, 3, 4, 5, 6, 7, 8, 9};

    $expect(lv.count()).toBe(0U);
    $expect(lv.refcount()).toEqual(2);
    $expect(iv[0].refcount()).toEqual(1);

    lv.ginsert(iv[0]);

    $expect(lv.count()).toBe(1U);
    $expect(iv[0].refcount()).toEqual(2);

    lv.remove(0);

    $expect(lv.count()).toEqual(0U);
    $expect(iv[0].refcount()).toEqual(1);

    { // just to ensure all auxiliary IntegerRef objects will release their references
      lv.ginsert(iv[0]);

      $expect(lv.count()).toEqual(1U);

      lv.ginsert(iv[1]);

      $expect(lv.count()).toEqual(2U);

      lv.ginsert(iv[2], 1); // insert after first index

      $expect(lv.count()).toEqual(3U);
      $expect(grt::IntegerRef::cast_from(lv.get(1))).toEqual(102);

      lv.ginsert(iv[3], 0); // insert by first index

      $expect(lv.count()).toEqual(4U);
      $expect(grt::IntegerRef::cast_from(lv.get(0))).toEqual(3);

      lv.ginsert(iv[4], 3); // insert before last index

      $expect(lv.count()).toEqual(5U);
      $expect(grt::IntegerRef::cast_from(lv.get(3))).toEqual(4);

      lv.ginsert(iv[5], 5); // insert by last index

      $expect(lv.count()).toEqual(6U);
      $expect(grt::IntegerRef::cast_from(lv.get(5))).toEqual(5);
    }

    $expect([&]() { lv.ginsert(iv[6], 7); }).toThrowError<grt::bad_item>("Index out of range");
    $expect(lv.count()).toEqual(6U);

    $expect(iv[0].refcount()).toEqual(2);

    for (int n = (int)lv.count() - 1; n >= 0; n--)
      lv.remove(n);

    $expect(iv[5].refcount()).toEqual(1);
    $expect(lv.count()).toEqual(0U);

    for (int n = 0; n < 6; n++)
      lv.ginsert(iv[n], n);

    $expect(lv.count()).toEqual(6U);

    for (int n = 0; n < 6; n++) {
      $expect(iv[n].refcount()).toEqual(2);
    }

    lv.gset(3, iv[5]);

    $expect(grt::IntegerRef::cast_from(lv.get(3))).toEqual(5);

    for (int i = 0; i < 8; i++) {
      int refcount = 0;
      switch (i) {
        case 3: {
          refcount = 1;
          break;
        } // was replaced in list by iv[5]
        case 5: {
          refcount = 3;
          break;
        } // was also assigned to iv[3]
        default: {
          refcount = (i > 5 ? 1 : 2);
          break;
        } // beginning from 6 were not used
      }

      $expect(iv[i].refcount()).toBe(refcount);
    }

    $expect([&]() { lv.gset(6, iv[6]); }).toThrowError<grt::bad_item>("Index out of range");
    $expect(lv.count()).toEqual(6U);

    while (lv.count())
      lv.remove(0);

    $expect(lv.refcount()).toEqual(2);
    $expect(lv.count()).toEqual(0U);

    for (int i = 0; i < 8; i++) {
      $expect(iv[i].refcount()).toEqual(1);
    }
  });


  $it("ValueList<IntegerRef>", [&]() {
    grt::IntegerListRef lv(grt::Initialized);
    grt::IntegerRef v[10] = {100, 101, 102, 3, 4, 5, 6, 7, 8, 9};
    test_list_value(lv, v);
  });

  $it("ValueList<DoubleRef>", [&]() {
    grt::DoubleListRef lv(grt::Initialized);
    grt::DoubleRef v[10] = {0.5, 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5, 9.5};
    test_list_value(lv, v);
  });

  $it("ValueList<StringRef>", [&]() {
    grt::StringListRef lv(grt::Initialized);
    grt::StringRef v[10] = {"_0", "_1", "_2", "_3", "_4", "_5", "_6", "_7", "_8", "_9"};
    test_list_value(lv, v);
  });

  $it("ListRef<ObjectRef>", [&]() {
    grt::ListRef<grt::internal::Object> lv(true);
    grt::ObjectRef v[10] = {grt::GRT::get()->create_object<grt::internal::Object>("test.Book"),
      grt::GRT::get()->create_object<grt::internal::Object>("test.Book"),
      grt::GRT::get()->create_object<grt::internal::Object>("test.Book"),
      grt::GRT::get()->create_object<grt::internal::Object>("test.Book"),
      grt::GRT::get()->create_object<grt::internal::Object>("test.Book"),
      grt::GRT::get()->create_object<grt::internal::Object>("test.Book"),
      grt::GRT::get()->create_object<grt::internal::Object>("test.Book"),
      grt::GRT::get()->create_object<grt::internal::Object>("test.Book"),
      grt::GRT::get()->create_object<grt::internal::Object>("test.Book"),
      grt::GRT::get()->create_object<grt::internal::Object>("test.Book")};
    // for (int i= 0; i<10; i++)
    //  v[i].release();
    test_list_value(lv, v);
    // for (int i= 0; i<10; i++)
    //  v[i].retain();
  });

  $it("typed list checks", [&]() {
    grt::BaseListRef ut(grt::AnyType);

    $expect((int)ut.content_type()).toBe(grt::AnyType);
    $expect(ut.content_class_name()).toBe("");

    grt::IntegerListRef il(grt::Initialized);
    $expect((int)il.content_type()).toBe(grt::IntegerType);
    $expect(il.content_class_name()).toBe("");

    grt::DoubleListRef dl(grt::Initialized);
    $expect((int)dl.content_type()).toBe(grt::DoubleType);
    $expect(dl.content_class_name()).toBe("");

    grt::StringListRef sl(grt::Initialized);
    $expect((int)sl.content_type()).toBe(grt::StringType);
    $expect(sl.content_class_name()).toBe("");

    // not supported
    // DictListRef cl(grt::Initialized);
    // ensure_equals("dict list content", cl.content_type(), DictType);
    // ensure_equals("dict list struct", cl.content_class_name(), "");

    grt::ObjectListRef ol(true);
    $expect((int)ol.content_type()).toBe(grt::ObjectType);
    $expect(ol.content_class_name()).toBe("Object");

    grt::ListRef<test_Author> al(true);
    $expect((int)al.content_type()).toBe(grt::ObjectType);
    $expect(al.content_class_name()).toBe("test.Author");
  });

  $it("check can_wrap for lists", [&]() {
    $expect(grt::ObjectListRef::can_wrap(grt::ObjectListRef(true))).toBeTrue();
    $expect(grt::ListRef<test_Book>::can_wrap(grt::ObjectListRef(true))).toBeFalse();
    $expect(grt::ObjectListRef::can_wrap(grt::ListRef<test_Book>(true))).toBeTrue();
    $expect(grt::ListRef<test_Publication>::can_wrap(grt::ListRef<test_Book>(true))).toBeTrue();
    $expect(grt::ListRef<test_Book>::can_wrap(grt::ListRef<test_Publication>(true))).toBeFalse();
    $expect(grt::BaseListRef::can_wrap(grt::ListRef<test_Book>(true))).toBeTrue();
    $expect(grt::ListRef<test_Book>::can_wrap(grt::BaseListRef(true))).toBeFalse();

    $expect(grt::ListRef<test_Publication>::can_wrap(grt::IntegerListRef(grt::Initialized))).toBeFalse();
    $expect(grt::IntegerListRef::can_wrap(grt::ListRef<test_Publication>(true))).toBeFalse();

    $expect(grt::StringListRef::can_wrap(grt::IntegerListRef(grt::Initialized))).toBeFalse();
    $expect(grt::IntegerListRef::can_wrap(grt::StringListRef(grt::Initialized))).toBeFalse();
  });

  // For dict test:
  //
  // - create untyped dict
  // - put stuff in it
  // - remove stuff
  // - overwrite stuff
  // - destroy
  // - duplication
  $it("dictionary", [&]() {
    grt::DictRef dv(true);
    grt::IntegerRef iv[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    grt::StringRef sv[10] = {"_0", "_1", "_2", "_3", "_4", "_5", "_6", "_7", "_8", "_9"};
    grt::StringRef k[10] = {"_0", "_1", "_2", "_3", "_4", "_5", "_6", "_7", "_8", "_9"};
    grt::ObjectRef obj(grt::GRT::get()->create_object<grt::internal::Object>("test.Book"));

    $expect(dv.count()).toEqual(0U);

    dv.retain();

    $expect(dv.refcount()).toEqual(2);

    for (int i = 0; i < 30; i += 3) // simulate random inserts
    {
      int n = i % 10;
      dv.set(k[n], iv[n]);
    }

    $expect(dv.count()).toBe(10U);

    for (int i = 2; i < 10; i++) {// skip 0 and 1 because they're internally cached
      $expect(iv[i].refcount()).toEqual(2);
    }

    for (int i = 0; i < 30; i += 3) // simulate random overwrites
    {
      int n = i % 10;
      dv.set(k[n], obj);
    }

    $expect(dv.count()).toEqual(10U);

    for (int i = 2; i < 10; i++) {
      $expect(iv[i].refcount()).toEqual(1);
    }

    $expect(obj.refcount()).toBe(11);

    for (int i = 0; i < 30; i += 3) // simulate random overwrites
    {
      int n = i % 10;
      dv.set(k[n], sv[n]);
    }

    $expect(dv.count()).toEqual(10U);
    $expect(obj.refcount()).toEqual(1);

    for (int i = 2; i < 10; i++) {
      $expect(sv[i].refcount()).toEqual(2);
    }

    for (int i = 0; i < 10; i++)
      dv.remove(k[i]);

    $expect(dv.refcount()).toEqual(2);
    $expect(dv.count()).toEqual(0U);

    for (int i = 2; i < 10; i++) {
      $expect(iv[i].refcount()).toEqual(1);
    }

    for (int i = 0; i < 10; i++) {
      $expect(sv[i].refcount()).toEqual(1);
    }

    // make sure that cached values are still there
    for (int i = 0; i < 2; i++) {
      $expect(iv[i].refcount()).toBeGreaterThan(1);
    }

    $expect(obj.refcount()).toEqual(1);
  });


  $it("test list reordering", [&]() {

    grt::IntegerListRef lv(grt::Initialized);
    grt::IntegerRef iv[4] = {0, 1, 2, 3};

    while (lv.count())
      lv.remove(0);

    for (int i = 0; i < 4; i++)
      lv.insert(iv[i]);

    lv.reorder(0, 4);

    $expect(*lv.get(0)).toEqual(1U);
    $expect(*lv.get(1)).toEqual(2U);
    $expect(*lv.get(2)).toEqual(3U);
    $expect(*lv.get(3)).toEqual(0U);

    while (lv.count())
      lv.remove(0);

    for (int i = 0; i < 4; i++)
      lv.insert(iv[i]);

    lv.reorder(1, 2);

    $expect(*lv.get(0)).toEqual(0U);
    $expect(*lv.get(1)).toEqual(2U);
    $expect(*lv.get(2)).toEqual(1U);
    $expect(*lv.get(3)).toEqual(3U);

    while (lv.count())
      lv.remove(0);

    for (int i = 0; i < 4; i++)
      lv.insert(iv[i]);

    lv.reorder(2, 1);

    $expect(*lv.get(0)).toEqual(0U);
    $expect(*lv.get(1)).toEqual(2U);
    $expect(*lv.get(2)).toEqual(1U);
    $expect(*lv.get(3)).toEqual(3U);

    while (lv.count())
      lv.remove(0);

    for (int i = 0; i < 4; i++)
      lv.insert(iv[i]);

    lv.reorder(3, 0);

    $expect(*lv.get(0)).toEqual(3U);
    $expect(*lv.get(1)).toEqual(0U);
    $expect(*lv.get(2)).toEqual(1U);
    $expect(*lv.get(3)).toEqual(2U);
  });

}

}
