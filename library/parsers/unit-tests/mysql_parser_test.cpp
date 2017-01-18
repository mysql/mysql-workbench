/*
 * Copyright (c) 2012, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "wb_helpers.h"

#include "MySQLLexer.h"

#include "grtsqlparser/sql_facade.h"
#include "mysql-parser.h"

#include <boost/assign/list_of.hpp>

// This file contains unit tests for the sql facade based statement splitter and the ANTLR based parser.
// These are low level tests. There's another set of high level tests (see test_mysql_sql_parser.cpp).

#define VERBOSE_OUTPUT 0

using namespace boost::assign;

//--------------------------------------------------------------------------------------------------

BEGIN_TEST_DATA_CLASS(mysql_parser_tests)
protected:
WBTester *_tester;
std::set<std::string> _charsets;
// std::auto_ptr<MySQLRecognizer> _recognizer;

bool parse(const char *sql, size_t size, bool is_utf8, long server_version, const std::string &sql_mode);

TEST_DATA_CONSTRUCTOR(mysql_parser_tests) {
  bec::GRTManager::get(); // make GRTManagaer live longer than wbtester
  _tester = new WBTester();
  // init datatypes
  populate_grt(*_tester);

  // The charset list contains also the 3 charsets that were introduced in 5.5.3.
  grt::ListRef<db_CharacterSet> list = _tester->get_rdbms()->characterSets();
  for (size_t i = 0; i < list->count(); i++)
    _charsets.insert(base::tolower(*list[i]->name()));

  //_recognizer.reset(new MySQLRecognizer(50620, "", _charsets));
}
END_TEST_DATA_CLASS

TEST_MODULE(mysql_parser_tests, "MySQL parser test suite (ANTLR)");

//--------------------------------------------------------------------------------------------------

/**
 * Parses the given string and returns true if no error occurred, otherwise false.
 */
bool Test_object_base<mysql_parser_tests>::parse(const char *sql, size_t size, bool is_utf8, long server_version,
                                                 const std::string &sql_mode) {
  // When reusing the recognizer at least one query consumes endless memory (until system crawls to hold).
  // So stay for now with a fresh parser on each test (which makes them slower than they need to be).

  MySQLRecognizer recognizer(server_version, sql_mode, _charsets);
  //_recognizer->set_server_version(server_version);
  //_recognizer->set_sql_mode(sql_mode);
  //_recognizer->parse(sql, size, is_utf8, PuGeneric);
  recognizer.parse(sql, size, is_utf8, MySQLParseUnit::PuGeneric);
  bool result = recognizer.error_info().size() == 0;

  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 *  Statement splitter test.
 */
TEST_FUNCTION(5) {
  const char *filename = "data/db/sakila-db/sakila-data.sql";
  const char *statement_filename = "data/db/sakila-db/single_statement.sql";

  ensure("SQL file does not exist.", g_file_test(filename, G_FILE_TEST_EXISTS) == TRUE);
  ensure("Statement file does not exist.", g_file_test(statement_filename, G_FILE_TEST_EXISTS) == TRUE);

  gchar *sql = NULL;
  gsize size = 0;
  GError *error = NULL;
  g_file_get_contents(filename, &sql, &size, &error);
  ensure("Error loading sql file", error == NULL);

  SqlFacade::Ref sql_facade = SqlFacade::instance_for_rdbms_name("Mysql");

#if VERBOSE_OUTPUT
  test_time_point t1;
#endif

  std::vector<std::pair<size_t, size_t> > ranges;
  sql_facade->splitSqlScript(sql, size, ";", ranges);

#if VERBOSE_OUTPUT
  test_time_point t2;

  float time_rate = 1000.0f / (t2 - t1).get_ticks();
  float size_per_sec = size * time_rate / 1024.0f / 1024.0f;
  std::cout << "Splitter performance test (no parsing): " << std::endl
            << "sakila-data.sql was processed in " << (t2 - t1) << " [" << size_per_sec << " MB/sec]" << std::endl;
#endif

  ensure("Unexpected number of statements returned from splitter", ranges.size() == 57);

  std::string s1(sql, ranges[0].first, ranges[0].second);
  ensure("Wrong statement", s1 == "SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0");

  std::string s3(sql, ranges[56].first, ranges[56].second);
  ensure("Wrong statement", s3 == "SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS");

  std::string s2(sql, ranges[30].first, ranges[30].second);

  g_free(sql);

  sql = NULL;
  size = 0;
  error = NULL;
  g_file_get_contents(statement_filename, &sql, &size, &error);
  ensure("Error loading statement sql file", error == NULL);
  ensure("Wrong statement", s2 == sql);
}

struct TestFile {
  const char *name;
  const char *line_break;
  const char *initial_delmiter;
  bool is_utf8;
};

static const TestFile test_files[] = {
  // Large set of all possible query types in different combinations.
  {"data/db/statements.txt", "\n", "$$", true},

  // A file with a number of create tables statements that stresses the use
  // of the grammar (e.g. using weird but still valid object names including \n, long
  // list of indices, all possible data types + the default values etc.).
  // Note: it is essential to use \r\n as normal line break in the file to allow usage of \n
  //       in object names.
  {"data/db/nasty_tables.sql", "\r\n", ";", true},

  // Not so many statements, but some very long insert statements.
  {"data/db/sakila-db/sakila-data.sql", "\n", ";", false}};

/**
 * Parse a number files with various statements.
 */
TEST_FUNCTION(10) {
#if VERBOSE_OUTPUT
  test_time_point t1;
#endif

  SqlFacade::Ref sql_facade = SqlFacade::instance_for_rdbms_name("Mysql");
  size_t count = 0;
  for (size_t i = 0; i < sizeof(test_files) / sizeof(test_files[0]); ++i) {
    ensure(base::strfmt("Statements file '%s' does not exist.", test_files[i].name),
           g_file_test(test_files[i].name, G_FILE_TEST_EXISTS) == TRUE);

    gchar *sql = NULL;
    gsize size = 0;
    GError *error = NULL;
    g_file_get_contents(test_files[i].name, &sql, &size, &error);
    ensure("Error loading sql file", error == NULL);

    std::vector<std::pair<size_t, size_t> >::const_iterator iterator;
    std::vector<std::pair<size_t, size_t> > ranges;
    sql_facade->splitSqlScript(sql, size, test_files[i].initial_delmiter, ranges, test_files[i].line_break);
    count += ranges.size();

    for (iterator = ranges.begin(); iterator != ranges.end(); ++iterator) {
      if (!parse(sql + iterator->first, iterator->second, test_files[i].is_utf8, 50604, "ANSI_QUOTES")) {
        std::string query(sql + iterator->first, iterator->second);
        ensure("This query failed to parse:\n" + query, false);
      }
    }

    g_free(sql);
  }

#if VERBOSE_OUTPUT
  test_time_point t2;

  std::cout << count << " queries parsed in " << (t2 - t1).get_ticks() / 1000.0 << " s" << std::endl;
#endif
}

//--------------------------------------------------------------------------------------------------

/**
 * This test generates queries with many (all?) MySQL function names used in foreign key creation
 * (parser bug #21114). Taken from the server test suite.
 */

static const char *functions[] = {
  "acos", "adddate",
  "addtime"
  "aes_decrypt",
  "aes_encrypt", "area", "asbinary", "asin", "astext", "aswkb", "aswkt", "atan", "atan2", "benchmark", "bin",
  "bit_count", "bit_length", "ceil", "ceiling", "centroid", "character_length", "char_length", "coercibility",
  "compress", "concat", "concat_ws", "connection_id", "conv", "convert_tz", "cos", "cot", "crc32", "crosses",
  "datediff", "date_format", "dayname", "dayofmonth", "dayofweek", "dayofyear", "decode", "degrees", "des_decrypt",
  "des_encrypt", "dimension", "disjoint", "elt", "encode", "encrypt", "endpoint", "envelope", "equals", "exp",
  "export_set", "exteriorring", "extractvalue", "find_in_set", "floor", "found_rows", "from_days", "from_unixtime",
  "geomcollfromtext", "geomcollfromwkb", "geometrycollectionfromtext", "geometrycollectionfromwkb", "geometryfromtext",
  "geometryfromwkb", "geometryn", "geometrytype", "geomfromtext", "geomfromwkb", "get_lock", "glength", "greatest",
  "hex", "ifnull", "inet_aton", "inet_ntoa", "instr", "interiorringn", "intersects", "isclosed", "isempty", "isnull",
  "issimple", "is_free_lock", "is_used_lock", "last_day", "last_insert_id", "lcase", "least", "length", "linefromtext",
  "linefromwkb", "linestringfromtext", "linestringfromwkb", "ln", "load_file", "locate", "log", "log10", "log2",
  "lower", "lpad", "ltrim", "makedate", "maketime", "make_set", "master_pos_wait", "mbrcontains", "mbrdisjoint",
  "mbrequal", "mbrintersects", "mbroverlaps", "mbrtouches", "mbrwithin", "md5", "mlinefromtext", "mlinefromwkb",
  "monthname", "mpointfromtext", "mpointfromwkb", "mpolyfromtext", "mpolyfromwkb", "multilinestringfromtext",
  "multilinestringfromwkb", "multipointfromtext", "multipointfromwkb", "multipolygonfromtext", "multipolygonfromwkb",
  "name_const", "nullif", "numgeometries", "numinteriorrings", "numpoints", "oct", "octet_length", "ord", "overlaps",
  "period_add", "period_diff", "pi", "pointfromtext", "pointfromwkb", "pointn", "polyfromtext", "polyfromwkb",
  "polygonfromtext", "polygonfromwkb", "pow", "power", "quote", "radians", "rand", "release_lock", "reverse", "round",
  "row_count", "rpad", "rtrim", "sec_to_time", "session_user", "sha", "sha1", "sign", "sin", "sleep", "soundex",
  "space", "sqrt", "srid", "startpoint", "strcmp", "str_to_date", "subdate", "substring_index", "subtime",
  "system_user", "tan", "timediff", "time_format", "time_to_sec", "touches", "to_days", "ucase", "uncompress",
  "uncompressed_length", "unhex", "unix_timestamp", "updatexml", "upper", "uuid", "version", "weekday", "weekofyear",
  "within", "x", "y", "yearweek"};

const char *query1 =
  "CREATE TABLE %s(\n"
  "col1 int not null,\n"
  "col2 int not null,\n"
  "col3 varchar(10),\n"
  "CONSTRAINT pk PRIMARY KEY (col1, col2)\n"
  ") ENGINE InnoDB";

const char *query2 =
  "CREATE TABLE bug21114_child(\n"
  "pk int not null,\n"
  "fk_col1 int not null,\n"
  "fk_col2 int not null,\n"
  "fk_col3 int not null,\n"
  "fk_col4 int not null,\n"
  "CONSTRAINT fk_fct FOREIGN KEY (fk_col1, fk_col2)\n"
  "REFERENCES %s(col1, col2),\n"
  "CONSTRAINT fk_fct_space FOREIGN KEY (fk_col3, fk_col4)\n"
  "REFERENCES %s (col1, col2)\n"
  ") ENGINE InnoDB";

TEST_FUNCTION(20) {
  int count = sizeof(functions) / sizeof(functions[0]);
  for (int i = 0; i < count; i++) {
    std::string query = base::strfmt(query1, functions[i]);
    ensure("A statement failed to parse", parse(query.c_str(), query.size(), true, 50530, "ANSI_QUOTES"));

    query = base::strfmt(query2, functions[i], functions[i]);
    ensure("A statement failed to parse", parse(query.c_str(), query.size(), true, 50530, "ANSI_QUOTES"));
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Parses the given string and checks the built AST. Returns true if no error occurred, otherwise false.
 */
bool parse_and_compare(const std::string &sql, long server_version, const std::string &sql_mode,
                       const std::set<std::string> &charsets, std::vector<ANTLR3_UINT32> tokens,
                       size_t error_count = 0) {
  MySQLRecognizer recognizer(server_version, sql_mode, charsets);
  recognizer.parse(sql.c_str(), sql.size(), true, MySQLParseUnit::PuGeneric);
  if (recognizer.error_info().size() != error_count)
    return false;

  // Walk the list of AST nodes recursively and match exactly the given list of tokens.
  MySQLRecognizerTreeWalker walker = recognizer.tree_walker();
  size_t i = 0;
  do {
    if (i >= tokens.size())
      return false;

    if (walker.tokenType() != tokens[i++])
      return false;
  } while (walker.next());

  return i == tokens.size();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Operator precedence tests. These were taken from the server parser test suite.
 */

static std::vector<std::string> precedenceTestQueries =
  list_of("select A, B, A OR B, A XOR B, A AND B from t1_30237_bool where C is null order by A, B")(
    "select A, B, C, (A OR B) OR C, A OR (B OR C), A OR B OR C from t1_30237_bool order by A, B, C")(
    "select count(*) from t1_30237_bool where ((A OR B) OR C) != (A OR (B OR C))")(
    "select A, B, C, (A XOR B) XOR C, A XOR (B XOR C), A XOR B XOR C from t1_30237_bool order by A, B, C")(
    "select count(*) from t1_30237_bool where ((A XOR B) XOR C) != (A XOR (B XOR C))")(
    "select A, B, C, (A AND B) AND C, A AND (B AND C), A AND B AND C from t1_30237_bool order by A, B, C")(
    "select count(*) from t1_30237_bool where ((A AND B) AND C) != (A AND (B AND C))")(
    "select A, B, C, (A OR B) AND C, A OR (B AND C), A OR B AND C from t1_30237_bool order by A, B, C")(
    "select count(*) from t1_30237_bool where (A OR (B AND C)) != (A OR B AND C)")(
    "select A, B, C, (A AND B) OR C, A AND (B OR C), A AND B OR C from t1_30237_bool order by A, B, C")(
    "select count(*) from t1_30237_bool where ((A AND B) OR C) != (A AND B OR C)")(
    "select A, B, C, (A XOR B) AND C, A XOR (B AND C), A XOR B AND C from t1_30237_bool order by A, B, C")(
    "select count(*) from t1_30237_bool where (A XOR (B AND C)) != (A XOR B AND C)")(
    "select A, B, C, (A AND B) XOR C, A AND (B XOR C), A AND B XOR C from t1_30237_bool order by A, B, C")(
    "select count(*) from t1_30237_bool where ((A AND B) XOR C) != (A AND B XOR C)")(
    "select A, B, C, (A XOR B) OR C, A XOR (B OR C), A XOR B OR C from t1_30237_bool order by A, B, C")(
    "select count(*) from t1_30237_bool where ((A XOR B) OR C) != (A XOR B OR C)")(
    "select A, B, C, (A OR B) XOR C, A OR (B XOR C), A OR B XOR C from t1_30237_bool order by A, B, C")(
    "select count(*) from t1_30237_bool where (A OR (B XOR C)) != (A OR B XOR C)")(
    "select (NOT FALSE) OR TRUE, NOT (FALSE OR TRUE), NOT FALSE OR TRUE")(
    "select (NOT FALSE) XOR FALSE, NOT (FALSE XOR FALSE), NOT FALSE XOR FALSE")(
    "select (NOT FALSE) AND FALSE, NOT (FALSE AND FALSE), NOT FALSE AND FALSE")(
    "select NOT NOT TRUE, NOT NOT NOT FALSE")("select (NOT NULL) IS TRUE, NOT (NULL IS TRUE), NOT NULL IS TRUE")(
    "select (NOT NULL) IS NOT TRUE, NOT (NULL IS NOT TRUE), NOT NULL IS NOT TRUE")(
    "select (NOT NULL) IS FALSE, NOT (NULL IS FALSE), NOT NULL IS FALSE")(
    "select (NOT NULL) IS NOT FALSE, NOT (NULL IS NOT FALSE), NOT NULL IS NOT FALSE")(
    "select (NOT TRUE) IS UNKNOWN, NOT (TRUE IS UNKNOWN), NOT TRUE IS UNKNOWN")(
    "select (NOT TRUE) IS NOT UNKNOWN, NOT (TRUE IS NOT UNKNOWN), NOT TRUE IS NOT UNKNOWN")(
    "select (NOT TRUE) IS NULL, NOT (TRUE IS NULL), NOT TRUE IS NULL")(
    "select (NOT TRUE) IS NOT NULL, NOT (TRUE IS NOT NULL), NOT TRUE IS NOT NULL")(
    "select FALSE IS NULL IS NULL IS NULL")("select TRUE IS NOT NULL IS NOT NULL IS NOT NULL")(
    "select 1 <=> 2 <=> 2, (1 <=> 2) <=> 2, 1 <=> (2 <=> 2)")("select 1 = 2 = 2, (1 = 2) = 2, 1 = (2 = 2)")(
    "select 1 != 2 != 3, (1 != 2) != 3, 1 != (2 != 3)")("select 1 <> 2 <> 3, (1 <> 2) <> 3, 1 <> (2 <> 3)")(
    "select 1 < 2 < 3, (1 < 2) < 3, 1 < (2 < 3)")("select 3 <= 2 <= 1, (3 <= 2) <= 1, 3 <= (2 <= 1)")(
    "select 1 > 2 > 3, (1 > 2) > 3, 1 > (2 > 3)")("select 1 >= 2 >= 3, (1 >= 2) >= 3, 1 >= (2 >= 3)")(
    "select 0xF0 | 0x0F | 0x55, (0xF0 | 0x0F) | 0x55, 0xF0 | (0x0F | 0x55)")(
    "select 0xF5 & 0x5F & 0x55, (0xF5 & 0x5F) & 0x55, 0xF5 & (0x5F & 0x55)")(
    "select 4 << 3 << 2, (4 << 3) << 2, 4 << (3 << 2)")("select 256 >> 3 >> 2, (256 >> 3) >> 2, 256 >> (3 >> 2)")(
    "select 0xF0 & 0x0F | 0x55, (0xF0 & 0x0F) | 0x55, 0xF0 & (0x0F | 0x55)")(
    "select 0x55 | 0xF0 & 0x0F, (0x55 | 0xF0) & 0x0F, 0x55 | (0xF0 & 0x0F)")(
    "select 0x0F << 4 | 0x0F, (0x0F << 4) | 0x0F, 0x0F << (4 | 0x0F)")(
    "select 0x0F | 0x0F << 4, (0x0F | 0x0F) << 4, 0x0F | (0x0F << 4)")(
    "select 0xF0 >> 4 | 0xFF, (0xF0 >> 4) | 0xFF, 0xF0 >> (4 | 0xFF)")(
    "select 0xFF | 0xF0 >> 4, (0xFF | 0xF0) >> 4, 0xFF | (0xF0 >> 4)")(
    "select 0x0F << 4 & 0xF0, (0x0F << 4) & 0xF0, 0x0F << (4 & 0xF0)")(
    "select 0xF0 & 0x0F << 4, (0xF0 & 0x0F) << 4, 0xF0 & (0x0F << 4)")(
    "select 0xF0 >> 4 & 0x55, (0xF0 >> 4) & 0x55, 0xF0 >> (4 & 0x55)")(
    "select 0x0F & 0xF0 >> 4, (0x0F & 0xF0) >> 4, 0x0F & (0xF0 >> 4)")(
    "select 0xFF >> 4 << 2, (0xFF >> 4) << 2, 0xFF >> (4 << 2)")(
    "select 0x0F << 4 >> 2, (0x0F << 4) >> 2, 0x0F << (4 >> 2)")("select 1 + 2 + 3, (1 + 2) + 3, 1 + (2 + 3)")(
    "select 1 - 2 - 3, (1 - 2) - 3, 1 - (2 - 3)")("select 1 + 2 - 3, (1 + 2) - 3, 1 + (2 - 3)")(
    "select 1 - 2 + 3, (1 - 2) + 3, 1 - (2 + 3)")(
    "select 0xF0 + 0x0F | 0x55, (0xF0 + 0x0F) | 0x55, 0xF0 + (0x0F | 0x55)")(
    "select 0x55 | 0xF0 + 0x0F, (0x55 | 0xF0) + 0x0F, 0x55 | (0xF0 + 0x0F)")(
    "select 0xF0 + 0x0F & 0x55, (0xF0 + 0x0F) & 0x55, 0xF0 + (0x0F & 0x55)")(
    "select 0x55 & 0xF0 + 0x0F, (0x55 & 0xF0) + 0x0F, 0x55 & (0xF0 + 0x0F)")(
    "select 2 + 3 << 4, (2 + 3) << 4, 2 + (3 << 4)")("select 3 << 4 + 2, (3 << 4) + 2, 3 << (4 + 2)")(
    "select 4 + 3 >> 2, (4 + 3) >> 2, 4 + (3 >> 2)")("select 3 >> 2 + 1, (3 >> 2) + 1, 3 >> (2 + 1)")(
    "select 0xFF - 0x0F | 0x55, (0xFF - 0x0F) | 0x55, 0xFF - (0x0F | 0x55)")(
    "select 0x55 | 0xFF - 0xF0, (0x55 | 0xFF) - 0xF0, 0x55 | (0xFF - 0xF0)")(
    "select 0xFF - 0xF0 & 0x55, (0xFF - 0xF0) & 0x55, 0xFF - (0xF0 & 0x55)")(
    "select 0x55 & 0xFF - 0xF0, (0x55 & 0xFF) - 0xF0, 0x55 & (0xFF - 0xF0)")(
    "select 16 - 3 << 2, (16 - 3) << 2, 16 - (3 << 2)")("select 4 << 3 - 2, (4 << 3) - 2, 4 << (3 - 2)")(
    "select 16 - 3 >> 2, (16 - 3) >> 2, 16 - (3 >> 2)")("select 16 >> 3 - 2, (16 >> 3) - 2, 16 >> (3 - 2)")(
    "select 2 * 3 * 4, (2 * 3) * 4, 2 * (3 * 4)")("select 2 * 0x40 | 0x0F, (2 * 0x40) | 0x0F, 2 * (0x40 | 0x0F)")(
    "select 0x0F | 2 * 0x40, (0x0F | 2) * 0x40, 0x0F | (2 * 0x40)")(
    "select 2 * 0x40 & 0x55, (2 * 0x40) & 0x55, 2 * (0x40 & 0x55)")(
    "select 0xF0 & 2 * 0x40, (0xF0 & 2) * 0x40, 0xF0 & (2 * 0x40)")("select 5 * 3 << 4, (5 * 3) << 4, 5 * (3 << 4)")(
    "select 2 << 3 * 4, (2 << 3) * 4, 2 << (3 * 4)")("select 3 * 4 >> 2, (3 * 4) >> 2, 3 * (4 >> 2)")(
    "select 4 >> 2 * 3, (4 >> 2) * 3, 4 >> (2 * 3)")("select 2 * 3 + 4, (2 * 3) + 4, 2 * (3 + 4)")(
    "select 2 + 3 * 4, (2 + 3) * 4, 2 + (3 * 4)")("select 4 * 3 - 2, (4 * 3) - 2, 4 * (3 - 2)")(
    "select 4 - 3 * 2, (4 - 3) * 2, 4 - (3 * 2)")("select 15 / 5 / 3, (15 / 5) / 3, 15 / (5 / 3)")(
    "select 105 / 5 | 2, (105 / 5) | 2, 105 / (5 | 2)")("select 105 | 2 / 5, (105 | 2) / 5, 105 | (2 / 5)")(
    "select 105 / 5 & 0x0F, (105 / 5) & 0x0F, 105 / (5 & 0x0F)")(
    "select 0x0F & 105 / 5, (0x0F & 105) / 5, 0x0F & (105 / 5)")(
    "select 0x80 / 4 << 2, (0x80 / 4) << 2, 0x80 / (4 << 2)")("select 0x80 << 4 / 2, (0x80 << 4) / 2, 0x80 << (4 / 2)")(
    "select 0x80 / 4 >> 2, (0x80 / 4) >> 2, 0x80 / (4 >> 2)")("select 0x80 >> 4 / 2, (0x80 >> 4) / 2, 0x80 >> (4 / 2)")(
    "select 0x80 / 2 + 2, (0x80 / 2) + 2, 0x80 / (2 + 2)")("select 0x80 + 2 / 2, (0x80 + 2) / 2, 0x80 + (2 / 2)")(
    "select 0x80 / 4 - 2, (0x80 / 4) - 2, 0x80 / (4 - 2)")("select 0x80 - 4 / 2, (0x80 - 4) / 2, 0x80 - (4 / 2)")(
    "select 0xFF ^ 0xF0 ^ 0x0F, (0xFF ^ 0xF0) ^ 0x0F, 0xFF ^ (0xF0 ^ 0x0F)")(
    "select 0xFF ^ 0xF0 ^ 0x55, (0xFF ^ 0xF0) ^ 0x55, 0xFF ^ (0xF0 ^ 0x55)")(
    "select 0xFF ^ 0xF0 | 0x0F, (0xFF ^ 0xF0) | 0x0F, 0xFF ^ (0xF0 | 0x0F)")(
    "select 0xF0 | 0xFF ^ 0xF0, (0xF0 | 0xFF) ^ 0xF0, 0xF0 | (0xFF ^ 0xF0)")(
    "select 0xFF ^ 0xF0 & 0x0F, (0xFF ^ 0xF0) & 0x0F, 0xFF ^ (0xF0 & 0x0F)")(
    "select 0x0F & 0xFF ^ 0xF0, (0x0F & 0xFF) ^ 0xF0, 0x0F & (0xFF ^ 0xF0)")(
    "select 0xFF ^ 0xF0 << 2, (0xFF ^ 0xF0) << 2, 0xFF ^ (0xF0 << 2)")(
    "select 0x0F << 2 ^ 0xFF, (0x0F << 2) ^ 0xFF, 0x0F << (2 ^ 0xFF)")(
    "select 0xFF ^ 0xF0 >> 2, (0xFF ^ 0xF0) >> 2, 0xFF ^ (0xF0 >> 2)")(
    "select 0xFF >> 2 ^ 0xF0, (0xFF >> 2) ^ 0xF0, 0xFF >> (2 ^ 0xF0)")(
    "select 0xFF ^ 0xF0 + 0x0F, (0xFF ^ 0xF0) + 0x0F, 0xFF ^ (0xF0 + 0x0F)")(
    "select 0x0F + 0xFF ^ 0xF0, (0x0F + 0xFF) ^ 0xF0, 0x0F + (0xFF ^ 0xF0)")(
    "select 0xFF ^ 0xF0 - 1, (0xFF ^ 0xF0) - 1, 0xFF ^ (0xF0 - 1)")(
    "select 0x55 - 0x0F ^ 0x55, (0x55 - 0x0F) ^ 0x55, 0x55 - (0x0F ^ 0x55)")(
    "select 0xFF ^ 0xF0 * 2, (0xFF ^ 0xF0) * 2, 0xFF ^ (0xF0 * 2)")(
    "select 2 * 0xFF ^ 0xF0, (2 * 0xFF) ^ 0xF0, 2 * (0xFF ^ 0xF0)")(
    "select 0xFF ^ 0xF0 / 2, (0xFF ^ 0xF0) / 2, 0xFF ^ (0xF0 / 2)")(
    "select 0xF2 / 2 ^ 0xF0, (0xF2 / 2) ^ 0xF0, 0xF2 / (2 ^ 0xF0)")(
    "select 0xFF ^ 0xF0 % 0x20, (0xFF ^ 0xF0) % 0x20, 0xFF ^ (0xF0 % 0x20)")(
    "select 0xFF % 0x20 ^ 0xF0, (0xFF % 0x20) ^ 0xF0, 0xFF % (0x20 ^ 0xF0)")(
    "select 0xFF ^ 0xF0 DIV 2, (0xFF ^ 0xF0) DIV 2, 0xFF ^ (0xF0 DIV 2)")(
    "select 0xF2 DIV 2 ^ 0xF0, (0xF2 DIV 2) ^ 0xF0, 0xF2 DIV (2 ^ 0xF0)")(
    "select 0xFF ^ 0xF0 MOD 0x20, (0xFF ^ 0xF0) MOD 0x20, 0xFF ^ (0xF0 MOD 0x20)")(
    "select 0xFF MOD 0x20 ^ 0xF0, (0xFF MOD 0x20) ^ 0xF0, 0xFF MOD (0x20 ^ 0xF0)");

typedef std::vector<ANTLR3_UINT32> TokenVector;

const std::vector<TokenVector> precedenceTestResults = list_of(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(
    SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(
    SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(
    IDENTIFIER)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(WHERE_SYMBOL)(EXPRESSION_TOKEN)(IS_SYMBOL)(COLUMN_REF_TOKEN)(
    IDENTIFIER)(NULL_SYMBOL)(ORDER_SYMBOL)(BY_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(
    EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(
    SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(OR_SYMBOL)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(
    IDENTIFIER)(CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(
    OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(OR_SYMBOL)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(ORDER_SYMBOL)(BY_SYMBOL)(EXPRESSION_TOKEN)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(
    EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(RUNTIME_FUNCTION_TOKEN)(COUNT_SYMBOL)(OPEN_PAR_SYMBOL)(
    MULT_OPERATOR)(CLOSE_PAR_SYMBOL)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(WHERE_SYMBOL)(EXPRESSION_TOKEN)(
    NOT_EQUAL_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(
    OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(CLOSE_PAR_SYMBOL)(
    ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(
    SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(XOR_SYMBOL)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(XOR_SYMBOL)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(ORDER_SYMBOL)(
    BY_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(
    IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(RUNTIME_FUNCTION_TOKEN)(COUNT_SYMBOL)(OPEN_PAR_SYMBOL)(
    MULT_OPERATOR)(CLOSE_PAR_SYMBOL)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(WHERE_SYMBOL)(EXPRESSION_TOKEN)(
    NOT_EQUAL_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(
    SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(AND_SYMBOL)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(AND_SYMBOL)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(ORDER_SYMBOL)(
    BY_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(
    IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(RUNTIME_FUNCTION_TOKEN)(COUNT_SYMBOL)(OPEN_PAR_SYMBOL)(
    MULT_OPERATOR)(CLOSE_PAR_SYMBOL)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(WHERE_SYMBOL)(EXPRESSION_TOKEN)(
    NOT_EQUAL_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(AND_SYMBOL)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(
    SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(AND_SYMBOL)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(
    IDENTIFIER)(CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(
    OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(AND_SYMBOL)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(ORDER_SYMBOL)(BY_SYMBOL)(EXPRESSION_TOKEN)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(
    EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(RUNTIME_FUNCTION_TOKEN)(COUNT_SYMBOL)(OPEN_PAR_SYMBOL)(
    MULT_OPERATOR)(CLOSE_PAR_SYMBOL)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(WHERE_SYMBOL)(EXPRESSION_TOKEN)(
    NOT_EQUAL_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(
    IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(CLOSE_PAR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(
    SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(OR_SYMBOL)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(OR_SYMBOL)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(ORDER_SYMBOL)(
    BY_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(
    IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(RUNTIME_FUNCTION_TOKEN)(COUNT_SYMBOL)(OPEN_PAR_SYMBOL)(
    MULT_OPERATOR)(CLOSE_PAR_SYMBOL)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(WHERE_SYMBOL)(EXPRESSION_TOKEN)(
    NOT_EQUAL_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(OR_SYMBOL)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(
    SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(AND_SYMBOL)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(AND_SYMBOL)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(ORDER_SYMBOL)(
    BY_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(
    IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(RUNTIME_FUNCTION_TOKEN)(COUNT_SYMBOL)(OPEN_PAR_SYMBOL)(
    MULT_OPERATOR)(CLOSE_PAR_SYMBOL)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(WHERE_SYMBOL)(EXPRESSION_TOKEN)(
    NOT_EQUAL_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(
    IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(CLOSE_PAR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(
    SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(XOR_SYMBOL)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(XOR_SYMBOL)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(ORDER_SYMBOL)(
    BY_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(
    IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(RUNTIME_FUNCTION_TOKEN)(COUNT_SYMBOL)(OPEN_PAR_SYMBOL)(
    MULT_OPERATOR)(CLOSE_PAR_SYMBOL)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(WHERE_SYMBOL)(EXPRESSION_TOKEN)(
    NOT_EQUAL_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(XOR_SYMBOL)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(
    SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(OR_SYMBOL)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(OR_SYMBOL)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(ORDER_SYMBOL)(
    BY_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(
    IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(RUNTIME_FUNCTION_TOKEN)(COUNT_SYMBOL)(OPEN_PAR_SYMBOL)(
    MULT_OPERATOR)(CLOSE_PAR_SYMBOL)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(WHERE_SYMBOL)(EXPRESSION_TOKEN)(
    NOT_EQUAL_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(OR_SYMBOL)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(
    SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(XOR_SYMBOL)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(
    IDENTIFIER)(CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(
    OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(ORDER_SYMBOL)(BY_SYMBOL)(EXPRESSION_TOKEN)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(
    EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(RUNTIME_FUNCTION_TOKEN)(COUNT_SYMBOL)(OPEN_PAR_SYMBOL)(
    MULT_OPERATOR)(CLOSE_PAR_SYMBOL)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(WHERE_SYMBOL)(EXPRESSION_TOKEN)(
    NOT_EQUAL_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(
    IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(CLOSE_PAR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(
    COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(OR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(NOT_SYMBOL)(FALSE_SYMBOL)(CLOSE_PAR_SYMBOL)(TRUE_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(NOT_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(FALSE_SYMBOL)(
    TRUE_SYMBOL)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(OR_SYMBOL)(NOT_SYMBOL)(
    FALSE_SYMBOL)(TRUE_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(XOR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(NOT_SYMBOL)(FALSE_SYMBOL)(CLOSE_PAR_SYMBOL)(FALSE_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(NOT_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(FALSE_SYMBOL)(
    FALSE_SYMBOL)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(XOR_SYMBOL)(NOT_SYMBOL)(
    FALSE_SYMBOL)(FALSE_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(AND_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(NOT_SYMBOL)(FALSE_SYMBOL)(CLOSE_PAR_SYMBOL)(FALSE_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(NOT_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(AND_SYMBOL)(FALSE_SYMBOL)(
    FALSE_SYMBOL)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(AND_SYMBOL)(NOT_SYMBOL)(
    FALSE_SYMBOL)(FALSE_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(NOT_SYMBOL)(TRUE_SYMBOL)(COMMA_SYMBOL)(
    SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(NOT_SYMBOL)(NOT_SYMBOL)(FALSE_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(IS_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(NOT_SYMBOL)(NULL_SYMBOL)(CLOSE_PAR_SYMBOL)(TRUE_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(NOT_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(IS_SYMBOL)(NULL_SYMBOL)(
    TRUE_SYMBOL)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(IS_SYMBOL)(
    NULL_SYMBOL)(TRUE_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(IS_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(NOT_SYMBOL)(NULL_SYMBOL)(CLOSE_PAR_SYMBOL)(NOT_SYMBOL)(TRUE_SYMBOL)(COMMA_SYMBOL)(
    SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(
    IS_SYMBOL)(NULL_SYMBOL)(NOT_SYMBOL)(TRUE_SYMBOL)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(NOT_SYMBOL)(IS_SYMBOL)(NULL_SYMBOL)(NOT_SYMBOL)(TRUE_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(IS_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(NOT_SYMBOL)(NULL_SYMBOL)(CLOSE_PAR_SYMBOL)(FALSE_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(NOT_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(IS_SYMBOL)(NULL_SYMBOL)(
    FALSE_SYMBOL)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(IS_SYMBOL)(
    NULL_SYMBOL)(FALSE_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(IS_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(NOT_SYMBOL)(NULL_SYMBOL)(CLOSE_PAR_SYMBOL)(NOT_SYMBOL)(FALSE_SYMBOL)(COMMA_SYMBOL)(
    SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(
    IS_SYMBOL)(NULL_SYMBOL)(NOT_SYMBOL)(FALSE_SYMBOL)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(NOT_SYMBOL)(IS_SYMBOL)(NULL_SYMBOL)(NOT_SYMBOL)(FALSE_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(IS_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(NOT_SYMBOL)(TRUE_SYMBOL)(CLOSE_PAR_SYMBOL)(UNKNOWN_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(NOT_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(IS_SYMBOL)(TRUE_SYMBOL)(
    UNKNOWN_SYMBOL)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(IS_SYMBOL)(
    TRUE_SYMBOL)(UNKNOWN_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(IS_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(NOT_SYMBOL)(TRUE_SYMBOL)(CLOSE_PAR_SYMBOL)(NOT_SYMBOL)(UNKNOWN_SYMBOL)(COMMA_SYMBOL)(
    SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(
    IS_SYMBOL)(TRUE_SYMBOL)(NOT_SYMBOL)(UNKNOWN_SYMBOL)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(NOT_SYMBOL)(IS_SYMBOL)(TRUE_SYMBOL)(NOT_SYMBOL)(UNKNOWN_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(IS_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(NOT_SYMBOL)(TRUE_SYMBOL)(CLOSE_PAR_SYMBOL)(NULL_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(NOT_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(IS_SYMBOL)(TRUE_SYMBOL)(
    NULL_SYMBOL)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(IS_SYMBOL)(
    TRUE_SYMBOL)(NULL_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(IS_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(NOT_SYMBOL)(TRUE_SYMBOL)(CLOSE_PAR_SYMBOL)(NOT_SYMBOL)(NULL_SYMBOL)(COMMA_SYMBOL)(
    SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(
    IS_SYMBOL)(TRUE_SYMBOL)(NOT_SYMBOL)(NULL_SYMBOL)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(
    EXPRESSION_TOKEN)(NOT_SYMBOL)(IS_SYMBOL)(TRUE_SYMBOL)(NOT_SYMBOL)(NULL_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(IS_SYMBOL)(IS_SYMBOL)(IS_SYMBOL)(FALSE_SYMBOL)(
    NULL_SYMBOL)(NULL_SYMBOL)(NULL_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(IS_SYMBOL)(IS_SYMBOL)(IS_SYMBOL)(TRUE_SYMBOL)(NOT_SYMBOL)(
    NULL_SYMBOL)(NOT_SYMBOL)(NULL_SYMBOL)(NOT_SYMBOL)(NULL_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NULL_SAFE_EQUAL_OPERATOR)(NULL_SAFE_EQUAL_OPERATOR)(
    INT_NUMBER)(INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NULL_SAFE_EQUAL_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(NULL_SAFE_EQUAL_OPERATOR)(INT_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NULL_SAFE_EQUAL_OPERATOR)(
    INT_NUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(NULL_SAFE_EQUAL_OPERATOR)(INT_NUMBER)(
    INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(EQUAL_OPERATOR)(EQUAL_OPERATOR)(INT_NUMBER)(INT_NUMBER)(
    INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(EQUAL_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(EQUAL_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(EQUAL_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(EQUAL_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_EQUAL_OPERATOR)(NOT_EQUAL_OPERATOR)(INT_NUMBER)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_EQUAL_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(NOT_EQUAL_OPERATOR)(INT_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_EQUAL_OPERATOR)(INT_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(NOT_EQUAL_OPERATOR)(INT_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_EQUAL2_OPERATOR)(NOT_EQUAL2_OPERATOR)(INT_NUMBER)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_EQUAL2_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(NOT_EQUAL2_OPERATOR)(INT_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_EQUAL2_OPERATOR)(INT_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(NOT_EQUAL2_OPERATOR)(INT_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(LESS_THAN_OPERATOR)(LESS_THAN_OPERATOR)(INT_NUMBER)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(LESS_THAN_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(LESS_THAN_OPERATOR)(INT_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(LESS_THAN_OPERATOR)(INT_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(LESS_THAN_OPERATOR)(INT_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(LESS_OR_EQUAL_OPERATOR)(LESS_OR_EQUAL_OPERATOR)(
    INT_NUMBER)(INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(LESS_OR_EQUAL_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(LESS_OR_EQUAL_OPERATOR)(INT_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(LESS_OR_EQUAL_OPERATOR)(
    INT_NUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(LESS_OR_EQUAL_OPERATOR)(INT_NUMBER)(
    INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(GREATER_THAN_OPERATOR)(GREATER_THAN_OPERATOR)(INT_NUMBER)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(GREATER_THAN_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(GREATER_THAN_OPERATOR)(INT_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(GREATER_THAN_OPERATOR)(INT_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(GREATER_THAN_OPERATOR)(INT_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(GREATER_OR_EQUAL_OPERATOR)(GREATER_OR_EQUAL_OPERATOR)(
    INT_NUMBER)(INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(GREATER_OR_EQUAL_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(GREATER_OR_EQUAL_OPERATOR)(INT_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(GREATER_OR_EQUAL_OPERATOR)(
    INT_NUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(GREATER_OR_EQUAL_OPERATOR)(INT_NUMBER)(
    INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(
    HEX_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(
    HEX_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(SHIFT_LEFT_OPERATOR)(INT_NUMBER)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INT_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(SHIFT_RIGHT_OPERATOR)(INT_NUMBER)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INT_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(
    HEX_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(BITWISE_AND_OPERATOR)(
    HEX_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(SHIFT_LEFT_OPERATOR)(HEX_NUMBER)(
    INT_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEX_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(INT_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(SHIFT_LEFT_OPERATOR)(
    HEX_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(SHIFT_RIGHT_OPERATOR)(HEX_NUMBER)(
    INT_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEX_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(INT_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(SHIFT_RIGHT_OPERATOR)(
    HEX_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(SHIFT_LEFT_OPERATOR)(HEX_NUMBER)(
    INT_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEX_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(INT_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(SHIFT_LEFT_OPERATOR)(
    HEX_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(SHIFT_RIGHT_OPERATOR)(HEX_NUMBER)(
    INT_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEX_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(INT_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(SHIFT_RIGHT_OPERATOR)(
    HEX_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(SHIFT_RIGHT_OPERATOR)(HEX_NUMBER)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEX_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(SHIFT_LEFT_OPERATOR)(HEX_NUMBER)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEX_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(PLUS_OPERATOR)(INT_NUMBER)(INT_NUMBER)(
    INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(MINUS_OPERATOR)(INT_NUMBER)(INT_NUMBER)(
    INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(PLUS_OPERATOR)(INT_NUMBER)(INT_NUMBER)(
    INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(MINUS_OPERATOR)(INT_NUMBER)(INT_NUMBER)(
    INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(PLUS_OPERATOR)(HEX_NUMBER)(
    HEX_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(
    HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(PLUS_OPERATOR)(
    HEX_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(HEX_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(PLUS_OPERATOR)(HEX_NUMBER)(
    HEX_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(
    HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(PLUS_OPERATOR)(
    HEX_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(HEX_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(PLUS_OPERATOR)(INT_NUMBER)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(
    INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INT_NUMBER)(PLUS_OPERATOR)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(PLUS_OPERATOR)(INT_NUMBER)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(
    INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INT_NUMBER)(PLUS_OPERATOR)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(MINUS_OPERATOR)(HEX_NUMBER)(
    HEX_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(
    HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(MINUS_OPERATOR)(
    HEX_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(HEX_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(MINUS_OPERATOR)(HEX_NUMBER)(
    HEX_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(
    HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(MINUS_OPERATOR)(
    HEX_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(HEX_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(MINUS_OPERATOR)(INT_NUMBER)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(
    INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INT_NUMBER)(MINUS_OPERATOR)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(MINUS_OPERATOR)(INT_NUMBER)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(
    INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INT_NUMBER)(MINUS_OPERATOR)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(MULT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(
    INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(MULT_OPERATOR)(INT_NUMBER)(
    HEX_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INT_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(
    HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(MULT_OPERATOR)(
    INT_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(HEX_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INT_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(MULT_OPERATOR)(INT_NUMBER)(
    HEX_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INT_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(
    HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(MULT_OPERATOR)(
    INT_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(HEX_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INT_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(MULT_OPERATOR)(INT_NUMBER)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(
    INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INT_NUMBER)(MULT_OPERATOR)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(MULT_OPERATOR)(INT_NUMBER)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(
    INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INT_NUMBER)(MULT_OPERATOR)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(MULT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(
    INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INT_NUMBER)(MULT_OPERATOR)(INT_NUMBER)(
    INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(MULT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(
    INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INT_NUMBER)(MULT_OPERATOR)(INT_NUMBER)(
    INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(DIV_OPERATOR)(INT_NUMBER)(INT_NUMBER)(
    INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(DIV_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(COMMA_SYMBOL)(
    SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(DIV_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(DIV_OPERATOR)(INT_NUMBER)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(
    INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(INT_NUMBER)(DIV_OPERATOR)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(DIV_OPERATOR)(INT_NUMBER)(
    INT_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(
    HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(INT_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(DIV_OPERATOR)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(DIV_OPERATOR)(HEX_NUMBER)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(
    INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEX_NUMBER)(DIV_OPERATOR)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(DIV_OPERATOR)(HEX_NUMBER)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(
    INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEX_NUMBER)(DIV_OPERATOR)(
    INT_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(DIV_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(
    INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(PLUS_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(HEX_NUMBER)(DIV_OPERATOR)(INT_NUMBER)(
    INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(PLUS_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(COMMA_SYMBOL)(
    SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(DIV_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(DIV_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(
    INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(MINUS_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(HEX_NUMBER)(DIV_OPERATOR)(INT_NUMBER)(
    INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(MINUS_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(COMMA_SYMBOL)(
    SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(DIV_OPERATOR)(INT_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(
    HEX_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(
    HEX_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(
    HEX_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(BITWISE_XOR_OPERATOR)(
    HEX_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEX_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(
    HEX_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(BITWISE_XOR_OPERATOR)(
    HEX_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEX_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(
    HEX_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEX_NUMBER)(BITWISE_XOR_OPERATOR)(
    INT_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEX_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(INT_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(
    HEX_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEX_NUMBER)(BITWISE_XOR_OPERATOR)(
    INT_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(
    CLOSE_PAR_SYMBOL)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEX_NUMBER)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(INT_NUMBER)(HEX_NUMBER)(
    CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(
    HEX_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(HEX_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(HEX_NUMBER)(BITWISE_XOR_OPERATOR)(
    HEX_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(
    HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(
    HEX_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(HEX_NUMBER)(BITWISE_XOR_OPERATOR)(
    HEX_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(
    HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(
    HEX_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INT_NUMBER)(BITWISE_XOR_OPERATOR)(
    HEX_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INT_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(
    HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INT_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(
    HEX_NUMBER)(INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(HEX_NUMBER)(BITWISE_XOR_OPERATOR)(
    INT_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(HEX_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(
    HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(INT_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MOD_OPERATOR)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(
    HEX_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MOD_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(HEX_NUMBER)(
    COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MOD_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MOD_OPERATOR)(HEX_NUMBER)(BITWISE_XOR_OPERATOR)(
    HEX_NUMBER)(HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(
    PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MOD_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(
    HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MOD_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_SYMBOL)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    INT_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(INT_NUMBER)(COMMA_SYMBOL)(
    SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(DIV_SYMBOL)(HEX_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_SYMBOL)(HEX_NUMBER)(BITWISE_XOR_OPERATOR)(INT_NUMBER)(
    HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_SYMBOL)(HEX_NUMBER)(INT_NUMBER)(CLOSE_PAR_SYMBOL)(HEX_NUMBER)(COMMA_SYMBOL)(
    SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_SYMBOL)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(INT_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MOD_SYMBOL)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(
    HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MOD_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(HEX_NUMBER)(COMMA_SYMBOL)(
    SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(MOD_SYMBOL)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>())(
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MOD_SYMBOL)(HEX_NUMBER)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(
    HEX_NUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(PAR_EXPRESSION_TOKEN)(
    OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MOD_SYMBOL)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(HEX_NUMBER)(COMMA_SYMBOL)(
    SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MOD_SYMBOL)(HEX_NUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(
    EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEX_NUMBER)(HEX_NUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF)
    .convert_to_container<TokenVector>());

TEST_FUNCTION(25) {
  ensure_equals("Data and result list must be equal in size", precedenceTestQueries.size(),
                precedenceTestResults.size());

  for (size_t i = 0; i < precedenceTestQueries.size(); ++i) {
    if (!parse_and_compare(precedenceTestQueries[i], 50530, "ANSI_QUOTES", _charsets, precedenceTestResults[i]))
      fail("Operator precedence test - query failed: " + precedenceTestQueries[i]);
  }
}

/**
 * Tests for all relevant SQL modes (ANSI, DB2, MAXDB, MSSQL, ORACLE, POSTGRESQL, MYSQL323, MYSQL40
 * ANSI_QUOTES, PIPES_AS_CONCAT, NO_BACKSLASH_ESCAPES, IGNORE_SPACE, HIGH_NOT_PRECEDENCE and combinations of them).
 */

struct SqlModeTestEntry {
  std::string query;
  std::string sqlMode;
  size_t errors;
};

static const std::vector<SqlModeTestEntry> sqlModeTestQueries = {
  // IGNORE_SPACE
  {"create table count (id int)", "", 0},
  {"create table count(id int)", "", 1},
  {"create table count (id int)", "IGNORE_SPACE", 1},
  {"create table count(id int)", "IGNORE_SPACE", 1},
  {"create table xxx (id int)", "", 0},
  {"create table xxx(id int)", "", 0},
  {"create table xxx (id int)", "IGNORE_SPACE", 0},
  {"create table xxx(id int)", "IGNORE_SPACE", 0},

  // ANSI_QUOTES
  {"select \"abc\" \"def\" 'ghi''\\n\\Z\\z'", "", 0},            // Double quoted text concatenated + alias.
  {"select \"abc\" \"def\" 'ghi''\\n\\Z\\z'", "ANSI_QUOTES", 1}, // column ref + alias + invalid single quoted text.

  // PIPES_AS_CONCAT
  {"select \"abc\" || \"def\"", "", 0},
  {"select \"abc\" || \"def\"", "PIPES_AS_CONCAT", 0},

  // HIGH_NOT_PRECEDENCE
  {"select not 1 between -5 and 5", "", 0},
  {"select not 1 between -5 and 5", "HIGH_NOT_PRECEDENCE", 0},

  // NO_BACKSLASH_ESCAPES
  {"select \"abc \\\"def\"", "", 0},
  {"select \"abc \\\"def\"", "NO_BACKSLASH_ESCAPES", 1},

  // TODO: add tests for sql modes that are synonyms for a combination of the base modes.
};

static const std::vector<TokenVector> sqlModeTestResults = {
  list_of(CREATE_SYMBOL)(TABLE_SYMBOL)(TABLE_NAME_TOKEN)(IDENTIFIER)(OPEN_PAR_SYMBOL)(CREATE_ITEM_TOKEN)(
    COLUMN_NAME_TOKEN)(IDENTIFIER)(DATA_TYPE_TOKEN)(INT_SYMBOL)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(CREATE_SYMBOL)(TABLE_SYMBOL)(TABLE_NAME_TOKEN)(ANTLR3_TOKEN_INVALID)(OPEN_PAR_SYMBOL)(CREATE_ITEM_TOKEN)(
    COLUMN_NAME_TOKEN)(IDENTIFIER)(DATA_TYPE_TOKEN)(INT_SYMBOL)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(CREATE_SYMBOL)(TABLE_SYMBOL)(TABLE_NAME_TOKEN)(ANTLR3_TOKEN_INVALID)(OPEN_PAR_SYMBOL)(CREATE_ITEM_TOKEN)(
    COLUMN_NAME_TOKEN)(IDENTIFIER)(DATA_TYPE_TOKEN)(INT_SYMBOL)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(CREATE_SYMBOL)(TABLE_SYMBOL)(TABLE_NAME_TOKEN)(ANTLR3_TOKEN_INVALID)(OPEN_PAR_SYMBOL)(CREATE_ITEM_TOKEN)(
    COLUMN_NAME_TOKEN)(IDENTIFIER)(DATA_TYPE_TOKEN)(INT_SYMBOL)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(CREATE_SYMBOL)(TABLE_SYMBOL)(TABLE_NAME_TOKEN)(IDENTIFIER)(OPEN_PAR_SYMBOL)(CREATE_ITEM_TOKEN)(
    COLUMN_NAME_TOKEN)(IDENTIFIER)(DATA_TYPE_TOKEN)(INT_SYMBOL)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(CREATE_SYMBOL)(TABLE_SYMBOL)(TABLE_NAME_TOKEN)(IDENTIFIER)(OPEN_PAR_SYMBOL)(CREATE_ITEM_TOKEN)(
    COLUMN_NAME_TOKEN)(IDENTIFIER)(DATA_TYPE_TOKEN)(INT_SYMBOL)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(CREATE_SYMBOL)(TABLE_SYMBOL)(TABLE_NAME_TOKEN)(IDENTIFIER)(OPEN_PAR_SYMBOL)(CREATE_ITEM_TOKEN)(
    COLUMN_NAME_TOKEN)(IDENTIFIER)(DATA_TYPE_TOKEN)(INT_SYMBOL)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(CREATE_SYMBOL)(TABLE_SYMBOL)(TABLE_NAME_TOKEN)(IDENTIFIER)(OPEN_PAR_SYMBOL)(CREATE_ITEM_TOKEN)(
    COLUMN_NAME_TOKEN)(IDENTIFIER)(DATA_TYPE_TOKEN)(INT_SYMBOL)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(STRING_TOKEN)(DOUBLE_QUOTED_TEXT)(DOUBLE_QUOTED_TEXT)(
    SINGLE_QUOTED_TEXT)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL)(ANTLR3_TOKEN_INVALID)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(LOGICAL_OR_OPERATOR)(STRING_TOKEN)(DOUBLE_QUOTED_TEXT)(
    STRING_TOKEN)(DOUBLE_QUOTED_TEXT)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(CONCAT_PIPES_SYMBOL)(STRING_TOKEN)(DOUBLE_QUOTED_TEXT)(
    STRING_TOKEN)(DOUBLE_QUOTED_TEXT)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(BETWEEN_SYMBOL)(INT_NUMBER)(MINUS_OPERATOR)(
    INT_NUMBER)(AND_SYMBOL)(INT_NUMBER)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BETWEEN_SYMBOL)(NOT2_SYMBOL)(INT_NUMBER)(MINUS_OPERATOR)(
    INT_NUMBER)(AND_SYMBOL)(INT_NUMBER)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(STRING_TOKEN)(DOUBLE_QUOTED_TEXT)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL)(ANTLR3_TOKEN_INVALID)(ANTLR3_TOKEN_EOF),
};

TEST_FUNCTION(30) {
  for (size_t i = 0; i < sqlModeTestQueries.size(); i++)
    if (!parse_and_compare(sqlModeTestQueries[i].query, 50610, sqlModeTestQueries[i].sqlMode, _charsets,
                           sqlModeTestResults[i], sqlModeTestQueries[i].errors)) {
      fail("SQL mode test - query failed: " + sqlModeTestQueries[i].query);
    }
}

/**
 * Tests the parser's string concatenation feature.
 */
TEST_FUNCTION(35) {
  std::string sql = "select \"abc\" \"def\" 'ghi''\\n\\z'";

  MySQLRecognizer recognizer(50610, "", _charsets);
  recognizer.parse(sql.c_str(), sql.size(), true, MySQLParseUnit::PuGeneric);
  ensure_equals("35.1 String concatenation", recognizer.error_info().size(), 0U);

  MySQLRecognizerTreeWalker walker = recognizer.tree_walker();
  ensure("35.2 String concatenation", walker.advanceToType(STRING_TOKEN, true));
  ensure_equals("35.3 String concatenation", walker.tokenText(), "abcdefghi'\nz");
}

struct VersionTestData {
  long version;
  std::string sql;
  size_t errorCount;
  VersionTestData(long version_, const std::string &sql_, size_t errors_) {
    version = version_;
    sql = sql_;
    errorCount = errors_;
  }
};

const std::vector<VersionTestData> versionTestResults =
  list_of(VersionTestData(50100, "grant all privileges on a to mike", 0U))(
    VersionTestData(50100, "grant all privileges on a to mike identified by 'blah'", 0U))(
    VersionTestData(50100, "grant all privileges on a to mike identified by password 'blah'", 0U))(
    VersionTestData(50100, "grant all privileges on a to mike identified by password 'blah'", 0U))(
    VersionTestData(50500, "grant all privileges on a to mike identified by password 'blah'", 0U))(
    VersionTestData(50710, "grant all privileges on a to mike identified by password 'blah'", 0U))(
    VersionTestData(50100, "grant select on *.* to mike identified with 'blah'", 1U))(
    VersionTestData(50600, "grant select on *.* to mike identified with 'blah'", 0U))(
    VersionTestData(50100, "grant select on *.* to mike identified with blah as 'blubb'", 1U))(
    VersionTestData(50600, "grant select on *.* to mike identified with blah as 'blubb'", 0U))(
    VersionTestData(50100, "grant select on *.* to mike identified with blah by 'blubb'", 1U))(
    VersionTestData(50600, "grant select on *.* to mike identified with blah by 'blubb'", 1U))(
    VersionTestData(50706, "grant select on *.* to mike identified with blah by 'blubb'", 0U));

// TODO: create tests for all server version dependent features.
// Will be obsolete if we support versions in the statements test file (or similar file).

TEST_FUNCTION(40) {
  // Version dependent parts of GRANT.
  MySQLRecognizer recognizer(50100, "", _charsets);
  for (size_t i = 0; i < versionTestResults.size(); ++i) {
    recognizer.set_server_version(versionTestResults[i].version);
    recognizer.parse(versionTestResults[i].sql.c_str(), versionTestResults[i].sql.size(), true,
                     MySQLParseUnit::PuGeneric);
    if (versionTestResults[i].errorCount != recognizer.error_info().size()) {
      std::stringstream ss;
      ss << "40." << i << " grant";
      ensure_equals(ss.str(), recognizer.error_info().size(), versionTestResults[i].errorCount);
    }
  }
}

// TODO: create tests for restricted content parsing (e.g. routines only, views only etc.).

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  delete _tester;
}

END_TESTS;

//----------------------------------------------------------------------------------------------------------------------
