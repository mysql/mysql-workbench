/*
 * Copyright (c) 2013, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "wb_helpers.h"

#include "mforms/code_editor.h"

#include "grtdb/db_helpers.h"
#include "sqlide/wb_sql_editor_form.h"
#include "sqlide/wb_sql_editor_help.h"

#include "tut_mysql_versions.h"

using namespace grt;
using namespace wb;
using namespace sql;
using namespace bec;

struct HelpTestEntry
{
  size_t version_first;  //  First supported version for this entry
  size_t version_last;   //  Last supported version for this entry
  std::string query;
  size_t line;
  size_t offset;
  std::string topic;
  unsigned testLineNumber;  // The line number of the test entry for reporting.
};

BEGIN_TEST_DATA_CLASS(wb_sql_editor_help_test)
public:
  WBTester *_tester;
  help::HelpContext *_helpContext;
  db_mysql_CatalogRef _catalog;
  unsigned long _version;

//----------------------------------------------------------------------------------------------------------------------

void checkTopics(size_t start, const std::vector<HelpTestEntry> entries)
{
  for (size_t i = start; i < entries.size(); i++)
  {
    // Ignore disabled test cases or those defined only for a higher server version.
    if (entries[i].version_first > _version || entries[i].version_last < _version)
      continue;

    // If there's no query given then scan backwards and use the first query we can.
    std::string statement = entries[i].query;
    if (statement.empty() && i > 0)
    {
      ssize_t j = i;
      while (--j >= 0)
        if (!entries[j].query.empty())
        {
          statement = entries[j].query;
          break;
        }
    }
    std::pair<size_t, size_t> caret(entries[i].offset, entries[i].line); // column, row

    std::string message = base::strfmt("Test %lu (line: %u), topics differ", i, entries[i].testLineNumber);
    std::string topic = help::DbSqlEditorContextHelp::get()->helpTopicFromPosition(_helpContext, statement, caret);
#if VERBOSE_TESTING
    std::cout << "Iteration     : " << i << std::endl
              << "Current topic : " << topic << std::endl
              << "Expected topic: " << entries[i].topic << std::endl
              << "Query         : " << entries[i].query << std::endl
              << std::endl
    ;
#endif
    ensure_equals(message, base::tolower(topic), base::tolower(entries[i].topic));
  }
}

//----------------------------------------------------------------------------------------------------------------------

TEST_DATA_CONSTRUCTOR(wb_sql_editor_help_test) : _version(0)
{
  bec::GRTManager::get();
  _tester = new WBTester();

  populate_grt(*_tester);
  _catalog = createEmptyCatalog();
  GrtVersionRef version = _catalog->version();
  _version = (unsigned long)(version->majorNumber() * 10000 + version->minorNumber() * 100 + version->releaseNumber());

  _helpContext = new help::HelpContext(_tester->get_rdbms()->characterSets(), "", _version);

  // Wait for the help to load its data.
  while (!help::DbSqlEditorContextHelp::helpReady()) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

END_TEST_DATA_CLASS;

TEST_MODULE(wb_sql_editor_help_test, "sql editor help test");

//----------------------------------------------------------------------------------------------------------------------

static std::vector<HelpTestEntry> singleTokenTests = {
  // Note: queries don't need to be valid, but must be in a form which allows to find a start context for the help search.
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "", 0, 0, "", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "blah foo bar nonsense", 0, 10, "", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "blah fóo µbar nonsense", 0, 10, "", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select !a = b", 0, 7, "!", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select 1 where a != b", 0, 18, "!=", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "sel from b where a % b", 0, 19, "%", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "set a = b & c", 0, 10, "&", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select where a && b", 0, 16, "AND", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a*b", 0, 4, "*", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a ++++++++ b", 0, 10, "+", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do 3-5", 0, 4, "- BINARY", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a +-+-+-+ -b", 0, 13, "- UNARY", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select a / b", 0, 9, "/", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "where a < b < c", 0, 12, "<", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a /* is much smaller than */ << b", 0, 32, "<<", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a <= b", 0, 6, "<=", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a<=>b", 0, 6, "<=>", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "set a = b", 0, 6, "assign-equal", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do is a > b?", 0, 8, ">", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "only when a >= b", 0, 13, ">=", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "/* Move it right */a /* now */>> b", 0, 30, ">>", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = abs(b)", 0, 8, "ABS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = acos(b)", 0, 8, "ACOS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = adddate(b)", 0, 8, "ADDDATE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = addtime(b)", 0, 8, "ADDTIME", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do aes_decrypt(aes_encrypt(blah))", 0, 3, "AES_DECRYPT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do aes_decrypt(aes_encrypt(blah))", 0, 23, "AES_ENCRYPT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do area(poly)", 0, 9, "AREA", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do asbinary(g)", 0, 8, "ASBINARY", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do AsWKB(g)", 0, 3, "ASBINARY", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select ascii(a)", 0, 7, "ASCII", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select a + asin(b)", 0, 17, "ASIN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "make this a =b", 0, 12, "ASSIGN-EQUAL", __LINE__ }, // "assign-equal" is the same as "assign-value" even tho one would expect differently.
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "set world:=home", 0, 10, "ASSIGN-VALUE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select astext(me)", 0, 10, "ASTEXT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = atan(b)", 0, 8, "ATAN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = atan2(b)", 0, 8, "ATAN2", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id int auto_increment)", 0, 36, "AUTO_INCREMENT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "set a = avg(b)", 0, 8, "AVG", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "begin this work and do not stop", 0, 0, "BEGIN END", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do end transaction", 0, 5, "BEGIN END", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select benchmark(a)", 0, 15, "BENCHMARK", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do decide between me and him", 0, 12, "BETWEEN AND", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id bigint)", 0, 20, "BIGINT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select bin(123)", 0, 8, "BIN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id binary(3))", 0, 20, "BINARY", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select binary a", 0, 7, "BINARY OPERATOR", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "binlog \"codecodecode\"", 0, 3, "BINLOG", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id bit)", 0, 20, "BIT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = bit_and(b)", 0, 8, "BIT_AND", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = bit_count(b)", 0, 8, "BIT_COUNT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = bit_length(b)", 0, 8, "BIT_LENGTH", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select bit_or(a)", 0, 8, "BIT_OR", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = bit_xor(b)", 0, 8, "BIT_XOR", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id blob)", 0, 20, "BLOB", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id boolean)", 0, 20, "BOOLEAN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "call me first", 0, 0, "CALL", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select cast (b as string)", 0, 20, "CAST", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = ceil(b)", 0, 8, "CEIL", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = ceiling(b)", 0, 8, "CEILING", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id national char(5))", 0, 20, "CHAR", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id char byte)", 0, 21, "CHAR BYTE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT CHARSET(CHAR(0x65)), CHARSET(CHAR(0x65 USING utf8));", 0, 16, "CHAR FUNCTION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = character_length(b)", 0, 9, "CHARACTER_LENGTH", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT CHARSET(CONVERT('abc' USING utf8));", 0, 8, "CHARSET", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = char_length(b)", 0, 9, "CHAR_LENGTH", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create function a () returns int begin close  a; end", 0, 45, "CLOSE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT COALESCE(NULL,1);", 0, 10, "COALESCE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT COERCIBILITY('abc' COLLATE latin1_swedish_ci);", 0, 15, "COERCIBILITY", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT COLLATION(_utf8'abc');", 0, 10, "COLLATION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT LENGTH(COMPRESS(REPEAT('a',16)));", 0, 20, "COMPRESS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT CONCAT('My', 'S', 'QL');", 0, 10, "CONCAT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT CONCAT_WS(',','First name',NULL,'Last Name');", 0, 10, "CONCAT_WS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT CONNECTION_ID();", 0, 8, "CONNECTION_ID", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select contains(a, b)", 0, 8, "CONTAINS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = conv(b)", 0, 8, "CONV", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = convert(b, char)", 0, 8, "CONVERT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = convert(b using utf8)", 0, 25, "CONVERT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = convert_tz(b)", 0, 8, "CONVERT_TZ", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = cos(b)", 0, 8, "COS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = cot(b)", 0, 8, "COT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select count(1) from dual", 0, 7, "COUNT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT COUNT(DISTINCT results) FROM student;", 0, 7, "COUNT DISTINCT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT COUNT(DISTINCT results) FROM student;", 0, 15, "COUNT DISTINCT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = crc32()", 0, 8, "CRC32", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do @b := -1, a = crosses(b)", 0, 26, "CROSSES", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = curdate()", 0, 8, "CURDATE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = current_date()", 0, 11, "CURRENT_DATE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = current_time()", 0, 13, "CURRENT_TIME", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select current_timestamp()", 0, 10, "CURRENT_TIMESTAMP", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "set a := current_user()", 0, 11, "CURRENT_USER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = curtime()", 0, 9, "CURTIME", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select database()", 0, 10, "DATABASE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (d date)", 0, 20, "DATE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT DATE('2003-12-31 01:02:03');", 0, 7, "DATE FUNCTION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select datediff(a, b)", 0, 10, "DATEDIFF", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select datetime(b)", 0, 10, "DATETIME", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select date_add(a, interval b minute)", 0, 29, "DATE_ADD", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select date_format(a)", 0, 10, "DATE_FORMAT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select @a := date_sub(a, interval b minute)", 0, 35, "DATE_SUB", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select a, b, day(c + 1)", 0, 22, "DAY", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = dayname(b)", 0, 8, "DAYNAME", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = dayofmonth(b)", 0, 8, "DAYOFMONTH", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = dayofweek(b)", 0, 8, "DAYOFWEEK", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = dayofyear(b)", 0, 8, "DAYOFYEAR", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id dec(5,3))", 0, 20, "DEC", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id decimal(5,3))", 0, 20, "DECIMAL", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do decode(str1, str2)", 0, 6, "DECODE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "UPDATE t SET i = DEFAULT(i)+1 WHERE id < 100;", 0, 23, "DEFAULT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT DEGREES(PI());", 0, 8, "DEGREES", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "delete from a where b = c;", 0, 3, "DELETE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_5_36, "describe select 1", 0, 4, "DESCRIBE", __LINE__ },
  { MYSQL_VERSION_5_5_36, MYSQL_VERSION_HIGHER, "describe select 1", 0, 4, "EXPLAIN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select a = des_decrypt(b)", 0, 12, "DES_DECRYPT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select a = des_encrypt(b)", 0, 12, "DES_ENCRYPT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select a div b", 0, 9, "DIV", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do release_lock()", 0, 1, "DO", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id double)", 0, 20, "DOUBLE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id double precision)", 0, 20, "DOUBLE PRECISION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select 1 from dual", 0, 15, "DUAL", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT ELT(1, 'ej', 'Heja', 'hej', 'foo');", 0, 9, "ELT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select a = encode(b)", 0, 12, "ENCODE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select a = encrypt(b)", 0, 12, "ENCRYPT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id enum('a', 'b'))", 0, 20, "ENUM", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "SELECT Dimension(GeomFromText('LineString(1 1,2 2)'));", 0, 10, "DIMENSION", __LINE__},
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "select @a := disjoint(g1, g2);", 0, 20, "DISJOINT", __LINE__},
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "SELECT AsText(EndPoint(GeomFromText(@ls)));", 0, 15, "ENDPOINT", __LINE__},
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "SELECT AsText(Envelope(GeomFromText('LineString(1 1,2 2)')));", 0, 15, "ENVELOPE", __LINE__},
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "select equals(g1, g2)", 0, 8, "EQUALS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "SELECT AsText(ExteriorRing(GeomFromText(@poly)));", 0, 16, "EXTERIORRING", __LINE__},
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "SELECT ST_Dimension(ST_GeomFromText('LineString(1 1,2 2)'));", 0, 10, "ST_DIMENSION", __LINE__},
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "select @a := ST_disjoint(g1, g2);", 0, 20, "ST_DISJOINT", __LINE__},
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "SELECT ST_AsText(ST_EndPoint(ST_GeomFromText(@ls)));", 0, 15, "ST_ENDPOINT", __LINE__},
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "SELECT ST_AsText(ST_Envelope(ST_GeomFromText('LineString(1 1,2 2)')));", 0, 15, "ST_ENVELOPE", __LINE__},
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "select ST_equals(g1, g2)", 0, 8, "ST_EQUALS", __LINE__},
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "SELECT ST_AsText(ST_ExteriorRing(ST_GeomFromText(@poly)));", 0, 16, "ST_EXTERIORRING", __LINE__},
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select a = exp(b)", 0, 12, "EXP", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "explain myself", 0, 2, "EXPLAIN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT EXPORT_SET(5,'Y','N',',',4);", 0, 8, "EXPORT_SET", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT EXTRACT(YEAR FROM '2009-07-02');", 0, 10, "EXTRACT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select a = extractvalue(b)", 0, 12, "EXTRACTVALUE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create procedure a() begin fetch it into water; end;", 0, 45, "FETCH", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT FIELD('ej', 'Hej', 'ej', 'Heja', 'hej', 'foo');", 0, 10, "FIELD", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select find_in_set(1, 2, 3)", 0, 10, "FIND_IN_SET", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id float)", 0, 20, "FLOAT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = floor(b)", 0, 8, "FLOOR", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "flush logs;", 0, 3, "FLUSH", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = format(a, b, c)", 0, 8, "FORMAT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select found_rows()", 0, 10, "FOUND_ROWS", __LINE__ },
  { MYSQL_VERSION_5_6, MYSQL_VERSION_HIGHER, "do a = from_base64(b)", 0, 8, "FROM_BASE64", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = from_days(b)", 0, 8, "FROM_DAYS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = from_unixtime(b)", 0, 8, "FROM_UNIXTIME", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = geomcollfromtext(b)", 0, 8, "GEOMCOLLFROMTEXT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = geomcollfromwkb(b)", 0, 8, "GEOMCOLLFROMWKB", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "CREATE TABLE geom (g GEOMETRY);", 0, 25, "GEOMETRY", __LINE__ }, // Topic "geometry" doesn't contain much. "Hierarchy" is better suited.
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = geometrycollection(b, c, d, e)", 0, 8, "GEOMETRYCOLLECTION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "SELECT AsText(GeometryN(GeomFromText(@gc),1));", 0, 15, "GEOMETRYN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "SELECT GeometryType(GeomFromText('POINT(1 1)'));", 0, 15, "GEOMETRYTYPE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "SELECT GeometryType(GeomFromText('POINT(1 1)'));", 0, 25, "GEOMFROMTEXT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "SELECT GeometryType(GeometryFromText('POINT(1 1)'));", 0, 25, "GEOMFROMTEXT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "do a = geomfromwkb(b, 'text')", 0, 8, "GEOMFROMWKB", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "do a = geometryfromwkb(b, 'text')", 0, 8, "GEOMFROMWKB", __LINE__ },
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "SELECT ST_AsText(ST_GeometryN(ST_GeomFromText(@gc),1));", 0, 15, "ST_GEOMETRYN", __LINE__},
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "SELECT ST_GeometryType(ST_GeomFromText('POINT(1 1)'));", 0, 15, "ST_GEOMETRYTYPE", __LINE__},
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "SELECT ST_GeometryType(ST_GeomFromText('POINT(1 1)'));", 0, 25, "ST_GEOMFROMTEXT", __LINE__},
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "SELECT ST_GeometryType(ST_GeometryFromText('POINT(1 1)'));", 0, 25, "ST_GEOMFROMTEXT", __LINE__},
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "a = st_geomfromwkb(b, 'text')", 0, 5, "ST_GEOMFROMWKB", __LINE__},
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "a = st_geometryfromwkb(b, 'text')", 0, 5, "ST_GEOMFROMWKB", __LINE__},
  { MYSQL_VERSION_5_6, MYSQL_VERSION_HIGHER, "create procedure a() begin get current diagnostics; end", 0, 38, "GET DIAGNOSTICS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT DATE_FORMAT('2003-10-03',GET_FORMAT(DATE,'EUR'));", 0, 34, "GET_FORMAT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT GET_LOCK('lock1',10);", 0, 10, "GET_LOCK", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "SELECT GLength(GeomFromText(@ls));", 0, 10, "GLENGTH", __LINE__ },
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "SELECT ST_GLength(ST_GeomFromText(@ls));", 0, 10, "ST_GLENGTH", __LINE__},
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "grant all to me", 0, 2, "GRANT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT GREATEST(34.0,3.0,5.0,767.0);", 0, 10, "GREATEST", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT student_name,\n  GROUP_CONCAT(test_score)\n  FROM student\n  GROUP BY student_name;",
    1, 5, "GROUP_CONCAT", __LINE__ }, 
  { MYSQL_VERSION_5_6, MYSQL_VERSION_HIGHER, "do a = gtid_subset(b, c)", 0, 8, "GTID_SUBSET", __LINE__ },
  { MYSQL_VERSION_5_6, MYSQL_VERSION_HIGHER, "do a = gtid_subtract(b, c)", 0, 8, "GTID_SUBTRACT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "handler a open as b", 0, 0, "HANDLER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT HEX(255), CONV(HEX(255),16,10);", 0, 9, "HEX", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT HOUR('10:05:03');", 0, 9, "HOUR", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT 2 IN (0,3,5,7);", 0, 10, "IN", __LINE__ },
  { MYSQL_VERSION_5_6, MYSQL_VERSION_HIGHER, "do a = inet6_aton(b)", 0, 8, "INET6_ATON", __LINE__ },
  { MYSQL_VERSION_5_6, MYSQL_VERSION_HIGHER, "do a = inet6_ntoa(b)", 0, 8, "INET6_NTOA", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = inet_aton(b)", 0, 8, "INET_ATON", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = inet_ntoa(b)", 0, 8, "INET_NTOA", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "insert into a set b = c", 0, 8, "INSERT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "install plugin a", 0, 5, "INSTALL PLUGIN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = instr(b)", 0, 8, "INSTR", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id int  )", 0, 23, "INT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id integer)", 0, 20, "INTEGER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = interiorringn(b)", 0, 8, "INTERIORRINGN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = intersects(b, c)", 0, 8, "INTERSECTS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT INTERVAL(23, 1, 15, 17, 30, 44, 200);", 0, 10, "INTERVAL", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT 1 IS TRUE, 0 IS FALSE, NULL IS UNKNOWN;", 0, 9, "IS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = isempty(b)", 0, 8, "ISEMPTY", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select isnull(a) ", 0, 9, "ISNULL", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "set transaction isolation level 1", 0, 20, "ISOLATION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select issimple(a)", 0, 9, "ISSIMPLE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select is_free_lock(a)", 0, 9, "IS_FREE_LOCK", __LINE__ },
  { MYSQL_VERSION_5_6, MYSQL_VERSION_HIGHER, "select is_ipv4()", 0, 9, "IS_IPV4", __LINE__ },
  { MYSQL_VERSION_5_6, MYSQL_VERSION_HIGHER, "select is_ipv4_compat()", 0, 9, "IS_IPV4_COMPAT", __LINE__ },
  { MYSQL_VERSION_5_6, MYSQL_VERSION_HIGHER, "select is_ipv4_mapped()", 0, 9, "IS_IPV4_MAPPED", __LINE__ },
  { MYSQL_VERSION_5_6, MYSQL_VERSION_HIGHER, "select is_ipv6()", 0, 9, "IS_IPV6", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select is_used_lock(a)", 0, 9, "IS_USED_LOCK", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create procedure a() begin iterate a", 0, 35, "ITERATE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT left_tbl.*\n"
    "  FROM left_tbl LEFT JOIN right_tbl ON left_tbl.id = right_tbl.id\n"
    "  WHERE right_tbl.id IS NULL;", 1, 23, "JOIN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "kill _connection 1", 0, 0, "KILL", __LINE__ },
  { MYSQL_VERSION_5_6, MYSQL_VERSION_HIGHER, "CREATE PROCEDURE doiterate(p1 INT)\n"
    "BEGIN\n"
    "  label1: LOOP\n"
    "    SET p1 = p1 + 1;\n"
    "    IF p1 < 10 THEN\n"
    "      ITERATE label1;\n"
    "    END IF;\n"
    "    LEAVE label1;\n"
    "  END LOOP label1;\n"
    "  SET @x = p1;\n"
    "END;", 2, 6, "LABELS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT LAST_DAY('2003-02-05');", 0, 9, "LAST_DAY", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select last_insert_id()", 0, 9, "LAST_INSERT_ID", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do  a = lcase(b)", 0, 8, "LCASE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = least(b)", 0, 8, "LEAST", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create procedure a()  leave a", 0, 29, "LEAVE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = left(b, 1)", 0, 8, "LEFT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = length(b)", 0, 8, "LENGTH", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT 'David!' LIKE '%D%v%';", 0, 18, "LIKE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = linefromtext(b)", 0, 8, "LINEFROMTEXT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = linefromwkb(b)", 0, 8, "LINEFROMWKB", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = linestring(b, c, d)", 0, 8, "LINESTRING", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = ln(b)", 0, 8, "LN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select localtime()", 0, 9, "LOCALTIME", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select localtimestamp()", 0, 9, "LOCALTIMESTAMP", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT LOCATE('bar', 'foobarbar');", 0, 9, "LOCATE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "lock table a", 0, 2, "LOCK", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = log(b)", 0, 8, "LOG", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = log10(b)", 0, 8, "LOG10", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = log2(b)", 0, 8, "LOG2", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id longblob)", 0, 20, "LONGBLOB", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id longtext)", 0, 20, "LONGTEXT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "CREATE PROCEDURE doiterate(p1 INT)\n"
    "BEGIN\n"
    "  label1: LOOP\n"
    "    SET p1 = p1 + 1;\n"
    "    IF p1 < 10 THEN\n"
    "      ITERATE label1;\n"
    "    END IF;\n"
    "    LEAVE label1;\n"
    "  END LOOP label1;\n"
    "  SET @x = p1;\n"
    "END;", 8, 7, "LOOP", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = lower(b)", 0, 8, "LOWER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = lpad(b)", 0, 8, "LPAD", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = ltrim(b)", 0, 8, "LTRIM", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = makedate(b)", 0, 8, "MAKEDATE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = maketime(b)", 0, 8, "MAKETIME", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = make_set(b)", 0, 8, "MAKE_SET", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do master_pos_wait(1, 2)", 0, 24, "MASTER_POS_WAIT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT id, body, MATCH (title,body) AGAINST\n"
    "  ('Security implications of running MySQL as root'\n"
    "  IN NATURAL LANGUAGE MODE) AS score\n"
    "  FROM articles WHERE MATCH (title,body) AGAINST\n"
    "  ('Security implications of running MySQL as root'\n"
    "  IN NATURAL LANGUAGE MODE);", 3, 25, "MATCH AGAINST", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = max(b)", 0, 8, "MAX", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT MBRContains(@g1,@g2), MBRContains(@g2,@g1);", 0, 35, "MBRCONTAINS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = mbrdisjoint(a, b)", 0, 8, "MBRDISJOINT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = mbrequal(a, b)", 0, 8, "MBREQUAL", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = mbrintersects(b, c)", 0, 8, "MBRINTERSECTS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = mbroverlaps(b, c)", 0, 8, "MBROVERLAPS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = mbrtouches(a, b)", 0, 8, "MBRTOUCHES", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = mbrwithin(b, c)", 0, 8, "MBRWITHIN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT MD5('testing');", 0, 9, "MD5", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id mediumblob)", 0, 20, "MEDIUMBLOB", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id mediumint)", 0, 20, "MEDIUMINT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id mediumtext)", 0, 20, "MEDIUMTEXT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "CREATE TABLE total (\n"
    "    a INT NOT NULL AUTO_INCREMENT,\n"
    "    message CHAR(20), INDEX(a))\n"
    "    ENGINE=MERGE UNION=(t1,t2) INSERT_METHOD=LAST;", 3, 13, "MERGE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT MICROSECOND('12:00:00.123456');", 0, 9, "MICROSECOND", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = mid(b, a)", 0, 8, "MID", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = min(b)", 0, 8, "MIN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT MINUTE('12:00:00.123456');", 0, 9, "MINUTE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "do a = mlinefromtext(b)", 0, 8, "MLINEFROMTEXT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "do a = mlinefromwkb(b)", 0, 8, "MLINEFROMWKB", __LINE__ },
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "a = ST_mlinefromtext(b)", 0, 5, "ST_MLINEFROMTEXT", __LINE__},
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "a = ST_mlinefromwkb(b)", 0, 5, "ST_MLINEFROMWKB", __LINE__},
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select * from a where a mod b > 0", 0, 25, "MOD", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT MONTH('12:00:00.123456');", 0, 10, "MONTH", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT MONTHNAME('12:00:00.123456');", 0, 10, "MONTHNAME", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "do a = mpointfromtext(b)", 0, 8, "MPOINTFROMTEXT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "do a = mpointfromwkb(b)", 0, 8, "MPOINTFROMWKB", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "do a = mpolyfromtext(b)", 0, 8, "MPOLYFROMTEXT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "do a = mpolyfromwkb(b)", 0, 8, "MPOLYFROMWKB", __LINE__ },
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "a = st_mpointfromtext(b)", 0, 5, "st_MPOINTFROMTEXT", __LINE__},
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "a = st_mpointfromwkb(b)", 0, 5, "st_MPOINTFROMWKB", __LINE__},
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "a = st_mpolyfromtext(b)", 0, 5, "st_MPOLYFROMTEXT", __LINE__},
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "a = st_mpolyfromwkb(b)", 0, 5, "st_MPOLYFROMWKB", __LINE__},
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = multilinestring(b)", 0, 8, "MULTILINESTRING", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = multipoint(b)", 0, 8, "MULTIPOINT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = multipolygon(b)", 0, 8, "MULTIPOLYGON", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = name_const(a, b)", 0, 8, "NAME_CONST", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select now()", 0, 9, "NOW", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT NULLIF(1,1);", 0, 10, "NULLIF", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "do a = numgeometries(b)", 0, 8, "NUMGEOMETRIES", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "do a = numinteriorrings(b)", 0, 8, "NUMINTERIORRINGS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "do a = numpoints(b)", 0, 8, "NUMPOINTS", __LINE__ },
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "a = st_numgeometries(b)", 0, 5, "st_NUMGEOMETRIES", __LINE__},
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "a = st_numinteriorrings(b)", 0, 5, "st_NUMINTERIORRINGS", __LINE__},
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "a = st_numpoints(b)", 0, 5, "st_NUMPOINTS", __LINE__},
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT OCT(12);", 0, 9, "OCT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = octet_length(b)", 0, 8, "OCTET_LENGTH", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = old_password(b)", 0, 8, "OLD_PASSWORD", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create procedure a() open    a", 0, 27, "OPEN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = ord(b)", 0, 8, "ORD", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = overlaps(b, c)", 0, 8, "OVERLAPS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT PASSWORD('badpwd');", 0, 10, "PASSWORD", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = period_add(b, c)", 0, 8, "PERIOD_ADD", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = period_diff(b, c)", 0, 8, "PERIOD_DIFF", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT PI();", 0, 8, "PI", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = point(b, c)", 0, 8, "POINT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "do a = pointfromtext(b)", 0, 8, "POINTFROMTEXT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "do a = pointfromwkb(b)", 0, 8, "POINTFROMWKB", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "do a = pointn(b)", 0, 8, "POINTN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "do a = polyfromtext(b)", 0, 8, "POLYFROMTEXT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "do a = polyfromwkb(b)", 0, 8, "POLYFROMWKB", __LINE__ },
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "a = st_pointfromtext(b)", 0, 5, "st_POINTFROMTEXT", __LINE__},
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "a = st_pointfromwkb(b)", 0, 5, "st_POINTFROMWKB", __LINE__},
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "a = st_pointn(b)", 0, 5, "st_POINTN", __LINE__},
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "a = st_polyfromtext(b)", 0, 5, "st_POLYFROMTEXT", __LINE__},
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "a = st_polyfromwkb(b)", 0, 5, "st_POLYFROMWKB", __LINE__},
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = polygon(p1, p2, p3)", 0, 8, "POLYGON", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = position(b in x)", 0, 8, "POSITION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = pow(2, 2)", 0, 8, "POW", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = power(2, 2)", 0, 8, "POWER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "prepare stmt from select * from ?", 0, 3, "PREPARE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT QUARTER('12:00:00.123456');", 0, 10, "QUARTER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT QUOTE('Don\\'t!');", 0, 10, "QUOTE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = radians(b)", 0, 8, "RADIANS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = rand(b)", 0, 8, "RAND", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT 'a' REGEXP 'A', 'a' REGEXP BINARY 'A';", 0, 30, "REGEXP", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "replace into a values(default)", 0, 3, "REPLACE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT REPLACE('www.mysql.com', 'w', 'Ww');", 0, 10, "REPLACE FUNCTION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "reset", 0, 0, "RESET", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "reset master", 0, 8, "RESET MASTER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "reset slave", 0, 8, "RESET SLAVE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "resignal sqlstate 1", 0, 0, "RESIGNAL", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create procedure y() return", 0, 27, "RETURN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT REVERSE('abc');", 0, 10, "REVERSE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "revoke all from mike", 0, 0, "REVOKE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = right(b, c)", 0, 8, "RIGHT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = round(b)", 0, 8, "ROUND", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select row_count();", 0, 10, "ROW_COUNT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = rpad(b)", 0, 8, "RPAD", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = rtrim(b)", 0, 8, "RTRIM", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "savepoint first", 0, 8, "SAVEPOINT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select schema()", 0, 10, "SCHEMA", __LINE__ }, // Can never appear, because the lexer automatically maps it to database.
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT SECOND('12:00:00.123456');", 0, 10, "SECOND", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = sec_to_time(a)", 0, 8, "SEC_TO_TIME", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select session_user()", 0, 10, "SESSION_USER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "set @var = 1", 0, 8, "SET", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = sha1(b)", 0, 8, "SHA1", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = sha2(b)", 0, 8, "SHA2", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show 1", 0, 5, "SHOW", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = sign(b)", 0, 8, "SIGN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "signal you, signal me", 0, 15, "SIGNAL", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = sin(b)", 0, 8, "SIN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select sleep(1000);", 0, 10, "SLEEP", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id smallint)", 0, 20, "SMALLINT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = soundex(b)", 0, 8, "SOUNDEX", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = space(b)", 0, 8, "SPACE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "ALTER TABLE geom ADD SPATIAL INDEX(g);", 0, 25, "SPATIAL", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = sqrt(b)", 0, 8, "SQRT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "do a = srid(b)", 0, 8, "SRID", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "SELECT AsText(StartPoint(GeomFromText(@ls)));", 0, 20, "STARTPOINT", __LINE__ },
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "a = st_srid(b)", 0, 5, "ST_SRID", __LINE__},
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "SELECT ST_AsText(ST_StartPoint(ST_GeomFromText(@ls)));", 0, 20, "ST_STARTPOINT", __LINE__},
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = std(b)", 0, 8, "STD", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = stddev(b)", 0, 8, "STDDEV", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = stddev_pop(b)", 0, 8, "STDDEV_POP", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = stddev_samp(b)", 0, 8, "STDDEV_SAMP", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "stop slave io_thread", 0, 15, "STOP SLAVE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = strcmp(b)", 0, 8, "STRCMP", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = str_to_date(b)", 0, 8, "STR_TO_DATE", __LINE__ },
  { MYSQL_VERSION_5_6, MYSQL_VERSION_HIGHER, "do a = st_contains(b)", 0, 8, "ST_CONTAINS", __LINE__ },
  { MYSQL_VERSION_5_6, MYSQL_VERSION_HIGHER, "do a = st_crosses(b)", 0, 8, "ST_CROSSES", __LINE__ },
  { MYSQL_VERSION_5_6, MYSQL_VERSION_HIGHER, "do a = st_disjoint(b)", 0, 8, "ST_DISJOINT", __LINE__ },
  { MYSQL_VERSION_5_6, MYSQL_VERSION_HIGHER, "do a = st_equals(b)", 0, 8, "ST_EQUALS", __LINE__ },
  { MYSQL_VERSION_5_6, MYSQL_VERSION_HIGHER, "do a = st_intersects(b)", 0, 8, "ST_INTERSECTS", __LINE__ },
  { MYSQL_VERSION_5_6, MYSQL_VERSION_HIGHER, "do a = st_overlaps(b)", 0, 8, "ST_OVERLAPS", __LINE__ },
  { MYSQL_VERSION_5_6, MYSQL_VERSION_HIGHER, "do a = st_touches(b)", 0, 8, "ST_TOUCHES", __LINE__ },
  { MYSQL_VERSION_5_6, MYSQL_VERSION_HIGHER, "do a = st_within(b)", 0, 8, "ST_WITHIN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = subdate(b)", 0, 8, "SUBDATE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = substr(b)", 0, 8, "SUBSTR", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = substring(b)", 0, 8, "SUBSTRING", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = substring_index(b)", 0, 8, "SUBSTRING_INDEX", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = subtime(b)", 0, 8, "SUBTIME", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select sum(a) from b", 0, 7, "SUM", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select sysdate()", 0, 9, "SYSDATE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select system_user()", 0, 9, "SYSTEM_USER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = tan(b)", 0, 8, "TAN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id text)", 0, 20, "TEXT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id time)", 0, 20, "TIME", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT TIME('2003-12-31 01:02:03');", 0, 9, "TIME FUNCTION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = timediff(b)", 0, 8, "TIMEDIFF", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id timestamp)", 0, 20, "TIMESTAMP", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT TIMESTAMP('2003-12-31');", 0, 9, "TIMESTAMP FUNCTION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = timestampadd(b, c)", 0, 8, "TIMESTAMPADD", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = timestampdiff(b, c)", 0, 8, "TIMESTAMPDIFF", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = time_format(b)", 0, 8, "TIME_FORMAT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = time_to_sec(b)", 0, 8, "TIME_TO_SEC", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id tinyblob)", 0, 20, "TINYBLOB", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id tinyint)", 0, 20, "TINYINT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id tinytext)", 0, 20, "TINYTEXT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = touches(g1, g2)", 0, 8, "TOUCHES", __LINE__ },
  { MYSQL_VERSION_5_6, MYSQL_VERSION_HIGHER, "do a = to_base64(b)", 0, 8, "TO_BASE64", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = to_days(b)", 0, 8, "TO_DAYS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = to_seconds(b)", 0, 8, "TO_SECONDS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = trim(b)", 0, 8, "TRIM", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select a is not true", 0, 19, "TRUE FALSE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = truncate(b)", 0, 8, "TRUNCATE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "/* Make it short */truncate table a", 0, 30, "TRUNCATE TABLE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = ucase(b)", 0, 8, "UCASE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = uncompress(b)", 0, 8, "UNCOMPRESS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = uncompressed_length(b)", 0, 8, "UNCOMPRESSED_LENGTH", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = unhex(5)", 0, 8, "UNHEX", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "uninstall /* do it */ plugin", 0, 25, "UNINSTALL PLUGIN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select 1 from a union select 2 from b union select 3 from c", 0, 40, "UNION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select unix_timestamp()", 0, 9, "UNIX_TIMESTAMP", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "update a set b = c", 0, 0, "UPDATE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT\n""  UpdateXML('<a><b>ccc</b><d></d></a>', '/a', '<e>fff</e>') AS val1", 1, 3, "UPDATEXML", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = upper(b)", 0, 8, "UPPER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "use sakila;", 0, 10, "USE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select user()", 0, 9, "USER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select utc_date()", 0, 9, "UTC_DATE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select utc_time()", 0, 9, "UTC_TIME", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select utc_timestamp()", 0, 9, "UTC_TIMESTAMP", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select uuid()", 0, 9, "UUID", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select uuid_short()", 0, 9, "UUID_SHORT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "INSERT INTO table (a,b,c) VALUES (1,2,3),(4,5,6)", 0, 30, "VALUES", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id varbinary)", 0, 20, "VARBINARY", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id varchar)", 0, 20, "VARCHAR", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = variance(b)", 0, 8, "VARIANCE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = var_pop(b)", 0, 8, "VAR_POP", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = var_samp(b)", 0, 8, "VAR_SAMP", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select version()", 0, 9, "VERSION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT WEEK('12:00:00.123456');", 0, 9, "WEEK", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT WEEKDAY('12:00:00.123456');", 0, 9, "WEEKDAY", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT WEEKOFYEAR('12:00:00.123456');", 0, 9, "WEEKOFYEAR", __LINE__ },
  { MYSQL_VERSION_5_6, MYSQL_VERSION_HIGHER, "do a = weight_string(b)", 0, 8, "WEIGHT_STRING", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "CREATE PROCEDURE dowhile()\n"
    "  BEGIN\n"
    "    DECLARE v1 INT DEFAULT 5;\n\n"
    "    WHILE v1 > 0 DO\n"
    "      SET v1 = v1 - 1;\n"
    "    END WHILE;\n"
    "  END;", 4, 7, "WHILE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "do a = within(b, c)", 0, 8, "WITHIN", __LINE__ },
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "do a = st_within(b, c)", 0, 8, "ST_WITHIN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do x = 1", 0, 3, "x", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "SELECT X(POINT(56.7, 53.34));", 0, 7, "X", __LINE__ },
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "SELECT ST_X(POINT(56.7, 53.34));", 0, 7, "ST_X", __LINE__},
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select a xor b", 0, 10, "XOR", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do y = 1", 0, 3, "y", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do z = 1", 0, 3, "DO", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_5_7_6, "SELECT Y(POINT(56.7, 53.34));", 0, 7, "Y", __LINE__ },
  { MYSQL_VERSION_5_7_6, MYSQL_VERSION_HIGHER, "SELECT ST_Y(POINT(56.7, 53.34));", 0, 7, "ST_Y", __LINE__},
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT YEAR('12:00:00.123456');", 0, 9, "YEAR", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id year(4))", 0, 20, "YEAR DATA TYPE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT YEARWEEK('12:00:00.123456');", 0, 9, "YEARWEEK", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = b ^ c", 0, 9, "^", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = b | c", 0, 9, "|", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = b || c", 0, 9, "||", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "do a = ~b", 0, 7, "~", __LINE__}
};

/**
 *  Single token topics or those derived from a single token.
 */
TEST_FUNCTION(5)
{
  checkTopics(0, singleTokenTests);
}

//----------------------------------------------------------------------------------------------------------------------

static std::vector<HelpTestEntry> multiTokenTests = {
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "alter database sakila", 0, 0, "ALTER DATABASE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "alter database sakila", 0, 10, "ALTER DATABASE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "alter event a", 0, 0, "ALTER EVENT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "alter event a", 0, 10, "ALTER EVENT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "alter function a", 0, 0, "ALTER FUNCTION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "alter function a", 0, 10, "ALTER FUNCTION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "alter procedure a", 0, 0, "ALTER PROCEDURE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "alter procedure a", 0, 10, "ALTER PROCEDURE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "alter server a", 0, 0, "ALTER SERVER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "alter server a", 0, 10, "ALTER SERVER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "alter table a", 0, 0, "ALTER TABLE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "alter table a", 0, 0, "ALTER TABLE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "alter user a", 0, 0, "ALTER USER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "alter user a", 0, 10, "ALTER USER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "alter view a", 0, 0, "ALTER VIEW", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "alter view a", 0, 10, "ALTER VIEW", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "analyze", 0, 7, "ANALYZE TABLE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "analyze table", 0, 0, "ANALYZE TABLE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "analyze table", 0, 10, "ANALYZE TABLE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "analyze no_write_to_bin_log table a", 0, 0, "ANALYZE TABLE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "analyze no_write_to_bin_log table a", 0, 35, "ANALYZE TABLE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "cache index actor partition (all) index (idx_actor_last_name, primary, primary) in sakila",
    0, 8, "CACHE INDEX", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT CASE WHEN 1 > 0 THEN 'true' ELSE 'false' END;", 0, 15, "CASE OPERATOR", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create procedure a() CASE 1 WHEN 1 THEN 'one'\n"
    "   WHEN 2 THEN 'two' ELSE 'more' END CASE;", 1, 27, "CASE STATEMENT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "change master to master_bind = 'blah', relay_log_pos = 1, ignore_server_ids = (1, 2, 3, 4)",
    0, 20, "CHANGE MASTER TO", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "check table sakila.actor, sakila.country for upgrade", 0, 20, "CHECK TABLE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "checksum table customer extended", 0, 20, "CHECKSUM TABLE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create schema a", 0, 0, "CREATE DATABASE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create database a", 0, 0, "CREATE DATABASE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create database a", 0, 8, "CREATE DATABASE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create event a", 0, 0, "CREATE EVENT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create definer = current_user event if not exists a on schedule every 1 second select 1",
    0, 45, "CREATE EVENT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create function sakila.f1 (a int)", 0, 0, "CREATE FUNCTION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create function sakila.f1 (a int)", 0, 20, "CREATE FUNCTION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create function f1 returns string", 0, 0, "CREATE FUNCTION UDF", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create aggregate function f1 returns string soname 'blah'", 0, 10, "CREATE FUNCTION UDF", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create online index", 0, 0, "CREATE INDEX", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create offline index", 0, 18, "CREATE INDEX", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create procedure sakila.p1 (in a int)", 0, 0, "CREATE PROCEDURE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create procedure sakila.p1 (in a int)", 0, 20, "CREATE PROCEDURE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create server a", 0, 0, "CREATE SERVER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create server 'a'", 0, 15, "CREATE SERVER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id tinytext)", 0, 0, "CREATE TABLE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id tinytext)", 0, 28, "CREATE TABLE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create trigger a before insert on sakila.t1 for each row", 0, 0, "CREATE TRIGGER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create trigger a before insert on sakila.t1 for each row", 0, 25, "CREATE TRIGGER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create user current_user", 0, 0, "CREATE USER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create user current_user", 0, 8, "CREATE USER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create sql security invoker view sakila.v1 (a, b, c) as select 1", 0, 0, "CREATE VIEW", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create sql security invoker view sakila.v1 (a, b, c) as select 1", 0, 44, "CREATE VIEW", __LINE__ }, 
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "deallocate prepare a", 0, 0, "DEALLOCATE PREPARE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "drop prepare b", 0, 10, "DEALLOCATE PREPARE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create procedure a() begin declare b condition for 1;", 0, 27, "DECLARE CONDITION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create procedure a() begin declare b condition for 1;", 0, 37, "DECLARE CONDITION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create procedure a() begin declare b cursor;", 0, 27, "DECLARE CURSOR", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create procedure a() begin declare b cursor;", 0, 39, "DECLARE CURSOR", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create procedure a() begin declare exit handler;", 0, 27, "DECLARE HANDLER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create procedure a() begin declare exit handler;", 0, 44, "DECLARE HANDLER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create procedure a() begin declare b, c int;", 0, 27, "DECLARE VARIABLE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create procedure a() begin declare b, c int;", 0, 38, "DECLARE VARIABLE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "drop database a", 0, 0, "DROP DATABASE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "drop schema if exists b", 0, 20, "DROP DATABASE", __LINE__ },
  { MYSQL_VERSION_5_1, MYSQL_VERSION_HIGHER, "drop event a", 0, 0, "DROP EVENT", __LINE__ },
  { MYSQL_VERSION_5_1, MYSQL_VERSION_HIGHER, "drop event if exists b", 0, 19, "DROP EVENT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "drop function a.b", 0, 0, "DROP FUNCTION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "drop function if exists a.b", 0, 26, "DROP FUNCTION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "drop function if exit a.b", 0, 20, "DROP FUNCTION", __LINE__ },
  { MYSQL_VERSION_HIGHER, MYSQL_VERSION_HIGHER, "", 0, 0, "DROP FUNCTION UDF", __LINE__ }, // Syntax is the same as for a normal drop function.
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "drop offline index a on b", 0, 0, "DROP INDEX", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "drop online index a on b", 0, 16, "DROP INDEX", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "drop procedure a.b", 0, 0, "DROP PROCEDURE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "drop procedure if exists a.b", 0, 7, "DROP PROCEDURE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "drop server 'a'", 0, 0, "DROP SERVER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "drop server if exists 'a'", 0, 7, "DROP SERVER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "drop table if exists a, b, c, d cascade", 0, 0, "DROP TABLE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "drop temporary tables if exists a, b, c, d cascade", 0, 17, "DROP TABLE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "drop trigger a.b", 0, 0, "DROP TRIGGER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "drop trigger if exists a.b", 0, 7, "DROP TRIGGER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "drop user current_user(), mike@localhost", 0, 0, "DROP USER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "drop user current_user(), mike@localhost", 0, 7, "DROP USER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "drop view a.b, c.d, e.f restrict", 0, 0, "DROP VIEW", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "drop view if exists a.b, c.d, e.f restrict", 0, 7, "DROP VIEW", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "execute a using @a", 0, 0, "EXECUTE STATEMENT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "flush no_write_to_binlog query cache", 0, 0, "FLUSH QUERY CACHE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "flush no_write_to_binlog query cache", 0, 28, "FLUSH QUERY CACHE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "flush local query cache", 0, 0, "FLUSH QUERY CACHE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "help 'on help'", 0, 2, "HELP COMMAND", __LINE__ },
  { MYSQL_VERSION_HIGHER, MYSQL_VERSION_HIGHER, "", 0, 0, "HELP STATEMENT", __LINE__ }, // Just like help command, just inferior. Same syntax.
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT IF(1 > 2,2,3);", 0, 9, "IF FUNCTION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "CREATE PROCEDURE doiterate(p1 INT)\n"
    "BEGIN\n"
    "  label1: LOOP\n"
    "    SET p1 = p1 + 1;\n"
    "    IF p1 < 10 THEN\n"
    "      ITERATE label1;\n"
    "    END IF;\n"
    "    LEAVE label1;\n"
    "  END LOOP label1;\n"
    "  SET @x = p1;\n"
    "END;", 4, 17, "IF STATEMENT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT IFNULL(1,0);", 0, 10, "IFNULL", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "insert delayed ignore into a.b set a = b", 0, 0, "INSERT DELAYED", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "insert delayed ignore into a.b set a = b", 0, 8, "INSERT DELAYED", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT INSERT('Quadratic', 3, 4, 'What');", 0, 9, "INSERT FUNCTION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "INSERT INTO tbl_temp2 (fld_id)\n"
    "  SELECT tbl_temp1.fld_order_id\n"
    "  FROM tbl_temp1 WHERE tbl_temp1.fld_order_id > 100;", 0, 0, "INSERT SELECT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "INSERT INTO tbl_temp2 (fld_id)\n"
    "  SELECT tbl_temp1.fld_order_id\n"
    "  FROM tbl_temp1 WHERE tbl_temp1.fld_order_id > 100;", 0, 24, "INSERT SELECT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT 1 IS NOT UNKNOWN, 0 IS NOT UNKNOWN, NULL IS NOT UNKNOWN;", 0, 9, "IS NOT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT 1 IS NOT NULL, 0 IS NOT UNKNOWN, NULL IS NOT UNKNOWN;", 0, 9, "IS NOT NULL", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT 1 IS NULL, 0 IS NOT UNKNOWN, NULL IS NOT UNKNOWN;", 0, 9, "IS NULL", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "load data low_priority local infile 'file'", 0, 0, "LOAD DATA", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "load data low_priority local infile 'file'", 0, 7, "LOAD DATA", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "load index into cache a.b", 0, 0, "LOAD INDEX", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "load index into cache a.b", 0, 7, "LOAD INDEX", __LINE__ },
  { MYSQL_VERSION_5_5, MYSQL_VERSION_HIGHER, "load xml low_priority local infile 'file'", 0, 0, "LOAD XML", __LINE__ },
  { MYSQL_VERSION_5_5, MYSQL_VERSION_HIGHER, "load xml low_priority local infile 'file'", 0, 7, "LOAD XML", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "UPDATE t\n  SET blob_col=LOAD_FILE('/tmp/picture')\n  WHERE id=1;", 1, 20, "LOAD_FILE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select a = b not between c and d", 0, 14, "NOT BETWEEN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select a = b not between c and d", 0, 17, "NOT BETWEEN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select a = b not in (c)", 0, 14, "NOT IN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select a = b not in (c)", 0, 19, "NOT IN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select a = b not like c", 0, 14, "NOT LIKE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select a = b not like c", 0, 19, "NOT LIKE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select a = b not regexp c", 0, 14, "NOT REGEXP", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select a = b not regexp c", 0, 19, "NOT REGEXP", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "optimize local table a.b, c.d", 0, 0, "OPTIMIZE TABLE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select 1 from dual procedure analyse()", 0, 20, "PROCEDURE ANALYSE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "purge binary logs to 'blah'", 0, 0, "PURGE BINARY LOGS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "release_lock('lock')", 0, 0, "RELEASE_LOCK", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "rename table a to b", 0, 0, "RENAME TABLE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "rename tables a to b", 0, 13, "RENAME TABLE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "rename user mike to ekim", 0, 0, "RENAME USER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "rename user ekim to mike", 0, 10, "RENAME USER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "repair table a", 0, 0, "REPAIR TABLE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "repair table a", 0, 7, "REPAIR TABLE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SELECT REPEAT('MySQL', 3);", 0, 8, "REPEAT FUNCTION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "CREATE PROCEDURE dorepeat(p1 INT)\n"
    "  BEGIN\n"
    "    SET @x = 0;\n"
    "    REPEAT\n"
    "      SET @x = @x + 1;\n"
    "    UNTIL @x > p1 END REPEAT;\n"
    "  END", 3, 5, "REPEAT LOOP", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "CREATE PROCEDURE dorepeat(p1 INT)\n"
    "  BEGIN\n"
    "    SET @x = 0;\n"
    "    REPEAT\n"
    "      SET @x = @x + 1;\n"
    "    UNTIL @x > p1 END REPEAT;\n"
    "  END", 5, 25, "REPEAT LOOP", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "CREATE PROCEDURE dorepeat(p1 INT)\n"
    "  BEGIN\n"
    "    SET @x = 0;\n"
    "    REPEAT\n"
    "      SET @x = @x + 1;\n"
    "    UNTIL @x > p1 END REPEAT;\n"
    "  END", 6, 0, "REPEAT LOOP", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create table a (id set('a', 'b'))", 0, 20, "SET DATA TYPE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SET GLOBAL sql_slave_skip_counter = 10", 0, 0, "SET GLOBAL SQL_SLAVE_SKIP_COUNTER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "set password for user", 0, 0, "SET PASSWORD", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "set password for user", 0, 4, "SET PASSWORD", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SET sql_log_bin = 0", 0, 0, "SET SQL_LOG_BIN", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "SET sql_log_bin = 0", 0, 4, "SET SQL_LOG_BIN", __LINE__ },
  { MYSQL_VERSION_LOWER, 50699, "show authors", 0, 0, "SHOW AUTHORS", __LINE__ },
  { MYSQL_VERSION_LOWER, 50699, "show authors", 0, 5, "SHOW AUTHORS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show binary logs", 0, 0, "SHOW BINARY LOGS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show binary logs", 0, 5, "SHOW BINARY LOGS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show binlog events", 0, 0, "SHOW BINLOG EVENTS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show binlog events", 0, 5, "SHOW BINLOG EVENTS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show char set like 'a'", 0, 0, "SHOW CHARACTER SET", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show char set like 'a'", 0, 5, "SHOW CHARACTER SET", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show collation", 0, 0, "SHOW COLLATION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show collation", 0, 5, "SHOW COLLATION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show full columns in a", 0, 0, "SHOW COLUMNS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show full columns in a", 0, 5, "SHOW COLUMNS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show contributors", 0, 0, "SHOW CONTRIBUTORS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show contributors", 0, 5, "SHOW CONTRIBUTORS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show count(*) warnings", 0, 0, "SHOW WARNINGS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show count(*) errors", 0, 5, "SHOW ERRORS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show create database a", 0, 0, "SHOW CREATE DATABASE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show create database a", 0, 12, "SHOW CREATE DATABASE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show create event a", 0, 0, "SHOW CREATE EVENT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show create event a", 0, 12, "SHOW CREATE EVENT", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show create function a.b", 0, 0, "SHOW CREATE FUNCTION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show create function a.b", 0, 12, "SHOW CREATE FUNCTION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show create procedure a.b", 0, 0, "SHOW CREATE PROCEDURE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show create procedure a.b", 0, 12, "SHOW CREATE PROCEDURE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show create table a.b", 0, 0, "SHOW CREATE TABLE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show create table a.b", 0, 12, "SHOW CREATE TABLE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show create trigger a.b", 0, 0, "SHOW CREATE TRIGGER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show create trigger a.b", 0, 12, "SHOW CREATE TRIGGER", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show create view a.b", 0, 0, "SHOW CREATE VIEW", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show create view a.b", 0, 12, "SHOW CREATE VIEW", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show databases", 0, 0, "SHOW DATABASES", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show databases", 0, 5, "SHOW DATABASES", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show schemas", 0, 0, "SHOW DATABASES", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show schemas", 0, 5, "SHOW DATABASES", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show engine", 0, 0, "SHOW ENGINE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show engine", 0, 5, "SHOW ENGINE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show engines", 0, 0, "SHOW ENGINES", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show engines", 0, 5, "SHOW ENGINES", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show errors", 0, 0, "SHOW ERRORS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show errors", 0, 5, "SHOW ERRORS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show events", 0, 0, "SHOW EVENTS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show events", 0, 5, "SHOW EVENTS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show function code a.b", 0, 0, "SHOW FUNCTION CODE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show function code a.b", 0, 15, "SHOW FUNCTION CODE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show function status a.b", 0, 0, "SHOW FUNCTION STATUS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show function status a.b", 0, 15, "SHOW FUNCTION STATUS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show grants", 0, 0, "SHOW GRANTS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show grants", 0, 5, "SHOW GRANTS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show index", 0, 0, "SHOW INDEX", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show index", 0, 5, "SHOW INDEX", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show master status", 0, 0, "SHOW MASTER STATUS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show master status", 0, 5, "SHOW MASTER STATUS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show open tables", 0, 0, "SHOW OPEN TABLES", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show open tables", 0, 5, "SHOW OPEN TABLES", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show plugins", 0, 0, "SHOW PLUGINS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show plugins", 0, 5, "SHOW PLUGINS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show privileges", 0, 0, "SHOW PRIVILEGES", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show privileges", 0, 5, "SHOW PRIVILEGES", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show procedure code a.b", 0, 0, "SHOW PROCEDURE CODE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show procedure code a.b", 0, 5, "SHOW PROCEDURE CODE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show procedure status a.b", 0, 0, "SHOW PROCEDURE STATUS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show procedure status a.b", 0, 5, "SHOW PROCEDURE STATUS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show processlist", 0, 0, "SHOW PROCESSLIST", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show processlist", 0, 5, "SHOW PROCESSLIST", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show profile", 0, 0, "SHOW PROFILE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show profile", 0, 5, "SHOW PROFILE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show profiles", 0, 0, "SHOW PROFILES", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show profiles", 0, 5, "SHOW PROFILES", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show relaylog events", 0, 0, "SHOW RELAYLOG EVENTS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show relaylog events", 0, 5, "SHOW RELAYLOG EVENTS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show relaylog events", 0, 14, "SHOW RELAYLOG EVENTS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show slave hosts", 0, 0, "SHOW SLAVE HOSTS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show slave hosts", 0, 5, "SHOW SLAVE HOSTS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show slave hosts", 0, 12, "SHOW SLAVE HOSTS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show slave status", 0, 0, "SHOW SLAVE STATUS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show slave status", 0, 5, "SHOW SLAVE STATUS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show slave status", 0, 11, "SHOW SLAVE STATUS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show status", 0, 0, "SHOW STATUS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show status", 0, 5, "SHOW STATUS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show table status", 0, 0, "SHOW TABLE STATUS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show table status", 0, 5, "SHOW TABLE STATUS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show table status", 0, 11, "SHOW TABLE STATUS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show tables", 0, 0, "SHOW TABLES", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show tables", 0, 5, "SHOW TABLES", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show triggers", 0, 0, "SHOW TRIGGERS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show triggers", 0, 5, "SHOW TRIGGERS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show variables", 0, 0, "SHOW VARIABLES", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show session variables where a = b", 0, 13, "SHOW VARIABLES", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show warnings", 0, 0, "SHOW WARNINGS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "show warnings", 0, 5, "SHOW WARNINGS", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select a = b sounds like c", 0, 13, "SOUNDS LIKE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "select a = b sounds like c", 0, 20, "SOUNDS LIKE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "start slave", 0, 0, "START SLAVE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "start slave", 0, 6, "START SLAVE", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "start transaction", 0, 0, "START TRANSACTION", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "start transaction", 0, 6, "START TRANSACTION", __LINE__ },
};

TEST_FUNCTION(10)
{
  checkTopics(0, multiTokenTests);
}

static std::vector<HelpTestEntry> complexTests = {
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create view view1 /* as (select 1 from b) */", 0, 14, "create view", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create view view1 /* as (select 1 from b) */", 0, 26, "create view", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create view view1 /*! as (select 1 from b) */", 0, 26, "select", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create view view1 /*! as (select 1 from b) */", 0, 24, "create view", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "CREATE TABLE product_order (no INT NOT NULL AUTO_INCREMENT,\n"
    "  product_category INT NOT NULL,\n"
    "  product_id INT NOT NULL,\n"
    "  customer_id INT NOT NULL,\n"
    "  PRIMARY KEY(no),\n"
    "  INDEX (product_category, product_id),\n"
    "  FOREIGN KEY (product_category, product_id)\n"
    "    REFERENCES product(category, id)\n"
    "    ON UPDATE CASCADE ON DELETE RESTRICT,\n"
    "  INDEX (customer_id),\n"
    "  CONSTRAINT mykey FOREIGN KEY (customer_id)\n"
    "    REFERENCES customer(id)) ENGINE=INNODB;", 6, 2, "constraint", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "", 6, 13, "constraint", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "", 7, 7, "constraint", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "", 8, 5, "constraint", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "", 8, 8, "constraint", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "", 8, 25, "constraint", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "", 10, 10, "constraint", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create definer = current_user event if not exists a", 0, 20, "current_user", __LINE__ },
  { MYSQL_VERSION_LOWER, MYSQL_VERSION_HIGHER, "create definer = mike@'localhost' event if not exists a", 0, 30, "CREATE EVENT", __LINE__ },
};

TEST_FUNCTION(15)
{
  checkTopics(0, complexTests);
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99)
{
  delete _tester;
  delete _helpContext;
}

END_TESTS

