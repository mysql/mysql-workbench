/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
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
#include "base/string_utilities.h"
#include "wb_helpers.h"

#ifdef __APPLE__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcomma"
#endif

#include <boost/lexical_cast.hpp>

#ifdef __APPLE__
#pragma GCC diagnostic pop
#endif

using namespace base;

#define MAX_SIZE_RANDOM_VALUES_ARRAY 25
#define MAX_SIZE_SQLSTRING_ARRAY 10000

BEGIN_TEST_DATA_CLASS(sqlstring_test)

protected:
std::string long_random_string; // Content doesn't matter. There must be no crash using it.

struct rdm_vals {
  int int_value;
  std::string string_value;
  double double_value;
  sqlstring sqlstring_value;
} random_values[MAX_SIZE_RANDOM_VALUES_ARRAY];

std::string placeholders;

TEST_DATA_CONSTRUCTOR(sqlstring_test) {
  int i;
  double tmp1, tmp2;

  srand((unsigned int)time(NULL));
  for (i = 0; i < 1000; i++) {
    long_random_string += ' ' + rand() * 64 / 32768;
    if (rand() > 16767)
      long_random_string += "\xE3\x8A\xA8"; // Valid Unicode character.
    if (i == 500)
      long_random_string += "\xE3\x8A\xA8"; // Ensure it is there at least once.
  }

  placeholders.clear();

  for (i = 0; i < MAX_SIZE_RANDOM_VALUES_ARRAY; i++) {
    placeholders += "? ! ? ! ";

    random_values[i].int_value = rand();
    tmp1 = rand() % 100;     // An int value between 0 and 99
    tmp2 = rand() % 100 + 1; // An int value between 1 and 100
    random_values[i].double_value = tmp1 / tmp2;

    random_values[i].sqlstring_value =
      base::sqlstring(boost::lexical_cast<std::string>(rand()).c_str(), base::sqlstring::sqlstringformat(0));
    random_values[i].string_value = boost::lexical_cast<std::string>(rand());
  }
}

END_TEST_DATA_CLASS;

TEST_MODULE(sqlstring_test, "sqlstring");

/*
 * Testing overridden operator << for numeric values
 */
TEST_FUNCTION(5) {
  std::int64_t int64_t_value1 = 1999;
  std::int64_t int64_t_value2 = -1999;
  int int_value1 = -2001;
  int int_value2 = 2001;
  float float_value1 = (float)3.141592;
  float float_value2 = (float)-3.141592;
  double double_value1 = 2.718281;
  double double_value2 = -2.718281;

  // test cases for simple use of operator <<
  std::string test_result = "";
  test_result.append(sqlstring("first_test ?", 0) << int64_t_value1);

  ensure_equals("TEST 5.1: Unexpected result replacing ? with a long long/int64 value", test_result, "first_test 1999");

  test_result.clear();
  test_result.append(sqlstring("second_test ?", 0) << int_value1);

  ensure_equals("TEST 5.2: Unexpected result replacing ? with an int value", test_result, "second_test -2001");

  test_result.clear();
  test_result.append(sqlstring("third_test ?", 0) << float_value1);

  ensure_equals("TEST 5.3: Unexpected result replacing ? with a float value", test_result, "third_test 3.141592");

  test_result.clear();
  test_result.append(sqlstring("fourth_test ?", 0) << double_value1);

  ensure_equals("TEST 5.4: Unexpected result replacing ? with a double value", test_result, "fourth_test 2.718281");

  // test cases for composed use of operator <<
  test_result.clear();
  test_result.append(sqlstring("? fifth_test ?", 0) << int64_t_value1 << int64_t_value2);

  ensure_equals("TEST 5.5: Unexpected result replacing ? with a long long/int64 numeric value", test_result,
                "1999 fifth_test -1999");

  test_result.clear();
  test_result.append(sqlstring("? sixth_test ?", 0) << int_value1 << int_value2);

  ensure_equals("TEST 5.6: Unexpected result replacing ? with an int value", test_result, "-2001 sixth_test 2001");

  test_result.clear();
  test_result.append(sqlstring("? seventh_test ?", 0) << float_value1 << float_value2);

  ensure_equals("TEST 5.7: Unexpected result replacing ? with a float value", test_result,
                "3.141592 seventh_test -3.141592");

  test_result.clear();
  test_result.append(sqlstring("? eighth_test ?", 0) << double_value1 << double_value2);

  ensure_equals("TEST 5.8: Unexpected result replacing ? with a double value", test_result,
                "2.718281 eighth_test -2.718281");
}

/*
 * Testing overridden operator << for numeric values
 *  -- Corner/Limit Values --
 */
TEST_FUNCTION(10) {
  double NaN = 2.51;
  bool flag = false;

  // test cases for simple use of operator <<
  std::string test_result = "";
  test_result.append(sqlstring("?", 0) << INT_MIN);

  ensure_equals("TEST 10.1: Unexpected result replacing ? with INT_MIN value", test_result,
                boost::lexical_cast<std::string>(INT_MIN));

  test_result.clear();
  test_result.append(sqlstring("?", 0) << INT_MAX);

  ensure_equals("TEST 10.2: Unexpected result replacing ? with INT_MAX value", test_result,
                boost::lexical_cast<std::string>(INT_MAX));

  test_result.clear();
  test_result.append(sqlstring("?", 0) << LONG_MIN);

  ensure_equals("TEST 10.3: Unexpected result replacing ? with LONG_MIN value", test_result,
                boost::lexical_cast<std::string>(LONG_MIN));

  test_result.clear();
  test_result.append(sqlstring("?", 0) << LONG_MAX);

  ensure_equals("TEST 10.4: Unexpected result replacing ? with LONG_MAX value", test_result,
                boost::lexical_cast<std::string>(LONG_MAX));

  test_result.clear();
  test_result.append(sqlstring("?", 0) << NaN);
  flag = (0.0 != atof(test_result.c_str()));

  ensure("TEST 10.5: Unexpected result replacing ? with a NaN value", flag);

  test_result.clear();
  test_result.append(sqlstring("?", 0) << SHRT_MIN);

  ensure_equals("TEST 10.6: Unexpected result replacing ? with SHRT_MIN value", test_result,
                boost::lexical_cast<std::string>(SHRT_MIN));

  test_result.clear();
  test_result.append(sqlstring("?", 0) << SHRT_MAX);

  ensure_equals("TEST 10.7: Unexpected result replacing ? with SHRT_MAX value", test_result,
                boost::lexical_cast<std::string>(SHRT_MAX));

  test_result.clear();
  test_result.append(sqlstring("?", 0) << LLONG_MIN);

  ensure_equals("TEST 10.8: Unexpected result replacing ? with LLONG_MIN value", test_result,
                boost::lexical_cast<std::string>(LLONG_MIN));

  test_result.clear();
  test_result.append(sqlstring("?", 0) << LLONG_MAX);

  ensure_equals("TEST 10.9: Unexpected result replacing ? with LLONG_MAX value", test_result,
                boost::lexical_cast<std::string>(LLONG_MAX));

  test_result.clear();
  test_result.append(sqlstring("?", 0) << CHAR_MIN);

  ensure_equals("TEST 10.10: Unexpected result replacing ? with CHAR_MIN value", test_result,
                boost::lexical_cast<std::string>(CHAR_MIN));

  test_result.clear();
  test_result.append(sqlstring("?", 0) << CHAR_MAX);

  ensure_equals("TEST 10.11: Unexpected result replacing ? with CHAR_MAX value", test_result,
                boost::lexical_cast<std::string>(CHAR_MAX));
}

/*
 * Testing overridden operator << for std::string values
 */
TEST_FUNCTION(15) {
  std::string tail = "TAIL";
  std::string head = "HEAD";

  // test cases for simple use of operator <<
  std::string test_result = "";
  test_result.append(sqlstring("first_test ?", QuoteOnlyIfNeeded) << tail);

  ensure_equals("TEST 15.1: Unexpected result quoting string", test_result, "first_test 'TAIL'");

  test_result.clear();
  test_result.append(sqlstring("second_test ?", UseAnsiQuotes) << tail);

  ensure_equals("TEST 15.2: Unexpected result quoting string", test_result, "second_test \"TAIL\"");

  test_result.clear();
  test_result.append(sqlstring("third_test !", QuoteOnlyIfNeeded) << tail);

  ensure_equals("TEST 15.3: Unexpected result quoting string", test_result, "third_test TAIL");

  test_result.clear();
  test_result.append(sqlstring("fourth_test !", UseAnsiQuotes) << tail);

  ensure_equals("TEST 15.4: Unexpected result quoting string", test_result, "fourth_test `TAIL`");

  // test cases for composed use of operator <<
  test_result.clear();
  test_result.append(sqlstring("? fifth_test ?", QuoteOnlyIfNeeded) << head << tail);

  ensure_equals("TEST 15.5: Unexpected result quoting string", test_result, "'HEAD' fifth_test 'TAIL'");

  test_result.clear();
  test_result.append(sqlstring("? sixth_test ?", UseAnsiQuotes) << head << tail);

  ensure_equals("TEST 15.6: Unexpected result quoting string", test_result, "\"HEAD\" sixth_test \"TAIL\"");

  test_result.clear();
  test_result.append(sqlstring("! seventh_test !", QuoteOnlyIfNeeded) << head << tail);

  ensure_equals("TEST 15.7: Unexpected result quoting string", test_result, "HEAD seventh_test TAIL");

  test_result.clear();
  test_result.append(sqlstring("! eighth_test !", UseAnsiQuotes) << head << tail);

  ensure_equals("TEST 15.8: Unexpected result quoting string", test_result, "`HEAD` eighth_test `TAIL`");
}

/*
 * Testing overridden operator << for std::string values
 *  -- Corner/Limit Values --
 */
TEST_FUNCTION(20) {
  // test cases for simple use of operator <<
  std::string test_result = "";
  std::string temp_long_random_string = "";
  std::string escaped = "";
  test_result.append(sqlstring("?", QuoteOnlyIfNeeded) << long_random_string);

  temp_long_random_string.append(escape_sql_string(long_random_string));
  temp_long_random_string.insert(0, "'");
  temp_long_random_string.insert(temp_long_random_string.size(), "'");

  ensure_equals("TEST 20.1: Unexpected result quoting long random string", test_result, temp_long_random_string);

  test_result.clear();
  temp_long_random_string.clear();
  test_result.append(sqlstring("?", UseAnsiQuotes) << long_random_string);

  temp_long_random_string.append(escape_sql_string(long_random_string));
  temp_long_random_string.insert(0, "\"");
  temp_long_random_string.insert(temp_long_random_string.size(), "\"");

  ensure_equals("TEST 20.2: Unexpected result quoting long random string", test_result, temp_long_random_string);

  test_result.clear();
  temp_long_random_string.clear();
  test_result.append(sqlstring("!", QuoteOnlyIfNeeded) << long_random_string);

  escaped = escape_backticks(long_random_string);
  temp_long_random_string.append(base::quote_identifier_if_needed(escaped, '`'));
  ;

  ensure_equals("TEST 20.3: Unexpected result quoting long random string", test_result, temp_long_random_string);

  test_result.clear();
  temp_long_random_string.clear();
  escaped.clear();
  test_result.append(sqlstring("!", UseAnsiQuotes) << long_random_string);

  escaped = escape_backticks(long_random_string);
  temp_long_random_string.append(base::quote_identifier(escaped, '`'));

  ensure_equals("TEST 20.4: Unexpected result quoting long random string", test_result, temp_long_random_string);
}

/*
 * Testing overridden operator << for (const char*) values
 */
TEST_FUNCTION(25) {
  const char* tail = "TAIL";
  const char* head = "HEAD";
  const char* null_str(0);

  // test cases for simple use of operator <<
  std::string test_result = "";
  test_result.append(sqlstring("first_test ?", QuoteOnlyIfNeeded) << tail);

  ensure_equals("TEST 25.1: Unexpected result quoting string", test_result, "first_test 'TAIL'");

  test_result.clear();
  test_result.append(sqlstring("second_test ?", UseAnsiQuotes) << tail);

  ensure_equals("TEST 25.2: Unexpected result quoting string", test_result, "second_test \"TAIL\"");

  test_result.clear();
  test_result.append(sqlstring("third_test !", QuoteOnlyIfNeeded) << tail);

  ensure_equals("TEST 25.3: Unexpected result quoting string", test_result, "third_test TAIL");

  test_result.clear();
  test_result.append(sqlstring("fourth_test !", UseAnsiQuotes) << tail);

  ensure_equals("TEST 25.4: Unexpected result quoting string", test_result, "fourth_test `TAIL`");

  // test cases for composed use of operator <<
  test_result.clear();
  test_result.append(sqlstring("? fifth_test ?", QuoteOnlyIfNeeded) << head << tail);

  ensure_equals("TEST 25.5: Unexpected result quoting string", test_result, "'HEAD' fifth_test 'TAIL'");

  test_result.clear();
  test_result.append(sqlstring("? sixth_test ?", UseAnsiQuotes) << head << tail);

  ensure_equals("TEST 25.6: Unexpected result quoting string", test_result, "\"HEAD\" sixth_test \"TAIL\"");

  test_result.clear();
  test_result.append(sqlstring("! seventh_test !", QuoteOnlyIfNeeded) << head << tail);

  ensure_equals("TEST 25.7: Unexpected result quoting string", test_result, "HEAD seventh_test TAIL");

  test_result.clear();
  test_result.append(sqlstring("! eighth_test !", UseAnsiQuotes) << head << tail);

  ensure_equals("TEST 25.8: Unexpected result quoting string", test_result, "`HEAD` eighth_test `TAIL`");

  // test cases for NULL parameter
  test_result.clear();
  test_result.append(sqlstring("ninth_test ?", QuoteOnlyIfNeeded) << null_str);

  ensure_equals("TEST 25.9: Unexpected result quoting string", test_result, "ninth_test NULL");

  test_result.clear();
  test_result.append(sqlstring("tenth_test ?", UseAnsiQuotes) << null_str);

  ensure_equals("TEST 25.10: Unexpected result quoting string", test_result, "tenth_test NULL");

  try {
    test_result.clear();
    test_result.append(sqlstring("eleventh_test !", QuoteOnlyIfNeeded) << null_str);
    fail("TEST 25.11: Unexpected result quoting null string");
  } catch (std::invalid_argument &e) {
    // Nothing to do, just catch the error and continue
  }

  try {
    test_result.clear();
    test_result.append(sqlstring("twelfth_test !", UseAnsiQuotes) << null_str);
    fail("TEST 25.12: Unexpected result quoting null string");
  } catch (std::invalid_argument &e) {
    // Nothing to do, just catch the error and continue
  }
}

/*
 * Testing overridden operator << for (const char*) values
 * -- Corner/Limit Values --
 */
TEST_FUNCTION(30) {
  // test cases for simple use of operator <<
  std::string test_result = "";
  std::string temp_long_random_string = "";
  std::string quoted = "";
  test_result.append(sqlstring("?", QuoteOnlyIfNeeded) << long_random_string.c_str());

  temp_long_random_string.append(escape_sql_string(long_random_string.c_str()));
  temp_long_random_string.insert(0, "'");
  temp_long_random_string.insert(temp_long_random_string.size(), "'");

  ensure_equals("TEST 30.1: Unexpected result quoting long random string", test_result, temp_long_random_string);

  test_result.clear();
  temp_long_random_string.clear();
  test_result.append(sqlstring("?", UseAnsiQuotes) << long_random_string.c_str());

  temp_long_random_string.append(escape_sql_string(long_random_string.c_str()));
  temp_long_random_string.insert(0, "\"");
  temp_long_random_string.insert(temp_long_random_string.size(), "\"");

  ensure_equals("TEST 30.2: Unexpected result quoting long random string", test_result, temp_long_random_string);

  test_result.clear();
  temp_long_random_string.clear();
  test_result.append(sqlstring("!", QuoteOnlyIfNeeded) << long_random_string.c_str());

  quoted = escape_backticks(long_random_string.c_str());
  if (quoted == long_random_string.c_str())
    temp_long_random_string.append(quoted);
  else {
    temp_long_random_string.append(quoted);
    temp_long_random_string.insert(0, "`");
    temp_long_random_string.insert(temp_long_random_string.size(), "`");
  }

  ensure_equals("TEST 30.3: Unexpected result quoting long random string", test_result, temp_long_random_string);

  test_result.clear();
  temp_long_random_string.clear();
  quoted.clear();
  test_result.append(sqlstring("!", UseAnsiQuotes) << long_random_string.c_str());

  quoted = escape_backticks(long_random_string.c_str());
  temp_long_random_string.append(quoted);
  temp_long_random_string.insert(0, "`");
  temp_long_random_string.insert(temp_long_random_string.size(), "`");

  ensure_equals("TEST 30.4: Unexpected result quoting long random string", test_result, temp_long_random_string);
}

/*
 * Testing overridden operator << for (const sqlstring&) values
 */
TEST_FUNCTION(40) {
  // test cases for simple use of operator <<
  std::string test_result = "";
  test_result.append(sqlstring("first_test ?", QuoteOnlyIfNeeded) << base::sqlstring("TAIL", 0));

  ensure_equals("TEST 40.1: Unexpected result quoting string", test_result, "first_test TAIL");

  test_result.clear();
  test_result.append(sqlstring("second_test ?", UseAnsiQuotes) << base::sqlstring("TAIL", 0));

  ensure_equals("TEST 40.2: Unexpected result quoting string", test_result, "second_test TAIL");

  test_result.clear();
  test_result.append(sqlstring("third_test !", QuoteOnlyIfNeeded) << base::sqlstring("TAIL", 0));

  ensure_equals("TEST 40.3: Unexpected result quoting string", test_result, "third_test TAIL");

  test_result.clear();
  test_result.append(sqlstring("fourth_test !", UseAnsiQuotes) << base::sqlstring("TAIL", 0));

  ensure_equals("TEST 40.4: Unexpected result quoting string", test_result, "fourth_test TAIL");

  // test cases for composed use of operator <<
  test_result.clear();
  test_result.append(sqlstring("? fifth_test ?", QuoteOnlyIfNeeded) << base::sqlstring("HEAD", 0)
                                                                    << base::sqlstring("TAIL", 0));

  ensure_equals("TEST 40.5: Unexpected result quoting string", test_result, "HEAD fifth_test TAIL");

  test_result.clear();
  test_result.append(sqlstring("? sixth_test ?", UseAnsiQuotes) << base::sqlstring("HEAD", 0)
                                                                << base::sqlstring("TAIL", 0));

  ensure_equals("TEST 40.6: Unexpected result quoting string", test_result, "HEAD sixth_test TAIL");

  test_result.clear();
  test_result.append(sqlstring("! seventh_test !", QuoteOnlyIfNeeded) << base::sqlstring("HEAD", 0)
                                                                      << base::sqlstring("TAIL", 0));

  ensure_equals("TEST 40.7: Unexpected result quoting string", test_result, "HEAD seventh_test TAIL");

  test_result.clear();
  test_result.append(sqlstring("! eighth_test !", UseAnsiQuotes) << base::sqlstring("HEAD", 0)
                                                                 << base::sqlstring("TAIL", 0));

  ensure_equals("TEST 40.8: Unexpected result quoting string", test_result, "HEAD eighth_test TAIL");
}

/*
 * Testing overridden operator << for (const sqlstring&) values
 * -- Corner/Limit Values --
 */
TEST_FUNCTION(45) {
  // test cases for simple use of operator <<
  sqlstring long_random_sqlstring(base::sqlstring(long_random_string.c_str(), 0));
  std::string test_result = "";
  test_result.append(sqlstring("?", QuoteOnlyIfNeeded) << long_random_sqlstring);

  ensure_equals("TEST 45.1: Unexpected result quoting long random string", test_result,
                (std::string)long_random_sqlstring);

  test_result.clear();
  test_result.append(sqlstring("?", UseAnsiQuotes) << long_random_sqlstring);

  ensure_equals("TEST 45.2: Unexpected result quoting long random string", test_result,
                (std::string)long_random_sqlstring);

  test_result.clear();
  test_result.append(sqlstring("!", QuoteOnlyIfNeeded) << long_random_sqlstring);

  ensure_equals("TEST 45.3: Unexpected result quoting long random string", test_result,
                (std::string)long_random_sqlstring);

  test_result.clear();
  test_result.append(sqlstring("!", UseAnsiQuotes) << long_random_sqlstring);

  ensure_equals("TEST 45.4: Unexpected result quoting long random string", test_result,
                (std::string)long_random_sqlstring);
}

/*
 * Testing overridden operator << for numeric values
 * -- Missing/Extra Parameters --
 */
TEST_FUNCTION(50) {
  std::int64_t int64_t_value1 = 1999;
  std::int64_t int64_t_value2 = -1999;
  int int_value1 = -2001;
  int int_value2 = 2001;
  float float_value1 = (float)3.141592;
  float float_value2 = (float)-3.141592;
  double double_value1 = 2.718281;
  double double_value2 = -2.718281;

  // test cases for missing parameters
  std::string test_result = "";
  test_result.append(sqlstring("? first_test ?", 0) << int64_t_value1);

  ensure_equals("TEST 50.1: Unexpected result replacing ? with a long long/int64 numeric value", test_result,
                "1999 first_test ?");

  test_result.clear();
  test_result.append(sqlstring("? second_test ?", 0) << int_value1);

  ensure_equals("TEST 50.2: Unexpected result replacing ? with an int value", test_result, "-2001 second_test ?");

  test_result.clear();
  test_result.append(sqlstring("? third_test ?", 0) << float_value1);

  ensure_equals("TEST 50.3: Unexpected result replacing ? with a float value", test_result, "3.141592 third_test ?");

  test_result.clear();
  test_result.append(sqlstring("? fourth_test ?", 0) << double_value1);

  ensure_equals("TEST 50.4: Unexpected result replacing ? with a double value", test_result, "2.718281 fourth_test ?");

  // test cases for extra parameters
  try {
    test_result.clear();
    test_result.append(sqlstring("fifth_test ?", 0) << int64_t_value1 << int64_t_value2);
    fail("TEST 50.5: Unexpected result replacing ? with a long long/int64 numeric value");
  } catch (...) {
    // Nothing to do, just catch the error and continue
  }

  try {
    test_result.clear();
    test_result.append(sqlstring("sixth_test ?", 0) << int_value1 << int_value2);
    fail("TEST 50.6: Unexpected result replacing ? with an int value");
  } catch (...) {
    // Nothing to do, just catch the error and continue
  }

  try {
    test_result.clear();
    test_result.append(sqlstring("seventh_test ?", 0) << float_value1 << float_value2);
    fail("TEST 50.7: Unexpected result replacing ? with a float value");
  } catch (...) {
    // Nothing to do, just catch the error and continue
  }

  try {
    test_result.clear();
    test_result.append(sqlstring("eighth_test ?", 0) << double_value1 << double_value2);
    fail("TEST 50.8: Unexpected result replacing ? with a double value");
  } catch (...) {
    // Nothing to do, just catch the error and continue
  }
}

/*
 * Testing overridden operator << for std::string values
 * -- Missing/Extra Parameters --
 */
TEST_FUNCTION(55) {
  std::string tail = "TAIL";
  std::string head = "HEAD";

  // test cases for missing parameters
  std::string test_result = "";
  test_result.append(sqlstring("? first_test ?", QuoteOnlyIfNeeded) << head);

  ensure_equals("TEST 55.1: Unexpected result quoting string", test_result, "'HEAD' first_test ?");

  test_result.clear();
  test_result.append(sqlstring("? second_test ?", UseAnsiQuotes) << head);

  ensure_equals("TEST 55.2: Unexpected result quoting string", test_result, "\"HEAD\" second_test ?");

  test_result.clear();
  test_result.append(sqlstring("! third_test !", QuoteOnlyIfNeeded) << head);

  ensure_equals("TEST 55.3: Unexpected result quoting string", test_result, "HEAD third_test !");

  test_result.clear();
  test_result.append(sqlstring("! fourth_test !", UseAnsiQuotes) << head);

  ensure_equals("TEST 55.4: Unexpected result quoting string", test_result, "`HEAD` fourth_test !");

  // test cases for extra parameters
  try {
    test_result.clear();
    test_result.append(sqlstring("? fifth_test", QuoteOnlyIfNeeded) << head << tail);
    fail("TEST 55.5: Unexpected result quoting string");
  } catch (...) {
    // Nothing to do, just catch the error and continue
  }

  try {
    test_result.clear();
    test_result.append(sqlstring("? sixth_test", UseAnsiQuotes) << head << tail);
    fail("TEST 55.6: Unexpected result quoting string");
  } catch (...) {
    // Nothing to do, just catch the error and continue
  }

  try {
    test_result.clear();
    test_result.append(sqlstring("! seventh_test", QuoteOnlyIfNeeded) << head << tail);
    fail("TEST 55.7: Unexpected result quoting string");
  } catch (...) {
    // Nothing to do, just catch the error and continue
  }

  try {
    test_result.clear();
    test_result.append(sqlstring("! eighth_test", UseAnsiQuotes) << head << tail);
    fail("TEST 55.8: Unexpected result quoting string");
  } catch (...) {
    // Nothing to do, just catch the error and continue
  }
}

/*
 * Testing overridden operator << for (const char*) values
 * -- Missing/Extra Parameters --
 */
TEST_FUNCTION(60) {
  const char* tail = "TAIL";
  const char* head = "HEAD";

  // test cases for missing parameters
  std::string test_result = "";
  test_result.append(sqlstring("? first_test ?", QuoteOnlyIfNeeded) << head);

  ensure_equals("TEST 60.1: Unexpected result quoting string", test_result, "'HEAD' first_test ?");

  test_result.clear();
  test_result.append(sqlstring("? second_test ?", UseAnsiQuotes) << head);

  ensure_equals("TEST 60.2: Unexpected result quoting string", test_result, "\"HEAD\" second_test ?");

  test_result.clear();
  test_result.append(sqlstring("! third_test !", QuoteOnlyIfNeeded) << head);

  ensure_equals("TEST 60.3: Unexpected result quoting string", test_result, "HEAD third_test !");

  test_result.clear();
  test_result.append(sqlstring("! fourth_test !", UseAnsiQuotes) << head);

  ensure_equals("TEST 60.4: Unexpected result quoting string", test_result, "`HEAD` fourth_test !");

  // test cases for extra parameters
  try {
    test_result.clear();
    test_result.append(sqlstring("? fifth_test", QuoteOnlyIfNeeded) << head << tail);
    fail("TEST 60.5: Unexpected result quoting string");
  } catch (...) {
    // Nothing to do, just catch the error and continue
  }

  try {
    test_result.clear();
    test_result.append(sqlstring("? sixth_test", UseAnsiQuotes) << head << tail);
    fail("TEST 60.6: Unexpected result quoting string");
  } catch (...) {
    // Nothing to do, just catch the error and continue
  }

  try {
    test_result.clear();
    test_result.append(sqlstring("! seventh_test", QuoteOnlyIfNeeded) << head << tail);
    fail("TEST 60.7: Unexpected result quoting string");
  } catch (...) {
    // Nothing to do, just catch the error and continue
  }

  try {
    test_result.clear();
    test_result.append(sqlstring("! eighth_test", UseAnsiQuotes) << head << tail);
    fail("TEST 60.8: Unexpected result quoting string");
  } catch (...) {
    // Nothing to do, just catch the error and continue
  }
}

/*
 * Testing overridden operator << for (const sqlstring&) values
 * -- Missing/Extra Parameters --
 */
TEST_FUNCTION(65) {
  // test cases for missing parameters
  std::string test_result = "";
  test_result.append(sqlstring("? first_test ?", QuoteOnlyIfNeeded) << base::sqlstring("HEAD", 0));

  ensure_equals("TEST 65.1: Unexpected result quoting string", test_result, "HEAD first_test ?");

  test_result.clear();
  test_result.append(sqlstring("? second_test ?", UseAnsiQuotes) << base::sqlstring("HEAD", 0));

  ensure_equals("TEST 65.2: Unexpected result quoting string", test_result, "HEAD second_test ?");

  test_result.clear();
  test_result.append(sqlstring("! third_test !", QuoteOnlyIfNeeded) << base::sqlstring("HEAD", 0));

  ensure_equals("TEST 65.3: Unexpected result quoting string", test_result, "HEAD third_test !");

  test_result.clear();
  test_result.append(sqlstring("! fourth_test !", UseAnsiQuotes) << base::sqlstring("HEAD", 0));

  ensure_equals("TEST 65.4: Unexpected result quoting string", test_result, "HEAD fourth_test !");

  // test cases for extra parameters
  try {
    test_result.clear();
    test_result.append(sqlstring("? fifth_test", QuoteOnlyIfNeeded) << base::sqlstring("HEAD", 0)
                                                                    << base::sqlstring("TAIL", 0));
    fail("TEST 65.5: Unexpected result quoting string");
  } catch (...) {
    // Nothing to do, just catch the error and continue
  }

  try {
    test_result.clear();
    test_result.append(sqlstring("? sixth_test", UseAnsiQuotes) << base::sqlstring("HEAD", 0)
                                                                << base::sqlstring("TAIL", 0));
    fail("TEST 65.6: Unexpected result quoting string");
  } catch (...) {
    // Nothing to do, just catch the error and continue
  }

  try {
    test_result.clear();
    test_result.append(sqlstring("! seventh_test", QuoteOnlyIfNeeded) << base::sqlstring("HEAD", 0)
                                                                      << base::sqlstring("TAIL", 0));
    fail("TEST 65.7: Unexpected result quoting string");
  } catch (...) {
    // Nothing to do, just catch the error and continue
  }

  try {
    test_result.clear();
    test_result.append(sqlstring("! eighth_test", UseAnsiQuotes) << base::sqlstring("HEAD", 0)
                                                                 << base::sqlstring("TAIL", 0));
    fail("TEST 65.8: Unexpected result quoting string");
  } catch (...) {
    // Nothing to do, just catch the error and continue
  }
}

/*
 * Testing c-tors
 */
TEST_FUNCTION(70) {
  // constructors
  sqlstring c_tor_flag_QuoteOnlyIfNeeded_num("test ? ? ? ?", QuoteOnlyIfNeeded);
  sqlstring c_tor_flag_UseAnsiQuotes_num("test ? ? ? ?", UseAnsiQuotes);
  sqlstring c_tor_flag_QuoteOnlyIfNeeded_str("test ? ! ? !", QuoteOnlyIfNeeded);
  sqlstring c_tor_flag_UseAnsiQuotes_str("test ? ! ? !", UseAnsiQuotes);
  sqlstring c_tor_flag_QuoteOnlyIfNeeded_sqlstr("test ? !", QuoteOnlyIfNeeded);
  sqlstring c_tor_flag_UseAnsiQuotes_sqlstr("test ? !", UseAnsiQuotes);
  sqlstring c_tor_copy_flag_QuoteOnlyIfNeeded_str(c_tor_flag_QuoteOnlyIfNeeded_str);
  sqlstring c_tor_copy_flag_UseAnsiQuotes_str(c_tor_flag_UseAnsiQuotes_str);
  sqlstring c_tor_empty_constructor;

  // miscellaneous variables
  std::int64_t int64_t_value = 1999;
  int int_value = -2001;
  float float_value = (float)3.141592;
  double double_value = 2.718281;
  std::string tail = "TAIL";
  const char* const_tail = "TAIL";

  // test cases for constructors & numeric values
  std::string test_result = "";
  test_result.append(c_tor_flag_QuoteOnlyIfNeeded_num << int64_t_value);

  ensure_equals("TEST 70.1: Unexpected result replacing ? with a long long/int64 value", test_result,
                "test 1999 ? ? ?");

  test_result.clear();
  test_result.append(c_tor_flag_QuoteOnlyIfNeeded_num << int_value);

  ensure_equals("TEST 70.2: Unexpected result replacing ? with an int value", test_result, "test 1999 -2001 ? ?");

  test_result.clear();
  test_result.append(c_tor_flag_QuoteOnlyIfNeeded_num << float_value);

  ensure_equals("TEST 70.3: Unexpected result replacing ? with a float value", test_result,
                "test 1999 -2001 3.141592 ?");

  test_result.clear();
  test_result.append(c_tor_flag_QuoteOnlyIfNeeded_num << double_value);

  ensure_equals("TEST 70.4: Unexpected result replacing ? with a double value", test_result,
                "test 1999 -2001 3.141592 2.718281");

  test_result.clear();
  test_result.append(c_tor_flag_UseAnsiQuotes_num << int64_t_value);

  ensure_equals("TEST 70.5: Unexpected result replacing ? with a long long/int64 value", test_result,
                "test 1999 ? ? ?");

  test_result.clear();
  test_result.append(c_tor_flag_UseAnsiQuotes_num << int_value);

  ensure_equals("TEST 70.6: Unexpected result replacing ? with an int value", test_result, "test 1999 -2001 ? ?");

  test_result.clear();
  test_result.append(c_tor_flag_UseAnsiQuotes_num << float_value);

  ensure_equals("TEST 70.7: Unexpected result replacing ? with a float value", test_result,
                "test 1999 -2001 3.141592 ?");

  test_result.clear();
  test_result.append(c_tor_flag_UseAnsiQuotes_num << double_value);

  ensure_equals("TEST 70.8: Unexpected result replacing ? with a double value", test_result,
                "test 1999 -2001 3.141592 2.718281");

  // test cases for copy constructors & std::string values
  test_result.clear();
  test_result.append(c_tor_copy_flag_QuoteOnlyIfNeeded_str << tail);

  ensure_equals("TEST 70.9: Unexpected result quoting string", test_result, "test 'TAIL' ! ? !");

  test_result.clear();
  test_result.append(c_tor_copy_flag_UseAnsiQuotes_str << tail);

  ensure_equals("TEST 70.10: Unexpected result quoting string", test_result, "test \"TAIL\" ! ? !");

  test_result.clear();
  test_result.append(c_tor_copy_flag_QuoteOnlyIfNeeded_str << tail);

  ensure_equals("TEST 70.11: Unexpected result quoting string", test_result, "test 'TAIL' TAIL ? !");

  test_result.clear();
  test_result.append(c_tor_copy_flag_UseAnsiQuotes_str << tail);

  ensure_equals("TEST 70.12: Unexpected result quoting string", test_result, "test \"TAIL\" `TAIL` ? !");

  // test cases for copy constructors & (const char*) values
  test_result.clear();
  test_result.append(c_tor_copy_flag_QuoteOnlyIfNeeded_str << const_tail);

  ensure_equals("TEST 70.13: Unexpected result quoting string", test_result, "test 'TAIL' TAIL 'TAIL' !");

  test_result.clear();
  test_result.append(c_tor_copy_flag_UseAnsiQuotes_str << const_tail);

  ensure_equals("TEST 70.14: Unexpected result quoting string", test_result, "test \"TAIL\" `TAIL` \"TAIL\" !");

  test_result.clear();
  test_result.append(c_tor_copy_flag_QuoteOnlyIfNeeded_str << const_tail);

  ensure_equals("TEST 70.15: Unexpected result quoting string", test_result, "test 'TAIL' TAIL 'TAIL' TAIL");

  test_result.clear();
  test_result.append(c_tor_copy_flag_UseAnsiQuotes_str << const_tail);

  ensure_equals("TEST 70.16: Unexpected result quoting string", test_result, "test \"TAIL\" `TAIL` \"TAIL\" `TAIL`");

  // test cases for constructors & (const sqlstring&) values
  test_result.clear();
  test_result.append(c_tor_flag_QuoteOnlyIfNeeded_sqlstr << base::sqlstring("TAIL", 0));

  ensure_equals("TEST 70.17: Unexpected result quoting string", test_result, "test TAIL !");

  test_result.clear();
  test_result.append(c_tor_flag_UseAnsiQuotes_sqlstr << base::sqlstring("TAIL", 0));

  ensure_equals("TEST 70.18: Unexpected result quoting string", test_result, "test TAIL !");

  test_result.clear();
  test_result.append(c_tor_flag_QuoteOnlyIfNeeded_sqlstr << base::sqlstring("TAIL", 0));

  ensure_equals("TEST 70.19: Unexpected result quoting string", test_result, "test TAIL TAIL");

  test_result.clear();
  test_result.append(c_tor_flag_UseAnsiQuotes_sqlstr << base::sqlstring("TAIL", 0));

  ensure_equals("TEST 70.20: Unexpected result quoting string", test_result, "test TAIL TAIL");

  // test case for empty constructor
  test_result.clear();
  test_result.append(c_tor_empty_constructor);

  ensure_equals("TEST 70.21: Unexpected result quoting string", test_result, "");
}

/*
 * Testing done()
 */
TEST_FUNCTION(75) {
  // constructor
  sqlstring sqlstr("test ? ! ? !", 0);

  std::string test_result = "";
  while (!sqlstr.done()) {
    test_result.append(sqlstr << 1 << "2" << 3.0 << base::sqlstring("4", 0));
  }

  ensure_equals("TEST 75.1: Unexpected result appending values", test_result, "test 1 `2` 3.000000 4");
}

/*
 * Testing overridden operator std::string
 */
TEST_FUNCTION(80) {
  // constructor
  sqlstring sqlstr("test ? ! ? ! ?", 0);

  std::string test_result = "";
  test_result.append(sqlstr << 1 << "2" << 3.0 << base::sqlstring("4", 0));

  ensure_equals("TEST 80.1: Unexpected result during operator std::string call", test_result, std::string(sqlstr));
}

/*
 * Testing memory management and/or memory leaks
 */
TEST_FUNCTION(85) {
  // constructor with (MAX_SIZE_RANDOM_VALUES_ARRAY x 4) placeholders
  sqlstring* sqlstr[MAX_SIZE_SQLSTRING_ARRAY];
  int i, j;

  std::string test_result;
  std::string expected_result;
  std::string error_msg;

  // Create MAX_SIZE_SQLSTRING_ARRAY sqlstring's
  for (i = 0; i < MAX_SIZE_SQLSTRING_ARRAY; i++) {
    // Each sqlstring has (MAX_SIZE_RANDOM_VALUES_ARRAY x 4) placeholders
    sqlstr[i] = new sqlstring(placeholders.c_str(), 0);
    test_result.clear();
    expected_result.clear();
    error_msg.clear();

    for (j = 0; j < MAX_SIZE_RANDOM_VALUES_ARRAY; j++) {
      *sqlstr[i] << random_values[j].int_value << random_values[j].string_value << random_values[j].double_value
                 << random_values[j].sqlstring_value;

      expected_result.append(strfmt("%d", random_values[j].int_value));
      expected_result.append(" `");
      expected_result.append(random_values[j].string_value);
      expected_result.append("` ");
      expected_result.append(strfmt("%f", random_values[j].double_value));
      expected_result.append(" ");
      expected_result.append(random_values[j].sqlstring_value);
      expected_result.append(" ");
    }
    test_result.assign(*sqlstr[i]);

    error_msg.append(strfmt("TEST 85.%d: Unexpected result appending values", i + 1));

    ensure_equals(error_msg, test_result, expected_result);
  }

  try {
    for (i = 0; i < MAX_SIZE_SQLSTRING_ARRAY; i++) {
      // Free the memory
      delete sqlstr[i];
    }
  } catch (std::exception& e) {
    error_msg.clear();
    error_msg.append(strfmt("TEST 85.%d: Error freeing the memory - %s", i + 1, e.what()));
    fail(error_msg);
  }
}

END_TESTS
