/* 
 * Copyright (c) 2011, 2015, Oracle and/or its affiliates. All rights reserved.
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

#include "base/string_utilities.h"
#include "grtpp.h"
#include "wb_helpers.h"

#include <algorithm>

using namespace base;

// updated as of 5.7
static const char *reserved_keywords[] = {
  "ACCESSIBLE",
  "ADD",
  "ALL",
  "ALTER",
  "ANALYZE",
  "AND",
  "AS",
  "ASC",
  "ASENSITIVE",
  "BEFORE",
  "BETWEEN",
  "BIGINT",
  "BINARY",
  "BLOB",
  "BOTH",
  "BY",
  "CALL",
  "CASCADE",
  "CASE",
  "CHANGE",
  "CHAR",
  "CHARACTER",
  "CHECK",
  "COLLATE",
  "COLUMN",
  "CONDITION",
  "CONSTRAINT",
  "CONTINUE",
  "CONVERT",
  "CREATE",
  "CROSS",
  "CURRENT_DATE",
  "CURRENT_TIME",
  "CURRENT_TIMESTAMP",
  "CURRENT_USER",
  "CURSOR",
  "DATABASE",
  "DATABASES",
  "DAY_HOUR",
  "DAY_MICROSECOND",
  "DAY_MINUTE",
  "DAY_SECOND",
  "DEC",
  "DECIMAL",
  "DECLARE",
  "DEFAULT",
  "DELAYED",
  "DELETE",
  "DESC",
  "DESCRIBE",
  "DETERMINISTIC",
  "DISTINCT",
  "DISTINCTROW",
  "DIV",
  "DOUBLE",
  "DROP",
  "DUAL",
  "EACH",
  "ELSE",
  "ELSEIF",
  "ENCLOSED",
  "ESCAPED",
  "EXISTS",
  "EXIT",
  "EXPLAIN",
  "FALSE",
  "FETCH",
  "FLOAT",
  "FLOAT4",
  "FLOAT8",
  "FOR",
  "FORCE",
  "FOREIGN",
  "FROM",
  "FULLTEXT",
  "GET",
  "GRANT",
  "GROUP",
  "HAVING",
  "HIGH_PRIORITY",
  "HOUR_MICROSECOND",
  "HOUR_MINUTE",
  "HOUR_SECOND",
  "IF",
  "IGNORE",
  "IN",
  "INDEX",
  "INFILE",
  "INNER",
  "INOUT",
  "INSENSITIVE",
  "INSERT",
  "INT",
  "INT1",
  "INT2",
  "INT3",
  "INT4",
  "INT8",
  "INTEGER",
  "INTERVAL",
  "INTO",
  "IO_AFTER_GTIDS",
  "IO_BEFORE_GTIDS",
  "IS",
  "ITERATE",
  "JOIN",
  "KEY",
  "KEYS",
  "KILL",
  "LEADING",
  "LEAVE",
  "LEFT",
  "LIKE",
  "LIMIT",
  "LINEAR",
  "LINES",
  "LOAD",
  "LOCALTIME",
  "LOCALTIMESTAMP",
  "LOCK",
  "LONG",
  "LONGBLOB",
  "LONGTEXT",
  "LOOP",
  "LOW_PRIORITY",
  "MASTER_BIND",
  "MASTER_SSL_VERIFY_SERVER_CERT",
  "MATCH",
  "MAXVALUE",
  "MEDIUMBLOB",
  "MEDIUMINT",
  "MEDIUMTEXT",
  "MIDDLEINT",
  "MINUTE_MICROSECOND",
  "MINUTE_SECOND",
  "MOD",
  "MODIFIES",
  "NATURAL",
  "NONBLOCKING",
  "NOT",
  "NO_WRITE_TO_BINLOG",
  "NULL",
  "NUMERIC",
  "ON",
  "OPTIMIZE",
  "OPTION",
  "OPTIONALLY",
  "OR",
  "ORDER",
  "OUT",
  "OUTER",
  "OUTFILE",
  "PARTITION",
  "PRECISION",
  "PRIMARY",
  "PROCEDURE",
  "PURGE",
  "RANGE",
  "READ",
  "READS",
  "READ_WRITE",
  "REAL",
  "REFERENCES",
  "REGEXP",
  "RELEASE",
  "RENAME",
  "REPEAT",
  "REPLACE",
  "REQUIRE",
  "RESIGNAL",
  "RESTRICT",
  "RETURN",
  "REVOKE",
  "RIGHT",
  "RLIKE",
  "SCHEMA",
  "SCHEMAS",
  "SECOND_MICROSECOND",
  "SELECT",
  "SENSITIVE",
  "SEPARATOR",
  "SET",
  "SHOW",
  "SIGNAL",
  "SMALLINT",
  "SPATIAL",
  "SPECIFIC",
  "SQL",
  "SQLEXCEPTION",
  "SQLSTATE",
  "SQLWARNING",
  "SQL_BIG_RESULT",
  "SQL_CALC_FOUND_ROWS",
  "SQL_SMALL_RESULT",
  "SSL",
  "STARTING",
  "STRAIGHT_JOIN",
  "TABLE",
  "TERMINATED",
  "THEN",
  "TINYBLOB",
  "TINYINT",
  "TINYTEXT",
  "TO",
  "TRAILING",
  "TRIGGER",
  "TRUE",
  "UNDO",
  "UNION",
  "UNIQUE",
  "UNLOCK",
  "UNSIGNED",
  "UPDATE",
  "USAGE",
  "USE",
  "USING",
  "UTC_DATE",
  "UTC_TIME",
  "UTC_TIMESTAMP",
  "VALUES",
  "VARBINARY",
  "VARCHAR",
  "VARCHARACTER",
  "VARYING",
  "WHEN",
  "WHERE",
  "WHILE",
  "WITH",
  "WRITE",
  "XOR",
  "YEAR_MONTH",
  "ZEROFILL",
  NULL
};


BEGIN_TEST_DATA_CLASS(string_utilities_test)

  protected:
    std::string long_random_string; // Content doesn't matter. There must be no crash using it.

TEST_DATA_CONSTRUCTOR(string_utilities_test)
{
  for (int i = 0; i < 1000; i++)
  {
    long_random_string += ' ' + rand() % 94; // Visible characters after space
    
    if (rand() > RAND_MAX / 2)
      long_random_string += "\xE3\x8A\xA8"; // Valid Unicode character.
    if (i == 500)
      long_random_string += "\xE3\x8A\xA8"; // Ensure it is there at least once.
  }
  long_random_string.erase(std::remove(long_random_string.begin(), long_random_string.end(), 0x7f), long_random_string.end()); // 0x7F is a special character that we use for tests
}

END_TEST_DATA_CLASS;

TEST_MODULE(string_utilities_test, "string utilities");


/* Testing base::quote_identifier */
TEST_FUNCTION(5)
{
  std::string test = "first_test";

  std::string test_result = base::quote_identifier(test, '`');
  ensure_equals("Unexpected result quoting string", test_result, "`first_test`");

  test = "second_test";
  test_result = base::quote_identifier(test, '\"');
  ensure_equals("Unexpected result quoting string", test_result, "\"second_test\"");

  test = "";
  test_result = base::quote_identifier(test, '\"');
  ensure_equals("Unexpected result quoting string", test_result, "\"\"");

  test = "Unicode \xE3\x8A\xA8"; // UTF-8 encoded: CIRCLED IDEOGRAPH RIGHT
  test_result = base::quote_identifier(test, '%');
  ensure_equals("Unexpected result quoting string", test_result, "%Unicode \xE3\x8A\xA8%");
}

/* Testing base::quote_identifier_if_needed */
TEST_FUNCTION(10)
{
  std::string test = "first_test";

  std::string test_result = base::quote_identifier_if_needed(test, '`');
  ensure_equals("Unexpected result quoting string", test_result, "first_test");

  test = "second_test";
  test_result = base::quote_identifier_if_needed(test, '\"');
  ensure_equals("Unexpected result quoting string", test_result, "second_test");

  test = "Unicode\xE3\x8A\xA8"; // UTF-8 encoded: CIRCLED IDEOGRAPH RIGHT
  test_result = base::quote_identifier_if_needed(test, '%');
  ensure_equals("Unexpected result quoting string", test_result, "Unicode\xE3\x8A\xA8");

  test = "test.case";
  test_result = base::quote_identifier_if_needed(test, '$');
  ensure_equals("Unexpected result quoting string", test_result, "$test.case$");

  // Note: currently there is no support to check if the given string contains the quote char already.
  test = "test$case";
  test_result = base::quote_identifier_if_needed(test, '$');
  ensure_equals("Unexpected result quoting string", test_result, "test$case");

  test = ".test$case";
  test_result = base::quote_identifier_if_needed(test, '$');
  ensure_equals("Unexpected result quoting string", test_result, "$.test$case$");

  test = "test-case";
  test_result = base::quote_identifier_if_needed(test, '`');
  ensure_equals("Unexpected result quoting string", test_result, "`test-case`");

  // Identifiers consisting only of digits cannot be distinguished from normal numbers
  // so they must be quoted.
  test = "12345";
  test_result = base::quote_identifier_if_needed(test, '`');
  ensure_equals("Unexpected result quoting string", test_result, "`12345`");
}

/**
 * Testing base::unquote_identifier
 */
TEST_FUNCTION(15)
{
  std::string test = "\"first_test\"";
  std::string test_result = base::unquote_identifier(test);
  ensure_equals("Unexpected result unquoting string", test_result, "first_test");

  test = "`second_test`";
  test_result = base::unquote_identifier(test);
  ensure_equals("Unexpected result unquoting string", test_result, "second_test");
}

static bool compare_string_lists(const std::vector<std::string> &slist,
                                const char *check[])
{
  size_t i;
  for (i = 0; i < slist.size(); i++)
  {
    if (check[i] == NULL)
    {
      g_message("result has more items than expected\n");
      return false;
    }
    if (slist[i] != check[i])
    {
      g_message("token comparison mismatch");
      for (size_t j= 0; j < slist.size(); j++)
         g_message("%s", slist[j].c_str());
      return false;
    }
  }
  if (check[i])
  {
    g_message("result has fewer items than expeced\n");
    return false;
  }
  return true;
}


TEST_FUNCTION(20)
{
  const char* empty[] = {"", NULL};
  const char* empty2[] = {"", "", NULL};
  const char* empty3[] = {"", "", "", NULL};
  const char* null_empty[] = {"NULL", "", NULL};
  const char* a[] = {"a", NULL};
  const char* a_empty1[] = {"a", "", NULL};
  const char *a_empty2[] = {"a", "", "", NULL};
  const char *ab_empty1[] = {"a", "b", "", NULL};
  const char *ab_empty2[] = {"a", "", "b", NULL};
  const char *ab_empty3[] = {"", "a", "b", NULL};
  const char* null_null[] = {"NULL", "NULL", NULL};
  const char* emptys_null[] = {"''", "NULL", NULL};
  const char* ab_null[] = {"'a,b'", "NULL", NULL};
  const char* ab_xxx[] = {"'a,b'", "\"x\\xx\"", "'fo''bar'", NULL};

  ensure("split tokens empty1", compare_string_lists(base::split_token_list("", ','), empty));
  ensure("split tokens empty2", compare_string_lists(base::split_token_list(",", ','), empty2));
  ensure("split tokens empty2a", compare_string_lists(base::split_token_list(" ,", ','), empty2));
  ensure("split tokens empty2b", compare_string_lists(base::split_token_list(", ", ','), empty2));
  ensure("split tokens empty3", compare_string_lists(base::split_token_list(",,", ','), empty3));
  ensure("split tokens empty4", compare_string_lists(base::split_token_list("NULL,", ','), null_empty));
  ensure("split tokens a", compare_string_lists(base::split_token_list("a", ','), a));
  ensure("split tokens a_empty1", compare_string_lists(base::split_token_list("a,", ','), a_empty1));
  ensure("split tokens a_empty2", compare_string_lists(base::split_token_list("a,,", ','), a_empty2));
  ensure("split tokens ab_empty1", compare_string_lists(base::split_token_list("a,b,", ','), ab_empty1));
  ensure("split tokens ab_empty2", compare_string_lists(base::split_token_list("a,,b", ','), ab_empty2));
  ensure("split tokens ab_empty3", compare_string_lists(base::split_token_list(",a,b", ','), ab_empty3));
  ensure("split tokens null,", compare_string_lists(base::split_token_list("NULL,", ','), null_empty));
  ensure("split tokens null,null", compare_string_lists(base::split_token_list("NULL,NULL", ','), null_null));
  ensure("split tokens '',null", compare_string_lists(base::split_token_list("'',NULL", ','), emptys_null));
  ensure("split tokens 'a,b',null", compare_string_lists(base::split_token_list("'a,b',NULL", ','), ab_null));
  ensure("split tokens 'a,b' , \"x\\xx\",'fo''bar'", 
    compare_string_lists(base::split_token_list("'a,b' , \"x\\xx\",'fo''bar'   ", ','), ab_xxx));
}

// Testing split_by_set.
TEST_FUNCTION(25)
{
  const char *input[] = {NULL};
  ensure("Split by set 1", compare_string_lists(base::split_by_set("", ""), input));
  ensure("Split by set 2", compare_string_lists(base::split_by_set("", " "), input));
  ensure("Split by set 3", compare_string_lists(base::split_by_set("", long_random_string), input));

  const char *input2[] = {long_random_string.c_str(), NULL};
  ensure("Split by set 4", compare_string_lists(base::split_by_set(long_random_string, ""), input2));
  ensure("Split by set 5", base::split_by_set(long_random_string, "\xA8").size() > 1); // Works only because our implementation is not utf-8 aware.

  const char *input3[] = {"Lorem", "ipsum", "dolor", "sit", "amet.", NULL};
  ensure("Split by set 6", compare_string_lists(base::split_by_set("Lorem ipsum dolor sit amet.", " "), input3));
  const char *input4[] = {"Lorem", "ipsum", "dolor sit amet.", NULL};
  ensure("Split by set 7", compare_string_lists(base::split_by_set("Lorem ipsum dolor sit amet.", " ", 2), input4));

  const char *input5[] = {"\"Lorem\"", "\"ipsum\"", "\"dolor\"", "\"sit\"", "\"amet\"", NULL};
  ensure("Split by set 8", compare_string_lists(base::split_by_set("\"Lorem\"\t\"ipsum\"\t\"dolor\"\t\"sit\"\t\"amet\"", "\t"), input5));

  const char *input6[] = {"\"Lorem\"", "\"ipsum\"", "\"dolor\"", "\"sit\"", "\"amet\"", NULL};
  ensure("Split by set 9", compare_string_lists(base::split_by_set("\"Lorem\"\t\"ipsum\"\n\"dolor\"\t\"sit\"\n\"amet\"", " \t\n"), input6));

  const char *input7[] = {"\"Lorem\"", "\"ip", "sum\"", "\"dolor\"", "\"sit\"", "\"amet\"", NULL};
  ensure("Split by set 10", compare_string_lists(base::split_by_set("\"Lorem\"\t\"ip sum\"\n\"dolor\"\t\"sit\"\n\"amet\"", " \t\n"), input7));

  const char *input8[] = {"", "Lorem", "", " ", "ipsum", "", " ", "dolor", "", " ", "sit", "", " ", "amet", "", NULL};
  ensure("Split by set 11", compare_string_lists(base::split_by_set("\"Lorem\", \"ipsum\", \"dolor\", \"sit\", \"amet\"", ",\""), input8));
  ensure("Split by set 12", compare_string_lists(base::split_by_set("\"Lorem\", \"ipsum\", \"dolor\", \"sit\", \"amet\"", ",\"", 100), input8));
  const char *input9[] = {"", "Lorem", "", " ", "ipsum", "", " ", "dolor\", \"sit\", \"amet\"", NULL};
  ensure("Split by set 13", compare_string_lists(base::split_by_set("\"Lorem\", \"ipsum\", \"dolor\", \"sit\", \"amet\"", ",\"", 7), input9));
}

// Testing trim_right, trim_left, trim.
TEST_FUNCTION(30)
{
  ensure_equals("Trim left 1", base::trim_left(""), "");
  ensure_equals("Trim left 2", base::trim_left("                                       "), "");
  ensure_equals("Trim left 3", base::trim_left("           \n\t\t\t\t      a"), "a");
  ensure_equals("Trim left 4", base::trim_left("a           \n\t\t\t\t      "), "a           \n\t\t\t\t      ");
  ensure_equals("Trim left 5", base::trim_left("", long_random_string), "");

  ensure_equals("Trim left 6", base::trim_left("\xE3\x8A\xA8\xE3\x8A\xA8\x7F", long_random_string), "\x7F");
  ensure_equals("Trim left 7", base::trim_left("\t\t\tLorem ipsum dolor sit amet\n\n\n"), "Lorem ipsum dolor sit amet\n\n\n");
  ensure_equals("Trim left 8", base::trim_left("\t\t\tLorem ipsum dolor sit amet\n\n\n", "L"), "\t\t\tLorem ipsum dolor sit amet\n\n\n");
  ensure_equals("Trim left 9", base::trim_left("\t\t\tLorem ipsum dolor sit amet\n\n\n", "\t\t\tL"), "orem ipsum dolor sit amet\n\n\n");

  ensure_equals("Trim right 1", base::trim_right(""), "");
  ensure_equals("Trim right 2", base::trim_right("                                       "), "");
  ensure_equals("Trim right 3", base::trim_right("           \n\t\t\t\t      a"), "           \n\t\t\t\t      a");
  ensure_equals("Trim right 4", base::trim_right("a           \n\t\t\t\t      "), "a");
  ensure_equals("Trim right 5", base::trim_right("", long_random_string), "");

  ensure_equals("Trim right 6", base::trim_right("\x7F\xE3\x8A\xA8\xE3\x8A\xA8", long_random_string), "\x7F");
  ensure_equals("Trim right 7", base::trim_right("\t\t\tLorem ipsum dolor sit amet\n\n\n"), "\t\t\tLorem ipsum dolor sit amet");
  ensure_equals("Trim right 8", base::trim_right("\t\t\tLorem ipsum dolor sit amet\n\n\n", "L"), "\t\t\tLorem ipsum dolor sit amet\n\n\n");
  ensure_equals("Trim right 9", base::trim_right("\t\t\tLorem ipsum dolor sit amet\n\n\n", "\n\n\nt"), "\t\t\tLorem ipsum dolor sit ame");

  ensure_equals("Trim 1", base::trim(""), "");
  ensure_equals("Trim 2", base::trim("                                       "), "");
  ensure_equals("Trim 3", base::trim("           \n\t\t\t\t      a"), "a");
  ensure_equals("Trim 4", base::trim("a           \n\t\t\t\t      "), "a");
  ensure_equals("Trim 5", base::trim("", long_random_string), "");

  ensure_equals("Trim 6", base::trim("\xE3\x8A\xA8\xE3\x8A\xA8\x7F\xE3\x8A\xA8\xE3\x8A\xA8", long_random_string), "\x7F");
  ensure_equals("Trim 7", base::trim("\t\t\tLorem ipsum dolor sit amet\n\n\n"), "Lorem ipsum dolor sit amet");
  ensure_equals("Trim 8", base::trim("\t\t\tLorem ipsum dolor sit amet\n\n\n", "L"), "\t\t\tLorem ipsum dolor sit amet\n\n\n");
  ensure_equals("Trim 9", base::trim("\t\t\tLorem ipsum dolor sit amet\n\n\n", "\n\n\t\t\ttL"), "orem ipsum dolor sit ame");

}

/**
 * Testing base::unescape_sql_string, which also includes escape sequence handling.
 */
TEST_FUNCTION(35)
{
  std::string test_result = base::unescape_sql_string("", '`');
  ensure_equals("Unquoting 35.1", test_result, "");

  test_result = base::unescape_sql_string("lorem ipsum dolor sit amet", '"');
  ensure_equals("Unquoting 35.2", test_result, "lorem ipsum dolor sit amet");

  test_result = base::unescape_sql_string("lorem ipsum dolor`` sit amet", '"');
  ensure_equals("Unquoting 35.3", test_result, "lorem ipsum dolor`` sit amet");

  test_result = base::unescape_sql_string("lorem ipsum \"\"dolor sit amet", '"');
  ensure_equals("Unquoting 35.4", test_result, "lorem ipsum \"dolor sit amet");

  test_result = base::unescape_sql_string("lorem \"\"\"\"ipsum \"\"dolor\"\" sit amet", '"');
  ensure_equals("Unquoting 35.5", test_result, "lorem \"\"ipsum \"dolor\" sit amet");

  test_result = base::unescape_sql_string("lorem \\\"\\\"ipsum\"\" \\\\dolor sit amet", '"');
  ensure_equals("Unquoting 35.6", test_result, "lorem \"\"ipsum\" \\dolor sit amet");

  test_result = base::unescape_sql_string("lorem \\\"\\\"ipsum\"\" \\\\dolor sit amet", '\'');
  ensure_equals("Unquoting 35.7", test_result, "lorem \"\"ipsum\"\" \\dolor sit amet");

  // Embedded 0 is more difficult to test due to limitations when comparing strings. So we do this
  // in a separate test.
  test_result = base::unescape_sql_string("lorem\\n ip\\t\\rsum dolor\\b sit \\Zamet", '"');
  ensure_equals("Unquoting 35.8", test_result, "lorem\n ip\t\rsum dolor\b sit \032amet");

  test_result = base::unescape_sql_string("lorem ipsum \\zd\\a\\olor sit amet", '"');
  ensure_equals("Unquoting 35.9", test_result, "lorem ipsum zdaolor sit amet");

  test_result = base::unescape_sql_string("\\0\\n\\t\\r\\b\\Z", '"');
  ensure("Unquoting 35.10", 
    test_result[0] == 0 && test_result[1] == 10 && test_result[2] == 9 && test_result[3] == 13 &&
    test_result[4] == '\b' && test_result[5] == 26 
  );

  std::string long_string;
  long_string.resize(2000);
  std::fill(long_string.begin(), long_string.end(), '`');
  test_result = base::unescape_sql_string(long_string, '`');
  ensure_equals("Unquoting 35.11", test_result, long_string.substr(0, 1000));
}

TEST_FUNCTION(36)
{
  std::string content1 = "11111111 22222 3333 444444 555555555 666666 77777777 88888 999999999 00000000";
  std::string content2 = "\xC3\x81\xC3\x81\xC3\x81\xC3\x81\xC3\x81\xC3\x81\xC3\x81\xC3\x81 \xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89 \xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D \xC3\x93\xC3\x93\xC3\x93\xC3\x93\xC3\x93\xC3\x93\xC3\x93\xC3\x93 \xC3\x9A\xC3\x9A\xC3\x9A\xC3\x9A\xC3\x9A\xC3\x9A\xC3\x9A\xC3\x9A";
  std::string content3 = "11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111";
  std::string content4 = "aaaaa bbbbb cccc dddd \xAA\xBB\xCC\xDD";
  std::string content5 = "aaaa bbbb cccc dddd eeee";
//   模板名，使用英文，保证唯一性。格式建议：“类型_动作”，如“blog_add”或“credit_blog_add”
//   Template name, in English, to ensure uniqueness. Format advice: "type _ action," such as "blog_add" or "credit_blog_add"
  unsigned char content6[] = 
  { 0x20
  , 0x20
  , 0x6e
  , 0x61
  , 0x6d
  , 0x65
  , 0x3a
  , 0x20
  , 0xe6, 0xa8, 0xa1
  , 0xe6, 0x9d, 0xbf  // 10
  , 0xe5, 0x90, 0x8d
  , 0xef, 0xbc, 0x8c
  , 0xe4, 0xbd, 0xbf
  , 0xe7, 0x94, 0xa8
  , 0xe8, 0x8b, 0xb1
  , 0xe6, 0x96, 0x87
  , 0xef, 0xbc, 0x8c
  , 0xe4, 0xbf, 0x9d
  , 0xe8, 0xaf, 0x81
  , 0xe5, 0x94, 0xaf  // 20
  , 0xe4, 0xb8, 0x80
  , 0xe6, 0x80, 0xa7
  , 0xe3, 0x80, 0x82
  , 0xe6, 0xa0, 0xbc
  , 0xe5, 0xbc, 0x8f
  , 0xe5, 0xbb, 0xba
  , 0xe8, 0xae, 0xae
  , 0xef, 0xbc, 0x9a
  , 0xe2, 0x80, 0x9c
  , 0xe7, 0xb1, 0xbb  // 30
  , 0xe5, 0x9e, 0x8b
  , 0x5f
  , 0xe5, 0x8a, 0xa8
  , 0xe4, 0xbd, 0x9c
  , 0xe2, 0x80, 0x9d
  , 0xef, 0xbc, 0x8c
  , 0xe5, 0xa6, 0x82
  , 0xe2, 0x80, 0x9c
  , 0x62
  , 0x6c              // 40
  , 0x6f
  , 0x67
  , 0x5f
  , 0x61
  , 0x64
  , 0x64
  , 0xe2, 0x80, 0x9d
  , 0xe6, 0x88, 0x96
  , 0xe2, 0x80, 0x9c
  , 0x63              // 50
  , 0x72
  , 0x65
  , 0x64
  , 0x69
  , 0x74
  , 0x5f
  , 0x62
  , 0x6c
  , 0x6f
  , 0x67              // 60
  , 0x5f
  , 0x61
  , 0x64
  , 0x64
  , 0xe2, 0x80, 0x9d  // 65
  , 0x00
  };

  std::string expected1 = "  11111111 22222 \n  3333 444444 \n  555555555 666666 \n  77777777 88888 \n  999999999 \n  00000000";
  std::string expected2 = "11111111 22222 \n  3333 444444 \n  555555555 666666 \n  77777777 88888 \n  999999999 \n  00000000";
  std::string expected3 = "  \xC3\x81\xC3\x81\xC3\x81\xC3\x81\xC3\x81\xC3\x81\xC3\x81\xC3\x81 \xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89 \n  \xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D\xC3\x8D \n  \xC3\x93\xC3\x93\xC3\x93\xC3\x93\xC3\x93\xC3\x93\xC3\x93\xC3\x93 \xC3\x9A\xC3\x9A\xC3\x9A\xC3\x9A\xC3\x9A\xC3\x9A\xC3\x9A\xC3\x9A";
  std::string expected4 = "  11111111 22222 \n  3333 444444 \n(...)";
  std::string expected5 = "  111111111111111111\n  111111111111111111\n  111111111111111111\n  111111111111111111\n  111111111111111111\n  111111111111111111\n  111111111111111111\n  11111111111111";
  std::string expected6 = "";
  std::string expected7 = "aaaa \nbbbb \ncccc \ndddd \neeee";
  
  std::string result;
  
  try
  {
    base::reflow_text(content4, 20, "  ");
    fail("TEST 36.1: Didn't throw an exception");
  }
  catch(std::invalid_argument)
  {
    // Exception was expected
  }
  
  try
  {
    result = base::reflow_text(content1, 20, "  ", true, 10);            //  Indentation
  }
  catch(std::logic_error &e)
  {
    fail(std::string("TEST 36.2: Unexpected exception: ") + e.what());
  }
  ensure_equals("Comparing word wrap with indentation", result, expected1);

  try
  {
    result = base::reflow_text(content1, 20, "  ", false, 10);           //  Do not indent first line
  }
  catch(std::logic_error &e)
  {
    fail(std::string("TEST 36.3: Unexpected exception: ") + e.what());
  }
  ensure_equals("Comparing word wrap without indentation", result, expected2);

  try
  {
    result = base::reflow_text(content2, 20, "  ", true, 10);            //  String with multi-byte characters
  }
  catch(std::logic_error &e)
  {
    fail(std::string("TEST 36.4: Unexpected exception: ") + e.what());
  }
  ensure_equals("Comparing word wrap with multi-byte", result, expected3);

  try
  {
    result = base::reflow_text(content1, 20, "  ", true, 2);             //  Line limit reached
  }
  catch(std::logic_error &e)
  {
    fail(std::string("TEST 36.5: Unexpected exception: ") + e.what());
  }
  ensure_equals("Comparing word wrap line limit", result, expected4);

  try
  {
    result = base::reflow_text(content3, 20, "  ", true, 10);            //  Big word
  }
  catch(std::logic_error &e)
  {
    fail(std::string("TEST 36.6: Unexpected exception: ") + e.what());
  }
  ensure_equals("Test with very big word", result, expected5);

  try
  {
    result = base::reflow_text("", 20, "  ", true, 10);                  //  Empty string
  }
  catch(std::logic_error &e)
  {
    fail(std::string("TEST 36.7: Unexpected exception: ") + e.what());
  }
  ensure_equals("Test with empty string", result, expected6);
    
  try
  {
    result = base::reflow_text(content5, 6, "    ", true, 10);            //  Left fill automatic removal
  }
  catch(std::logic_error &e)
  {
    fail(std::string("TEST 36.8: Unexpected exception: ") + e.what());
  }
  ensure_equals("Left fill automatic removal", result, expected7);
  
  try
  {
    result = base::reflow_text(content5, 4, "    ", true, 10);            //  Invalid line length
  }
  catch(std::logic_error &e)
  {
    fail(std::string("TEST 36.9: Unexpected exception: ") + e.what());
  }
  ensure_equals("Invalid line length", result, "");
  
  //  This test is to ensure that a big string won't mess up algorithm
  std::string dictionary[] = { "one", "big", "speech", "made", "words", "a", "of", "out" };
  std::string long_text;
    
  while (long_text.size() < SHRT_MAX)
  {
    int index = rand() % 8;   //  8 is the size of the dictionary
    long_text += dictionary[index] + " ";
  }
  
  try
  {
    result = base::reflow_text(long_text, 100, "  ", true, 1000);          //  Short string, long line
  }
  catch(std::logic_error &e)
  {
    fail(std::string("TEST 36.9: Unexpected exception: ") + e.what());
  }
  
  try
  {
    result = base::reflow_text(std::string((char*)content6), 10, "  ", false);          //  Short string, long line
  }
  catch(std::logic_error &e)
  {
    fail(std::string("TEST 36.10: Unexpected exception: ") + e.what());
  }  

  //  Remove the line feed and fill to verify coherence
  std::size_t position = 0;
  while((position = result.find("\n  ", position)) != std::string::npos)
    result.replace(position, 3, "");
    
  ensure_equals("Removing separators test", result, std::string((char*)content6));
  
  //  This test was a specific case of a bug
  try
  {
    result = base::reflow_text(std::string((char*)content6), 60, "    ");          //  Short string, long line
  }
  catch(std::logic_error &e)
  {
    fail(std::string("TEST 36.10: Unexpected exception: ") + e.what());
  }
  
}

TEST_FUNCTION(37)
{
  ensure_equals("TEST 37.1: unable to extract and convert number from string", base::atoi<int>("10G", 0), 10);
  ensure_equals("TEST 37.2: unable to convert string to number", base::atoi<int>("10", 0), 10);
  ensure_equals("TEST 37.3: default return value mismatch ", base::atoi<int>("G", -1), -1);
  bool test_exception = false;
  try
  {
    base::atoi<int>("G");
    test_exception = true;
  } catch (std::exception &)
  {

  }
  ensure_equals("TEST 37.4: missed exception on mismatched string", test_exception, false);
}


TEST_FUNCTION(40)
{
  std::string test_string = "This is a test string...to test.";
  std::string test_string_unicode = "\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89...Now that is a unicode string...\xE3\x8A\xA8";

  //  Base tests
  ensure_equals("TEST 40.1: Starts with (success test)", base::starts_with(test_string, "This"), true);
  ensure_equals("TEST 40.2: Starts with (exists somwehere in the middle)", base::starts_with(test_string, "is"), false);
  ensure_equals("TEST 40.3: Starts with (exists in the end)", base::starts_with(test_string, "test."), false);
  ensure_equals("TEST 40.4: Starts with (search text don't exist)", base::starts_with(test_string, "blablabla"), false);
  ensure_equals("TEST 40.5: Starts with (search an empty string)", base::starts_with(test_string, ""), true);
  ensure_equals("TEST 40.6: Starts with (starting on the second character)", base::starts_with(test_string, "his"), false);
  ensure_equals("TEST 40.7: Starts with (whole string)", base::starts_with(test_string, test_string), true);
  ensure_equals("TEST 40.8: Starts with (more then the original string)", base::starts_with(test_string, test_string + " Second part..."), false);
  ensure_equals("TEST 40.9: Starts with (empty source)", base::starts_with("", "blablabla"), false);
  ensure_equals("TEST 40.10: Starts with (empty source, empty search)", base::starts_with("", ""), true);

  ensure_equals("TEST 40.11: Ends with (success test)", base::ends_with(test_string, "test."), true);
  ensure_equals("TEST 40.12: Ends with (exists somwehere in the middle)", base::ends_with(test_string, "to "), false);
  ensure_equals("TEST 40.13: Ends with (exists at the beginning)", base::ends_with(test_string, "This"), false);
  ensure_equals("TEST 40.14: Ends with (search text don't exist)", base::ends_with(test_string, "blablabla"), false);
  ensure_equals("TEST 40.15: Ends with (search an empty string)", base::ends_with(test_string, ""), true);
  ensure_equals("TEST 40.16: Ends with (starting on the second last character)", base::ends_with(test_string, "test"), false);
  ensure_equals("TEST 40.17: Ends with (whole string)", base::ends_with(test_string, test_string), true);
  ensure_equals("TEST 40.18: Ends with (more then the original string)", base::ends_with(test_string, test_string + " Second part..."), false);
  ensure_equals("TEST 40.19: Ends with (empty source)", base::ends_with("", "blablabla"), false);
  ensure_equals("TEST 40.20: Ends with (empty source, empty search)", base::ends_with("", ""), true);

  
  // Unicode tests
  ensure_equals("TEST 40.21: [Unicode]Starts with (success test)", base::starts_with(test_string_unicode, "\xC3\x89\xC3\x89"), true);
  ensure_equals("TEST 40.22: [Unicode]Starts with (exists somwehere in the middle)", base::starts_with(test_string_unicode, "is"), false);
  ensure_equals("TEST 40.23: [Unicode]Starts with (exists in the end)", base::starts_with(test_string_unicode, "\xE3\x8A\xA8"), false);
  ensure_equals("TEST 40.24: [Unicode]Starts with (search text don't exist)", base::starts_with(test_string_unicode, "blablabla"), false);
  ensure_equals("TEST 40.25: [Unicode]Starts with (search an empty string)", base::starts_with(test_string_unicode, ""), true);
  ensure_equals("TEST 40.26: [Unicode]Starts with (starting on the second character)", base::starts_with(test_string_unicode, "\x89\xC3\x89\xC3"), false);
  ensure_equals("TEST 40.27: [Unicode]Starts with (whole string)", base::starts_with(test_string_unicode, test_string_unicode), true);
  ensure_equals("TEST 40.28: [Unicode]Starts with (more then the original string)", base::starts_with(test_string_unicode, test_string_unicode + ". Second part..."), false);

  ensure_equals("TEST 40.29: [Unicode]Ends with (success test)", base::ends_with(test_string_unicode, ".\xE3\x8A\xA8"), true);
  ensure_equals("TEST 40.30: [Unicode]Ends with (exists somwehere in the middle)", base::ends_with(test_string_unicode, "to "), false);
  ensure_equals("TEST 40.31: [Unicode]Ends with (exists at the beginning)", base::ends_with(test_string_unicode, "\xC3\x89\xC3\x89"), false);
  ensure_equals("TEST 40.32: [Unicode]Ends with (search text don't exist)", base::ends_with(test_string_unicode, "blablabla"), false);
  ensure_equals("TEST 40.33: [Unicode]Ends with (search an empty string)", base::ends_with(test_string_unicode, ""), true);
  ensure_equals("TEST 40.34: [Unicode]Ends with (starting on the second last character)", base::ends_with(test_string_unicode, ".\xE3\x8A"), false);
  ensure_equals("TEST 40.35: [Unicode]Ends with (whole string)", base::ends_with(test_string_unicode, test_string_unicode), true);
  ensure_equals("TEST 40.36: [Unicode]Ends with (more then the original string)", base::ends_with(test_string_unicode, test_string_unicode + ". Second part..."), false);
}

TEST_FUNCTION(41)
{
  std::string test_string = "This is a test string...to test.";
  std::string test_string_unicode = "\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89\xC3\x89...Now that is a unicode string...\xE3\x8A\xA8";

  // Base tests
  ensure_equals("TEST 40.1: String left (regular testing)", base::left(test_string, 5), "This ");
  ensure_equals("TEST 40.2: String left (last character", base::left(test_string, 1), "T");
  ensure_equals("TEST 40.3: String left (zero characters)", base::left(test_string, 0), "");
  ensure_equals("TEST 40.4: String left (with empty string)", base::left("", 5), "");
  ensure_equals("TEST 40.5: String left (whole string)", base::left(test_string, test_string.length()), test_string);
  ensure_equals("TEST 40.6: String left (more chars then the original string)", base::left(test_string, 50), test_string);

  ensure_equals("TEST 40.7: String right (regular testing)", base::right(test_string, 5), "test.");
  ensure_equals("TEST 40.8: String right (last character", base::right(test_string, 1), ".");
  ensure_equals("TEST 40.9: String right (zero characters)", base::right(test_string, 0), "");
  ensure_equals("TEST 40.10: String right (with empty string)", base::right("", 5), "");
  ensure_equals("TEST 40.11: String right (whole string)", base::right(test_string, test_string.length()), test_string);
  ensure_equals("TEST 40.12: String right (more chars then the original string)", base::right(test_string, 50), test_string);

  //  Unicode tests
  ensure_equals("TEST 40.13: [Unicode]String left (regular testing)", base::left(test_string_unicode, 5), "\xC3\x89\xC3\x89\xC3");
  ensure_equals("TEST 40.14: [Unicode]String left (last character", base::left(test_string_unicode, 1), "\xC3");
  ensure_equals("TEST 40.15: [Unicode]String left (zero characters)", base::left(test_string_unicode, 0), "");
  ensure_equals("TEST 40.16: [Unicode]String left (with empty string)", base::left("", 5), "");
  ensure_equals("TEST 40.17: [Unicode]String left (whole string)", base::left(test_string_unicode, test_string_unicode.length()), test_string_unicode);
  ensure_equals("TEST 40.18: [Unicode]String left (more chars then the original string)", base::left(test_string_unicode, 500), test_string_unicode);

  ensure_equals("TEST 40.19: [Unicode]String right (regular testing)", base::right(test_string_unicode, 5), "..\xE3\x8A\xA8");
  ensure_equals("TEST 40.20: [Unicode]String right (last character", base::right(test_string_unicode, 1), "\xA8");
  ensure_equals("TEST 40.21: [Unicode]String right (zero characters)", base::right(test_string_unicode, 0), "");
  ensure_equals("TEST 40.22: [Unicode]String right (with empty string)", base::right("", 5), "");
  ensure_equals("TEST 40.23: [Unicode]String right (whole string)", base::right(test_string_unicode, test_string_unicode.length()), test_string_unicode);
  ensure_equals("TEST 40.24: [Unicode]String right (more chars then the original string)", base::right(test_string_unicode, 500), test_string_unicode);
}


TEST_FUNCTION(42)
{
  for (const char **kw = reserved_keywords; *kw != NULL; ++kw)
    ensure("TEST 42.1: Testing keywords", base::is_reserved_word(*kw) == true);
  
  ensure("TEST 42.2: Testing no keyword", base::is_reserved_word("some_word") == false);
  ensure("TEST 42.3: Testing unicode keywords", base::is_reserved_word("\xC3\x89\xC3\x89\xC3") == false);
  ensure("TEST 42.4: Testing empty string", base::is_reserved_word("") == false);
  ensure("TEST 42.5: Testing similar string", base::is_reserved_word("COLUMNA") == false);
  ensure("TEST 42.6: Testing similar string", base::is_reserved_word("ACOLUMN") == false);
  ensure("TEST 42.7: Testing similar string", base::is_reserved_word("COLUM") == false);
  ensure("TEST 42.7: Testing duplicated string", base::is_reserved_word("COLUMNCOLUMN") == false);
}

TEST_FUNCTION(43)
{
  std::string font_description;
  std::string font;
  float size = 0;
  bool bold = false;
  bool italic = false;
  
  font_description = "Sans 10";
  ensure_true(base::strfmt("TEST 43.1: Parsing for \"%s\"", font_description.c_str()), base::parse_font_description(font_description, font, size, bold, italic));
  ensure_equals("", font, "Sans");
  ensure_equals("", size, 10);
  assure_false(bold);
  assure_false(italic);
  
  font_description = "Sans 12";
  ensure_true(base::strfmt("TEST 43.2: Parsing for \"%s\"", font_description.c_str()), base::parse_font_description(font_description, font, size, bold, italic));
  ensure_equals("", font, "Sans");
  ensure_equals("", size, 12);
  assure_false(bold);
  assure_false(italic);
  
  font_description = "Sans 10 bold";
  ensure_true(base::strfmt("TEST 43.3: Parsing for \"%s\"", font_description.c_str()), base::parse_font_description(font_description, font, size, bold, italic));
  ensure_equals("", font, "Sans");
  ensure_equals("", size, 10);
  assure_true(bold);
  assure_false(italic);
  
  font_description = "Sans 10 BOLD";
  ensure_true(base::strfmt("TEST 43.4: Parsing for \"%s\"", font_description.c_str()), base::parse_font_description(font_description, font, size, bold, italic));
  ensure_equals("", font, "Sans");
  ensure_equals("", size, 10);
  assure_true(bold);
  assure_false(italic);
  
  font_description = "Sans 10 italic";
  ensure_true(base::strfmt("TEST 43.5: Parsing for \"%s\"", font_description.c_str()), base::parse_font_description(font_description, font, size, bold, italic));
  ensure_equals("", font, "Sans");
  ensure_equals("", size, 10);
  assure_false(bold);
  assure_true(italic);
  
  font_description = "Sans 10 ITALIC";
  ensure_true(base::strfmt("TEST 43.6: Parsing for \"%s\"", font_description.c_str()), base::parse_font_description(font_description, font, size, bold, italic));
  ensure_equals("", font, "Sans");
  ensure_equals("", size, 10);
  assure_false(bold);
  assure_true(italic);
  
  font_description = "Sans 10 bold italic";
  ensure_true(base::strfmt("TEST 43.7: Parsing for \"%s\"", font_description.c_str()), base::parse_font_description(font_description, font, size, bold, italic));
  ensure_equals("", font, "Sans");
  ensure_equals("", size, 10);
  assure_true(bold);
  assure_true(italic);
  
  font_description = "Sans 10 BOLD ITALIC";
  ensure_true(base::strfmt("TEST 43.8: Parsing for \"%s\"", font_description.c_str()), base::parse_font_description(font_description, font, size, bold, italic));
  ensure_equals("", font, "Sans");
  ensure_equals("", size, 10);
  assure_true(bold);
  assure_true(italic);

  font_description = "My Font 10 BOLD ITALIC";
  ensure_true(base::strfmt("TEST 43.8: Parsing for \"%s\"", font_description.c_str()), base::parse_font_description(font_description, font, size, bold, italic));
  ensure_equals("", font, "My Font");
  ensure_equals("", size, 10);
  assure_true(bold);
  assure_true(italic);
  
  font_description = "Helvetica Bold 12";
  ensure_true(base::strfmt("TEST 43.8: Parsing for \"%s\"", font_description.c_str()), base::parse_font_description(font_description, font, size, bold, italic));
  ensure_equals("", font, "Helvetica");
  ensure_equals("", size, 12);
  assure_true(bold);
  assure_false(italic);
}


END_TESTS
