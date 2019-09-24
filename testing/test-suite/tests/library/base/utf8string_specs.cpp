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

#include "base/utf8string.h"
#include "base/string_utilities.h"

#include "casmine.h"

namespace {

$ModuleEnvironment() {};

struct LangStringDetails {
  const char *const _text = nullptr;
  size_t _length;
  size_t _bytes;
  LangStringDetails() = default;
  LangStringDetails(const LangStringDetails &) = default;
  LangStringDetails(const char *text, size_t length, size_t bytes) : _text(text), _length(length), _bytes(bytes) {
  }
};

$TestData {
  const std::map<std::string, LangStringDetails> languageStrings = {
    { "english", { "This is a lazy test", 19, 19 }},
    { "polish", { "zażółć", 6, 10 }},
    { "greek", { "ὕαλον ϕαγεῖν", 12, 25 }},
    { "russian", { "Я могу есть стекло", 18, 33 }},
    { "arabic", { "هذا لا يؤلمني", 13, 24 }},
    { "chinese", { "我可以吞下茶", 6, 18 }},
    { "japanese", { "私はお茶を飲み込むことができます", 16, 48 }},
    { "portuguese", { "Há açores e cães ávidos no chão", 31, 36 }}};

  std::map<std::string, const char *> substrings = {
    { "english", "his " }, { "polish", "ażół" }, { "greek", "αλον"}, { "russian", " мог" },
    { "arabic", "ذا ل" },  { "chinese", "可以吞下" }, { "japanese", "はお茶を" }, { "portuguese", "á aç" }};
};

$describe("utf8string") {

  $it("constructor", []() {
    base::utf8string str1;
    $expect(str1.validate()).toBe(true);
    $expect(str1.length()).toBe(0U);
    $expect(str1.bytes()).toBe(0U);
    $expect(str1.empty()).toBe(true);

    base::utf8string str2 = "";
    $expect(str2.validate()).toBe(true);
    $expect(str2.length()).toBe(0U);
    $expect(str2.bytes()).toBe(0U);
    $expect(str2.empty()).toBe(true);
  });

  $it("utf8string(const char *s)", [this]() {
    // TODO: update cycle name
    for (auto iter : data->languageStrings) {
      LangStringDetails &current = iter.second;
      base::utf8string str1(current._text); //  from char *

      $expect(str1.validate()).toBe(true);
      $expect(str1).toEqual(current._text);
      $expect(str1.length()).toBe(current._length);
      $expect(str1.bytes()).toBe(current._bytes);
      $expect(str1.empty()).toBe(false);
    }
  });

  $it("utf8string(const std::string &s)", [this]() {
    for (auto iter : data->languageStrings) {
      LangStringDetails &current = iter.second;
      std::string str_to_init(current._text);
      base::utf8string str2(str_to_init); //  from std::string

      $expect(str2.validate()).toBe(true);
      $expect(str2).toEqual(current._text);
      $expect(str2.length()).toBe(current._length);
      $expect(str2.bytes()).toBe(current._bytes);
      $expect(str2.empty()).toBe(false);
    }
  });

  $it("utf8string(const char *s)", [this]() {
    for (auto iter : data->languageStrings) {
      LangStringDetails &current = iter.second;
      base::utf8string str_to_init(current._text);
      base::utf8string str1(str_to_init); //  from char *

      $expect(str1.validate()).toBe(true);
      $expect(str1).toEqual(current._text);
      $expect(str1.length()).toBe(current._length);
      $expect(str1.bytes()).toBe(current._bytes);
      $expect(str1.empty()).toBe(false);
    }
  });

  $it("utf8string(const wchar_t *s)", []() {
    $pending("needs implementation");
  });

  $it("utf8string(const std::wstring &s)", []() {
    $pending("needs implementation");
  });

  $it("utf8string(const char *s, size_t pos, size_t len)", [this]() {
    for (auto iter : data->languageStrings) {
      LangStringDetails &current = iter.second;

      base::utf8string str1(current._text, 1, 4);   //  from char *
      base::utf8string str2(current._text, 500, 4); //  sub-string from invalid index
      base::utf8string str3(current._text, 1, 500); //  sub-string with huge length
      base::utf8string str4(current._text, 1, 0);   //  sub-string with zero length

      base::utf8string right_to_compare = base::utf8string(current._text).right(current._length - 1);

      $expect(str1.validate()).toBe(true);
      $expect(str1).toEqual(data->substrings[iter.first]);
      $expect(str1.size()).toEqual(4U);
      $expect(str1.empty()).toBe(false);

      $expect(str2.validate()).toBe(true);
      $expect(str2.size()).toEqual(0U);
      $expect(str2.length()).toEqual(0U);
      $expect(str2.empty()).toBe(true);

      $expect(str3.validate()).toBe(true);
      $expect(str3).toEqual(right_to_compare);
      $expect(str3.size()).toBe(current._length - 1);
      $expect(str3.length()).toBe(current._length - 1);
      $expect(str3.empty()).toBe(false);

      $expect(str4.validate()).toBe(true);
      $expect(str4.size()).toEqual(0U);
      $expect(str4.length()).toEqual(0U);
      $expect(str4.empty()).toBe(true);
    }
  });

  $it("utf8string(const std::string &str, size_t pos, size_t len)", [this]() {
    for (auto iter : data->languageStrings) {
      LangStringDetails &current = iter.second;

      std::string str_to_init(current._text);

      base::utf8string str1(str_to_init, 1, 4);   //  from char *
      base::utf8string str2(str_to_init, 500, 4); //  sub-string from invalid index
      base::utf8string str3(str_to_init, 1, 500); //  sub-string with huge length
      base::utf8string str4(str_to_init, 1, 0);   //  sub-string with zero length

      base::utf8string right_to_compare = base::utf8string(current._text).right(current._length - 1);

      $expect(str1.validate()).toBe(true);
      $expect(str1).toEqual(data->substrings[iter.first]);
      $expect(str1.size()).toEqual(4U);
      $expect(str1.empty()).toBe(false);

      $expect(str2.validate()).toBe(true);
      $expect(str2.size()).toEqual(0U);
      $expect(str2.length()).toEqual(0U);
      $expect(str2.empty()).toBe(true);

      $expect(str3.validate()).toBe(true);
      $expect(str3).toEqual(right_to_compare);
      $expect(str3.size()).toBe(current._length - 1);
      $expect(str3.length()).toBe(current._length - 1);
      $expect(str3.empty()).toBe(false);

      $expect(str4.validate()).toBe(true);
      $expect(str4.size()).toEqual(0U);
      $expect(str4.length()).toEqual(0U);
      $expect(str4.empty()).toBe(true);
    }
  });


  $it("utf8string(const utf8string &str, size_t pos, size_t len)", [this]() {
    for (auto iter : data->languageStrings) {
      LangStringDetails &current = iter.second;

      base::utf8string str_to_init(current._text);

      base::utf8string str1(str_to_init, 1, 4);   //  from char *
      base::utf8string str2(str_to_init, 500, 4); //  sub-string from invalid index
      base::utf8string str3(str_to_init, 1, 500); //  sub-string with huge length
      base::utf8string str4(str_to_init, 1, 0);   //  sub-string with zero length

      base::utf8string right_to_compare = base::utf8string(current._text).right(current._length - 1);

      $expect(str1.validate()).toBe(true);
      $expect(str1).toEqual(data->substrings[iter.first]);
      $expect(str1.size()).toEqual(4U);
      $expect(str1.empty()).toBe(false);

      $expect(str2.validate()).toBe(true);
      $expect(str2.size()).toEqual(0U);
      $expect(str2.length()).toEqual(0U);
      $expect(str2.empty()).toBe(true);

      $expect(str3.validate()).toBe(true);
      $expect(str3).toEqual(right_to_compare);
      $expect(str3.size()).toBe(current._length - 1);
      $expect(str3.length()).toBe(current._length - 1);
      $expect(str3.empty()).toBe(false);

      $expect(str4.validate()).toBe(true);
      $expect(str4.size()).toEqual(0U);
      $expect(str4.length()).toEqual(0U);
      $expect(str4.empty()).toBe(true);
    }
  });

  $it("utf8string(size_t size, char c)", []() {
    base::utf8string str(10, 'a');

    $expect(str.validate()).toBe(true);
    $expect(str).toEqual("aaaaaaaaaa");
    $expect(str.size()).toEqual(10U);
    $expect(str.length()).toEqual(10U);
    $expect(str.bytes()).toEqual(10U);
    $expect(str.empty()).toBe(false);
  });

  $it("utf8string(size_t size, utf8char c) [unicode]", []() {
    base::utf8string str(10, base::utf8string::utf8char("ł"));

    $expect(str.validate()).toBe(true);
    $expect(str).toEqual("łłłłłłłłłł");
    $expect(str.size()).toEqual(10U);
    $expect(str.length()).toEqual(10U);
    $expect(str.bytes()).toBe(20U);
    $expect(str.empty()).toBe(false);
  });

  $it("utf8string(size_t size, utf8char c) [non-unicode]", []() {
    base::utf8string str(10, base::utf8string::utf8char("a"));

    $expect(str.validate()).toBe(true);
    $expect(str).toEqual("aaaaaaaaaa");
    $expect(str.size()).toEqual(10U);
    $expect(str.length()).toEqual(10U);
    $expect(str.bytes()).toEqual(10U);
    $expect(str.empty()).toBe(false);
  });

  $it("operator[], at()", []() {
    base::utf8string str = std::string("zażółć");
    base::utf8string::utf8char res1("ó");
    base::utf8string::utf8char res2("ć");
    $expect(str[3]).toEqual(res1);
    $expect(str[5]).toEqual(res2);

    // TODO: test utf8string::at()
  });

  $it("utf8string substr", []() {
    base::utf8string str = std::string("zażółć");
    base::utf8string res1 = "żółć";
    base::utf8string res2 = "aż";
    $expect(str.substr(2)).toEqual(res1);
    $expect(str.substr(1, 2)).toEqual(res2);
  });

  $it("operators +, +=, =, ==, !=", []() {
    base::utf8string str1 = std::string("zażółć");
    base::utf8string str2 = std::string("gęślą");
    base::utf8string result = std::string("zażółćgęślą");

    $expect(str1 + str2).toEqual(result);
    str1 += str2;
    $expect(str1).toEqual(result);
    str1 = str2;
    $expect(str1).toEqual(str2);
    $expect(str1 == str2).toBe(true);
    $expect(str1 != result).toBe(true);
  });

  $it("String conversion", [this]() {
    for (auto iter : data->languageStrings) {
      LangStringDetails &current = iter.second;

      base::utf8string str(current._text);
      $expect(strcmp(str.c_str(), current._text)).toEqual(0);
      $expect(str.to_string() == std::string(current._text)).toBe(true);
      $expect(str.to_wstring() == base::string_to_wstring(current._text)).toBe(true);
    }
  });

  $it("move constructor and operator=", []() {
    //  TODO: test in all languages
    base::utf8string str1(std::string("za") + std::string("żółć"));
    $expect(str1.length()).toEqual(6U);
    $expect(str1.bytes()).toEqual(10U);
    $expect(str1.empty()).toBe(false);

    base::utf8string str2 = std::move(str1);
    $expect(str2.length()).toEqual(6U);
    $expect(str2.bytes()).toEqual(10U);
    $expect(str2.empty()).toBe(false);
  });

  $it("trim* functions", []() {
    //  TODO: test in all languages
    $expect(base::utf8string("  zażółć    ").trim_left().length()).toEqual(10U);
    $expect(base::utf8string("  zażółć    ").trim_left().bytes()).toEqual(14U);
    $expect(base::utf8string("  zażółć    ").trim_right().length()).toEqual(8U);
    $expect(base::utf8string("  zażółć    ").trim_right().bytes()).toBe(12U);
    $expect(base::utf8string("  zażółć    ").trim().length()).toEqual(6U);
    $expect(base::utf8string("  zażółć    ").trim().bytes()).toEqual(10U);
  });

  $it("toUpper, toLower, truncate, toCaseFold, validate", []() {
    $expect(base::utf8string("zażółć").to_upper()).toEqual(base::utf8string("ZAŻÓŁĆ"));
    $expect(base::utf8string("ZAŻÓŁĆ").to_lower()).toEqual(base::utf8string("zażółć"));

    $expect(base::utf8string("zAżóŁć").to_case_fold()).toEqual(base::utf8string("zażółć"));
    $expect(base::utf8string("grüßen").validate()).toBe(true);
  });
  
  $it("truncate, substr, left, right", []() {
    $expect(base::utf8string("zażółć").truncate(0)).toEqual(base::utf8string("..."));
    $expect(base::utf8string("zażółć").truncate(1)).toEqual(base::utf8string("z..."));
    $expect(base::utf8string("zażółć").truncate(2)).toEqual(base::utf8string("za..."));
    $expect(base::utf8string("zażółć").truncate(3)).toEqual("zażółć");
    $expect(base::utf8string("zażółć").truncate(4)).toEqual("zażółć");
    $expect(base::utf8string("zażółć").truncate(5)).toEqual("zażółć");
    $expect(base::utf8string("zażółć").truncate(6)).toEqual("zażółć");
    $expect(base::utf8string("zażółć").truncate(7)).toEqual("zażółć");

    $expect(base::utf8string("zażółć").left(0)).toEqual("");
    $expect(base::utf8string("zażółć").left(1)).toEqual("z");
    $expect(base::utf8string("zażółć").left(2)).toEqual("za");
    $expect(base::utf8string("zażółć").left(3)).toEqual("zaż");
    $expect(base::utf8string("zażółć").left(4)).toEqual("zażó");
    $expect(base::utf8string("zażółć").left(5)).toEqual("zażół");
    $expect(base::utf8string("zażółć").left(6)).toEqual("zażółć");
    $expect(base::utf8string("zażółć").left(7)).toEqual("zażółć");

    $expect(base::utf8string("zażółć").right(0)).toEqual("");
    $expect(base::utf8string("zażółć").right(1)).toEqual("ć");
    $expect(base::utf8string("zażółć").right(2)).toEqual("łć");
    $expect(base::utf8string("zażółć").right(3)).toEqual("ółć");
    $expect(base::utf8string("zażółć").right(4)).toEqual("żółć");
    $expect(base::utf8string("zażółć").right(5)).toEqual("ażółć");
    $expect(base::utf8string("zażółć").right(6)).toEqual("zażółć");
    $expect(base::utf8string("zażółć").right(7)).toEqual("zażółć");
  });

  $it("starts_with, ends_with and contains", []() {
    base::utf8string str = std::string("zażółć");
    $expect(str.starts_with("za")).toBe(true);
    $expect(str.starts_with("kk")).toBe(false);
    $expect(str.starts_with("toolongstring")).toBe(false);
    $expect(str.ends_with("ółć")).toBe(true);
    $expect(str.ends_with("ÓŁa")).toBe(false);
    $expect(str.ends_with("toolongstring")).toBe(false);
    $expect(str.contains("żół")).toBe(true);
    $expect(str.contains("ŻÓŁ")).toBe(false);
    $expect(str.contains("ŻÓŁ", false)).toBe(true);
    $expect(str.contains("", false)).toBe(false);
  });

  $it("charIndexToByteOffset, byteOffsetToCharIndex", []() {
    base::utf8string str = std::string("zażółć");
    $expect(str.charIndexToByteOffset(2)).toEqual(2U);
    $expect(str.charIndexToByteOffset(3)).toEqual(4U);
    $expect(str.charIndexToByteOffset(4)).toEqual(6U);
    $expect(str.byteOffsetToCharIndex(2)).toEqual(2U);
    $expect(str.byteOffsetToCharIndex(5)).toEqual(4U);
    $expect(str.byteOffsetToCharIndex(6)).toEqual(4U);
  });

  $it("iterator", []() {
    base::utf8string str = std::string("zażółć");
    base::utf8string::iterator iter = str.begin();
    
    $expect(*iter).toEqual(base::utf8string::utf8char("z"));
    $expect(iter == str.begin()).toBe(true);
    $expect(iter == str.end()).toBe(false);
    ++iter;
    $expect(*iter).toEqual(base::utf8string::utf8char("a"));
    for (size_t i = 0; i < 5; i++) {
      ++iter;
    }
    $expect(iter == str.end()).toBe(true);
    --iter;
    $expect(*iter).toEqual(base::utf8string::utf8char("ć"));
  });
}

}
