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

#include "grtpp_util.h"

#include "testgrt.h"
#include "structs.test.h"
#include "grt_values_test_data.h"
#include "grt_test_utility.h"

BEGIN_TEST_DATA_CLASS(grt_value)
public:
END_TEST_DATA_CLASS

TEST_MODULE(grt_value, "GRT: values");

TEST_FUNCTION(1) {
  grt::GRT::get()->load_metaclasses("data/structs.test.xml");
  grt::GRT::get()->end_loading_metaclasses();

  ensure_equals("load structs", grt::GRT::get()->get_metaclasses().size(), 6U);
}

TEST_FUNCTION(2) {
  // Base ValueRef tests.
  IntegerRef iv(6666);

  ensure_equals("Reference count", iv.refcount(), 1);

  {
    ValueRef tmp = iv;
    ensure_equals("Reference count", iv.refcount(), 2);
  }
  ensure_equals("Reference count", iv.refcount(), 1);

  iv.retain();
  iv.retain();
  iv.retain();
  ensure_equals("Reference count", iv.refcount(), 4);

  grt::internal::Value* value = iv.valueptr();
  iv.clear();
  ensure("Completely release value", iv.valueptr() == NULL);
  ensure_equals("Reference count", value->refcount(), 3);

  value->release();
  value->release();
  value->release();
}

/*
 * For each simple value type test:
 *
 * - create value
 * - get value content
 * - destroy value
 * - reference counting (initial should be 1 etc)
 * - test duplication
 * - test == and < operators
 */

TEST_FUNCTION(4) {
  IntegerRef iv(1234);
  ssize_t i;

  ensure("IntegerRef(NULL)", !IntegerRef((grt::internal::Integer*)NULL).is_valid());
  ensure("IntegerRef(0)", IntegerRef(0).is_valid());
  ensure("IntegerRef()", !IntegerRef().is_valid());

  i = iv;
  ensure_equals("IntegerRef to int", i, 1234);

  ensure_equals("IntegerRef refcount", iv.refcount(), 1);

  IntegerRef iv2(iv);
  ensure("IntegerRef copy constr", iv2.valueptr() == iv.valueptr());
  ensure_equals("IntegerRef refcount+1", iv.refcount(), 2);

  IntegerRef iv3;
  ensure("empty constructor", iv3.valueptr() == 0);

  ensure("cast_from nil value", !IntegerRef::cast_from(iv3).is_valid());

  iv3 = iv;
  ensure("IntegerRef assign value", iv3.valueptr() == iv.valueptr());

  iv3 = 5;
  ensure_equals("IntegerRef assign immediate", iv3, 5);

  iv3 = iv3 + 10;
  ensure_equals("IntegerRef assign", iv3, 15);

  iv2 = 0;
  ensure_equals("IntegerRef refcount-1", iv.refcount(), 1);

  StringRef s("hi");
  bool fail = false;
  ensure_equals("IntegerRef can_wrap", iv.can_wrap(s), false);
  try {
    iv.cast_from(s);
  } catch (...) {
    fail = true;
  };
  ensure("IntegerRef bad assign", fail == true);
}

TEST_FUNCTION(5) {
  DoubleRef iv(1234.5678);
  double i;

  // will not compile ensure("DoubleRef(0)", DoubleRef(0).is_valid());
  ensure("DoubleRef(NULL)", !DoubleRef((grt::internal::Double*)NULL).is_valid());
  ensure("DoubleRef(0.0)", DoubleRef(0.0).is_valid());
  ensure("DoubleRef()", !DoubleRef().is_valid());

  i = iv;
  ensure_equals("DoubleRef to double", i, 1234.5678);

  DoubleRef iv2(iv);
  ensure("DoubleRef copy constr", iv2.valueptr() == iv.valueptr());
  ensure_equals("DoubleRef refcount+1", iv.refcount(), 2);

  DoubleRef iv3;
  ensure("empty constructor", iv3.valueptr() == 0);

  iv3 = iv;
  ensure("DoubleRef assign value", iv3.valueptr() == iv.valueptr());

  iv3 = 1.5;
  ensure_equals("DoubleRef assign immediate", (double)iv3, 1.5);

  iv3 = iv3 + 10;
  ensure_equals("DoubleRef assign", (double)iv3, 11.5);

  IntegerRef v(1234);
  ensure("DoubleRef cast from int", !DoubleRef::can_wrap(v));

  bool fail = false;
  try {
    iv.cast_from(v);
  } catch (...) {
    fail = true;
  };
  ensure("DoubleRef bad assign", fail == true);
}

TEST_FUNCTION(10) {
  StringRef iv("hello");
  std::string s;

  ensure("StringRef(NUL)", !StringRef((grt::internal::String*)NULL).is_valid());
  ensure("StringRef()", !StringRef().is_valid());
  ensure("StringRef(\"\")", StringRef("").is_valid());

  s = iv;
  ensure_equals("StringRef to string", s, "hello");

  StringRef iv2(iv);
  ensure("StringRef copy constr", iv2.valueptr() == iv.valueptr());
  ensure_equals("StringRef refcount+1", iv.refcount(), 2);

  StringRef iv3;
  ensure("empty constructor", iv3.valueptr() == 0);

  iv3 = iv;
  ensure("StringRef assign value", iv3.valueptr() == iv.valueptr());

  iv3 = std::string("test");
  ensure_equals("StringRef assign immediate", (std::string)iv3, "test");

  iv3 = (std::string)iv + " world";
  ensure_equals("StringRef assign", (std::string)iv3, "hello world");

  IntegerRef v(1234);
  ensure("StringRef cast from int", !StringRef::can_wrap(v));

  bool fail = false;
  try {
    iv.cast_from(v);
  } catch (...) {
    fail = true;
  };
  ensure("StringRef bad assign", fail == true);
}

TEST_FUNCTION(11) { // test == operator
  ValueRef tmp1, tmp2;

  ensure("null ==", tmp1 == tmp2);

  IntegerRef i1234(1234);
  IntegerRef i1235(1235);

  ensure("i == operator", !(i1234 == IntegerRef()));
  ensure("i != operator", i1234 != IntegerRef());

  ensure("i == operator", i1234 == i1234);
  ensure("i != operator", !(i1234 != i1234));
  ensure("i == operator", i1234 == IntegerRef(1234));
  ensure("i != operator", !(i1234 != IntegerRef(1234)));
  ensure("i == operator", i1234 == 1234);
  ensure("i != operator", !(i1234 != 1234));
  ensure("i != operator", i1234 != 0);
  ensure("i == operator", !(i1234 == 0));
  ensure("i == operator", !(i1234 == i1235));
  ensure("i != operator", (i1234 != i1235));
  tmp1 = i1234;
  tmp2 = IntegerRef(1234);
  ensure("i == operator with valueref", tmp1 == tmp2);
  ensure("i != operator with valueref", !(tmp1 != tmp2));
  tmp2 = IntegerRef(111);
  ensure("i == operator with valueref", !(tmp1 == tmp2));
  ensure("i != operator with valueref", (tmp1 != tmp2));

  DoubleRef d1234(123.4);
  DoubleRef d1235(123.5);

  ensure("d != operator", d1234 != DoubleRef());
  ensure("d == operator", d1234 == d1234);
  ensure("d != operator", !(d1234 != d1234));
  ensure("d == operator", d1234 == DoubleRef(123.4));
  ensure("d != operator", !(d1234 != DoubleRef(123.4)));
  ensure("d == operator", d1234 == 123.4);
  ensure("d != operator", !(d1234 != 123.4));
  ensure("d != operator", d1234 != 0.0);
  ensure("d == operator", !(d1234 == 0.0));
  ensure("d == operator", !(d1234 == d1235));
  ensure("d != operator", (d1234 != d1235));
  tmp1 = d1234;
  tmp2 = DoubleRef(123.4);
  ensure("d == operator with valueref", tmp1 == tmp2);
  ensure("d != operator with valueref", !(tmp1 != tmp2));
  tmp2 = DoubleRef(111);
  ensure("d == operator with valueref", !(tmp1 == tmp2));
  ensure("d != operator with valueref", (tmp1 != tmp2));

  StringRef sbar("bar");
  StringRef sbaz("baz");

  ensure("s == operator", StringRef() == StringRef());
  ensure("s == operator", !(sbar == StringRef()));
  ensure("s != operator", sbar != StringRef());
  ensure("s == operator", sbar == sbar);
  ensure("s != operator", !(sbar != sbar));
  ensure("s == operator", sbar == StringRef("bar"));
  ensure("s != operator", !(sbar != StringRef("bar")));
  ensure("s == operator", (sbar == "bar"));
  ensure("s != operator", (sbar != "baz"));
  ensure("s == operator", !(sbar == sbaz));
  ensure("s != operator", (sbar != sbaz));
  tmp1 = sbar;
  tmp2 = StringRef("bar");
  ensure("s == operator with valueref", tmp1 == tmp2);
  ensure("s != operator with valueref", !(tmp1 != tmp2));
  tmp2 = StringRef("BAR");
  ensure("s == operator with valueref", !(tmp1 == tmp2));
  ensure("s != operator with valueref", (tmp1 != tmp2));

  BaseListRef l1(true);
  BaseListRef l2(true);

  BaseListRef l3;

  ensure("l == operator", !(l1 == l3));
  ensure("l != operator", l1 != l2);
  l3 = l1;
  ensure("l == operator", (l1 == l3));
  ensure("l == operator", !(l1 != l3));

  tmp1 = l1;
  tmp2 = l1;
  ensure("l == operator with valueref", tmp1 == tmp2);
  ensure("l != operator with valueref", !(tmp1 != tmp2));
  tmp2 = l2;
  ensure("l == operator with valueref", !(tmp1 == tmp2));
  ensure("l != operator with valueref", (tmp1 != tmp2));

  DictRef d1(true);
  DictRef d2(true);
  DictRef d3;

  ensure("d == operator", !(d1 == d3));
  ensure("d != operator", d1 != d2);
  d3 = d1;
  ensure("d == operator", (d1 == d3));
  ensure("d != operator", !(d1 != d3));

  tmp1 = d1;
  tmp2 = d1;
  ensure("d == operator with valueref", tmp1 == tmp2);
  ensure("d != operator with valueref", !(tmp1 != tmp2));
  tmp2 = d2;
  ensure("d == operator with valueref", !(tmp1 == tmp2));
  ensure("d != operator with valueref", (tmp1 != tmp2));

  test_BookRef o1(grt::Initialized);
  test_BookRef o2(grt::Initialized);
  test_BookRef o3;

  ensure("o == operator", !(o1 == o3));
  ensure("o != operator", o1 != o2);
  o3 = o1;
  ensure("o == operator", (o1 == o3));
  ensure("o != operator", !(o1 != o3));

  tmp1 = o1;
  tmp2 = o1;
  ensure("o == operator with valueref", tmp1 == tmp2);
  ensure("o != operator with valueref", !(tmp1 != tmp2));
  tmp2 = o2;
  ensure("o == operator with valueref", !(tmp1 == tmp2));
  ensure("o != operator with valueref", (tmp1 != tmp2));

  // compare different types
  ValueRef x[6] = {i1234, d1234, sbar, l1, d1, o1};

  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 6; j++) {
      if (i == j)
        continue;

      ensure("compare objects of different type", x[i] != x[j]);
      ensure("compare objects of different type", !(x[i] == x[j]));
    }
  }
}

TEST_FUNCTION(12) {
  ValueRef tmp1, tmp2;
  IntegerRef i1(10), i2(11);
  DoubleRef d1(10.1), d2(10.2);
  StringRef s1("aaa"), s2("bbb");
  BaseListRef l1(true), l2(true);
  DictRef t1(true), t2(true);
  test_BookRef o1(grt::Initialized), o2(grt::Initialized);

  ensure("i <", i1 < i2);
  ensure("i <", !(i2 < i1));
  tmp1 = i1;
  tmp2 = i2;
  ensure("i <", tmp1 < tmp2);
  ensure("i <", !(tmp2 < tmp1));

  ensure("d <", d1 < d2);
  ensure("d < ", !(d2 < d1));
  tmp1 = d1;
  tmp2 = d2;
  ensure("d <", tmp1 < tmp2);
  ensure("d <", !(tmp2 < tmp1));

  ensure("s <", s1 < s2);
  ensure("s < ", !(s2 < s1));
  tmp1 = s1;
  tmp2 = s2;
  ensure("s <", tmp1 < tmp2);
  ensure("s <", !(tmp2 < tmp1));

  ensure("list <", (l1 < l2 || l2 < l1) && !(l1 < l2 && l2 < l1));
  tmp1 = s1;
  tmp2 = s2;
  ensure("list <", (tmp1 < tmp2 || tmp2 < tmp1) && !(tmp1 < tmp2 && tmp2 < tmp1));

  ensure("dict <", (t1 < t2 || t2 < t1) && !(t1 < t2 && t2 < t1));
  tmp1 = t1;
  tmp2 = t2;
  ensure("dict <", (tmp1 < tmp2 || tmp2 < tmp1) && !(tmp1 < tmp2 && tmp2 < tmp1));

  ensure("obj <", (o1 < o2 || o2 < o1) && !(o1 < o2 && o2 < o1));
  tmp1 = o1;
  tmp2 = o2;
  ensure("obj <", (tmp1 < tmp2 || tmp2 < tmp1) && !(tmp1 < tmp2 && tmp2 < tmp1));

  ValueRef x[6] = {i1, d1, s1, l1, t1, o1};

  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 6; j++) {
      if (i < j) {
        ensure("<", x[i] < x[j]);
        ensure("<", !(x[j] < x[i]));
      } else if (i > j) {
        ensure("<", x[j] < x[i]);
        ensure("<", !(x[i] < x[j]));
      } else
        ensure("!<", !(x[j] < x[i] || x[i] < x[j]));
    }
  }
}

TEST_FUNCTION(20) {
  // set/get value by path

  DictRef root(DictRef::cast_from(create_grt_tree1()));
  ValueRef value;

  value = get_value_by_path(root, "/");

  ensure("get /", value.is_valid());
  ensure("value of /", value.valueptr() == root.valueptr());

  value = get_value_by_path(root, "/books");
  ensure("get /books", value.is_valid());
  ensure("value of /books", value.valueptr() == root.get("books").valueptr());

  value = get_value_by_path(root, "/books/0");
  ensure("get /books/0", value.is_valid());
  ensure("value of /books/0", test_BookRef::can_wrap(value));
}

template <class ItemType>
void test_list_value(ListRef<ItemType>& lv, Ref<ItemType> v[]) {
  Type grt_type(v[0].type());
  lv.retain();
  ensure_equals(format_msg("retained list refcount", NULL, grt_type), lv.refcount(), 2);

  ensure_equals(format_msg("list initial items count", NULL, grt_type), lv.count(), 0U);
  ensure_equals(format_msg("var refcount before insert to list", NULL, grt_type), v[0].refcount(), 1);
  lv.insert(v[0]);
  ensure_equals(format_msg("list count after insert to list", NULL, grt_type), lv.count(), 1U);
  ensure_equals(format_msg("var refcount after insert to list", NULL, grt_type), v[0].refcount(), 2);
  lv.remove(0);
  ensure_equals(format_msg("list count after remove from list", NULL, grt_type), lv.count(), 0U);
  ensure_equals(format_msg("var refcount after remove from list", NULL, grt_type), v[0].refcount(), 1);

  lv.insert(v[0]);
  ensure_equals(format_msg("list items count after random insert", NULL, grt_type), lv.count(), 1U);
  lv.insert(v[1]);
  ensure_equals(format_msg("list items count after random insert", NULL, grt_type), lv.count(), 2U);
  lv.insert(v[2], 1); // insert after first index
  ensure_equals(format_msg("list items count after random insert", NULL, grt_type), lv.count(), 3U);
  ensure(format_msg("list item value after random insert", NULL, grt_type), lv.get(1) == v[2]);
  lv.insert(v[3], 0); // insert by first index
  ensure_equals(format_msg("list items count after random insert", NULL, grt_type), lv.count(), 4U);
  ensure(format_msg("list item value after random insert", NULL, grt_type), lv.get(0) == v[3]);
  lv.insert(v[4], 3); // insert before last index
  ensure_equals(format_msg("list items count after random insert", NULL, grt_type), lv.count(), 5U);
  ensure(format_msg("list item value after random insert", NULL, grt_type), lv.get(3) == v[4]);
  lv.insert(v[5], 5); // insert by last index
  ensure_equals(format_msg("list items count after random insert", NULL, grt_type), lv.count(), 6U);
  ensure(format_msg("list item value after random insert", NULL, grt_type), lv.get(5) == v[5]);

  bool exception_thrown(false);
  try {
    lv.insert(v[6], 7); // invalid index
  } catch (grt::bad_item&) {
    exception_thrown = true;
  } catch (...) {
  }
  ensure(format_msg("try to insert value by invalid index (out of bounds)", NULL, grt_type), exception_thrown);
  ensure_equals(format_msg("list items count after try to insert new item by invalid index", NULL, grt_type),
                lv.count(), 6U);

  ensure_equals(format_msg("var refcount before remove from list", NULL, grt_type), v[0].refcount(), 2);
  for (int n = (int)lv.count() - 1; n >= 0; n--)
    lv.remove(n);
  ensure_equals(format_msg("var refcount after remove from list", NULL, grt_type), v[5].refcount(), 1);
  ensure_equals(format_msg("list items count after remove all items", NULL, grt_type), lv.count(), 0U);

  for (int n = 0; n < 6; n++)
    lv.insert(v[n], n);
  ensure_equals(format_msg("list items count after insert", NULL, grt_type), lv.count(), 6U);
  for (int n = 0; n < 6; n++)
    ensure_equals(format_msg("var refcount after insert to list", NULL, grt_type, n), v[n].refcount(), 2);

  lv.set(3, v[5]);
  ensure(format_msg("list item value after being overwritten", NULL, grt_type), lv.get(3) == v[5]);
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
      } // beggining from 6 were not used
    }
    ensure_equals(format_msg("var refcount after misc overwriting", NULL, grt_type, i), v[i].refcount(), refcount);
  }

  exception_thrown = false;
  try {
    lv.set(6, v[6]); // invalid index
  } catch (grt::bad_item&) {
    exception_thrown = true;
  } catch (...) {
  }
  ensure(format_msg("try to set item by invalid index (out of bounds)", NULL, grt_type), exception_thrown);
  ensure_equals(format_msg("list items count after try to set item by invalid index", NULL, grt_type), lv.count(), 6U);

  while (lv.count())
    lv.remove(0);
  ensure_equals(format_msg("list refcount after retained list release", NULL, grt_type), lv.refcount(), 2);
  ensure_equals(format_msg("list items count after list release", NULL, grt_type), lv.count(), 0U);
  for (int i = 0; i < 8; i++)
    ensure_equals(format_msg("var refcount after list release", NULL, grt_type, i), v[i].refcount(), 1);
}

/*
 * For list test:
 *
 * - create untyped list
 * - add values to list end
 * - insert value in random order
 * - remove values from list
 * - get back inserted values
 * - set new item
 * - check refcount of inserted values
 * - count items
 * - destroy value
 * - create typed list
 * - test duplication
 */
TEST_FUNCTION(25) {
  // BaseListRef

  {
    BaseListRef lv(AnyType);
    lv.retain();
    IntegerRef iv[10] = {100, 101, 102, 3, 4, 5, 6, 7, 8, 9};
    ensure_equals("list initial items count", lv.count(), 0U);
    ensure_equals("retained list refcount", lv.refcount(), 2);
    ensure_equals("var refcount before insert to list", iv[0].refcount(), 1);
    lv.ginsert(iv[0]);
    ensure_equals("list count after insert to list", lv.count(), 1U);
    ensure_equals("var refcount after insert to list", iv[0].refcount(), 2);
    lv.remove(0);
    ensure_equals("list count after remove from list", lv.count(), 0U);
    ensure_equals("var refcount after remove from list", iv[0].refcount(), 1);

    { // just to ensure all auxiliary IntegerRef objects will release their references
      lv.ginsert(iv[0]);
      ensure_equals("list items count after random insert", lv.count(), 1U);
      lv.ginsert(iv[1]);
      ensure_equals("list items count after random insert", lv.count(), 2U);
      lv.ginsert(iv[2], 1); // insert after first index
      ensure_equals("list items count after random insert", lv.count(), 3U);
      ensure("list item value after random insert", IntegerRef::cast_from(lv.get(1)) == 102);
      lv.ginsert(iv[3], 0); // insert by first index
      ensure_equals("list items count after random insert", lv.count(), 4U);
      ensure("list item value after random insert", IntegerRef::cast_from(lv.get(0)) == 3);
      lv.ginsert(iv[4], 3); // insert before last index
      ensure_equals("list items count after random insert", lv.count(), 5U);
      ensure("list item value after random insert", IntegerRef::cast_from(lv.get(3)) == 4);
      lv.ginsert(iv[5], 5); // insert by last index
      ensure_equals("list items count after random insert", lv.count(), 6U);
      ensure("list item value after random insert", IntegerRef::cast_from(lv.get(5)) == 5);
    }

    bool exception_thrown(false);
    try {
      lv.ginsert(iv[6], 7); // invalid index
    } catch (grt::bad_item&) {
      exception_thrown = true;
    } catch (...) {
    }
    ensure("try to insert value by invalid index (out of bounds)", exception_thrown);
    ensure_equals("list items count after try to insert new item by invalid index", lv.count(), 6U);

    ensure_equals("var refcount before remove from list", iv[0].refcount(), 2);
    for (int n = (int)lv.count() - 1; n >= 0; n--)
      lv.remove(n);
    ensure_equals("var refcount after remove from list", iv[5].refcount(), 1);
    ensure_equals("list items count after remove all items", lv.count(), 0U);

    for (int n = 0; n < 6; n++)
      lv.ginsert(iv[n], n);
    ensure_equals("list items count after insert", lv.count(), 6U);
    for (int n = 0; n < 6; n++)
      ensure_equals("var refcount after insert to list", iv[n].refcount(), 2);

    lv.gset(3, iv[5]);
    ensure("list item value after being overwritten", IntegerRef::cast_from(lv.get(3)) == 5);
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
      tut::ensure_equals(format_msg("Reference counts differ", NULL, AnyType, i).c_str(), iv[i].refcount(), refcount);
    }

    exception_thrown = false;
    try {
      lv.gset(6, iv[6]); // invalid index
    } catch (grt::bad_item&) {
      exception_thrown = true;
    } catch (...) {
    }
    ensure("try to set item by invalid index (out of bounds)", exception_thrown);
    ensure_equals("list items count after try to set item by invalid index", lv.count(), 6U);

    while (lv.count())
      lv.remove(0);
    ensure_equals("list refcount after retained list release", lv.refcount(), 2);
    ensure_equals("list items count after list release", lv.count(), 0U);
    for (int i = 0; i < 8; i++)
      ensure_equals("var refcount after list release", iv[i].refcount(), 1);
  }
}

TEST_FUNCTION(26) {
  // ValueList<IntegerRef>
  {
    IntegerListRef lv(grt::Initialized);
    IntegerRef v[10] = {100, 101, 102, 3, 4, 5, 6, 7, 8, 9};
    test_list_value(lv, v);
  }
}

TEST_FUNCTION(27) {
  // ValueList<DoubleRef>
  {
    DoubleListRef lv(grt::Initialized);
    DoubleRef v[10] = {0.5, 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5, 9.5};
    test_list_value(lv, v);
  }
}

TEST_FUNCTION(28) {
  // ValueList<StringRef>
  {
    StringListRef lv(grt::Initialized);
    StringRef v[10] = {"_0", "_1", "_2", "_3", "_4", "_5", "_6", "_7", "_8", "_9"};
    test_list_value(lv, v);
  }
}

TEST_FUNCTION(29) {
  // ListRef <ObjectRef>

  ListRef<grt::internal::Object> lv(true);
  ObjectRef v[10] = {grt::GRT::get()->create_object<grt::internal::Object>("test.Book"),
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
}

TEST_FUNCTION(30) {
  // typed list checks

  BaseListRef ut(AnyType);

  ensure_equals("untyped list content", ut.content_type(), AnyType);
  ensure_equals("untyped list struct", ut.content_class_name(), "");

  IntegerListRef il(grt::Initialized);
  ensure_equals("int list content", il.content_type(), IntegerType);
  ensure_equals("int list struct", il.content_class_name(), "");

  DoubleListRef dl(grt::Initialized);
  ensure_equals("double list content", dl.content_type(), DoubleType);
  ensure_equals("double list struct", dl.content_class_name(), "");

  StringListRef sl(grt::Initialized);
  ensure_equals("str list content", sl.content_type(), StringType);
  ensure_equals("str list struct", sl.content_class_name(), "");

  /* not supported
  DictListRef cl(grt::Initialized);
  ensure_equals("dict list content", cl.content_type(), DictType);
  ensure_equals("dict list struct", cl.content_class_name(), "");
   */

  ObjectListRef ol(true);
  ensure_equals("obj list content", ol.content_type(), grt::ObjectType);
  ensure_equals("obj list struct", ol.content_class_name(), "Object");

  ListRef<test_Author> al(true);
  ensure_equals("author list content", al.content_type(), grt::ObjectType);
  ensure_equals("author list struct", al.content_class_name(), "test.Author");
}

TEST_FUNCTION(31) {
  // check can_wrap for lists

  ensure("wrap List<Object>/List<Object>", ObjectListRef::can_wrap(ObjectListRef(true)));
  ensure("wrap List<test_Book>/List<Object>", !ListRef<test_Book>::can_wrap(ObjectListRef(true)));
  ensure("wrap List<Object>/List<test_Book>", ObjectListRef::can_wrap(ListRef<test_Book>(true)));
  ensure("wrap List<test_Publication>/List<test_Book>", ListRef<test_Publication>::can_wrap(ListRef<test_Book>(true)));
  ensure("wrap List<test_Book>/List<test_Publication>", !ListRef<test_Book>::can_wrap(ListRef<test_Publication>(true)));
  ensure("wrap BaseList/List<test_Book>", BaseListRef::can_wrap(ListRef<test_Book>(true)));
  ensure("wrap List<test_Book>/BaseList", !ListRef<test_Book>::can_wrap(BaseListRef(true)));

  ensure("wrap List<test_Publication>/IntegerList",
         !ListRef<test_Publication>::can_wrap(IntegerListRef(grt::Initialized)));
  ensure("wrap IntegerList/ObjectList", !IntegerListRef::can_wrap(ListRef<test_Publication>(true)));

  ensure("wrap StringList/IntegerList", !StringListRef::can_wrap(IntegerListRef(grt::Initialized)));
  ensure("wrap IntegerList/StringListRef", !IntegerListRef::can_wrap(StringListRef(grt::Initialized)));
}

/*
 * For dict test:
 *
 * - create untyped dict
 * - put stuff in it
 * - remove stuff
 * - overwrite stuff
 * - destroy
 * - duplication
 */
TEST_FUNCTION(35) {
  // dictionary

  DictRef dv(true);
  IntegerRef iv[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  StringRef sv[10] = {"_0", "_1", "_2", "_3", "_4", "_5", "_6", "_7", "_8", "_9"};
  StringRef k[10] = {"_0", "_1", "_2", "_3", "_4", "_5", "_6", "_7", "_8", "_9"};
  ObjectRef obj(grt::GRT::get()->create_object<grt::internal::Object>("test.Book"));

  ensure_equals("initial size == 0", dv.count(), 0U);

  dv.retain();
  ensure_equals("retain dict", dv.refcount(), 2);

  for (int i = 0; i < 30; i += 3) // simulate random inserts
  {
    int n = i % 10;
    dv.set(k[n], iv[n]);
  }
  ensure_equals("dict items count after inserts", dv.count(), 10U);
  for (int i = 2; i < 10; i++) // skip 0 and 1 because they're internally cached
    ensure_equals("var refcount after insert to dict", iv[i].refcount(), 2);

  for (int i = 0; i < 30; i += 3) // simulate random overwrites
  {
    int n = i % 10;
    dv.set(k[n], obj);
  }
  ensure_equals("dict items count after dict items overwrites", dv.count(), 10U);
  for (int i = 2; i < 10; i++)
    ensure_equals("var refcount after remove from dict", iv[i].refcount(), 1);
  ensure_equals("obj refcount after insert to dict", obj.refcount(), 11);

  for (int i = 0; i < 30; i += 3) // simulate random overwrites
  {
    int n = i % 10;
    dv.set(k[n], sv[n]);
  }
  ensure_equals("dict items count after dict items overwrites", dv.count(), 10U);
  ensure_equals("var refcount after remove from dict", obj.refcount(), 1);
  for (int i = 2; i < 10; i++)
    ensure_equals("var refcount after insert to dict", sv[i].refcount(), 2);

  for (int i = 0; i < 10; i++)
    dv.remove(k[i]);
  ensure_equals("dict refcount after dict (retained) release", dv.refcount(), 2);
  ensure_equals("dict items count after dict release", dv.count(), 0U);
  for (int i = 2; i < 10; i++)
    ensure_equals(format_msg("var refcount after dict release", NULL, AnyType, i), iv[i].refcount(), 1);
  for (int i = 0; i < 10; i++)
    ensure_equals("var refcount after dict release", sv[i].refcount(), 1);

  // make sure that cached values are still there
  for (int i = 0; i < 2; i++)
    ensure(format_msg("var refcount after dict release", NULL, AnyType, i), iv[i].refcount() > 1);

  ensure_equals("var refcount after dict release", obj.refcount(), 1);
}

TEST_FUNCTION(36) { // test list reordering

  IntegerListRef lv(grt::Initialized);
  IntegerRef iv[4] = {0, 1, 2, 3};

  while (lv.count())
    lv.remove(0);
  for (int i = 0; i < 4; i++)
    lv.insert(iv[i]);

  lv.reorder(0, 4);
  ensure_equals("reorder 0,4", *lv.get(0), 1);
  ensure_equals("reorder 0,4", *lv.get(1), 2);
  ensure_equals("reorder 0,4", *lv.get(2), 3);
  ensure_equals("reorder 0,4", *lv.get(3), 0);

  while (lv.count())
    lv.remove(0);
  for (int i = 0; i < 4; i++)
    lv.insert(iv[i]);

  lv.reorder(1, 2);
  ensure_equals("reorder 1,2", *lv.get(0), 0);
  ensure_equals("reorder 1,2", *lv.get(1), 2);
  ensure_equals("reorder 1,2", *lv.get(2), 1);
  ensure_equals("reorder 1,2", *lv.get(3), 3);

  while (lv.count())
    lv.remove(0);
  for (int i = 0; i < 4; i++)
    lv.insert(iv[i]);

  lv.reorder(2, 1);
  ensure_equals("reorder 2,1", *lv.get(0), 0);
  ensure_equals("reorder 2,1", *lv.get(1), 2);
  ensure_equals("reorder 2,1", *lv.get(2), 1);
  ensure_equals("reorder 2,1", *lv.get(3), 3);

  while (lv.count())
    lv.remove(0);
  for (int i = 0; i < 4; i++)
    lv.insert(iv[i]);

  lv.reorder(3, 0);
  ensure_equals("reorder 3,0", *lv.get(0), 3);
  ensure_equals("reorder 3,0", *lv.get(1), 0);
  ensure_equals("reorder 3,0", *lv.get(2), 1);
  ensure_equals("reorder 3,0", *lv.get(3), 2);
}

END_TESTS
