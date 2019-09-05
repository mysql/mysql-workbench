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

#include "base/sqlstring.h"

#include "casmine.h"

namespace {

$ModuleEnvironment() {};

$TestData {
  std::string long_random_string; // Content doesn't matter. There must be no crash using it.
};

$describe("string utilities") {

  $beforeAll([this]() {
    for (int i = 0; i < 1000; i++) {
      data->long_random_string += ' ' + rand() % 94; // Visible characters after space

      if (rand() > RAND_MAX / 2)
        data->long_random_string += "\xE3\x8A\xA8"; // Valid Unicode character.
      if (i == 500)
        data->long_random_string += "\xE3\x8A\xA8"; // Ensure it is there at least once.
    }
    data->long_random_string.erase(std::remove(data->long_random_string.begin(), data->long_random_string.end(), 0x7f),
                             data->long_random_string.end()); // 0x7F is a special character that we use for tests
  });

  $it("base::quote_identifier", []() {
    $expect(base::quote_identifier("first_test", '`')).toBe("`first_test`");
    $expect(base::quote_identifier("second_test", '\"')).toBe("\"second_test\"");
    $expect(base::quote_identifier("", '\"')).toBe("\"\"");
    // UTF-8 encoded: CIRCLED IDEOGRAPH RIGHT
    $expect(base::quote_identifier("Unicode \xE3\x8A\xA8", '%')).toBe("%Unicode \xE3\x8A\xA8%");
  });

  $it("base::quoteIdentifierIfNeeded", []() {
    $expect(base::quoteIdentifierIfNeeded("first_test", '`', base::MySQLVersion::MySQL80)).toBe("first_test");
    $expect(base::quoteIdentifierIfNeeded("second_test", '\"', base::MySQLVersion::MySQL80)).toBe("second_test");

    // UTF-8 encoded: CIRCLED IDEOGRAPH RIGHT
    $expect(base::quoteIdentifierIfNeeded("Unicode\xE3\x8A\xA8", '%', base::MySQLVersion::MySQL80))
      .toBe("Unicode\xE3\x8A\xA8");
    $expect(base::quoteIdentifierIfNeeded("test.case", '$', base::MySQLVersion::MySQL80)).toBe("$test.case$");

    // Note: currently there is no support to check if the given string contains the quote char already.
    $expect(base::quoteIdentifierIfNeeded("test$case", '$', base::MySQLVersion::MySQL80)).toBe("test$case");
    $expect(base::quoteIdentifierIfNeeded(".test$case", '$', base::MySQLVersion::MySQL80)).toBe("$.test$case$");
    $expect(base::quoteIdentifierIfNeeded("test-case", '`', base::MySQLVersion::MySQL80)).toBe("`test-case`");

    // Identifiers consisting only of digits cannot be distinguished from normal numbers
    // so they must be quoted.
    $expect(base::quoteIdentifierIfNeeded("12345", '`', base::MySQLVersion::MySQL80)).toBe("`12345`");
  });

  $it("base::unquote_identifier", []() {
    std::string test = "\"first_test\"";
    std::string test_result = base::unquote_identifier(test);
    $expect(test_result).toBe("first_test");

    test = "`second_test`";
    test_result = base::unquote_identifier(test);
    $expect(test_result).toBe("second_test");
  });

  $it("base::split_token_list", []() {
    std::vector<std::string> empty = { "" };
    std::vector<std::string> empty2 = { "", "" };
    std::vector<std::string> empty3 = { "", "", "" };
    std::vector<std::string> null_empty = { "NULL", "" };
    std::vector<std::string> a = { "a" };
    std::vector<std::string> a_empty1 = { "a", "" };
    std::vector<std::string> a_empty2 = { "a", "", "" };
    std::vector<std::string> ab_empty1 = { "a", "b", "" };
    std::vector<std::string> ab_empty2 = { "a", "", "b" };
    std::vector<std::string> ab_empty3 = { "", "a", "b" };
    std::vector<std::string> null_null = { "NULL", "NULL" };
    std::vector<std::string> emptys_null = { "''", "NULL" };
    std::vector<std::string> ab_null = { "'a,b'", "NULL" };
    std::vector<std::string> ab_xxx = { "'a,b'", "\"x\\xx\"", "'fo''bar'" };

    $expect(base::split_token_list("", ',')).toEqual(empty);
    $expect(base::split_token_list(",", ',')).toEqual(empty2);
    $expect(base::split_token_list(" ,", ',')).toEqual(empty2);
    $expect(base::split_token_list(", ", ',')).toEqual(empty2);
    $expect(base::split_token_list(",,", ',')).toEqual(empty3);
    $expect(base::split_token_list("NULL,", ',')).toEqual(null_empty);
    $expect(base::split_token_list("a", ',')).toEqual(a);
    $expect(base::split_token_list("a,", ',')).toEqual(a_empty1);
    $expect(base::split_token_list("a,,", ',')).toEqual(a_empty2);
    $expect(base::split_token_list("a,b,", ',')).toEqual(ab_empty1);
    $expect(base::split_token_list("a,,b", ',')).toEqual(ab_empty2);
    $expect(base::split_token_list(",a,b", ',')).toEqual(ab_empty3);
    $expect(base::split_token_list("NULL,", ',')).toEqual(null_empty);
    $expect(base::split_token_list("NULL,NULL", ',')).toEqual(null_null);
    $expect(base::split_token_list("'',NULL", ',')).toEqual(emptys_null);
    $expect(base::split_token_list("'a,b',NULL", ',')).toEqual(ab_null);
    $expect(base::split_token_list("'a,b' , \"x\\xx\",'fo''bar'   ", ',')).toEqual(ab_xxx);
  });

  $it("split_by_set", [this]() {
    std::vector<std::string> input;
    $expect(base::split_by_set("", "")).toEqual(input);
    $expect(base::split_by_set("", " ")).toEqual(input);
    $expect(base::split_by_set("", data->long_random_string)).toEqual(input);

    std::vector<std::string> input2 = {data-> long_random_string.c_str() };
    $expect(base::split_by_set(data->long_random_string, "")).toEqual(input2);
    $expect(base::split_by_set(data->long_random_string, "\xA8").size()).toBeGreaterThan(1U); // Works only because our implementation is not utf-8 aware.

    std::vector<std::string> input3 = { "Lorem", "ipsum", "dolor", "sit", "amet." };
    $expect(base::split_by_set("Lorem ipsum dolor sit amet.", " ")).toEqual(input3);
    std::vector<std::string> input4 = { "Lorem", "ipsum", "dolor sit amet." };
    $expect(base::split_by_set("Lorem ipsum dolor sit amet.", " ", 2)).toEqual(input4);

    std::vector<std::string> input5 = { "\"Lorem\"", "\"ipsum\"", "\"dolor\"", "\"sit\"", "\"amet\"" };
    $expect(base::split_by_set("\"Lorem\"\t\"ipsum\"\t\"dolor\"\t\"sit\"\t\"amet\"", "\t")).toEqual(input5);

    std::vector<std::string> input6 = { "\"Lorem\"", "\"ipsum\"", "\"dolor\"", "\"sit\"", "\"amet\"" };
    $expect(base::split_by_set("\"Lorem\"\t\"ipsum\"\n\"dolor\"\t\"sit\"\n\"amet\"", " \t\n")).toEqual(input6);

    std::vector<std::string> input7 = { "\"Lorem\"", "\"ip", "sum\"", "\"dolor\"", "\"sit\"", "\"amet\"" };
    $expect(base::split_by_set("\"Lorem\"\t\"ip sum\"\n\"dolor\"\t\"sit\"\n\"amet\"", " \t\n")).toEqual(input7);

    std::vector<std::string> input8 = { "", "Lorem", "", " ", "ipsum", "", " ", "dolor", "", " ", "sit", "", " ", "amet", "" };
    $expect(base::split_by_set("\"Lorem\", \"ipsum\", \"dolor\", \"sit\", \"amet\"", ",\"")).toEqual(input8);
    $expect(base::split_by_set("\"Lorem\", \"ipsum\", \"dolor\", \"sit\", \"amet\"", ",\"", 100)).toEqual(input8);
    std::vector<std::string> input9 = { "", "Lorem", "", " ", "ipsum", "", " ", "dolor\", \"sit\", \"amet\"" };
    $expect(base::split_by_set("\"Lorem\", \"ipsum\", \"dolor\", \"sit\", \"amet\"", ",\"", 7)).toEqual(input9);
  });

  $it("trim_right, trim_left, trim", [this]() {
    $expect(base::trim_left("")).toBe("");
    $expect(base::trim_left("                                       ")).toBe("");
    $expect(base::trim_left("           \n\t\t\t\t      a")).toBe("a");
    $expect(base::trim_left("a           \n\t\t\t\t      ")).toBe("a           \n\t\t\t\t      ");
    $expect(base::trim_left("", data->long_random_string)).toBe("");

    $expect(base::trim_left("\xE3\x8A\xA8\xE3\x8A\xA8\x7F", data->long_random_string)).toBe("\x7F");
    $expect(base::trim_left("\t\t\tLorem ipsum dolor sit amet\n\n\n")).toBe("Lorem ipsum dolor sit amet\n\n\n");
    $expect(base::trim_left("\t\t\tLorem ipsum dolor sit amet\n\n\n", "L")).toBe("\t\t\tLorem ipsum dolor sit amet\n\n\n");
    $expect(base::trim_left("\t\t\tLorem ipsum dolor sit amet\n\n\n", "\t\t\tL")).toBe("orem ipsum dolor sit amet\n\n\n");

    $expect(base::trim_right("")).toBe("");
    $expect(base::trim_right("                                       ")).toBe( "");
    $expect(base::trim_right("           \n\t\t\t\t      a")).toBe("           \n\t\t\t\t      a");
    $expect(base::trim_right("a           \n\t\t\t\t      ")).toBe("a");
    $expect(base::trim_right("", data->long_random_string)).toBe("");

    $expect(base::trim_right("\x7F\xE3\x8A\xA8\xE3\x8A\xA8", data->long_random_string)).toBe("\x7F");
    $expect(base::trim_right("\t\t\tLorem ipsum dolor sit amet\n\n\n")).toBe("\t\t\tLorem ipsum dolor sit amet");
    $expect(base::trim_right("\t\t\tLorem ipsum dolor sit amet\n\n\n", "L")).toBe("\t\t\tLorem ipsum dolor sit amet\n\n\n");
    $expect(base::trim_right("\t\t\tLorem ipsum dolor sit amet\n\n\n", "\n\n\nt")).toBe("\t\t\tLorem ipsum dolor sit ame");

    $expect(base::trim("")).toBe("");
    $expect(base::trim("                                       ")).toBe("");
    $expect(base::trim("           \n\t\t\t\t      a")).toBe("a");
    $expect(base::trim("a           \n\t\t\t\t      ")).toBe("a");
    $expect(base::trim("", data->long_random_string)).toBe("");

    $expect(base::trim("\xE3\x8A\xA8\xE3\x8A\xA8\x7F\xE3\x8A\xA8\xE3\x8A\xA8", data->long_random_string)).toBe("\x7F");
    $expect(base::trim("\t\t\tLorem ipsum dolor sit amet\n\n\n")).toBe("Lorem ipsum dolor sit amet");
    $expect(base::trim("\t\t\tLorem ipsum dolor sit amet\n\n\n", "L")).toBe("\t\t\tLorem ipsum dolor sit amet\n\n\n");
    $expect(base::trim("\t\t\tLorem ipsum dolor sit amet\n\n\n", "\n\n\t\t\ttL")).toBe("orem ipsum dolor sit ame");
  });

  $it("Unescape + escape sequence handling", []() {
    std::string test_result = base::unescape_sql_string("", '`');
    $expect(test_result).toBe("");

    test_result = base::unescape_sql_string("lorem ipsum dolor sit amet", '"');
    $expect(test_result).toBe("lorem ipsum dolor sit amet");

    test_result = base::unescape_sql_string("lorem ipsum dolor`` sit amet", '"');
    $expect(test_result).toBe("lorem ipsum dolor`` sit amet");

    test_result = base::unescape_sql_string("lorem ipsum \"\"dolor sit amet", '"');
    $expect(test_result).toBe("lorem ipsum \"dolor sit amet");

    test_result = base::unescape_sql_string("lorem \"\"\"\"ipsum \"\"dolor\"\" sit amet", '"');
    $expect(test_result).toBe("lorem \"\"ipsum \"dolor\" sit amet");

    test_result = base::unescape_sql_string("lorem \\\"\\\"ipsum\"\" \\\\dolor sit amet", '"');
    $expect(test_result).toBe("lorem \"\"ipsum\" \\dolor sit amet");

    test_result = base::unescape_sql_string("lorem \\\"\\\"ipsum\"\" \\\\dolor sit amet", '\'');
    $expect(test_result).toBe("lorem \"\"ipsum\"\" \\dolor sit amet");

    // Embedded 0 is more difficult to test due to limitations when comparing strings. So we do this
    // in a separate test.
    test_result = base::unescape_sql_string("lorem\\n ip\\t\\rsum dolor\\b sit \\Zamet", '"');
    $expect(test_result).toBe("lorem\n ip\t\rsum dolor\b sit \032amet");

    test_result = base::unescape_sql_string("lorem ipsum \\zd\\a\\olor sit amet", '"');
    $expect(test_result).toBe("lorem ipsum zdaolor sit amet");

    test_result = base::unescape_sql_string("\\0\\n\\t\\r\\b\\Z", '"');
    $expect(test_result[0] == 0 && test_result[1] == 10 && test_result[2] == 9 &&
           test_result[3] == 13 && test_result[4] == '\b' &&
           test_result[5] == 26).toBeTrue();

    std::string long_string;
    long_string.resize(2000);
    std::fill(long_string.begin(), long_string.end(), '`');
    test_result = base::unescape_sql_string(long_string, '`');
    $expect(test_result).toBe(long_string.substr(0, 1000));
  });

  $it("Text reflow", []() {
    std::string content1 = "11111111 22222 3333 444444 555555555 666666 77777777 88888 999999999 00000000";
    std::string content2 =
      "\xC3\x81\xC3\x81\xC3\x81\xC3\x81\xC3\x81\xC3\x81\xC3\x81\xC3\x81 "
      "\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89 "
      "\xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D "
      "\xC3\x93\xC3\x93\xC3\x93\xC3\x93\xC3\x93\xC3\x93\xC3\x93\xC3\x93 "
      "\xC3\x9A\xC3\x9A\xC3\x9A\xC3\x9A\xC3\x9A\xC3\x9A\xC3\x9A\xC3\x9A";
    std::string content3 =
      "111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111"
      "11111111111111111111111111";
    std::string content4 = "aaaaa bbbbb cccc dddd \xAA\xBB\xCC\xDD";
    std::string content5 = "aaaa bbbb cccc dddd eeee";
    //   模板名，使用英文，保证唯一性。格式建议：“类型_动作”，如“blog_add”或“credit_blog_add”
    //   Template name, in English, to ensure uniqueness. Format advice: "type _ action," such as "blog_add" or
    //   "credit_blog_add"
    unsigned char content6[] = {
      0x20, 0x20, 0x6e, 0x61, 0x6d, 0x65, 0x3a, 0x20, 0xe6, 0xa8, 0xa1, 0xe6, 0x9d, 0xbf, // 10
      0xe5, 0x90, 0x8d, 0xef, 0xbc, 0x8c, 0xe4, 0xbd, 0xbf, 0xe7, 0x94, 0xa8, 0xe8, 0x8b, 0xb1, 0xe6, 0x96, 0x87, 0xef,
      0xbc, 0x8c, 0xe4, 0xbf, 0x9d, 0xe8, 0xaf, 0x81, 0xe5, 0x94, 0xaf, // 20
      0xe4, 0xb8, 0x80, 0xe6, 0x80, 0xa7, 0xe3, 0x80, 0x82, 0xe6, 0xa0, 0xbc, 0xe5, 0xbc, 0x8f, 0xe5, 0xbb, 0xba, 0xe8,
      0xae, 0xae, 0xef, 0xbc, 0x9a, 0xe2, 0x80, 0x9c, 0xe7, 0xb1, 0xbb, // 30
      0xe5, 0x9e, 0x8b, 0x5f, 0xe5, 0x8a, 0xa8, 0xe4, 0xbd, 0x9c, 0xe2, 0x80, 0x9d, 0xef, 0xbc, 0x8c, 0xe5, 0xa6, 0x82,
      0xe2, 0x80, 0x9c, 0x62, 0x6c, // 40
      0x6f, 0x67, 0x5f, 0x61, 0x64, 0x64, 0xe2, 0x80, 0x9d, 0xe6, 0x88, 0x96, 0xe2, 0x80, 0x9c, 0x63, // 50
      0x72, 0x65, 0x64, 0x69, 0x74, 0x5f, 0x62, 0x6c, 0x6f, 0x67, // 60
      0x5f, 0x61, 0x64, 0x64, 0xe2, 0x80, 0x9d, // 65
      0x00
    };

    std::string expected1 =
      "  11111111 22222 \n  3333 444444 \n  555555555 666666 \n  77777777 88888 \n  999999999 \n  00000000";
    std::string expected2 =
      "11111111 22222 \n  3333 444444 \n  555555555 666666 \n  77777777 88888 \n  999999999 \n  00000000";
    std::string expected3 =
      "  \xC3\x81\xC3\x81\xC3\x81\xC3\x81\xC3\x81\xC3\x81\xC3\x81\xC3\x81 "
      "\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89 \n  "
      "\xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D \n  "
      "\xC3\x93\xC3\x93\xC3\x93\xC3\x93\xC3\x93\xC3\x93\xC3\x93\xC3\x93 "
      "\xC3\x9A\xC3\x9A\xC3\x9A\xC3\x9A\xC3\x9A\xC3\x9A\xC3\x9A\xC3\x9A";
    std::string expected4 = "  11111111 22222 \n  3333 444444 \n(...)";
    std::string expected5 =
      "  111111111111111111\n  111111111111111111\n  111111111111111111\n  111111111111111111\n  111111111111111111\n  "
      "111111111111111111\n  111111111111111111\n  11111111111111";
    std::string expected6 = "";
    std::string expected7 = "aaaa \nbbbb \ncccc \ndddd \neeee";

    std::string result;

    $expect([&]() { base::reflow_text(content4, 20, "  "); }).toThrowError<std::invalid_argument>("^base::reflow_text received an invalid string: .*");
    $expect([&]() { result = base::reflow_text(content1, 20, "  ", true, 10); }).Not.toThrow();

    $expect(result).toBe(expected1);

    //  Do not indent first line
    $expect([&]() { result = base::reflow_text(content1, 20, "  ", false, 10); }).Not.toThrow();
    $expect(result).toBe(expected2);

    //  String with multi-byte characters
    $expect([&]() { result = base::reflow_text(content2, 20, "  ", true, 10); }).Not.toThrow();
    $expect(result).toBe(expected3);

    //  Line limit reached
    $expect([&]() { result = base::reflow_text(content1, 20, "  ", true, 2); }).Not.toThrow();
    $expect(result).toBe(expected4);

    //  Big word
    $expect([&]() { result = base::reflow_text(content3, 20, "  ", true, 10); }).Not.toThrow();
    $expect(result).toBe(expected5);

    //  Empty string
    $expect([&]() { result = base::reflow_text("", 20, "  ", true, 10); }).Not.toThrow();
    $expect(result).toBe(expected6);

    //  Left fill automatic removal
    $expect([&]() { result = base::reflow_text(content5, 6, "    ", true, 10); }).Not.toThrow();
    $expect(result).toBe(expected7);

    //  Invalid line length
    $expect([&]() { result = base::reflow_text(content5, 4, "    ", true, 10); }).Not.toThrow();
    $expect(result).toBe("");

    //  This test is to ensure that a big string won't mess up algorithm
    std::string dictionary[] = { "one", "big", "speech", "made", "words", "a", "of", "out" };
    std::string long_text;

    while (long_text.size() < SHRT_MAX) {
      int index = rand() % 8; //  8 is the size of the dictionary
      long_text += dictionary[index] + " ";
    }

    //  Short string, long line
    $expect([&]() { result = base::reflow_text(long_text, 100, "  ", true, 1000); }).Not.toThrow();

    //  Short string, long line
    $expect([&]() { result = base::reflow_text(std::string((char *)content6), 10, "  ", false); }).Not.toThrow();

    //  Remove the line feed and fill to verify coherence
    std::size_t position = 0;
    while ((position = result.find("\n  ", position)) != std::string::npos)
      result.replace(position, 3, "");

    $expect(result).toBe(std::string((char *)content6));

    //  This test was a specific case of a bug
    //  Short string, long line
    $expect([&]() { result = base::reflow_text(std::string((char *)content6), 60, "    "); }).Not.toThrow();
  });

  $it("atoi", []() {
    $expect(base::atoi<int>("10G", 0)).toEqual(10);
    $expect(base::atoi<int>("10", 0)).toEqual(10);
    $expect(base::atoi<int>("G", -1)).toBe(-1);
    $expect([&]() { base::atoi<int>("G"); }).toThrow();
  });

  $it("Prefix + suffix", []() {
    std::string test_string = "This is a test string...to test.";
    std::string test_string_unicode =
      "\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89...Now that is a unicode string...\xE3\x8A\xA8";

    //  Base tests
    $expect(base::hasPrefix(test_string, "This")).toBeTrue();
    $expect(base::hasPrefix(test_string, "is")).toBeFalse();
    $expect(base::hasPrefix(test_string, "test.")).toBeFalse();
    $expect(base::hasPrefix(test_string, "blablabla")).toBeFalse();
    $expect(base::hasPrefix(test_string, "")).toBeTrue();
    $expect(base::hasPrefix(test_string, "his")).toBeFalse();
    $expect(base::hasPrefix(test_string, test_string)).toBeTrue();
    $expect(base::hasPrefix(test_string, test_string + " Second part...")).toBeFalse();
    $expect(base::hasPrefix("", "blablabla")).toBeFalse();
    $expect(base::hasPrefix("", "")).toBeTrue();

    $expect(base::hasSuffix(test_string, "test.")).toBeTrue();
    $expect(base::hasSuffix(test_string, "to ")).toBeFalse();
    $expect(base::hasSuffix(test_string, "This")).toBeFalse();
    $expect(base::hasSuffix(test_string, "blablabla")).toBeFalse();
    $expect(base::hasSuffix(test_string, "")).toBeTrue();
    $expect(base::hasSuffix(test_string, "test")).toBeFalse();
    $expect(base::hasSuffix(test_string, test_string)).toBeTrue();
    $expect(base::hasSuffix(test_string, test_string + " Second part...")).toBeFalse();
    $expect(base::hasSuffix("", "blablabla")).toBeFalse();
    $expect(base::hasSuffix("", "")).toBeTrue();

    // Unicode tests
    $expect(base::hasPrefix(test_string_unicode, "\xC3\x89\xC3\x89")).toBeTrue();
    $expect(base::hasPrefix(test_string_unicode, "is")).toBeFalse();
    $expect(base::hasPrefix(test_string_unicode, "\xE3\x8A\xA8")).toBeFalse();
    $expect(base::hasPrefix(test_string_unicode, "blablabla")).toBeFalse();
    $expect(base::hasPrefix(test_string_unicode, "")).toBeTrue();
    $expect(base::hasPrefix(test_string_unicode, "\x89\xC3\x89\xC3")).toBeFalse();
    $expect(base::hasPrefix(test_string_unicode, test_string_unicode)).toBeTrue();
    $expect(base::hasPrefix(test_string_unicode, test_string_unicode + ". Second part...")).toBeFalse();

    $expect(base::hasSuffix(test_string_unicode, ".\xE3\x8A\xA8")).toBeTrue();
    $expect(base::hasSuffix(test_string_unicode, "to ")).toBeFalse();
    $expect(base::hasSuffix(test_string_unicode, "\xC3\x89\xC3\x89")).toBeFalse();
    $expect(base::hasSuffix(test_string_unicode, "blablabla")).toBeFalse();
    $expect(base::hasSuffix(test_string_unicode, "")).toBeTrue();
    $expect(base::hasSuffix(test_string_unicode, ".\xE3\x8A")).toBeFalse();
    $expect(base::hasSuffix(test_string_unicode, test_string_unicode)).toBeTrue();
    $expect(base::hasSuffix(test_string_unicode, test_string_unicode + ". Second part...")).toBeFalse();
  });

  $it("Text extraction", []() {
    std::string test_string = "This is a test string...to test.";
    std::string test_string_unicode =
    "\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89...Now that is a unicode string...\xE3\x8A\xA8";

    // Base tests
    $expect(base::left(test_string, 5)).toBe("This ");
    $expect(base::left(test_string, 1)).toBe("T");
    $expect(base::left(test_string, 0)).toBe("");
    $expect(base::left("", 5)).toBe("");
    $expect(base::left(test_string, test_string.length())).toBe(test_string);
    $expect(base::left(test_string, 50)).toBe(test_string);

    $expect(base::right(test_string, 5)).toBe("test.");
    $expect(base::right(test_string, 1)).toBe(".");
    $expect(base::right(test_string, 0)).toBe("");
    $expect(base::right("", 5)).toBe("");
    $expect(base::right(test_string, test_string.length())).toBe(test_string);
    $expect(base::right(test_string, 50)).toBe(test_string);

    //  Unicode tests
    $expect(base::left(test_string_unicode, 5)).toBe("\xC3\x89\xC3\x89\xC3");
    $expect(base::left(test_string_unicode, 1)).toBe("\xC3");
    $expect(base::left(test_string_unicode, 0)).toBe("");
    $expect(base::left("", 5)).toBe("");
    $expect(base::left(test_string_unicode, test_string_unicode.length())).toBe(test_string_unicode);
    $expect(base::left(test_string_unicode, 500)).toBe(test_string_unicode);

    $expect(base::right(test_string_unicode, 5)).toBe("..\xE3\x8A\xA8");
    $expect(base::right(test_string_unicode, 1)).toBe("\xA8");
    $expect(base::right(test_string_unicode, 0)).toBe("");
    $expect(base::right("", 5)).toBe("");
    $expect(base::right(test_string_unicode, test_string_unicode.length())).toBe(test_string_unicode);
    $expect(base::right(test_string_unicode, 500)).toBe(test_string_unicode);

  });

  $it("Font description parsing", []() {
    std::string font_description;
    std::string font;
    float size = 0;
    bool bold = false;
    bool italic = false;

    font_description = "Sans 10";
    $expect(base::parse_font_description(font_description, font, size, bold, italic));
    $expect(font).toBe("Sans");
    $expect(size).toEqual(10U);
    $expect(bold);
    $expect(italic);

    font_description = "Sans 12";
    $expect(base::parse_font_description(font_description, font, size, bold, italic)).toBeTrue();
    $expect(font).toBe("Sans");
    $expect(size).toBe(12);
    $expect(bold).toBeFalse();
    $expect(italic).toBeFalse();

    font_description = "Sans 10 bold";
    $expect(base::parse_font_description(font_description, font, size, bold, italic)).toBeTrue();
    $expect(font).toBe("Sans");
    $expect(size).toEqual(10U);
    $expect(bold).toBeTrue();
    $expect(italic).toBeFalse();

    font_description = "Sans 10 BOLD";
    $expect(base::parse_font_description(font_description, font, size, bold, italic)).toBeTrue();
    $expect(font).toBe("Sans");
    $expect(size).toEqual(10U);
    $expect(bold).toBeTrue();
    $expect(italic).toBeFalse();

    font_description = "Sans 10 italic";
    $expect(base::parse_font_description(font_description, font, size, bold, italic)).toBeTrue();
    $expect(font).toBe("Sans");
    $expect(size).toEqual(10U);
    $expect(bold).toBeFalse();
    $expect(italic).toBeTrue();

    font_description = "Sans 10 ITALIC";
    $expect(base::parse_font_description(font_description, font, size, bold, italic)).toBeTrue();
    $expect(font).toBe("Sans");
    $expect(size).toEqual(10U);
    $expect(bold).toBeFalse();
    $expect(italic).toBeTrue();

    font_description = "Sans 10 bold italic";
    $expect(base::parse_font_description(font_description, font, size, bold, italic)).toBeTrue();
    $expect(font).toBe("Sans");
    $expect(size).toEqual(10U);
    $expect(bold).toBeTrue();
    $expect(italic).toBeTrue();

    font_description = "Sans 10 BOLD ITALIC";
    $expect(base::parse_font_description(font_description, font, size, bold, italic)).toBeTrue();
    $expect(font).toBe("Sans");
    $expect(size).toEqual(10U);
    $expect(bold).toBeTrue();
    $expect(italic).toBeTrue();

    font_description = "My Font 10 BOLD ITALIC";
    $expect(base::parse_font_description(font_description, font, size, bold, italic)).toBeTrue();
    $expect(font).toBe("My Font");
    $expect(size).toEqual(10U);
    $expect(bold).toBeTrue();
    $expect(italic).toBeTrue();

    font_description = "Helvetica Bold 12";
    $expect(base::parse_font_description(font_description, font, size, bold, italic)).toBeTrue();
    $expect(font).toBe("Helvetica");
    $expect(size).toBe(12);
    $expect(bold).toBeTrue();
    $expect(italic).toBeFalse();
  });

  $it("Path normalization", []() {
    std::string separator(1, G_DIR_SEPARATOR);

    $expect(base::normalize_path("")).toBe("");
    $expect(base::normalize_path("/")).toBe(separator);
    $expect(base::normalize_path("\\")).toBe(separator);
    $expect(base::normalize_path("/////////")).toBe(separator);
    $expect(base::normalize_path("../../../")).toBe("");
    $expect(base::normalize_path("abc/././../def")).toBe("def");
    $expect(base::normalize_path("a/./b/.././d/./..")).toBe("a");
    $expect(base::normalize_path("a/b/c/../d/../")).toBe(base::replaceString("a/b/", "/", separator));
    $expect(base::normalize_path("/path///to/my//////dir")).toBe(base::replaceString("/path/to/my/dir", "/", separator));
    $expect(base::normalize_path("D:\\files\\to//scan")).toBe(base::replaceString("D:/files/to/scan", "/", separator));
  });
}

}
