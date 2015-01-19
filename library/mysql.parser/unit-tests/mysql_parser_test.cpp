/* 
 * Copyright (c) 2012, 2015, Oracle and/or its affiliates. All rights reserved.
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

BEGIN_TEST_DATA_CLASS(mysql_parser_test)
protected:
  WBTester _tester;
  std::set<std::string> _charsets;

TEST_DATA_CONSTRUCTOR(mysql_parser_test)
{
  // init datatypes
  populate_grt(_tester.grt, _tester);

  // The charset list contains also the 3 charsets that were introduced in 5.5.3.
  grt::ListRef<db_CharacterSet> list= _tester.get_rdbms()->characterSets();
  for (size_t i = 0; i < list->count(); i++)
    _charsets.insert(base::tolower(*list[i]->name()));
}

END_TEST_DATA_CLASS

TEST_MODULE(mysql_parser_test, "MySQL parser test suite (ANTLR)");

//--------------------------------------------------------------------------------------------------

/**
 * Parses the given string and returns true if no error occurred, otherwise false.
 */
bool parse(const char *sql, size_t size, bool is_utf8, long server_version, const std::string &sql_mode,
  const std::set<std::string> &charsets)
{
  MySQLRecognizer recognizer(server_version, sql_mode, charsets);
  recognizer.parse(sql, size, is_utf8, PuGeneric);
  bool result = recognizer.error_info().size() == 0;

  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 *  Statement splitter test.
 */
TEST_FUNCTION(5)
{
  const char *filename = "data/db/sakila-db/sakila-data.sql";
  const char *statement_filename = "data/db/sakila-db/single_statement.sql";

  ensure("SQL file does not exist.", g_file_test(filename, G_FILE_TEST_EXISTS) == TRUE);
  ensure("Statement file does not exist.", g_file_test(statement_filename, G_FILE_TEST_EXISTS) == TRUE);

  gchar *sql = NULL;
  gsize  size = 0;
  GError *error = NULL;
  g_file_get_contents(filename, &sql, &size, &error);
  ensure("Error loading sql file", error == NULL);

  SqlFacade::Ref sql_facade = SqlFacade::instance_for_rdbms_name(_tester.grt, "Mysql");

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

struct TestFile 
{
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
  {"data/db/sakila-db/sakila-data.sql", "\n", ";", false}
};

/**
 * Parse a number files with various statements.
 */
TEST_FUNCTION(10)
{
#if VERBOSE_OUTPUT
  test_time_point t1;
#endif

  SqlFacade::Ref sql_facade = SqlFacade::instance_for_rdbms_name(_tester.grt, "Mysql");
  size_t count = 0;
  for (size_t i = 0; i < sizeof(test_files) / sizeof(test_files[0]); ++i)
  {
    ensure(base::strfmt("Statements file '%s' does not exist.", test_files[i].name),
      g_file_test(test_files[i].name, G_FILE_TEST_EXISTS) == TRUE);

    gchar *sql = NULL;
    gsize  size = 0;
    GError *error = NULL;
    g_file_get_contents(test_files[i].name, &sql, &size, &error);
    ensure("Error loading sql file", error == NULL);

    std::vector<std::pair<size_t, size_t> >::const_iterator iterator;
    std::vector<std::pair<size_t, size_t> > ranges;
    sql_facade->splitSqlScript(sql, size, test_files[i].initial_delmiter, ranges, test_files[i].line_break);
    count += ranges.size();

    for (iterator = ranges.begin(); iterator != ranges.end(); ++iterator)
    {
      if (!parse(sql + iterator->first, iterator->second, test_files[i].is_utf8, 50604, "ANSI_QUOTES", _charsets))
      {
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

static const char* functions[] = {
  "acos",
  "adddate",
  "addtime"
  "aes_decrypt",
  "aes_encrypt",
  "area",
  "asbinary",
  "asin",
  "astext",
  "aswkb",
  "aswkt",
  "atan",
  "atan2",
  "benchmark",
  "bin",
  "bit_count",
  "bit_length",
  "ceil",
  "ceiling",
  "centroid",
  "character_length",
  "char_length",
  "coercibility",
  "compress",
  "concat",
  "concat_ws",
  "connection_id",
  "conv",
  "convert_tz",
  "cos",
  "cot",
  "crc32",
  "crosses",
  "datediff",
  "date_format",
  "dayname",
  "dayofmonth",
  "dayofweek",
  "dayofyear",
  "decode",
  "degrees",
  "des_decrypt",
  "des_encrypt",
  "dimension",
  "disjoint",
  "elt",
  "encode",
  "encrypt",
  "endpoint",
  "envelope",
  "equals",
  "exp",
  "export_set",
  "exteriorring",
  "extractvalue",
  "find_in_set",
  "floor",
  "found_rows",
  "from_days",
  "from_unixtime",
  "geomcollfromtext",
  "geomcollfromwkb",
  "geometrycollectionfromtext",
  "geometrycollectionfromwkb",
  "geometryfromtext",
  "geometryfromwkb",
  "geometryn",
  "geometrytype",
  "geomfromtext",
  "geomfromwkb",
  "get_lock",
  "glength",
  "greatest",
  "hex",
  "ifnull",
  "inet_aton",
  "inet_ntoa",
  "instr",
  "interiorringn",
  "intersects",
  "isclosed",
  "isempty",
  "isnull",
  "issimple",
  "is_free_lock",
  "is_used_lock",
  "last_day",
  "last_insert_id",
  "lcase",
  "least",
  "length",
  "linefromtext",
  "linefromwkb",
  "linestringfromtext",
  "linestringfromwkb",
  "ln",
  "load_file",
  "locate",
  "log",
  "log10",
  "log2",
  "lower",
  "lpad",
  "ltrim",
  "makedate",
  "maketime",
  "make_set",
  "master_pos_wait",
  "mbrcontains",
  "mbrdisjoint",
  "mbrequal",
  "mbrintersects",
  "mbroverlaps",
  "mbrtouches",
  "mbrwithin",
  "md5",
  "mlinefromtext",
  "mlinefromwkb",
  "monthname",
  "mpointfromtext",
  "mpointfromwkb",
  "mpolyfromtext",
  "mpolyfromwkb",
  "multilinestringfromtext",
  "multilinestringfromwkb",
  "multipointfromtext",
  "multipointfromwkb",
  "multipolygonfromtext",
  "multipolygonfromwkb",
  "name_const",
  "nullif",
  "numgeometries",
  "numinteriorrings",
  "numpoints",
  "oct",
  "octet_length",
  "ord",
  "overlaps",
  "period_add",
  "period_diff",
  "pi",
  "pointfromtext",
  "pointfromwkb",
  "pointn",
  "polyfromtext",
  "polyfromwkb",
  "polygonfromtext",
  "polygonfromwkb",
  "pow",
  "power",
  "quote",
  "radians",
  "rand",
  "release_lock",
  "reverse",
  "round",
  "row_count",
  "rpad",
  "rtrim",
  "sec_to_time",
  "session_user",
  "sha",
  "sha1",
  "sign",
  "sin",
  "sleep",
  "soundex",
  "space",
  "sqrt",
  "srid",
  "startpoint",
  "strcmp",
  "str_to_date",
  "subdate",
  "substring_index",
  "subtime",
  "system_user",
  "tan",
  "timediff",
  "time_format",
  "time_to_sec",
  "touches",
  "to_days",
  "ucase",
  "uncompress",
  "uncompressed_length",
  "unhex",
  "unix_timestamp",
  "updatexml",
  "upper",
  "uuid",
  "version",
  "weekday",
  "weekofyear",
  "within",
  "x",
  "y",
  "yearweek"};

  const char *query1 = "CREATE TABLE %s(\n"
    "col1 int not null,\n"
    "col2 int not null,\n"
    "col3 varchar(10),\n"
    "CONSTRAINT pk PRIMARY KEY (col1, col2)\n"
    ") ENGINE InnoDB";

  const char *query2 = "CREATE TABLE bug21114_child(\n"
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

TEST_FUNCTION(20)
{
  int count = sizeof(functions) / sizeof(functions[0]);
  for (int i = 0; i < count; i++)
  {
    std::string query = base::strfmt(query1, functions[i]);
    ensure("A statement failed to parse", parse(query.c_str(), query.size(), true, 50530, "ANSI_QUOTES", _charsets));

    query = base::strfmt(query2, functions[i], functions[i]);
    ensure("A statement failed to parse", parse(query.c_str(), query.size(), true, 50530, "ANSI_QUOTES", _charsets));
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Parses the given string and checks the built AST. Returns true if no error occurred, otherwise false.
 */
bool parse_and_compare(const std::string &sql, long server_version, const std::string &sql_mode,
  const std::set<std::string> &charsets, std::vector<ANTLR3_UINT32> tokens, unsigned int error_count = 0)
{
  MySQLRecognizer recognizer(server_version, sql_mode, charsets);
  recognizer.parse(sql.c_str(), sql.size(), true, PuGeneric);
  if (recognizer.error_info().size() != error_count)
    return false;

  // Walk the list of AST nodes recursively and match exactly the given list of tokens
  // (except for the starting nil node, but including the trailing EOF node).
  MySQLRecognizerTreeWalker walker = recognizer.tree_walker();
  size_t i = 0;
  do
  {
    if (i >= tokens.size())
      return false;

    if (walker.token_type() != tokens[i++])
      return false;
  } while (walker.next());

  return i == tokens.size();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Operator precedence tests. These were taken from the server parser test suite.
 */

struct pred_test_entry
{
  std::string query;
  std::vector<ANTLR3_UINT32> tokens;
};

static std::vector<std::string> precedenceTestQueries = {
  "select A, B, A OR B, A XOR B, A AND B from t1_30237_bool where C is null order by A, B",
  "select A, B, C, (A OR B) OR C, A OR (B OR C), A OR B OR C from t1_30237_bool order by A, B, C",
  "select count(*) from t1_30237_bool where ((A OR B) OR C) != (A OR (B OR C))",
  "select A, B, C, (A XOR B) XOR C, A XOR (B XOR C), A XOR B XOR C from t1_30237_bool order by A, B, C",
  "select count(*) from t1_30237_bool where ((A XOR B) XOR C) != (A XOR (B XOR C))",
  "select A, B, C, (A AND B) AND C, A AND (B AND C), A AND B AND C from t1_30237_bool order by A, B, C",
  "select count(*) from t1_30237_bool where ((A AND B) AND C) != (A AND (B AND C))",
  "select A, B, C, (A OR B) AND C, A OR (B AND C), A OR B AND C from t1_30237_bool order by A, B, C",
  "select count(*) from t1_30237_bool where (A OR (B AND C)) != (A OR B AND C)",
  "select A, B, C, (A AND B) OR C, A AND (B OR C), A AND B OR C from t1_30237_bool order by A, B, C",
  "select count(*) from t1_30237_bool where ((A AND B) OR C) != (A AND B OR C)",
  "select A, B, C, (A XOR B) AND C, A XOR (B AND C), A XOR B AND C from t1_30237_bool order by A, B, C",
  "select count(*) from t1_30237_bool where (A XOR (B AND C)) != (A XOR B AND C)",
  "select A, B, C, (A AND B) XOR C, A AND (B XOR C), A AND B XOR C from t1_30237_bool order by A, B, C",
  "select count(*) from t1_30237_bool where ((A AND B) XOR C) != (A AND B XOR C)",
  "select A, B, C, (A XOR B) OR C, A XOR (B OR C), A XOR B OR C from t1_30237_bool order by A, B, C",
  "select count(*) from t1_30237_bool where ((A XOR B) OR C) != (A XOR B OR C)",
  "select A, B, C, (A OR B) XOR C, A OR (B XOR C), A OR B XOR C from t1_30237_bool order by A, B, C",
  "select count(*) from t1_30237_bool where (A OR (B XOR C)) != (A OR B XOR C)",
  "select (NOT FALSE) OR TRUE, NOT (FALSE OR TRUE), NOT FALSE OR TRUE",
  "select (NOT FALSE) XOR FALSE, NOT (FALSE XOR FALSE), NOT FALSE XOR FALSE",
  "select (NOT FALSE) AND FALSE, NOT (FALSE AND FALSE), NOT FALSE AND FALSE",
  "select NOT NOT TRUE, NOT NOT NOT FALSE",
  "select (NOT NULL) IS TRUE, NOT (NULL IS TRUE), NOT NULL IS TRUE",
  "select (NOT NULL) IS NOT TRUE, NOT (NULL IS NOT TRUE), NOT NULL IS NOT TRUE",
  "select (NOT NULL) IS FALSE, NOT (NULL IS FALSE), NOT NULL IS FALSE",
  "select (NOT NULL) IS NOT FALSE, NOT (NULL IS NOT FALSE), NOT NULL IS NOT FALSE",
  "select (NOT TRUE) IS UNKNOWN, NOT (TRUE IS UNKNOWN), NOT TRUE IS UNKNOWN",
  "select (NOT TRUE) IS NOT UNKNOWN, NOT (TRUE IS NOT UNKNOWN), NOT TRUE IS NOT UNKNOWN",
  "select (NOT TRUE) IS NULL, NOT (TRUE IS NULL), NOT TRUE IS NULL",
  "select (NOT TRUE) IS NOT NULL, NOT (TRUE IS NOT NULL), NOT TRUE IS NOT NULL",
  "select FALSE IS NULL IS NULL IS NULL",
  "select TRUE IS NOT NULL IS NOT NULL IS NOT NULL",
  "select 1 <=> 2 <=> 2, (1 <=> 2) <=> 2, 1 <=> (2 <=> 2)",
  "select 1 = 2 = 2, (1 = 2) = 2, 1 = (2 = 2)",
  "select 1 != 2 != 3, (1 != 2) != 3, 1 != (2 != 3)",
  "select 1 <> 2 <> 3, (1 <> 2) <> 3, 1 <> (2 <> 3)",
  "select 1 < 2 < 3, (1 < 2) < 3, 1 < (2 < 3)",
  "select 3 <= 2 <= 1, (3 <= 2) <= 1, 3 <= (2 <= 1)",
  "select 1 > 2 > 3, (1 > 2) > 3, 1 > (2 > 3)",
  "select 1 >= 2 >= 3, (1 >= 2) >= 3, 1 >= (2 >= 3)",
  "select 0xF0 | 0x0F | 0x55, (0xF0 | 0x0F) | 0x55, 0xF0 | (0x0F | 0x55)",
  "select 0xF5 & 0x5F & 0x55, (0xF5 & 0x5F) & 0x55, 0xF5 & (0x5F & 0x55)",
  "select 4 << 3 << 2, (4 << 3) << 2, 4 << (3 << 2)",
  "select 256 >> 3 >> 2, (256 >> 3) >> 2, 256 >> (3 >> 2)",
  "select 0xF0 & 0x0F | 0x55, (0xF0 & 0x0F) | 0x55, 0xF0 & (0x0F | 0x55)",
  "select 0x55 | 0xF0 & 0x0F, (0x55 | 0xF0) & 0x0F, 0x55 | (0xF0 & 0x0F)",
  "select 0x0F << 4 | 0x0F, (0x0F << 4) | 0x0F, 0x0F << (4 | 0x0F)",
  "select 0x0F | 0x0F << 4, (0x0F | 0x0F) << 4, 0x0F | (0x0F << 4)",
  "select 0xF0 >> 4 | 0xFF, (0xF0 >> 4) | 0xFF, 0xF0 >> (4 | 0xFF)",
  "select 0xFF | 0xF0 >> 4, (0xFF | 0xF0) >> 4, 0xFF | (0xF0 >> 4)",
  "select 0x0F << 4 & 0xF0, (0x0F << 4) & 0xF0, 0x0F << (4 & 0xF0)",
  "select 0xF0 & 0x0F << 4, (0xF0 & 0x0F) << 4, 0xF0 & (0x0F << 4)",
  "select 0xF0 >> 4 & 0x55, (0xF0 >> 4) & 0x55, 0xF0 >> (4 & 0x55)",
  "select 0x0F & 0xF0 >> 4, (0x0F & 0xF0) >> 4, 0x0F & (0xF0 >> 4)",
  "select 0xFF >> 4 << 2, (0xFF >> 4) << 2, 0xFF >> (4 << 2)",
  "select 0x0F << 4 >> 2, (0x0F << 4) >> 2, 0x0F << (4 >> 2)",
  "select 1 + 2 + 3, (1 + 2) + 3, 1 + (2 + 3)",
  "select 1 - 2 - 3, (1 - 2) - 3, 1 - (2 - 3)",
  "select 1 + 2 - 3, (1 + 2) - 3, 1 + (2 - 3)",
  "select 1 - 2 + 3, (1 - 2) + 3, 1 - (2 + 3)",
  "select 0xF0 + 0x0F | 0x55, (0xF0 + 0x0F) | 0x55, 0xF0 + (0x0F | 0x55)",
  "select 0x55 | 0xF0 + 0x0F, (0x55 | 0xF0) + 0x0F, 0x55 | (0xF0 + 0x0F)",
  "select 0xF0 + 0x0F & 0x55, (0xF0 + 0x0F) & 0x55, 0xF0 + (0x0F & 0x55)",
  "select 0x55 & 0xF0 + 0x0F, (0x55 & 0xF0) + 0x0F, 0x55 & (0xF0 + 0x0F)",
  "select 2 + 3 << 4, (2 + 3) << 4, 2 + (3 << 4)",
  "select 3 << 4 + 2, (3 << 4) + 2, 3 << (4 + 2)",
  "select 4 + 3 >> 2, (4 + 3) >> 2, 4 + (3 >> 2)",
  "select 3 >> 2 + 1, (3 >> 2) + 1, 3 >> (2 + 1)",
  "select 0xFF - 0x0F | 0x55, (0xFF - 0x0F) | 0x55, 0xFF - (0x0F | 0x55)",
  "select 0x55 | 0xFF - 0xF0, (0x55 | 0xFF) - 0xF0, 0x55 | (0xFF - 0xF0)",
  "select 0xFF - 0xF0 & 0x55, (0xFF - 0xF0) & 0x55, 0xFF - (0xF0 & 0x55)",
  "select 0x55 & 0xFF - 0xF0, (0x55 & 0xFF) - 0xF0, 0x55 & (0xFF - 0xF0)",
  "select 16 - 3 << 2, (16 - 3) << 2, 16 - (3 << 2)",
  "select 4 << 3 - 2, (4 << 3) - 2, 4 << (3 - 2)",
  "select 16 - 3 >> 2, (16 - 3) >> 2, 16 - (3 >> 2)",
  "select 16 >> 3 - 2, (16 >> 3) - 2, 16 >> (3 - 2)",
  "select 2 * 3 * 4, (2 * 3) * 4, 2 * (3 * 4)",
  "select 2 * 0x40 | 0x0F, (2 * 0x40) | 0x0F, 2 * (0x40 | 0x0F)",
  "select 0x0F | 2 * 0x40, (0x0F | 2) * 0x40, 0x0F | (2 * 0x40)",
  "select 2 * 0x40 & 0x55, (2 * 0x40) & 0x55, 2 * (0x40 & 0x55)",
  "select 0xF0 & 2 * 0x40, (0xF0 & 2) * 0x40, 0xF0 & (2 * 0x40)",
  "select 5 * 3 << 4, (5 * 3) << 4, 5 * (3 << 4)",
  "select 2 << 3 * 4, (2 << 3) * 4, 2 << (3 * 4)",
  "select 3 * 4 >> 2, (3 * 4) >> 2, 3 * (4 >> 2)",
  "select 4 >> 2 * 3, (4 >> 2) * 3, 4 >> (2 * 3)",
  "select 2 * 3 + 4, (2 * 3) + 4, 2 * (3 + 4)",
  "select 2 + 3 * 4, (2 + 3) * 4, 2 + (3 * 4)",
  "select 4 * 3 - 2, (4 * 3) - 2, 4 * (3 - 2)",
  "select 4 - 3 * 2, (4 - 3) * 2, 4 - (3 * 2)",
  "select 15 / 5 / 3, (15 / 5) / 3, 15 / (5 / 3)",
  "select 105 / 5 | 2, (105 / 5) | 2, 105 / (5 | 2)",
  "select 105 | 2 / 5, (105 | 2) / 5, 105 | (2 / 5)",
  "select 105 / 5 & 0x0F, (105 / 5) & 0x0F, 105 / (5 & 0x0F)",
  "select 0x0F & 105 / 5, (0x0F & 105) / 5, 0x0F & (105 / 5)",
  "select 0x80 / 4 << 2, (0x80 / 4) << 2, 0x80 / (4 << 2)",
  "select 0x80 << 4 / 2, (0x80 << 4) / 2, 0x80 << (4 / 2)",
  "select 0x80 / 4 >> 2, (0x80 / 4) >> 2, 0x80 / (4 >> 2)",
  "select 0x80 >> 4 / 2, (0x80 >> 4) / 2, 0x80 >> (4 / 2)",
  "select 0x80 / 2 + 2, (0x80 / 2) + 2, 0x80 / (2 + 2)",
  "select 0x80 + 2 / 2, (0x80 + 2) / 2, 0x80 + (2 / 2)",
  "select 0x80 / 4 - 2, (0x80 / 4) - 2, 0x80 / (4 - 2)",
  "select 0x80 - 4 / 2, (0x80 - 4) / 2, 0x80 - (4 / 2)",
  "select 0xFF ^ 0xF0 ^ 0x0F, (0xFF ^ 0xF0) ^ 0x0F, 0xFF ^ (0xF0 ^ 0x0F)",
  "select 0xFF ^ 0xF0 ^ 0x55, (0xFF ^ 0xF0) ^ 0x55, 0xFF ^ (0xF0 ^ 0x55)",
  "select 0xFF ^ 0xF0 | 0x0F, (0xFF ^ 0xF0) | 0x0F, 0xFF ^ (0xF0 | 0x0F)",
  "select 0xF0 | 0xFF ^ 0xF0, (0xF0 | 0xFF) ^ 0xF0, 0xF0 | (0xFF ^ 0xF0)",
  "select 0xFF ^ 0xF0 & 0x0F, (0xFF ^ 0xF0) & 0x0F, 0xFF ^ (0xF0 & 0x0F)",
  "select 0x0F & 0xFF ^ 0xF0, (0x0F & 0xFF) ^ 0xF0, 0x0F & (0xFF ^ 0xF0)",
  "select 0xFF ^ 0xF0 << 2, (0xFF ^ 0xF0) << 2, 0xFF ^ (0xF0 << 2)",
  "select 0x0F << 2 ^ 0xFF, (0x0F << 2) ^ 0xFF, 0x0F << (2 ^ 0xFF)",
  "select 0xFF ^ 0xF0 >> 2, (0xFF ^ 0xF0) >> 2, 0xFF ^ (0xF0 >> 2)",
  "select 0xFF >> 2 ^ 0xF0, (0xFF >> 2) ^ 0xF0, 0xFF >> (2 ^ 0xF0)",
  "select 0xFF ^ 0xF0 + 0x0F, (0xFF ^ 0xF0) + 0x0F, 0xFF ^ (0xF0 + 0x0F)",
  "select 0x0F + 0xFF ^ 0xF0, (0x0F + 0xFF) ^ 0xF0, 0x0F + (0xFF ^ 0xF0)",
  "select 0xFF ^ 0xF0 - 1, (0xFF ^ 0xF0) - 1, 0xFF ^ (0xF0 - 1)",
  "select 0x55 - 0x0F ^ 0x55, (0x55 - 0x0F) ^ 0x55, 0x55 - (0x0F ^ 0x55)",
  "select 0xFF ^ 0xF0 * 2, (0xFF ^ 0xF0) * 2, 0xFF ^ (0xF0 * 2)",
  "select 2 * 0xFF ^ 0xF0, (2 * 0xFF) ^ 0xF0, 2 * (0xFF ^ 0xF0)",
  "select 0xFF ^ 0xF0 / 2, (0xFF ^ 0xF0) / 2, 0xFF ^ (0xF0 / 2)",
  "select 0xF2 / 2 ^ 0xF0, (0xF2 / 2) ^ 0xF0, 0xF2 / (2 ^ 0xF0)",
  "select 0xFF ^ 0xF0 % 0x20, (0xFF ^ 0xF0) % 0x20, 0xFF ^ (0xF0 % 0x20)",
  "select 0xFF % 0x20 ^ 0xF0, (0xFF % 0x20) ^ 0xF0, 0xFF % (0x20 ^ 0xF0)",
  "select 0xFF ^ 0xF0 DIV 2, (0xFF ^ 0xF0) DIV 2, 0xFF ^ (0xF0 DIV 2)",
  "select 0xF2 DIV 2 ^ 0xF0, (0xF2 DIV 2) ^ 0xF0, 0xF2 DIV (2 ^ 0xF0)",
  "select 0xFF ^ 0xF0 MOD 0x20, (0xFF ^ 0xF0) MOD 0x20, 0xFF ^ (0xF0 MOD 0x20)",
  "select 0xFF MOD 0x20 ^ 0xF0, (0xFF MOD 0x20) ^ 0xF0, 0xFF MOD (0x20 ^ 0xF0)",
};

const std::vector<std::vector<ANTLR3_UINT32> > precedenceTestResults = {
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(WHERE_SYMBOL)(EXPRESSION_TOKEN)(IS_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(NULL_SYMBOL)(ORDER_SYMBOL)(BY_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(OR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(OR_SYMBOL)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(ORDER_SYMBOL)(BY_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(RUNTIME_FUNCTION_TOKEN)(COUNT_SYMBOL)(OPEN_PAR_SYMBOL)(MULT_OPERATOR)(CLOSE_PAR_SYMBOL)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(WHERE_SYMBOL)(EXPRESSION_TOKEN)(NOT_EQUAL_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(XOR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(XOR_SYMBOL)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(ORDER_SYMBOL)(BY_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(RUNTIME_FUNCTION_TOKEN)(COUNT_SYMBOL)(OPEN_PAR_SYMBOL)(MULT_OPERATOR)(CLOSE_PAR_SYMBOL)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(WHERE_SYMBOL)(EXPRESSION_TOKEN)(NOT_EQUAL_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(AND_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(AND_SYMBOL)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(ORDER_SYMBOL)(BY_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(RUNTIME_FUNCTION_TOKEN)(COUNT_SYMBOL)(OPEN_PAR_SYMBOL)(MULT_OPERATOR)(CLOSE_PAR_SYMBOL)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(WHERE_SYMBOL)(EXPRESSION_TOKEN)(NOT_EQUAL_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(AND_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(AND_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(ORDER_SYMBOL)(BY_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(RUNTIME_FUNCTION_TOKEN)(COUNT_SYMBOL)(OPEN_PAR_SYMBOL)(MULT_OPERATOR)(CLOSE_PAR_SYMBOL)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(WHERE_SYMBOL)(EXPRESSION_TOKEN)(NOT_EQUAL_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(CLOSE_PAR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(OR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(OR_SYMBOL)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(ORDER_SYMBOL)(BY_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(RUNTIME_FUNCTION_TOKEN)(COUNT_SYMBOL)(OPEN_PAR_SYMBOL)(MULT_OPERATOR)(CLOSE_PAR_SYMBOL)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(WHERE_SYMBOL)(EXPRESSION_TOKEN)(NOT_EQUAL_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(AND_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(ORDER_SYMBOL)(BY_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(RUNTIME_FUNCTION_TOKEN)(COUNT_SYMBOL)(OPEN_PAR_SYMBOL)(MULT_OPERATOR)(CLOSE_PAR_SYMBOL)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(WHERE_SYMBOL)(EXPRESSION_TOKEN)(NOT_EQUAL_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(CLOSE_PAR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(XOR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(XOR_SYMBOL)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(ORDER_SYMBOL)(BY_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(RUNTIME_FUNCTION_TOKEN)(COUNT_SYMBOL)(OPEN_PAR_SYMBOL)(MULT_OPERATOR)(CLOSE_PAR_SYMBOL)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(WHERE_SYMBOL)(EXPRESSION_TOKEN)(NOT_EQUAL_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(AND_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(OR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(OR_SYMBOL)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(ORDER_SYMBOL)(BY_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(RUNTIME_FUNCTION_TOKEN)(COUNT_SYMBOL)(OPEN_PAR_SYMBOL)(MULT_OPERATOR)(CLOSE_PAR_SYMBOL)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(WHERE_SYMBOL)(EXPRESSION_TOKEN)(NOT_EQUAL_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(XOR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(ORDER_SYMBOL)(BY_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(COMMA_SYMBOL)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(IDENTIFIER)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(RUNTIME_FUNCTION_TOKEN)(COUNT_SYMBOL)(OPEN_PAR_SYMBOL)(MULT_OPERATOR)(CLOSE_PAR_SYMBOL)(FROM_SYMBOL)(TABLE_REF_TOKEN)(IDENTIFIER)(WHERE_SYMBOL)(EXPRESSION_TOKEN)(NOT_EQUAL_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(CLOSE_PAR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(XOR_SYMBOL)(COLUMN_REF_TOKEN)(IDENTIFIER)(COLUMN_REF_TOKEN)(IDENTIFIER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(OR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(NOT_SYMBOL)(FALSE_SYMBOL)(CLOSE_PAR_SYMBOL)(TRUE_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(OR_SYMBOL)(FALSE_SYMBOL)(TRUE_SYMBOL)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(OR_SYMBOL)(NOT_SYMBOL)(FALSE_SYMBOL)(TRUE_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(XOR_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(NOT_SYMBOL)(FALSE_SYMBOL)(CLOSE_PAR_SYMBOL)(FALSE_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(XOR_SYMBOL)(FALSE_SYMBOL)(FALSE_SYMBOL)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(XOR_SYMBOL)(NOT_SYMBOL)(FALSE_SYMBOL)(FALSE_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(AND_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(NOT_SYMBOL)(FALSE_SYMBOL)(CLOSE_PAR_SYMBOL)(FALSE_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(AND_SYMBOL)(FALSE_SYMBOL)(FALSE_SYMBOL)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(AND_SYMBOL)(NOT_SYMBOL)(FALSE_SYMBOL)(FALSE_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(NOT_SYMBOL)(TRUE_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(NOT_SYMBOL)(NOT_SYMBOL)(FALSE_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(IS_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(NOT_SYMBOL)(NULL_SYMBOL)(CLOSE_PAR_SYMBOL)(TRUE_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(IS_SYMBOL)(NULL_SYMBOL)(TRUE_SYMBOL)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(IS_SYMBOL)(NULL_SYMBOL)(TRUE_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(IS_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(NOT_SYMBOL)(NULL_SYMBOL)(CLOSE_PAR_SYMBOL)(NOT_SYMBOL)(TRUE_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(IS_SYMBOL)(NULL_SYMBOL)(NOT_SYMBOL)(TRUE_SYMBOL)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(IS_SYMBOL)(NULL_SYMBOL)(NOT_SYMBOL)(TRUE_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(IS_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(NOT_SYMBOL)(NULL_SYMBOL)(CLOSE_PAR_SYMBOL)(FALSE_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(IS_SYMBOL)(NULL_SYMBOL)(FALSE_SYMBOL)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(IS_SYMBOL)(NULL_SYMBOL)(FALSE_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(IS_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(NOT_SYMBOL)(NULL_SYMBOL)(CLOSE_PAR_SYMBOL)(NOT_SYMBOL)(FALSE_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(IS_SYMBOL)(NULL_SYMBOL)(NOT_SYMBOL)(FALSE_SYMBOL)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(IS_SYMBOL)(NULL_SYMBOL)(NOT_SYMBOL)(FALSE_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(IS_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(NOT_SYMBOL)(TRUE_SYMBOL)(CLOSE_PAR_SYMBOL)(UNKNOWN_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(IS_SYMBOL)(TRUE_SYMBOL)(UNKNOWN_SYMBOL)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(IS_SYMBOL)(TRUE_SYMBOL)(UNKNOWN_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(IS_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(NOT_SYMBOL)(TRUE_SYMBOL)(CLOSE_PAR_SYMBOL)(NOT_SYMBOL)(UNKNOWN_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(IS_SYMBOL)(TRUE_SYMBOL)(NOT_SYMBOL)(UNKNOWN_SYMBOL)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(IS_SYMBOL)(TRUE_SYMBOL)(NOT_SYMBOL)(UNKNOWN_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(IS_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(NOT_SYMBOL)(TRUE_SYMBOL)(CLOSE_PAR_SYMBOL)(NULL_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(IS_SYMBOL)(TRUE_SYMBOL)(NULL_SYMBOL)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(IS_SYMBOL)(TRUE_SYMBOL)(NULL_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(IS_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(NOT_SYMBOL)(TRUE_SYMBOL)(CLOSE_PAR_SYMBOL)(NOT_SYMBOL)(NULL_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(IS_SYMBOL)(TRUE_SYMBOL)(NOT_SYMBOL)(NULL_SYMBOL)(CLOSE_PAR_SYMBOL)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(IS_SYMBOL)(TRUE_SYMBOL)(NOT_SYMBOL)(NULL_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(IS_SYMBOL)(IS_SYMBOL)(IS_SYMBOL)(FALSE_SYMBOL)(NULL_SYMBOL)(NULL_SYMBOL)(NULL_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(IS_SYMBOL)(IS_SYMBOL)(IS_SYMBOL)(TRUE_SYMBOL)(NOT_SYMBOL)(NULL_SYMBOL)(NOT_SYMBOL)(NULL_SYMBOL)(NOT_SYMBOL)(NULL_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NULL_SAFE_EQUAL_OPERATOR)(NULL_SAFE_EQUAL_OPERATOR)(INTEGER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NULL_SAFE_EQUAL_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(NULL_SAFE_EQUAL_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NULL_SAFE_EQUAL_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(NULL_SAFE_EQUAL_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(EQUAL_OPERATOR)(EQUAL_OPERATOR)(INTEGER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(EQUAL_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(EQUAL_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(EQUAL_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(EQUAL_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_EQUAL_OPERATOR)(NOT_EQUAL_OPERATOR)(INTEGER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_EQUAL_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(NOT_EQUAL_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_EQUAL_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(NOT_EQUAL_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_EQUAL2_OPERATOR)(NOT_EQUAL2_OPERATOR)(INTEGER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_EQUAL2_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(NOT_EQUAL2_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_EQUAL2_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(NOT_EQUAL2_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(LESS_THAN_OPERATOR)(LESS_THAN_OPERATOR)(INTEGER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(LESS_THAN_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(LESS_THAN_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(LESS_THAN_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(LESS_THAN_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(LESS_OR_EQUAL_OPERATOR)(LESS_OR_EQUAL_OPERATOR)(INTEGER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(LESS_OR_EQUAL_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(LESS_OR_EQUAL_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(LESS_OR_EQUAL_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(LESS_OR_EQUAL_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(GREATER_THAN_OPERATOR)(GREATER_THAN_OPERATOR)(INTEGER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(GREATER_THAN_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(GREATER_THAN_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(GREATER_THAN_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(GREATER_THAN_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(GREATER_OR_EQUAL_OPERATOR)(GREATER_OR_EQUAL_OPERATOR)(INTEGER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(GREATER_OR_EQUAL_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(GREATER_OR_EQUAL_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(GREATER_OR_EQUAL_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(GREATER_OR_EQUAL_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(BITWISE_OR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(BITWISE_AND_OPERATOR)(HEXNUMBER)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(SHIFT_LEFT_OPERATOR)(INTEGER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(SHIFT_RIGHT_OPERATOR)(INTEGER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(BITWISE_AND_OPERATOR)(HEXNUMBER)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(BITWISE_AND_OPERATOR)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(SHIFT_LEFT_OPERATOR)(HEXNUMBER)(INTEGER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(INTEGER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(SHIFT_LEFT_OPERATOR)(HEXNUMBER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(SHIFT_RIGHT_OPERATOR)(HEXNUMBER)(INTEGER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(INTEGER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(SHIFT_RIGHT_OPERATOR)(HEXNUMBER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(SHIFT_LEFT_OPERATOR)(HEXNUMBER)(INTEGER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(INTEGER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(SHIFT_LEFT_OPERATOR)(HEXNUMBER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(SHIFT_RIGHT_OPERATOR)(HEXNUMBER)(INTEGER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(INTEGER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(SHIFT_RIGHT_OPERATOR)(HEXNUMBER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(SHIFT_RIGHT_OPERATOR)(HEXNUMBER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(SHIFT_LEFT_OPERATOR)(HEXNUMBER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(PLUS_OPERATOR)(INTEGER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(MINUS_OPERATOR)(INTEGER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(PLUS_OPERATOR)(INTEGER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(MINUS_OPERATOR)(INTEGER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(PLUS_OPERATOR)(HEXNUMBER)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(PLUS_OPERATOR)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(PLUS_OPERATOR)(HEXNUMBER)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(PLUS_OPERATOR)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(PLUS_OPERATOR)(INTEGER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INTEGER)(PLUS_OPERATOR)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(PLUS_OPERATOR)(INTEGER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INTEGER)(PLUS_OPERATOR)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(MINUS_OPERATOR)(HEXNUMBER)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(MINUS_OPERATOR)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(MINUS_OPERATOR)(HEXNUMBER)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(MINUS_OPERATOR)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(MINUS_OPERATOR)(INTEGER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INTEGER)(MINUS_OPERATOR)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(MINUS_OPERATOR)(INTEGER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INTEGER)(MINUS_OPERATOR)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(MULT_OPERATOR)(INTEGER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(MULT_OPERATOR)(INTEGER)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INTEGER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(MULT_OPERATOR)(INTEGER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INTEGER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(MULT_OPERATOR)(INTEGER)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INTEGER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(MULT_OPERATOR)(INTEGER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INTEGER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(MULT_OPERATOR)(INTEGER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INTEGER)(MULT_OPERATOR)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(MULT_OPERATOR)(INTEGER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INTEGER)(MULT_OPERATOR)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(MULT_OPERATOR)(INTEGER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INTEGER)(MULT_OPERATOR)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(MULT_OPERATOR)(INTEGER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INTEGER)(MULT_OPERATOR)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(DIV_OPERATOR)(INTEGER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(DIV_OPERATOR)(INTEGER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(INTEGER)(DIV_OPERATOR)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(DIV_OPERATOR)(INTEGER)(INTEGER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(INTEGER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(DIV_OPERATOR)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(DIV_OPERATOR)(HEXNUMBER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEXNUMBER)(DIV_OPERATOR)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(DIV_OPERATOR)(HEXNUMBER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEXNUMBER)(DIV_OPERATOR)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(DIV_OPERATOR)(HEXNUMBER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(HEXNUMBER)(DIV_OPERATOR)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(DIV_OPERATOR)(HEXNUMBER)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(HEXNUMBER)(DIV_OPERATOR)(INTEGER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(INTEGER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_OR_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_AND_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEXNUMBER)(BITWISE_XOR_OPERATOR)(INTEGER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_LEFT_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(INTEGER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEXNUMBER)(BITWISE_XOR_OPERATOR)(INTEGER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(SHIFT_RIGHT_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(INTEGER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(HEXNUMBER)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(PLUS_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(HEXNUMBER)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MINUS_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INTEGER)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INTEGER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MULT_OPERATOR)(INTEGER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(HEXNUMBER)(BITWISE_XOR_OPERATOR)(INTEGER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_OPERATOR)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(INTEGER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MOD_OPERATOR)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MOD_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MOD_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MOD_OPERATOR)(HEXNUMBER)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MOD_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MOD_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_SYMBOL)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(INTEGER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_SYMBOL)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_SYMBOL)(HEXNUMBER)(BITWISE_XOR_OPERATOR)(INTEGER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(DIV_SYMBOL)(HEXNUMBER)(INTEGER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(DIV_SYMBOL)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(INTEGER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MOD_SYMBOL)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MOD_SYMBOL)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MOD_SYMBOL)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MOD_SYMBOL)(HEXNUMBER)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(MOD_SYMBOL)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(HEXNUMBER)(COMMA_SYMBOL)(SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(MOD_SYMBOL)(HEXNUMBER)(PAR_EXPRESSION_TOKEN)(OPEN_PAR_SYMBOL)(EXPRESSION_TOKEN)(BITWISE_XOR_OPERATOR)(HEXNUMBER)(HEXNUMBER)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
};

TEST_FUNCTION(25)
{
  ensure_equals("Data and result list must be equal in size", precedenceTestQueries.size(), precedenceTestResults.size());

  for (size_t i = 0; i < precedenceTestQueries.size(); ++i)
  {
    if (!parse_and_compare(precedenceTestQueries[i], 50530, "ANSI_QUOTES", _charsets, precedenceTestResults[i]))
      fail("Operator precedence test - query failed: " + precedenceTestQueries[i]);
  }
}

/**
 * Tests for all relevant SQL modes (ANSI, DB2, MAXDB, MSSQL, ORACLE, POSTGRESQL, MYSQL323, MYSQL40
 * ANSI_QUOTES, PIPES_AS_CONCAT, NO_BACKSLASH_ESCAPES, IGNORE_SPACE, HIGH_NOT_PRECEDENCE and combinations of them).
 */

struct sql_mode_test_entry
{
  std::string query;
  std::string modes;
  unsigned int errors;
  std::vector<ANTLR3_UINT32> tokens;
};

const sql_mode_test_entry sql_mode_test_data[] = {
  // IGNORE_SPACE
  {"create table count (id int)", "", 0,
    list_of(CREATE_SYMBOL) (TABLE_SYMBOL)(TABLE_NAME_TOKEN)(IDENTIFIER)(OPEN_PAR_SYMBOL)(CREATE_ITEM_TOKEN)(COLUMN_NAME_TOKEN)(IDENTIFIER)(DATA_TYPE_TOKEN)(INT_SYMBOL)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  },
  {"create table count(id int)", "", 1,
    list_of(CREATE_SYMBOL) (TABLE_SYMBOL)(TABLE_NAME_TOKEN)(ANTLR3_TOKEN_INVALID)(OPEN_PAR_SYMBOL)(CREATE_ITEM_TOKEN)(COLUMN_NAME_TOKEN)(IDENTIFIER)(DATA_TYPE_TOKEN)(INT_SYMBOL)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  },
  {"create table count (id int)", "IGNORE_SPACE", 1, 
    list_of(CREATE_SYMBOL) (TABLE_SYMBOL)(TABLE_NAME_TOKEN)(ANTLR3_TOKEN_INVALID)(OPEN_PAR_SYMBOL)(CREATE_ITEM_TOKEN)(COLUMN_NAME_TOKEN)(IDENTIFIER)(DATA_TYPE_TOKEN)(INT_SYMBOL)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  },
  {"create table count(id int)", "IGNORE_SPACE", 1, 
    list_of(CREATE_SYMBOL) (TABLE_SYMBOL)(TABLE_NAME_TOKEN)(ANTLR3_TOKEN_INVALID)(OPEN_PAR_SYMBOL)(CREATE_ITEM_TOKEN)(COLUMN_NAME_TOKEN)(IDENTIFIER)(DATA_TYPE_TOKEN)(INT_SYMBOL)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  },
  {"create table xxx (id int)", "", 0,
    list_of(CREATE_SYMBOL) (TABLE_SYMBOL)(TABLE_NAME_TOKEN)(IDENTIFIER)(OPEN_PAR_SYMBOL)(CREATE_ITEM_TOKEN)(COLUMN_NAME_TOKEN)(IDENTIFIER)(DATA_TYPE_TOKEN)(INT_SYMBOL)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  },
  {"create table xxx(id int)", "", 0,
    list_of(CREATE_SYMBOL) (TABLE_SYMBOL)(TABLE_NAME_TOKEN)(IDENTIFIER)(OPEN_PAR_SYMBOL)(CREATE_ITEM_TOKEN)(COLUMN_NAME_TOKEN)(IDENTIFIER)(DATA_TYPE_TOKEN)(INT_SYMBOL)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  },
  {"create table xxx (id int)", "IGNORE_SPACE", 0,
    list_of(CREATE_SYMBOL) (TABLE_SYMBOL)(TABLE_NAME_TOKEN)(IDENTIFIER)(OPEN_PAR_SYMBOL)(CREATE_ITEM_TOKEN)(COLUMN_NAME_TOKEN)(IDENTIFIER)(DATA_TYPE_TOKEN)(INT_SYMBOL)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  },
  {"create table xxx(id int)", "IGNORE_SPACE", 0,
    list_of(CREATE_SYMBOL) (TABLE_SYMBOL)(TABLE_NAME_TOKEN)(IDENTIFIER)(OPEN_PAR_SYMBOL)(CREATE_ITEM_TOKEN)(COLUMN_NAME_TOKEN)(IDENTIFIER)(DATA_TYPE_TOKEN)(INT_SYMBOL)(CLOSE_PAR_SYMBOL)(ANTLR3_TOKEN_EOF),
  },
  // ANSI_QUOTES
  {"select \"abc\" \"def\" 'ghi''\\n\\Z\\z'", "", 0,
    list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(STRING_TOKEN)(DOUBLE_QUOTED_TEXT)(DOUBLE_QUOTED_TEXT)(SINGLE_QUOTED_TEXT)(ANTLR3_TOKEN_EOF),
  },
  {"select \"abc\" \"def\" 'ghi''\\n\\Z\\z'", "ANSI_QUOTES", 1,
  list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(COLUMN_REF_TOKEN)(DOUBLE_QUOTED_TEXT)(DOUBLE_QUOTED_TEXT)(ANTLR3_TOKEN_EOF),
  },
  // PIPES_AS_CONCAT
  {"select \"abc\" || \"def\"", "", 0,
    list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(LOGICAL_OR_OPERATOR)(STRING_TOKEN)(DOUBLE_QUOTED_TEXT)(STRING_TOKEN)(DOUBLE_QUOTED_TEXT)(ANTLR3_TOKEN_EOF),
  },
  {"select \"abc\" || \"def\"", "PIPES_AS_CONCAT", 0,
    list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(CONCAT_PIPES_SYMBOL)(STRING_TOKEN)(DOUBLE_QUOTED_TEXT)(STRING_TOKEN)(DOUBLE_QUOTED_TEXT)(ANTLR3_TOKEN_EOF),
  },
  // HIGH_NOT_PRECEDENCE
  {"select not 1 between -5 and 5", "", 0,
    list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(NOT_SYMBOL)(BETWEEN_SYMBOL)(INTEGER)(MINUS_OPERATOR)(INTEGER)(AND_SYMBOL)(INTEGER)(ANTLR3_TOKEN_EOF),
  },
  {"select not 1 between -5 and 5", "HIGH_NOT_PRECEDENCE", 0,
    list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(BETWEEN_SYMBOL)(NOT2_SYMBOL)(INTEGER)(MINUS_OPERATOR)(INTEGER)(AND_SYMBOL)(INTEGER)(ANTLR3_TOKEN_EOF),
  },
  // NO_BACKSLASH_ESCAPES
  {"select \"abc \\\"def\"", "", 0,
    list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(STRING_TOKEN)(DOUBLE_QUOTED_TEXT)(ANTLR3_TOKEN_EOF),
  },
  {"select \"abc \\\"def\"", "NO_BACKSLASH_ESCAPES", 1,
    list_of(SELECT_SYMBOL) (SELECT_EXPR_TOKEN)(EXPRESSION_TOKEN)(STRING_TOKEN)(DOUBLE_QUOTED_TEXT)(IDENTIFIER)(ANTLR3_TOKEN_EOF),
  },

  // TODO: add tests for sql modes that are synonyms for a combination of the base modes.
};

TEST_FUNCTION(30)
{
  for (unsigned int i = 0; i < sizeof(sql_mode_test_data) / sizeof(sql_mode_test_data[0]); i++)
    ensure(base::strfmt("30.%i SQL_MODE test", i),
      parse_and_compare(sql_mode_test_data[i].query, 50610, sql_mode_test_data[i].modes,
        _charsets, sql_mode_test_data[i].tokens, sql_mode_test_data[i].errors));
}

/**
 * Tests the parser's string concatenation feature.
 */
TEST_FUNCTION(35)
{
  std::string sql = "select \"abc\" \"def\" 'ghi''\\n\\z'";

  MySQLRecognizer recognizer(50610, "", _charsets);
  recognizer.parse(sql.c_str(), sql.size(), true, PuGeneric);
  ensure_equals("35.1 String concatenation", recognizer.error_info().size(), 0U);
  
  MySQLRecognizerTreeWalker walker = recognizer.tree_walker();
  ensure("35.2 String concatenation", walker.advance_to_type(STRING_TOKEN, true));
  ensure_equals("35.3 String concatenation", walker.token_text(), "abcdefghi'\nz");
}

// TODO: create tests for all server version dependent features.

// TODO: create tests for restricted content parsing.

END_TESTS;

//----------------------------------------------------------------------------------------------------------------------

