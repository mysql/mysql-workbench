/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "base/utf8string.h"
#include <utility>
#include "wb_helpers.h"
#include <algorithm>
#include <cctype>
#include <functional>
#include <map>
using namespace base;

TEST_MODULE(utf8string_test, "utf8string");

struct lang_string_details {
  const char *const _text;
  size_t _length;
  size_t _bytes;
  lang_string_details() = default;
  lang_string_details(const lang_string_details &) = default;
  lang_string_details(const char *text, size_t length, size_t bytes) : _text(text), _length(length), _bytes(bytes) {
  }
};

const std::map<std::string, lang_string_details> LanguageStrings = {
  {"english", {"This is a lazy test", 19, 19}},
  {"polish", {"zażółć", 6, 10}},
  {"greek", {"ὕαλον ϕαγεῖν", 12, 25}},
  {"russian", {"Я могу есть стекло", 18, 33}},
  {"arabic", {"هذا لا يؤلمني", (size_t)13, 24}},
  {"chinese", {"我可以吞下茶", 6, 18}},
  {"japanese", {"私はお茶を飲み込むことができます", 16, 48}},
  {"portuguese", {"Há açores e cães ávidos no chão", 31, 36}}};

template <unsigned int TestCaseNumber>
struct __test_data {
  unsigned int _test_item_number;
  std::string _section;
  std::string _location;
  std::string _description;
  __test_data() : _test_item_number(0), _section("general") {
  }

  void set_section(const std::string &section) {
    _section = section;
  }
  void set_test(const std::string &location, const std::string &description) {
    _location = location;
    _description = description;
    ++_test_item_number;
  }

  template <typename T, typename Q>
  void throw_exception(const T &first, const Q &second) {
    std::stringstream ss;
    ss << "[" << _section << "] TEST " << TestCaseNumber << "." << _test_item_number;

    if (!_description.empty())
      ss << " - " << _description;

    ss << std::endl
       << "  location: " << _location << std::endl
       << "  section : " << _section << std::endl
       << "  expected: '" << second << "'" << std::endl
       << "  actual  : '" << first << "'";
    throw failure(ss.str());
  }

  template <typename T, typename Q>
  void test(const T &first, const Q &second) {
    if (first != second)
      throw_exception(first, second);
  }

  template <typename Q>
  void test(size_t first, const Q &second) {
    if (first != (size_t)second)
      throw_exception(first, second);
  }

  template <typename T, typename Q>
  void test_not_equal(const T &first, const Q &second) {
    if (first == second)
      throw_exception(first, second);
  }

  void test_true(bool first) {
    test(first, true);
  }
  void test_false(bool first) {
    test(first, false);
  }
  void test_null(const void *first) {
    test(first, nullptr);
  }
  void test_not_null(const void *first) {
    test_not_equal(first, nullptr);
  }
};

#define INITIALIZE_TEST_FUNCTION(number) __test_data<number> __test_data__
#define SECTION(sec) __test_data__.set_section(sec)
#define TEST(desc, first, second)           \
  __test_data__.set_test(__LOCATION, desc); \
  __test_data__.test(first, second)
#define TEST_TRUE(desc, first)              \
  __test_data__.set_test(__LOCATION, desc); \
  __test_data__.test_true(first)
#define TEST_FALSE(desc, first)             \
  __test_data__.set_test(__LOCATION, desc); \
  __test_data__.test_false(first)
#define TEST_NULL(desc, first)              \
  __test_data__.set_test(__LOCATION, desc); \
  __test_data__.test_null(first)
#define TEST_NOT_NULL(desc, first)          \
  __test_data__.set_test(__LOCATION, desc); \
  __test_data__.test_not_null(first)

/*
 * Constructor tests
 * */
TEST_FUNCTION(5) {
  INITIALIZE_TEST_FUNCTION(5);

  {
    SECTION("utf8string()");

    base::utf8string str1;
    TEST_TRUE("validate string", str1.validate());
    TEST("verify length", str1.length(), (size_t)0);
    TEST("verify bytes", str1.bytes(), (size_t)0);
    TEST_TRUE("verify empty", str1.empty());

    base::utf8string str2 = "";
    TEST_TRUE("validate string", str2.validate());
    TEST("verify length", str2.length(), (size_t)0);
    TEST("verify bytes", str2.bytes(), (size_t)0);
    TEST_TRUE("verify empty", str2.empty());
  }

  for (auto iter : LanguageStrings) {
    SECTION(std::string("utf8string(const char *s) - ") + iter.first);
    lang_string_details &current = iter.second;
    base::utf8string str1(current._text); //  from char *

    TEST_TRUE("validate string", str1.validate());
    TEST("compare to char * string", str1, current._text);
    TEST("verify length", str1.length(), current._length);
    TEST("verify bytes", str1.bytes(), current._bytes);
    TEST_FALSE("verify empty", str1.empty());
  }

  for (auto iter : LanguageStrings) {
    SECTION(std::string("utf8string(const std::string &s) - ") + iter.first);
    lang_string_details &current = iter.second;
    std::string str_to_init(current._text);
    base::utf8string str2(str_to_init); //  from std::string

    TEST_TRUE("validate string", str2.validate());
    TEST("compare to char * string", str2, current._text);
    TEST("verify length", str2.length(), current._length);
    TEST("verify bytes", str2.bytes(), current._bytes);
    TEST_FALSE("verify empty", str2.empty());
  }

  for (auto iter : LanguageStrings) {
    SECTION(std::string("utf8string(const char *s) - ") + iter.first);
    lang_string_details &current = iter.second;
    base::utf8string str_to_init(current._text);
    base::utf8string str1(str_to_init); //  from char *

    TEST_TRUE("validate string", str1.validate());
    TEST("compare to char * string", str1, current._text);
    TEST("verify length", str1.length(), current._length);
    TEST("verify bytes", str1.bytes(), current._bytes);
    TEST_FALSE("verify empty", str1.empty());
  }

  SECTION(std::string("utf8string(const wchar_t *s)"));
  //    TODO: implement this section

  SECTION(std::string("utf8string(const std::wstring &s)"));
  //    TODO: implement this section

  {
    std::map<std::string, const char *> substrings = {
      {"english", "his "}, {"polish", "ażół"},      {"greek", "αλον"},        {"russian", " мог"},
      {"arabic", "ذا ل"},  {"chinese", "可以吞下"}, {"japanese", "はお茶を"}, {"portuguese", "á aç"}};

    for (auto iter : LanguageStrings) {
      SECTION(std::string("utf8string(const char *s, size_t pos, size_t len) - ") + iter.first);
      lang_string_details &current = iter.second;

      base::utf8string str1(current._text, 1, 4);   //  from char *
      base::utf8string str2(current._text, 500, 4); //  sub-string from invalid index
      base::utf8string str3(current._text, 1, 500); //  sub-string with huge length
      base::utf8string str4(current._text, 1, 0);   //  sub-string with zero length

      base::utf8string right_to_compare = base::utf8string(current._text).right(current._length - 1);

      TEST_TRUE("validate string", str1.validate());
      TEST("data compare", str1, substrings[iter.first]);
      TEST("size test", str1.size(), (size_t)4);
      TEST_FALSE("verify empty", str1.empty());

      TEST_TRUE("validate string", str2.validate());
      TEST("size test", str2.size(), 0);
      TEST("length test", str2.length(), 0);
      TEST_TRUE("verify empty", str2.empty());

      TEST_TRUE("validate string", str3.validate());
      TEST("data compare", str3, right_to_compare);
      TEST("size test", str3.size(), current._length - 1);
      TEST("length test", str3.length(), current._length - 1);
      TEST_FALSE("verify empty", str3.empty());

      TEST_TRUE("validate string", str4.validate());
      TEST("size test", str4.size(), 0);
      TEST("length test", str4.length(), 0);
      TEST_TRUE("verify empty", str4.empty());
    }

    for (auto iter : LanguageStrings) {
      SECTION(std::string("utf8string(const std::string &str, size_t pos, size_t len); - ") + iter.first);
      lang_string_details &current = iter.second;

      std::string str_to_init(current._text);

      base::utf8string str1(str_to_init, 1, 4);   //  from char *
      base::utf8string str2(str_to_init, 500, 4); //  sub-string from invalid index
      base::utf8string str3(str_to_init, 1, 500); //  sub-string with huge length
      base::utf8string str4(str_to_init, 1, 0);   //  sub-string with zero length

      base::utf8string right_to_compare = base::utf8string(current._text).right(current._length - 1);

      TEST_TRUE("validate string", str1.validate());
      TEST("data compare", str1, substrings[iter.first]);
      TEST("size test", str1.size(), (size_t)4);
      TEST_FALSE("verify empty", str1.empty());

      TEST_TRUE("validate string", str2.validate());
      TEST("size test", str2.size(), 0);
      TEST("length test", str2.length(), 0);
      TEST_TRUE("verify empty", str2.empty());

      TEST_TRUE("validate string", str3.validate());
      TEST("data compare", str3, right_to_compare);
      TEST("size test", str3.size(), current._length - 1);
      TEST("length test", str3.length(), current._length - 1);
      TEST_FALSE("verify empty", str3.empty());

      TEST_TRUE("validate string", str4.validate());
      TEST("size test", str4.size(), 0);
      TEST("length test", str4.length(), 0);
      TEST_TRUE("verify empty", str4.empty());
    }

    for (auto iter : LanguageStrings) {
      SECTION(std::string("utf8string(const utf8string &str, size_t pos, size_t len) - ") + iter.first);
      lang_string_details &current = iter.second;

      base::utf8string str_to_init(current._text);

      base::utf8string str1(str_to_init, 1, 4);   //  from char *
      base::utf8string str2(str_to_init, 500, 4); //  sub-string from invalid index
      base::utf8string str3(str_to_init, 1, 500); //  sub-string with huge length
      base::utf8string str4(str_to_init, 1, 0);   //  sub-string with zero length

      base::utf8string right_to_compare = base::utf8string(current._text).right(current._length - 1);

      TEST_TRUE("validate string", str1.validate());
      TEST("data compare", str1, substrings[iter.first]);
      TEST("size test", str1.size(), (size_t)4);
      TEST_FALSE("verify empty", str1.empty());

      TEST_TRUE("validate string", str2.validate());
      TEST("size test", str2.size(), 0);
      TEST("length test", str2.length(), 0);
      TEST_TRUE("verify empty", str2.empty());

      TEST_TRUE("validate string", str3.validate());
      TEST("data compare", str3, right_to_compare);
      TEST("size test", str3.size(), current._length - 1);
      TEST("length test", str3.length(), current._length - 1);
      TEST_FALSE("verify empty", str3.empty());

      TEST_TRUE("validate string", str4.validate());
      TEST("size test", str4.size(), 0);
      TEST("length test", str4.length(), 0);
      TEST_TRUE("verify empty", str4.empty());
    }
  }

  {
    SECTION("utf8string(size_t size, char c)");
    base::utf8string str(10, 'a');

    TEST_TRUE("validate string", str.validate());
    TEST("compare data", str, "aaaaaaaaaa");
    TEST("size test", str.size(), 10);
    TEST("length test", str.length(), 10);
    TEST("bytes test", str.bytes(), 10);
    TEST_FALSE("verify empty", str.empty());
  }

  {
    SECTION("utf8string(size_t size, utf8char c) [unicode]");
    base::utf8string str(10, base::utf8string::utf8char("ł"));

    TEST_TRUE("validate string", str.validate());
    TEST("compare data", str, "łłłłłłłłłł");
    TEST("size test", str.size(), 10);
    TEST("length test", str.length(), 10);
    TEST("bytes test", str.bytes(), 20);
    TEST_FALSE("verify empty", str.empty());
  }

  {
    SECTION("utf8string(size_t size, utf8char c) [non-unicode]");
    base::utf8string str(10, base::utf8string::utf8char("a"));

    TEST_TRUE("validate string", str.validate());
    TEST("compare data", str, "aaaaaaaaaa");
    TEST("size test", str.size(), 10);
    TEST("length test", str.length(), 10);
    TEST("bytes test", str.bytes(), 10);
    TEST_FALSE("verify empty", str.empty());
  }
}

/*
 * Testing utf8string operator[], at()
 */
TEST_FUNCTION(25) {
  base::utf8string str = std::string("zażółć");
  base::utf8string::utf8char res1("ó");
  base::utf8string::utf8char res2("ć");
  ensure_equals("TEST 25.1: index of 3", str[3], res1);
  ensure_equals("TEST 25.2: index of 5", str[5], res2);

  // TODO: test utf8string::at()
}

/*
 * Testing utf8string substr
 */
TEST_FUNCTION(30) {
  base::utf8string str = std::string("zażółć");
  base::utf8string res1 = "żółć";
  base::utf8string res2 = "aż";
  ensure_equals("TEST 30.1: substr 2", str.substr(2), res1);
  ensure_equals("TEST 30.2: substr 1,2", str.substr(1, 2), res2);
}

/*
 * Testing utf8string operators +, +=, =, ==, !=
 */
TEST_FUNCTION(35) {
  base::utf8string str1 = std::string("zażółć");
  base::utf8string str2 = std::string("gęślą");
  base::utf8string result = std::string("zażółćgęślą");

  ensure_equals("TEST 35.1: operator +", str1 + str2, result);
  str1 += str2;
  ensure_equals("TEST 35.2: operator +=", str1, result);
  str1 = str2;
  ensure_equals("TEST 35.3: operator =", str1, str2);
  ensure_equals("TEST 35.4: operator ==", str1 == str2, true);
  ensure_equals("TEST 35.5: operator !=", str1 != result, true);
}

/*
 * Testing functions c_str, toString, toWString
 */
TEST_FUNCTION(40) {
  INITIALIZE_TEST_FUNCTION(40);
  for (auto iter : LanguageStrings) {
    SECTION(std::string("String conversion - ") + iter.first);
    lang_string_details &current = iter.second;

    base::utf8string str(current._text);
    ensure_equals("TEST 40.1: c_str", strcmp(str.c_str(), current._text), 0);
    ensure_equals("TEST 40.2: toString", str.to_string() == std::string(current._text), true);
    ensure_equals("TEST 40.3: toWString", str.to_wstring() == base::string_to_wstring(current._text), true);
  }
}

/*
 * Testing move constructor and operator=
 */
TEST_FUNCTION(45) {
  //  TODO: test in all languages
  base::utf8string str1(std::string("za") + std::string("żółć"));
  ensure_equals("TEST 45.1: has length", str1.length(), (size_t)6);
  ensure_equals("TEST 45.2: has bytes", str1.bytes(), (size_t)10);
  ensure_equals("TEST 45.3: empty", str1.empty(), false);

  base::utf8string str2 = std::move(str1);
  ensure_equals("TEST 45.4: str2 has length", str2.length(), (size_t)6);
  ensure_equals("TEST 45.5: str2 has bytes", str2.bytes(), (size_t)10);
  ensure_equals("TEST 45.6: str2 empty", str2.empty(), false);
}

/*
 * Testing trim* functions
 */
TEST_FUNCTION(50) {
  //  TODO: test in all languages
  ensure_equals("TEST 50.1: has length", base::utf8string("  zażółć    ").trim_left().length(), (size_t)10);
  ensure_equals("TEST 50.2: has bytes", base::utf8string("  zażółć    ").trim_left().bytes(), (size_t)14);
  ensure_equals("TEST 50.3: has length", base::utf8string("  zażółć    ").trim_right().length(), (size_t)8);
  ensure_equals("TEST 50.4: has bytes", base::utf8string("  zażółć    ").trim_right().bytes(), (size_t)12);
  ensure_equals("TEST 50.5: has length", base::utf8string("  zażółć    ").trim().length(), (size_t)6);
  ensure_equals("TEST 50.6: has bytes", base::utf8string("  zażółć    ").trim().bytes(), (size_t)10);
}

/*
 * Testing functions toUpper, toLower, truncate, toCaseFold, validate
 */
TEST_FUNCTION(55) {
  ensure_equals("TEST 55.1: toUpper", base::utf8string("zażółć").to_upper(), base::utf8string("ZAŻÓŁĆ"));
  ensure_equals("TEST 55.1: toLower", base::utf8string("ZAŻÓŁĆ").to_lower(), base::utf8string("zażółć"));

  ensure_equals("TEST 55.4: tocaseFold", base::utf8string("zAżóŁć").to_case_fold(), base::utf8string("zażółć"));
  ensure_equals("TEST 55.5: validate", base::utf8string("grüßen").validate(), true);
}

/*
 * Testing functions truncate, substr, left, right
 */
TEST_FUNCTION(56) {
  ensure_equals("TEST 56.1: truncate", base::utf8string("zażółć").truncate(0), base::utf8string("..."));
  ensure_equals("TEST 56.2: truncate", base::utf8string("zażółć").truncate(1), base::utf8string("z..."));
  ensure_equals("TEST 56.3: truncate", base::utf8string("zażółć").truncate(2), base::utf8string("za..."));
  ensure_equals("TEST 56.4: truncate", base::utf8string("zażółć").truncate(3), "zażółć");
  ensure_equals("TEST 56.5: truncate", base::utf8string("zażółć").truncate(4), "zażółć");
  ensure_equals("TEST 56.6: truncate", base::utf8string("zażółć").truncate(5), "zażółć");
  ensure_equals("TEST 56.7: truncate", base::utf8string("zażółć").truncate(6), "zażółć");
  ensure_equals("TEST 56.8: truncate", base::utf8string("zażółć").truncate(7), "zażółć");

  ensure_equals("TEST 56.9: left", base::utf8string("zażółć").left(0), "");
  ensure_equals("TEST 56.10: left", base::utf8string("zażółć").left(1), "z");
  ensure_equals("TEST 56.11: left", base::utf8string("zażółć").left(2), "za");
  ensure_equals("TEST 56.12: left", base::utf8string("zażółć").left(3), "zaż");
  ensure_equals("TEST 56.13: left", base::utf8string("zażółć").left(4), "zażó");
  ensure_equals("TEST 56.14: left", base::utf8string("zażółć").left(5), "zażół");
  ensure_equals("TEST 56.15: left", base::utf8string("zażółć").left(6), "zażółć");
  ensure_equals("TEST 56.16: left", base::utf8string("zażółć").left(7), "zażółć");

  ensure_equals("TEST 56.17: right", base::utf8string("zażółć").right(0), "");
  ensure_equals("TEST 56.18: right", base::utf8string("zażółć").right(1), "ć");
  ensure_equals("TEST 56.19: right", base::utf8string("zażółć").right(2), "łć");
  ensure_equals("TEST 56.20: right", base::utf8string("zażółć").right(3), "ółć");
  ensure_equals("TEST 56.21: right", base::utf8string("zażółć").right(4), "żółć");
  ensure_equals("TEST 56.22: right", base::utf8string("zażółć").right(5), "ażółć");
  ensure_equals("TEST 56.23: right", base::utf8string("zażółć").right(6), "zażółć");
  ensure_equals("TEST 56.24: right", base::utf8string("zażółć").right(7), "zażółć");
}

/*
 * Testing functions contains*
 */
TEST_FUNCTION(60) {
  base::utf8string str = std::string("zażółć");
  ensure_equals("TEST 60.1: starts_with", str.starts_with("za"), true);
  ensure_equals("TEST 60.2: starts_with", str.starts_with("kk"), false);
  ensure_equals("TEST 60.3: starts_with", str.starts_with("toolongstring"), false);
  ensure_equals("TEST 60.4: ends_with", str.ends_with("ółć"), true);
  ensure_equals("TEST 60.5: ends_with", str.ends_with("ÓŁa"), false);
  ensure_equals("TEST 60.6: ends_with", str.ends_with("toolongstring"), false);
  ensure_equals("TEST 60.7: contains", str.contains("żół"), true);
  ensure_equals("TEST 60.8: contains", str.contains("ŻÓŁ"), false);
  ensure_equals("TEST 60.9: contains", str.contains("ŻÓŁ", false), true);
  ensure_equals("TEST 60.10: contains", str.contains("", false), false);
}

/*
 * Testing functions charIndexToByteOffset, byteOffsetToCharIndex
 */
TEST_FUNCTION(65) {
  base::utf8string str = std::string("zażółć");
  ensure_equals("TEST 65.1: charIndexToByteOffset", str.charIndexToByteOffset(2), (size_t)2);
  ensure_equals("TEST 65.2: charIndexToByteOffset", str.charIndexToByteOffset(3), (size_t)4);
  ensure_equals("TEST 65.3: charIndexToByteOffset", str.charIndexToByteOffset(4), (size_t)6);
  ensure_equals("TEST 65.4: byteOffsetToCharIndex", str.byteOffsetToCharIndex(2), (size_t)2);
  ensure_equals("TEST 65.5: byteOffsetToCharIndex", str.byteOffsetToCharIndex(5), (size_t)4);
  ensure_equals("TEST 65.6: byteOffsetToCharIndex", str.byteOffsetToCharIndex(6), (size_t)4);
}

/*
 * Testing itertaor
 */
TEST_FUNCTION(70) {
  base::utf8string str = std::string("zażółć");
  base::utf8string::iterator it = str.begin();
  ensure_equals("TEST 70.1: begin", *it, base::utf8string::utf8char("z"));
  ensure_equals("TEST 70.1: == begin", it == str.begin(), true);
  ensure_equals("TEST 70.2: != end", it != str.end(), true);
  ++it;
  ensure_equals("TEST 70.3: ++it", *it, base::utf8string::utf8char("a"));
  for (size_t i = 0; i < 5; i++) {
    ++it;
  }
  ensure_equals("TEST 70.4: ==end", it == str.end(), true);
  --it;
  ensure_equals("TEST 70.5: --it", *it, base::utf8string::utf8char("ć"));
}

END_TESTS
