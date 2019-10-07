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

#include "casmine.h"

using namespace casmine;

namespace {

$ModuleEnvironment() {};

// These tests all succeed, but they are meant to indicate compilation problems, if some exist.
$describe("Casmine internal") {

#ifdef _MSC_VER
  #pragma warning(disable: 4756 4723) // Overflow in constant arithmetic and Potential divide by 0
#endif

  $it("Scalar matcher (numeric)", []() {
    $expect(1).toEqual(1);
    $expect(10).toBeLessThan(11);
    $expect(10).toBeLessThanOrEqual(10);
    $expect(11).toBeGreaterThan(10);
    $expect(10).toBeGreaterThanOrEqual(10);
    $expect(10).toBeTrue();
    $expect(122.0001).toBeCloseTo(122.0, 0.5);
    $expect(100).toBeCloseTo(0, 1000);
    $expect(101).toBeOddNumber();
    $expect(100).toBeEvenNumber();
    $expect(10.0f).toBeWholeNumber();
    $expect(10LL).toBeWholeNumber();
    $expect(-123e3).toBeWithinRange(-1000e3, 0);

    float val = 1e-38f;
    $expect(1e38f / val).toBeInf();
    $expect(-1e38f / val).toBeInf();

    $expect(1e38f / val).Not.toBeNaN();
    double temp = 0;
    $expect(0 / temp).toBeNaN();

  });

#ifdef _MSC_VER
  #pragma warning(default: 4756 4723)
#endif

  $it("Scalar matcher (pointer)", []() {
    void *ptr = nullptr;
    $expect(ptr).toBeNull();

    int i = 1;
    $expect(&i).Not.toBeNull();

    class A {
    public:
      virtual ~A() {}
    };
    class B : public A {
    public:
      virtual ~B() {}
    };
    class C : public A {
    public:
      virtual ~C() {}
    };

    A *a = new A();
    B *b = new B();
    $expect(b).toBeInstanceOf<A>();
    $expect(a).Not.toBeInstanceOf<B>();
    $expect(b).Not.toBeInstanceOf<std::string>();
    $expect(b).Not.toBeSameType<A>();
    $expect(b).toBeSameType<B *>();
    $expect(b).Not.toBeSameType<B>();
    $expect(b).Not.toBeSameType<std::string>();

    delete a;
    delete b;

    C *c = nullptr;
    $expect(c).Not.toBeInstanceOf<B>();
  });

  $it("Scalar matcher (string/wstring)", []() {
    const char *value1 = "test 1";
    $expect(value1).toBe(value1);
    $expect(value1).toBe("test 1");
    $expect(value1).toStartWith("test");

    const char value2[] = "test 2";
    $expect(value2).toBe("test 2");
    $expect(value2).toBe(value2);
    $expect(value2).toStartWith("test");

    $expect("test 2").toEqual((const char*)"test 2");
    $expect("test 2").toEqual(value2);
    $expect("test 2").toStartWith("test");

    std::string value3 = "test 3";
    $expect(value3).toBe("test 3");
    $expect(value3).toBe(value3);
    $expect(value3).toStartWith("test");

    const wchar_t *value4 = L"test 4";
    $expect(value4).toBe(value4);
    $expect(value4).toBe(L"test 4");
    $expect(value4).toStartWith(L"test");

    const wchar_t value5[] = L"test 5";
    $expect(value5).toBe(L"test 5");
    $expect(value5).toBe(value5);
    $expect(value5).toStartWith(L"test");

    $expect(L"test 5").toEqual((const wchar_t*)L"test 5");
    $expect(L"test 5").toEqual(value5);
    $expect(L"test 5").toStartWith(L"test");

    std::wstring value6 = L"test 6";
    $expect(value6).toBe(L"test 6");
    $expect(value6).toBe(value6);
    $expect(value6).toStartWith(L"test");
    $expect(value6).toMatch(std::wregex(L".+[0-9]+"));
    $expect(value6).toMatch(L".+[0-9]+");
    $expect(value6).toMatch(L".+[0-9]+"s);

    std::string value7 = "Lorem ipsum dolor sit";
    $expect(value7).toBeLessThan("lorem ipsum");
    $expect(value7).toBeLessThanOrEqual("Lorem ipsum dolor sit");
    $expect(value7).toBeGreaterThan("Korem");
    $expect(value7).toBeGreaterThanOrEqual("Lorem");
    $expect(value7).toContain("sum");
    $expect(value7).toStartWith("Lore");
    $expect(value7).toEndWith(" sit");
    $expect("   \t\t\t\n\r\n").toContainOnlyWhiteSpaces();
    $expect("LoremIpsumDolorSitAmet").toContainNoWhiteSpaces();
    $expect("").toBeEmpty();
    $expect("").toBeEmpty("Some message");
    $expect(value7).toBeSameLengthAs("tis rolod muspi meroL");
    $expect(value7).toBeLongerThan("ipsum");
    $expect(value7).toBeShorterThan("Lorem ipsum dolor sit amet");
    $expect(value7).toMatch(std::regex(".*d[ol]+r.*"));
    $expect(value7).toMatch(".*d[ol]+r.*");
    $expect(value7).toMatch(".*d[ol]+r.*"s);
    $expect(value7 + "\n" + value7 + "\n" + value7)
      .toEqualContentOfFile(casmine::CasmineContext::get()->tmpDataDir() + "/res/casmine_data.txt");

    std::u16string value8 { 0x2B00, 0x2B01 }; // ⬀⬁
    $expect(value8).toEqual(value8);
    $expect(value8).toEqual(value8.c_str());
    $expect(value8.c_str()).toEqual(value8);
    $expect(value8.c_str()).toEqual(value8.c_str());

#ifdef _MSC_VER
    $expect(value8).toEqual(u"\u2B00\u2B01"); // Unicode string literals are broken in VS.
#else
    $expect(value8).toEqual(u"⬀⬁");
#endif
    $expect(value8).toEqual({ 0x2B00, 0x2B01 });
    $expect(value8.c_str()).toStartWith({ 0x2B00 });
    $expect(value8).toBeGreaterThan(u"⫰");

    std::u32string value9 { 0x2B00, 0x2B01 }; // ⬀⬁
    $expect(value9).toEqual(value9);
    $expect(value9).toEqual(value9.c_str());
    $expect(value9.c_str()).toEqual(value9);
    $expect(value9.c_str()).toEqual(value9.c_str());
    $expect(value9).toEqual({ 0x2B00, 0x2B01 });
#ifdef _MSC_VER
    $expect(value9).toEqual(U"\u2B00\u2B01");
#else
    $expect(value9).toEqual(U"⬀⬁");
#endif
    $expect(value9.c_str()).toStartWith({ 0x2B00 });
    $expect(value9).toBeGreaterThan(U"⫰");

    const char16_t *value10 = u"test 10";
    $expect(value10).toBe(value10);
    $expect(value10).toBe(u"test 10");
    $expect(value10).toStartWith(u"test");

    const char16_t value11[] = u"test 11";
    $expect(value11).toBe(value11);
    $expect(value11).toBe(u"test 11");
    $expect(value11).toStartWith(u"test");

    const char32_t *value12 = U"test 12";
    $expect(value12).toBe(U"test 12");
    $expect(value12).toBe(value12);
    $expect(value12).toStartWith(U"test");

    const char32_t value13[] = U"test 13";
    $expect(value13).toBe(U"test 13");
    $expect(value13).toBe(value13);
    $expect(value13).toStartWith(U"test");
  });

  $it("Scalar matcher (enum)", []() {
    enum { North, South, West, East } direction = South;
    $expect(direction).toBe(South);

    enum class Color { Red, Green, Blue } color = Color::Red;
    $expect(color).toBe(Color::Red);
    $expect(color).Not.toBe(Color::Green);
  });

  $it("Scalar matcher (bool)", []() {
    $expect(true).toBe(true);
    $expect(true).toBeTrue();
    $expect(true).toBeGreaterThanOrEqual(true);
    $expect(true).Not.toEqual(false);
    $expect(true).toBeCloseTo(1.0, 1e-10);
    $expect(true).toBeOddNumber();
    $expect(true).Not.toBeEvenNumber();
    $expect(true).toBeWholeNumber();
    $expect(true).toBeWithinRange(false, true);
    $expect(true).Not.toBeNaN();
    $expect(true).Not.toBeInf();
  });

  $it("Class matcher", []() {
    class A {
    public:
      int i;
      bool operator == (A const& other) const {
        return i == other.i;
      }
    };
    class B : public A {};

    A a;
    a.i = 0;
    $expect(a).toEqual(a);
    $expect(&a).toBe(&a);
    $expect(&a).toEqual(&a);

    B b;
    b.i = 0;
    $expect(a).toEqual(b);
    $expect(&a).Not.toBe(&b);
    $expect(&a).toEqual(&b);

    b.i = 1;
    $expect(a).Not.toEqual(b);
    $expect(&a).Not.toEqual(&b);
  });

  $it("Exception matcher", []() {
    $expect([]() { throw "Error"; }).toThrow();
    $expect([]() {  }).Not.toThrow();

    $expect([]() { throw "Error"; }).toThrowError("Error");
    $expect([]() { throw "Error"; }).Not.toThrowError<std::runtime_error>("Error");
    $expect([]() { throw std::runtime_error("Error"); }).toThrowError<std::runtime_error>("Error");
    $expect([]() { throw std::runtime_error("Error"); }).Not.toThrowError("Error");

    class ErrorA : public std::runtime_error {
    public:
      ErrorA(std::string const& e) : std::runtime_error(e) {};
    };
    class ErrorB : public ErrorA  {
    public:
      ErrorB(std::string const& e) : ErrorA(e) {};
    };
    class ErrorC : public std::runtime_error  {
    public:
      ErrorC(std::string const& e) : std::runtime_error(e) {};
    };

    // Heap allocated error objects should not be used, however we support them anyway.
    // Heap allocated non-exception objects (like new std::string) cannot be used as actual value, though.
    $expect([]() { throw ErrorA("Error"); }).toThrowError<std::runtime_error>("Error");
    $expect([]() { throw ErrorB("Error"); }).toThrowError<std::runtime_error>("Error");
    $expect([]() { throw ErrorB("Error"); }).toThrowError<ErrorA>("Error");
    $expect([]() { throw ErrorC("Error"); }).Not.toThrowError<ErrorA>("Error");
    $expect([]() { throw ErrorA("Error"); }).Not.toThrowError<ErrorB>("Error");

    $expect([]() { throw new ErrorA("Error"); }).toThrowError<std::runtime_error *>("Error");
    $expect([]() { throw new ErrorA("Error"); }).toThrowError<ErrorA *>("Error");
    $expect([]() { throw new ErrorB("Error"); }).toThrowError<ErrorA *>("Error");
    $expect([]() { throw new ErrorA("Error"); }).Not.toThrowError<std::runtime_error>("Error");

    $expect([]() { throw new std::string("Error"); }).Not.toThrowError("Error");
    //$expect([]() { throw "Error"; }).Not.toThrowError(new std::string("Error")); Doesn't compile.
    $expect([]() { throw "Error"; }).Not.toThrowError("Success");
    $expect([]() { throw ErrorA("Error"); }).Not.toThrowError<std::runtime_error>("Success");
  });

  $it("Containers", []() {
    // Note: it is not possible to use an initializer list in the expect macro call.
    std::array<size_t, 5> a = {{ 1, 2, 3, 4, 5 }};
    $expect(a).toContain(4);
    $expect(a).toContainValues({ 4, 2, 2, 4, 4 });
    $expect(a).toHaveSize(5);
    $expect(a).toEqual({ 1, 2, 3, 4, 5 });

    std::vector v = { 1, 2, 3, 4, 5 };
    $expect(v).toContain(4);
    $expect(v).toContainValues({ 4, 2, 2 });
    $expect(v).toHaveSize(5);
    $expect(v).toEqual({ 1, 2, 3, 4, 5 });

    std::deque d = { 1, 2, 3, 4, 5 };
    $expect(d).toContain(4);
    $expect(d).toContainValues({ 4, 2, 2 });
    $expect(d).toHaveSize(5);
    $expect(d).toEqual({ 1, 2, 3, 4, 5 });

    std::forward_list f = { 1, 2, 3, 4, 5 };
    $expect(f).toContain(4);
    $expect(f).toContainValues({ 4, 2, 2 });
    //$expect(f).toHaveSize(5); forward_list has no size() member.
    $expect(f).toEqual({ 1, 2, 3, 4, 5 });

    std::list l = { 1, 2, 3, 4, 5 };
    $expect(l).toContain(4);
    $expect(l).toContainValues({ 4, 2, 2 });
    $expect(l).toHaveSize(5);
    $expect(l).toEqual({ 1, 2, 3, 4, 5 });

    std::set s = { 1, 2, 3, 4, 5 };
    $expect(s).toContain(4);
    $expect(s).toContainValues({ 4, 2, 2 });
    $expect(s).toHaveSize(5);
    $expect(s).toEqual({ 1, 2, 3, 4, 5 });

    std::multiset<std::size_t> ms = { 1, 2, 3, 4, 5 };
    $expect(ms).toContain(4);
    $expect(ms).toContainValues({ 4, 2, 2 });
    $expect(ms).toHaveSize(5);
    $expect(ms).toEqual({ 1, 2, 3, 4, 5 });

    std::map<size_t, std::string> m = {{ 1, "a"}, { 2, "b" }};
    $expect(m).toContainKey(1);
    $expect(m).toContainValue("b");
    $expect(m).Not.toContainValue("x");
    $expect(m).toHaveSize(2);
    $expect(m).toEqual({{ 1, "a"}, { 2, "b" }});

    std::multimap<size_t, std::string> mm = {{ 1, "a"}, { 2, "b" }};
    $expect(mm).toContainKey(1);
    $expect(mm).toContainValue("b");
    $expect(mm).toHaveSize(2);
    $expect(mm).toEqual({{ 1, "a"}, { 2, "b" }});

    std::unordered_set<int> us = { 1, 2, 3, 4, 5 };
    $expect(us).toContain(4);
    $expect(us).toContainValues({ 4, 2, 2 });
    $expect(us).toHaveSize(5);
    $expect(us).toEqual({ 1, 2, 3, 4, 5 });

    std::unordered_multiset<int> ums({ 1, 2, 3, 4, 5 });
    $expect(ums).toContain(4);
    $expect(ums).toContainValues({ 4, 2, 2 });
    $expect(ums).toHaveSize(5);
    $expect(ums).toEqual({ 1, 2, 3, 4, 5 });

    std::unordered_map<size_t, std::string> um = {{ 1, "a"}, { 2, "b" }};
    $expect(um).toContainKey(1);
    $expect(um).toContainValue("b");
    $expect(um).toHaveSize(2);
    $expect(um).toEqual({{ 1, "a"}, { 2, "b" }});

    std::unordered_multimap<size_t, std::string> umm = {{ 1, "a"}, { 2, "b" }};
    $expect(umm).toContainKey(1);
    $expect(umm).toContainValue("b");
    $expect(umm).toHaveSize(2);
    $expect(umm).toEqual({{ 2, "b" }, { 1, "a" }});

    // These containers don't support iterators:
    // std::stack<int> st;
    // std::queue<int> q;
    // std::priority_queue<int> pq;
  });

  $it("Pairs and tuples", []() {
    $pending("not implemented yet");
  });

  $it("Variants and optionals", []() {
    $pending("not implemented yet");
  });

  $it("Any", []() {
    $pending("not implemented yet");
  });

  $it("Date and time", []() {
    $pending("not implemented yet");
  });

  $it("Smart pointers", []() {
    class A {};
    std::unique_ptr<A> up;
    $expect(up).toBeNull();
    $expect(up).Not.toBeValid();

    up.reset(new A());
    $expect(up).Not.toBeNull();
    $expect(up).toBeValid();

    std::shared_ptr<A> sp = std::make_shared<A>();
    $expect(sp).Not.toBeNull();
    $expect(sp).toBeValid();

    std::weak_ptr<A> wp;
    $expect(wp).toBeNull();
    $expect(wp).Not.toBeValid();

    wp = sp;
    $expect(wp).Not.toBeNull();
    $expect(wp).toBeValid();
  });
}

}
