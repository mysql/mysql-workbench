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

#define MAX_SIZE_RANDOM_VALUES_ARRAY 25
#define MAX_SIZE_SQLSTRING_ARRAY 10000

struct rdm_vals {
  int int_value;
  std::string string_value;
  double double_value;
  base::sqlstring sqlstring_value;
} random_values[MAX_SIZE_RANDOM_VALUES_ARRAY];

$TestData {
  std::string placeholders;
  std::string long_random_string; // Content doesn't matter. There must be no crash using it.
};

$describe("sqlstring") {

  $beforeAll([this]() {
    int i;
    double tmp1, tmp2;

    srand((unsigned int)time(NULL));
    for (i = 0; i < 1000; i++) {
      data->long_random_string += ' ' + rand() * 64 / 32768;
      if (rand() > 16767)
        data->long_random_string += "\xE3\x8A\xA8"; // Valid Unicode character.
      if (i == 500)
        data->long_random_string += "\xE3\x8A\xA8"; // Ensure it is there at least once.
    }

    data->placeholders.clear();

    for (i = 0; i < MAX_SIZE_RANDOM_VALUES_ARRAY; i++) {
      data->placeholders += "? ! ? ! ";

      random_values[i].int_value = rand();
      tmp1 = rand() % 100;     // An int value between 0 and 99
      tmp2 = rand() % 100 + 1; // An int value between 1 and 100
      random_values[i].double_value = tmp1 / tmp2;

      random_values[i].sqlstring_value =
        base::sqlstring(std::to_string(rand()).c_str(), base::sqlstring::sqlstringformat(0));
      random_values[i].string_value = std::to_string(rand());
    }
  });

  $it("overridden operator << for numeric values", []() {
    std::int64_t int64_t_value1 = 1999;
    std::int64_t int64_t_value2 = -1999;
    int int_value1 = -2001;
    int int_value2 = 2001;
    float float_value1 = 3.141592f;
    float float_value2 = -3.141592f;
    double double_value1 = 2.718281;
    double double_value2 = -2.718281;

    // test cases for simple use of operator <<
    $expect(base::sqlstring("first_test ?", 0) << int64_t_value1).toEqual("first_test 1999");
    $expect(base::sqlstring("second_test ?", 0) << int_value1).toEqual("second_test -2001");
    $expect(base::sqlstring("third_test ?", 0) << float_value1).toEqual("third_test 3.141592");
    $expect(base::sqlstring("fourth_test ?", 0) << double_value1).toEqual("fourth_test 2.718281");

    // test cases for composed use of operator <<
    $expect(base::sqlstring("? fifth_test ?", 0) << int64_t_value1 << int64_t_value2).toEqual("1999 fifth_test -1999");
    $expect(base::sqlstring("? sixth_test ?", 0) << int_value1 << int_value2).toEqual("-2001 sixth_test 2001");
    $expect(base::sqlstring("? seventh_test ?", 0) << float_value1 << float_value2).toEqual("3.141592 seventh_test -3.141592");
    $expect(base::sqlstring("? eighth_test ?", 0) << double_value1 << double_value2).toEqual("2.718281 eighth_test -2.718281");

  });

  $it("overridden operator << for numeric values [Corner/Limit Values]", []() {
    double NaN = 2.51;

    // test cases for simple use of operator <<
    $expect(base::sqlstring("?", 0) << INT_MIN).toEqual(std::to_string(INT_MIN));
    $expect(base::sqlstring("?", 0) << INT_MAX).toEqual(std::to_string(INT_MAX));
    $expect(base::sqlstring("?", 0) << LONG_MIN).toEqual(std::to_string(LONG_MIN));
    $expect(base::sqlstring("?", 0) << LONG_MAX).toEqual(std::to_string(LONG_MAX));
    $expect(base::sqlstring("?", 0) << NaN).Not.toEqual(std::to_string(0.0));
    $expect(base::sqlstring("?", 0) << SHRT_MIN).toEqual(std::to_string(SHRT_MIN));
    $expect(base::sqlstring("?", 0) << SHRT_MAX).toEqual(std::to_string(SHRT_MAX));
    $expect(base::sqlstring("?", 0) << LLONG_MIN).toEqual(std::to_string(LLONG_MIN));
    $expect(base::sqlstring("?", 0) << LLONG_MAX).toEqual(std::to_string(LLONG_MAX));
    $expect(base::sqlstring("?", 0) << CHAR_MIN).toEqual(std::to_string(CHAR_MIN));
    $expect(base::sqlstring("?", 0) << CHAR_MAX).toEqual(std::to_string(CHAR_MAX));
  });

  $it("overridden operator << for std::string values", []() {
    std::string tail = "TAIL";
    std::string head = "HEAD";

    // test cases for simple use of operator <<
    $expect(base::sqlstring("first_test ?", base::QuoteOnlyIfNeeded) << tail).toEqual("first_test 'TAIL'");
    $expect(base::sqlstring("second_test ?", base::UseAnsiQuotes) << tail).toEqual("second_test \"TAIL\"");
    $expect(base::sqlstring("third_test !", base::QuoteOnlyIfNeeded) << tail).toEqual("third_test TAIL");
    $expect(base::sqlstring("fourth_test !", base::UseAnsiQuotes) << tail).toEqual("fourth_test `TAIL`");

    // test cases for composed use of operator <<
    $expect(base::sqlstring("? fifth_test ?", base::QuoteOnlyIfNeeded) << head << tail).toEqual("'HEAD' fifth_test 'TAIL'");
    $expect(base::sqlstring("? sixth_test ?", base::UseAnsiQuotes) << head << tail).toEqual("\"HEAD\" sixth_test \"TAIL\"");
    $expect(base::sqlstring("! seventh_test !", base::QuoteOnlyIfNeeded) << head << tail).toEqual("HEAD seventh_test TAIL");
    $expect(base::sqlstring("! eighth_test !", base::UseAnsiQuotes) << head << tail).toEqual("`HEAD` eighth_test `TAIL`");
  });

  $it("overridden operator << for std::string values [Corner/Limit Values]", [this]() {
    // test cases for simple use of operator <<
    std::string test_result = "";
    std::string temp_long_random_string = "";
    std::string escaped = "";
    test_result.append(base::sqlstring("?", base::QuoteOnlyIfNeeded) << data->long_random_string);

    temp_long_random_string.append(base::escape_sql_string(data->long_random_string));
    temp_long_random_string.insert(0, "'");
    temp_long_random_string.insert(temp_long_random_string.size(), "'");

    $expect(test_result).toEqual(temp_long_random_string);

    test_result.clear();
    temp_long_random_string.clear();
    test_result.append(base::sqlstring("?", base::UseAnsiQuotes) << data->long_random_string);

    temp_long_random_string.append(base::escape_sql_string(data->long_random_string));
    temp_long_random_string.insert(0, "\"");
    temp_long_random_string.insert(temp_long_random_string.size(), "\"");

    $expect(test_result).toEqual(temp_long_random_string);

    test_result.clear();
    temp_long_random_string.clear();
    test_result.append(base::sqlstring("!", base::QuoteOnlyIfNeeded) << data->long_random_string);

    escaped = base::escape_backticks(data->long_random_string);
    temp_long_random_string.append(base::quoteIdentifierIfNeeded(escaped, '`', base::MySQLVersion::MySQL57));

    $expect(test_result).toEqual(temp_long_random_string);

    test_result.clear();
    temp_long_random_string.clear();
    escaped.clear();
    test_result.append(base::sqlstring("!", base::UseAnsiQuotes) << data->long_random_string);

    escaped = base::escape_backticks(data->long_random_string);
    temp_long_random_string.append(base::quote_identifier(escaped, '`'));

    $expect(test_result).toBe(temp_long_random_string);
  });

  $it("overridden operator << for (const char*) values", []() {
    const char* tail = "TAIL";
    const char* head = "HEAD";
    const char* null_str(0);

    // test cases for simple use of operator <<
    $expect(base::sqlstring("first_test ?", base::QuoteOnlyIfNeeded) << tail).toEqual("first_test 'TAIL'");
    $expect(base::sqlstring("second_test ?", base::UseAnsiQuotes) << tail).toEqual("second_test \"TAIL\"");
    $expect(base::sqlstring("third_test !", base::QuoteOnlyIfNeeded) << tail).toEqual("third_test TAIL");
    $expect(base::sqlstring("fourth_test !", base::UseAnsiQuotes) << tail).toEqual("fourth_test `TAIL`");

    // test cases for composed use of operator <<
    $expect(base::sqlstring("? fifth_test ?", base::QuoteOnlyIfNeeded) << head << tail).toEqual("'HEAD' fifth_test 'TAIL'");
    $expect(base::sqlstring("? sixth_test ?", base::UseAnsiQuotes) << head << tail).toEqual("\"HEAD\" sixth_test \"TAIL\"");
    $expect(base::sqlstring("! seventh_test !", base::QuoteOnlyIfNeeded) << head << tail).toEqual("HEAD seventh_test TAIL");
    $expect(base::sqlstring("! eighth_test !", base::UseAnsiQuotes) << head << tail).toEqual("`HEAD` eighth_test `TAIL`");

    // test cases for NULL parameter
    $expect(base::sqlstring("ninth_test ?", base::QuoteOnlyIfNeeded) << null_str).toEqual("ninth_test NULL");
    $expect(base::sqlstring("tenth_test ?", base::UseAnsiQuotes) << null_str).toEqual("tenth_test NULL");

    $expect([&]() { base::sqlstring("eleventh_test !", base::QuoteOnlyIfNeeded) << null_str; })
      .toThrowError<std::invalid_argument>("Error formatting SQL query: NULL value found for identifier");
    $expect([&]() { base::sqlstring("twelfth_test !", base::UseAnsiQuotes) << null_str; })
      .toThrowError<std::invalid_argument>("Error formatting SQL query: NULL value found for identifier");
  });

  $it("overridden operator << for (const char*) values [Corner/Limit Values]", [this]() {
    // test cases for simple use of operator <<
    std::string test_result;
    std::string temp_long_random_string;
    std::string quoted;
    test_result.append(base::sqlstring("?", base::QuoteOnlyIfNeeded) << data->long_random_string.c_str());

    temp_long_random_string.append(base::escape_sql_string(data->long_random_string.c_str()));
    temp_long_random_string.insert(0, "'");
    temp_long_random_string.insert(temp_long_random_string.size(), "'");

    $expect(test_result).toEqual(temp_long_random_string);

    test_result.clear();
    temp_long_random_string.clear();
    test_result.append(base::sqlstring("?", base::UseAnsiQuotes) << data->long_random_string.c_str());

    temp_long_random_string.append(base::escape_sql_string(data->long_random_string.c_str()));
    temp_long_random_string.insert(0, "\"");
    temp_long_random_string.insert(temp_long_random_string.size(), "\"");

    $expect(test_result).toEqual(temp_long_random_string);

    test_result.clear();
    temp_long_random_string.clear();
    test_result.append(base::sqlstring("!", base::QuoteOnlyIfNeeded) << data->long_random_string.c_str());

    quoted = base::escape_backticks(data->long_random_string.c_str());
    if (quoted == data->long_random_string.c_str())
      temp_long_random_string.append(quoted);
    else {
      temp_long_random_string.append(quoted);
      temp_long_random_string.insert(0, "`");
      temp_long_random_string.insert(temp_long_random_string.size(), "`");
    }

    $expect(test_result).toEqual(temp_long_random_string);

    test_result.clear();
    temp_long_random_string.clear();
    quoted.clear();
    test_result.append(base::sqlstring("!", base::UseAnsiQuotes) << data->long_random_string.c_str());

    quoted = base::escape_backticks(data->long_random_string.c_str());
    temp_long_random_string.append(quoted);
    temp_long_random_string.insert(0, "`");
    temp_long_random_string.insert(temp_long_random_string.size(), "`");

    $expect(test_result).toEqual(temp_long_random_string);
  });

  $it("overridden operator << for (const sqlstring&) values", []() {
    // test cases for simple use of operator <<
    std::string test_result = "";
    test_result.append(base::sqlstring("first_test ?", base::QuoteOnlyIfNeeded) << base::sqlstring("TAIL", 0));

    $expect(test_result).toBe("first_test TAIL");

    test_result.clear();
    test_result.append(base::sqlstring("second_test ?", base::UseAnsiQuotes) << base::sqlstring("TAIL", 0));

    $expect(test_result).toBe("second_test TAIL");

    test_result.clear();
    test_result.append(base::sqlstring("third_test !", base::QuoteOnlyIfNeeded) << base::sqlstring("TAIL", 0));

    $expect(test_result).toBe("third_test TAIL");

    test_result.clear();
    test_result.append(base::sqlstring("fourth_test !", base::UseAnsiQuotes) << base::sqlstring("TAIL", 0));

    $expect(test_result).toBe("fourth_test TAIL");

    // test cases for composed use of operator <<
    test_result.clear();
    test_result.append(base::sqlstring("? fifth_test ?", base::QuoteOnlyIfNeeded) << base::sqlstring("HEAD", 0)
                                                                      << base::sqlstring("TAIL", 0));

    $expect(test_result).toBe("HEAD fifth_test TAIL");

    test_result.clear();
    test_result.append(base::sqlstring("? sixth_test ?", base::UseAnsiQuotes) << base::sqlstring("HEAD", 0)
                                                                              << base::sqlstring("TAIL", 0));

    $expect(test_result).toBe("HEAD sixth_test TAIL");

    test_result.clear();
    test_result.append(base::sqlstring("! seventh_test !", base::QuoteOnlyIfNeeded) << base::sqlstring("HEAD", 0)
                                                                                    << base::sqlstring("TAIL", 0));

    $expect(test_result).toBe("HEAD seventh_test TAIL");

    test_result.clear();
    test_result.append(base::sqlstring("! eighth_test !", base::UseAnsiQuotes) << base::sqlstring("HEAD", 0)
                                                                               << base::sqlstring("TAIL", 0));

    $expect(test_result).toBe("HEAD eighth_test TAIL");
  });

  $it("overridden operator << for (const sqlstring&) values [Corner/Limit Values]", [this]() {
    // test cases for simple use of operator <<
    base::sqlstring long_random_sqlstring(base::sqlstring(data->long_random_string.c_str(), 0));

    $expect(base::sqlstring("?", base::QuoteOnlyIfNeeded) << long_random_sqlstring).toEqual((std::string)long_random_sqlstring);
    $expect(base::sqlstring("?", base::UseAnsiQuotes) << long_random_sqlstring).toEqual((std::string)long_random_sqlstring);
    $expect(base::sqlstring("!", base::QuoteOnlyIfNeeded) << long_random_sqlstring).toEqual((std::string)long_random_sqlstring);
    $expect(base::sqlstring("!", base::UseAnsiQuotes) << long_random_sqlstring).toEqual((std::string)long_random_sqlstring);
  });

  $it("overridden operator << for numeric values [Missing/Extra Parameters]", []() {
    std::int64_t int64_t_value1 = 1999;
    std::int64_t int64_t_value2 = -1999;
    int int_value1 = -2001;
    int int_value2 = 2001;
    float float_value1 = (float)3.141592;
    float float_value2 = (float)-3.141592;
    double double_value1 = 2.718281;
    double double_value2 = -2.718281;

    // test cases for missing parameters
    $expect(base::sqlstring("? first_test ?", 0) << int64_t_value1).toEqual("1999 first_test ?");
    $expect(base::sqlstring("? second_test ?", 0) << int_value1).toEqual("-2001 second_test ?");
    $expect(base::sqlstring("? third_test ?", 0) << float_value1).toEqual("3.141592 third_test ?");
    $expect(base::sqlstring("? fourth_test ?", 0) << double_value1).toEqual("2.718281 fourth_test ?");

    // test cases for extra parameters
    $expect([&]() { base::sqlstring("fifth_test ?", 0) << int64_t_value1 << int64_t_value2; }).toThrow();
    $expect([&]() { base::sqlstring("sixth_test ?", 0) << int_value1 << int_value2; }).toThrow();
    $expect([&]() { base::sqlstring("seventh_test ?", 0) << float_value1 << float_value2; }).toThrow();
    $expect([&]() { base::sqlstring("eighth_test ?", 0) << double_value1 << double_value2; }).toThrow();
  });

  $it("overridden operator << for std::string values [Missing/Extra Parameters]", []() {
    std::string tail = "TAIL";
    std::string head = "HEAD";

    // test cases for missing parameters
    $expect(base::sqlstring("? first_test ?", base::QuoteOnlyIfNeeded) << head).toEqual("'HEAD' first_test ?");
    $expect(base::sqlstring("? second_test ?", base::UseAnsiQuotes) << head).toEqual("\"HEAD\" second_test ?");
    $expect(base::sqlstring("! third_test !", base::QuoteOnlyIfNeeded) << head).toEqual("HEAD third_test !");
    $expect(base::sqlstring("! fourth_test !", base::UseAnsiQuotes) << head).toEqual("`HEAD` fourth_test !");

    // test cases for extra parameters
    $expect([&]() { base::sqlstring("? fifth_test", base::QuoteOnlyIfNeeded) << head << tail; }).toThrow();
    $expect([&]() { base::sqlstring("? sixth_test", base::UseAnsiQuotes) << head << tail; }).toThrow();
    $expect([&]() { base::sqlstring("! seventh_test", base::QuoteOnlyIfNeeded) << head << tail; }).toThrow();
    $expect([&]() { base::sqlstring("! eighth_test", base::UseAnsiQuotes) << head << tail; }).toThrow();
  });

  $it("overridden operator << for (const char*) values [Missing/Extra Parameters]", []() {
    const char* tail = "TAIL";
    const char* head = "HEAD";

    // test cases for missing parameters
    $expect(base::sqlstring("? first_test ?", base::QuoteOnlyIfNeeded) << head).toEqual("'HEAD' first_test ?");
    $expect(base::sqlstring("? second_test ?", base::UseAnsiQuotes) << head).toEqual("\"HEAD\" second_test ?");
    $expect(base::sqlstring("! third_test !", base::QuoteOnlyIfNeeded) << head).toEqual("HEAD third_test !");
    $expect(base::sqlstring("! fourth_test !", base::UseAnsiQuotes) << head).toEqual("`HEAD` fourth_test !");

    // test cases for extra parameters
    $expect([&]() { base::sqlstring("? fifth_test", base::QuoteOnlyIfNeeded) << head << tail; }).toThrow();
    $expect([&]() { base::sqlstring("? sixth_test", base::UseAnsiQuotes) << head << tail; }).toThrow();
    $expect([&]() { base::sqlstring("! seventh_test", base::QuoteOnlyIfNeeded) << head << tail; }).toThrow();
    $expect([&]() { base::sqlstring("! eighth_test", base::UseAnsiQuotes) << head << tail; }).toThrow();
  });

  $it("overridden operator << for (const sqlstring&) values [Missing/Extra Parameters]", []() {
    // test cases for missing parameters
    $expect(base::sqlstring("? first_test ?", base::QuoteOnlyIfNeeded) << base::sqlstring("HEAD", 0)).toEqual("HEAD first_test ?");
    $expect(base::sqlstring("? second_test ?", base::UseAnsiQuotes) << base::sqlstring("HEAD", 0)).toEqual("HEAD second_test ?");
    $expect(base::sqlstring("! third_test !", base::QuoteOnlyIfNeeded) << base::sqlstring("HEAD", 0)).toEqual("HEAD third_test !");
    $expect(base::sqlstring("! fourth_test !", base::UseAnsiQuotes) << base::sqlstring("HEAD", 0)).toEqual("HEAD fourth_test !");

    // test cases for extra parameters
    $expect([&]() { base::sqlstring("? fifth_test", base::QuoteOnlyIfNeeded)
                      << base::sqlstring("HEAD", 0)
                      << base::sqlstring("TAIL", 0); }).toThrow();
    $expect([&]() { base::sqlstring("? sixth_test", base::UseAnsiQuotes)
                      << base::sqlstring("HEAD", 0)
                      << base::sqlstring("TAIL", 0); }).toThrow();
    $expect([&]() { base::sqlstring("! seventh_test", base::QuoteOnlyIfNeeded)
                      << base::sqlstring("HEAD", 0)
                      << base::sqlstring("TAIL", 0); }).toThrow();
    $expect([&]() { base::sqlstring("! eighth_test", base::UseAnsiQuotes)
                      << base::sqlstring("HEAD", 0)
                      << base::sqlstring("TAIL", 0); }).toThrow();
  });

  $it("c-tors", []() {
    // constructors
    base::sqlstring c_tor_flag_QuoteOnlyIfNeeded_num("test ? ? ? ?", base::QuoteOnlyIfNeeded);
    base::sqlstring c_tor_flag_UseAnsiQuotes_num("test ? ? ? ?", base::UseAnsiQuotes);
    base::sqlstring c_tor_flag_QuoteOnlyIfNeeded_str("test ? ! ? !", base::QuoteOnlyIfNeeded);
    base::sqlstring c_tor_flag_UseAnsiQuotes_str("test ? ! ? !", base::UseAnsiQuotes);
    base::sqlstring c_tor_flag_QuoteOnlyIfNeeded_sqlstr("test ? !", base::QuoteOnlyIfNeeded);
    base::sqlstring c_tor_flag_UseAnsiQuotes_sqlstr("test ? !", base::UseAnsiQuotes);
    base::sqlstring c_tor_copy_flag_QuoteOnlyIfNeeded_str(c_tor_flag_QuoteOnlyIfNeeded_str);
    base::sqlstring c_tor_copy_flag_UseAnsiQuotes_str(c_tor_flag_UseAnsiQuotes_str);
    base::sqlstring c_tor_empty_constructor;

    // miscellaneous variables
    std::int64_t int64_t_value = 1999;
    int int_value = -2001;
    float float_value = (float)3.141592;
    double double_value = 2.718281;
    std::string tail = "TAIL";
    const char* const_tail = "TAIL";

    // test cases for constructors & numeric values
    $expect(c_tor_flag_QuoteOnlyIfNeeded_num << int64_t_value).toEqual("test 1999 ? ? ?");
    $expect(c_tor_flag_QuoteOnlyIfNeeded_num << int_value).toEqual("test 1999 -2001 ? ?");
    $expect(c_tor_flag_QuoteOnlyIfNeeded_num << float_value).toEqual("test 1999 -2001 3.141592 ?");
    $expect(c_tor_flag_QuoteOnlyIfNeeded_num << double_value).toEqual("test 1999 -2001 3.141592 2.718281");
    $expect(c_tor_flag_UseAnsiQuotes_num << int64_t_value).toEqual("test 1999 ? ? ?");
    $expect(c_tor_flag_UseAnsiQuotes_num << int_value).toEqual("test 1999 -2001 ? ?");
    $expect(c_tor_flag_UseAnsiQuotes_num << float_value).toEqual("test 1999 -2001 3.141592 ?");
    $expect(c_tor_flag_UseAnsiQuotes_num << double_value).toEqual("test 1999 -2001 3.141592 2.718281");

    // test cases for copy constructors & std::string values
    $expect(c_tor_copy_flag_QuoteOnlyIfNeeded_str << tail).toEqual("test 'TAIL' ! ? !");
    $expect(c_tor_copy_flag_UseAnsiQuotes_str << tail).toEqual("test \"TAIL\" ! ? !");
    $expect(c_tor_copy_flag_QuoteOnlyIfNeeded_str << tail).toEqual("test 'TAIL' TAIL ? !");
    $expect(c_tor_copy_flag_UseAnsiQuotes_str << tail).toEqual("test \"TAIL\" `TAIL` ? !");

    // test cases for copy constructors & (const char*) values
    $expect(c_tor_copy_flag_QuoteOnlyIfNeeded_str << const_tail).toEqual("test 'TAIL' TAIL 'TAIL' !");
    $expect(c_tor_copy_flag_UseAnsiQuotes_str << const_tail).toEqual("test \"TAIL\" `TAIL` \"TAIL\" !");
    $expect(c_tor_copy_flag_QuoteOnlyIfNeeded_str << const_tail).toEqual("test 'TAIL' TAIL 'TAIL' TAIL");
    $expect(c_tor_copy_flag_UseAnsiQuotes_str << const_tail).toEqual("test \"TAIL\" `TAIL` \"TAIL\" `TAIL`");

    // test cases for constructors & (const sqlstring&) values
    $expect(c_tor_flag_QuoteOnlyIfNeeded_sqlstr << base::sqlstring("TAIL", 0)).toEqual("test TAIL !");
    $expect(c_tor_flag_UseAnsiQuotes_sqlstr << base::sqlstring("TAIL", 0)).toEqual("test TAIL !");
    $expect(c_tor_flag_QuoteOnlyIfNeeded_sqlstr << base::sqlstring("TAIL", 0)).toEqual("test TAIL TAIL");
    $expect(c_tor_flag_UseAnsiQuotes_sqlstr << base::sqlstring("TAIL", 0)).toEqual("test TAIL TAIL");

    // test case for empty constructor
    $expect(c_tor_empty_constructor).toEqual("");
  });

  $it("done()", []() {
    // constructor
    base::sqlstring sqlstr("test ? ! ? !", 0);

    std::string test_result = "";
    while (!sqlstr.done()) {
      test_result.append(sqlstr << 1 << "2" << 3.0 << base::sqlstring("4", 0));
    }

    $expect(test_result).toEqual("test 1 `2` 3.000000 4");
  });

  $it("overridden operator std::string", []() {
    // constructor
    base::sqlstring sqlstr("test ? ! ? ! ?", 0);

    $expect(sqlstr << 1 << "2" << 3.0 << base::sqlstring("4", 0)).toEqual(std::string(sqlstr));
  });

  $it("memory management and/or memory leaks", [this]() {
    // constructor with (MAX_SIZE_RANDOM_VALUES_ARRAY x 4) placeholders
    base::sqlstring* sqlstr[MAX_SIZE_SQLSTRING_ARRAY];
    int i, j;

    // Create MAX_SIZE_SQLSTRING_ARRAY sqlstring's
    std::vector<std::string> expected;
    std::vector<std::string> actual;
    for (i = 0; i < MAX_SIZE_SQLSTRING_ARRAY; i++) {
      std::string test_result;
      std::string expected_result;
      // Each sqlstring has (MAX_SIZE_RANDOM_VALUES_ARRAY x 4) placeholders
      sqlstr[i] = new base::sqlstring(data->placeholders.c_str(), 0);

      for (j = 0; j < MAX_SIZE_RANDOM_VALUES_ARRAY; j++) {
        *sqlstr[i] << random_values[j].int_value
                   << random_values[j].string_value
                   << random_values[j].double_value
                   << random_values[j].sqlstring_value;

        expected_result.append(std::to_string(random_values[j].int_value))
                       .append(" `")
                       .append(random_values[j].string_value)
                       .append("` ")
                       .append(std::to_string(random_values[j].double_value))
                       .append(" ")
                       .append(random_values[j].sqlstring_value)
                       .append(" ");
      }

      expected.push_back(expected_result);
      actual.push_back(*sqlstr[i]);
    }
    $expect(expected).toEqual(actual);

    $expect([&]() {
      for (i = 0; i < MAX_SIZE_SQLSTRING_ARRAY; i++)
        delete sqlstr[i];
    }).Not.toThrow();

  });
}

}
