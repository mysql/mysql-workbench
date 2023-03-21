/*
 * Copyright (c) 2019, 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "wb_version.h"
#include "wb_test_helpers.h"

#include "mysql/MySQLLexer.h"
#include "mysql/MySQLParser.h"
#include "mysql/MySQLParserBaseListener.h"
#include "mysql/MySQLParserBaseVisitor.h"

#include "grtsqlparser/mysql_parser_services.h"

// This file contains unit tests for the statement splitter and the ANTLR based parser.
// These are low level tests. There's another set of high level tests (see test_mysql_sqldata->parser.cpp).

using namespace parsers;
using namespace antlr4;
using namespace antlr4::atn;
using namespace antlr4::tree;

//----------------------------------------------------------------------------------------------------------------------

namespace {

$ModuleEnvironment() {};

struct TestFile {
  std::string name;
  const char *line_break;
  const char *initial_delmiter;
};

static const std::vector<TestFile> testFiles = {
  // Large set of all possible query types in different combinations and versions.
  {"/db/statements.txt", "\n", "$$"},

  // A file with a number of create tables statements that stresses the use
  // of the grammar (e.g. using weird but still valid object names including \n, long
  // list of indices, all possible data types + the default values etc.).
  // Note: it is essential to use \r\n as normal line break in the file to allow usage of \n
  //       in object names.
  {"/db/nasty_tables.sql", "\r\n", ";"},

#ifndef WITH_VALGRIND
  // Not so many, but some very long insert statements.
  {"/db/sakila-db/sakila-data.sql", "\n", ";"}
#endif
};

using namespace antlrcpp;

// This evaluator only implements the necessary parts for this test case. It's not a full blown evaluator.
struct EvalValue {
  enum ValueType {
    Float,
    Int,
    Null,
    NotNull
  } type; // NULL and NOT NULL are possible literals, so we need an enum for them.
  double number;

  EvalValue(ValueType aType, double aNumber) : type(aType), number(aNumber) {
  }

  bool isNullType() {
    return type == Null || type == NotNull;
  }

  static EvalValue fromBool(bool value) {
    return EvalValue(Int, value ? 1 : 0);
  };
  static EvalValue fromNumber(double number) {
    return EvalValue(Float, number);
  };
  static EvalValue fromNumber(long long number) {
    return EvalValue(Int, (double)number);
  };
  static EvalValue fromNull() {
    return EvalValue(Null, 0);
  };
  static EvalValue fromNotNull() {
    return EvalValue(NotNull, 0);
  };
};

class EvalParseVisitor : public MySQLParserBaseVisitor {
public:
  std::vector<EvalValue> results; // One entry for each select item.

  bool asBool(EvalValue in) {
    if (!in.isNullType() && in.number != 0)
      return true;
    return false;
  };

  virtual Any visitSelectItem(MySQLParser::SelectItemContext *context) override {
    Any result = visitChildren(context);
    results.push_back(std::any_cast<EvalValue>(result));
    return result;
  }

  virtual Any visitExprNot(MySQLParser::ExprNotContext *context) override {
    EvalValue value = std::any_cast<EvalValue>(visit(context->expr()));
    switch (value.type) {
      case EvalValue::Null:
        return EvalValue::fromNotNull();
      case EvalValue::NotNull:
        return EvalValue::fromNull();
      default:
        return EvalValue::fromBool(!asBool(value));
    }
  }

  virtual Any visitExprAnd(MySQLParser::ExprAndContext *context) override {
    EvalValue left = std::any_cast<EvalValue>(visit(context->expr(0)));
    EvalValue right = std::any_cast<EvalValue>(visit(context->expr(1)));

    if (left.isNullType() || right.isNullType())
      return EvalValue::fromNull();
    return EvalValue::fromBool(asBool(left) && asBool(right));

    return visitChildren(context);
  }

  virtual Any visitExprXor(MySQLParser::ExprXorContext *context) override {
    EvalValue left = std::any_cast<EvalValue>(visit(context->expr(0)));
    EvalValue right = std::any_cast<EvalValue>(visit(context->expr(1)));

    if (left.isNullType() || right.isNullType())
      return EvalValue::fromNull();
    return EvalValue::fromBool(asBool(left) != asBool(right));
  }

  virtual Any visitExprOr(MySQLParser::ExprOrContext *context) override {
    EvalValue left = std::any_cast<EvalValue>(visit(context->expr(0)));
    EvalValue right = std::any_cast<EvalValue>(visit(context->expr(1)));

    if (left.isNullType() || right.isNullType())
      return EvalValue::fromNull();
    return EvalValue::fromBool(asBool(left) || asBool(right));
  }

  virtual Any visitExprIs(MySQLParser::ExprIsContext *context) override {
    EvalValue value = std::any_cast<EvalValue>(visit(context->boolPri()));
    if (context->IS_SYMBOL() == nullptr)
      return value;

    bool result = false;
    switch (context->type->getType()) {
      case MySQLLexer::FALSE_SYMBOL:
      case MySQLLexer::TRUE_SYMBOL:
        if (!value.isNullType())
          result = asBool(value);
        break;
      default: // Must be UNKOWN.
        result = value.isNullType();
        break;
    }

    if (context->notRule() != nullptr)
      result = !result;
    return EvalValue::fromBool(result);
  }

  virtual Any visitPrimaryExprIsNull(MySQLParser::PrimaryExprIsNullContext *context) override {
    EvalValue value = std::any_cast<EvalValue>(visit(context->boolPri()));
    if (context->notRule() == nullptr)
      return EvalValue::fromBool(value.type == EvalValue::Null);
    return EvalValue::fromBool(value.type != EvalValue::Null);
  }

  virtual Any visitPrimaryExprCompare(MySQLParser::PrimaryExprCompareContext *context) override {
    EvalValue left = std::any_cast<EvalValue>(visit(context->boolPri()));
    EvalValue right = std::any_cast<EvalValue>(visit(context->predicate()));

    ssize_t op = context->compOp()->getStart()->getType();
    if (left.isNullType() || right.isNullType()) {
      if (op == MySQLLexer::NULL_SAFE_EQUAL_OPERATOR)
        return EvalValue::fromBool(left.type == right.type);
      return EvalValue::fromNull();
    }

    switch (op) {
      case MySQLLexer::EQUAL_OPERATOR:
      case MySQLLexer::NULL_SAFE_EQUAL_OPERATOR:
        return EvalValue::fromBool(left.number == right.number);
      case MySQLLexer::GREATER_OR_EQUAL_OPERATOR:
        return EvalValue::fromBool(left.number >= right.number);
      case MySQLLexer::GREATER_THAN_OPERATOR:
        return EvalValue::fromBool(left.number > right.number);
      case MySQLLexer::LESS_OR_EQUAL_OPERATOR:
        return EvalValue::fromBool(left.number <= right.number);
      case MySQLLexer::LESS_THAN_OPERATOR:
        return EvalValue::fromBool(left.number < right.number);
      case MySQLLexer::NOT_EQUAL_OPERATOR:
        return EvalValue::fromBool(left.number != right.number);
    }
    return EvalValue::fromBool(false);
  }

  virtual Any visitPredicate(MySQLParser::PredicateContext *context) override {
    return visit(context->bitExpr()[0]);
  }

  static unsigned long long shiftLeftWithOverflow(double l, double r) {
    // Shift with overflow if r is larger than the data type size of unsigned long long (64)
    // (which would be undefined if using the standard << operator).
    std::bitset<64> bits = (unsigned long long)llround(l);
    bits <<= (unsigned long long)llround(r);
    return bits.to_ullong();
  }

  virtual Any visitBitExpr(MySQLParser::BitExprContext *context) override {
    if (context->simpleExpr() != nullptr)
      return visit(context->simpleExpr());

    EvalValue left = std::any_cast<EvalValue>(visit(context->bitExpr(0)));
    EvalValue right = std::any_cast<EvalValue>(visit(context->bitExpr(1)));

    if (left.isNullType() || right.isNullType())
      return EvalValue::fromNull();

    switch (context->op->getType()) {
      case MySQLLexer::BITWISE_OR_OPERATOR:
        return EvalValue::fromNumber((double)(llround(left.number) | llround(right.number)));
      case MySQLLexer::BITWISE_AND_OPERATOR:
        return EvalValue::fromNumber(llround(left.number) & llround(right.number));
      case MySQLLexer::BITWISE_XOR_OPERATOR:
        return EvalValue::fromNumber(llround(left.number) ^ llround(right.number));
      case MySQLLexer::SHIFT_LEFT_OPERATOR:
        return EvalValue::fromNumber((long long)shiftLeftWithOverflow(left.number, right.number));
      case MySQLLexer::SHIFT_RIGHT_OPERATOR:
        return EvalValue::fromNumber(llround(left.number) >> llround(right.number));
      case MySQLLexer::PLUS_OPERATOR: // Not handling INTERVAL here for +/-.
        return EvalValue::fromNumber(left.number + right.number);
      case MySQLLexer::MINUS_OPERATOR:
        return EvalValue::fromNumber(left.number - right.number);
      case MySQLLexer::MULT_OPERATOR:
        return EvalValue::fromNumber(left.number * right.number);
      case MySQLLexer::DIV_OPERATOR:
        return EvalValue::fromNumber(left.number / right.number);
      case MySQLLexer::DIV_SYMBOL: // Integer div
        return EvalValue::fromNumber(llround(left.number) / llround(right.number));
      case MySQLLexer::MOD_OPERATOR:
      case MySQLLexer::MOD_SYMBOL: {
        if (left.type == EvalValue::Int && right.type == EvalValue::Int)
          return EvalValue::fromNumber((long long)left.number % (long long)right.number);
        return EvalValue::fromNumber(fmod(left.number, right.number));
      }
    }
    return EvalValue::fromNull();
  }

  virtual Any visitExprListWithParentheses(MySQLParser::ExprListWithParenthesesContext *context) override {
    return visit(context->exprList());
  }

  virtual Any visitSimpleExprList(MySQLParser::SimpleExprListContext *context) override {
    return visit(context->exprList());
  }

  virtual Any visitSimpleExprLiteral(MySQLParser::SimpleExprLiteralContext *context) override {
    switch (context->start->getType()) {
      case MySQLLexer::TRUE_SYMBOL:
        return EvalValue::fromBool(true);
      case MySQLLexer::FALSE_SYMBOL:
        return EvalValue::fromBool(false);
      case MySQLLexer::NULL_SYMBOL:
        return EvalValue::fromNull();
      case MySQLLexer::HEX_NUMBER:
        return EvalValue::fromNumber(std::stoll(context->start->getText(), nullptr, 16));
      case MySQLLexer::BIN_NUMBER: {
        std::bitset<64> bits(context->start->getText());
        return EvalValue::fromNumber((long long)bits.to_ullong());
      }
      case MySQLLexer::INT_NUMBER: {
        std::string text = context->start->getText();
        return EvalValue::fromNumber(std::stoll(text, nullptr, 10));
      }
      default:
        return EvalValue::fromNumber(std::atof(context->start->getText().c_str()));
    }
  }

  virtual Any visitSimpleExprNot(MySQLParser::SimpleExprNotContext *context) override {
    EvalValue value = std::any_cast<EvalValue>(visit(context->simpleExpr()));
    if (value.isNullType())
      return EvalValue::fromNull();
    return EvalValue::fromBool(!asBool(value));
  }
};

//----------------------------------------------------------------------------------------------------------------------

/**
 * Determines if the version info in the statement matches the given version (if there's version info at all).
 * The version info is removed from the statement, if any.
 */
static bool versionMatches(std::string &statement, unsigned long serverVersion) {
  static std::regex versionPattern("^\\[(<|<=|>|>=|=)(\\d{5})\\]");
  static std::map<std::string, int> relationMap = {
    { "<", 0 }, { "<=", 1 }, { "=", 2 }, { ">=", 3 }, { ">", 4 }
  };

  std::smatch matches;
  if (std::regex_search(statement, matches, versionPattern)) {
    auto relation = matches[1].str();
    unsigned long targetVersion = std::stoul(matches[2].str());

    switch (relationMap[relation]) {
      case 0:
        if (serverVersion >= targetVersion)
          return false;
        break;
      case 1:
        if (serverVersion > targetVersion)
          return false;
        break;
      case 2:
        if (serverVersion != targetVersion)
          return false;
        break;
      case 3:
        if (serverVersion < targetVersion)
          return false;
        break;
      case 4:
        if (serverVersion <= targetVersion)
          return false;
        break;
    }

    statement = std::regex_replace(statement, versionPattern, "");
  }

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

class TestErrorListener : public BaseErrorListener {
public:
  std::string lastErrors;

  virtual void syntaxError(Recognizer *recognizer, antlr4::Token *offendingSymbol, size_t line,
                           size_t charPositionInLine, const std::string &msg, std::exception_ptr e) override {
    // Here we use the message provided by the DefaultErrorStrategy class.
    if (!lastErrors.empty())
      lastErrors += "\n";
    lastErrors += "line " + std::to_string(line) + ":" + std::to_string(charPositionInLine) + " " + msg;
  }
};

//----------------------------------------------------------------------------------------------------------------------

$TestData {
  /**
   * This test generates queries with many (all?) MySQL function names used in foreign key creation
   * (parser bug #21114). Taken from the server test suite.
   */
  std::vector<const char*> functions = {
    "acos", "adddate",
    "addtime",
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
    "within", "x", "y", "yearweek"
  };

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

  /**
   * Test data for all relevant SQL modes (ANSI, DB2, MAXDB, MSSQL, ORACLE, POSTGRESQL, MYSQL323, MYSQL40
   * ANSI_QUOTES, PIPES_AS_CONCAT, NO_BACKSLASH_ESCAPES, IGNORE_SPACE, HIGH_NOT_PRECEDENCE and combinations of them).
   */
  struct SqlModeTestEntry {
    std::string query;
    std::string sqlMode;
    size_t errors;
  };

  const std::vector<SqlModeTestEntry> sqlModeTestQueries = {
    // IGNORE_SPACE
    {"create table count (id int)", "", 0},
    {"create table count(id int)", "", 1},
    {"create table count (id int)", "IGNORE_SPACE", 2},
    {"create table count(id int)", "IGNORE_SPACE", 1},
    {"create table xxx (id int)", "", 0},
    {"create table xxx(id int)", "", 0},
    {"create table xxx (id int)", "IGNORE_SPACE", 0},
    {"create table xxx(id int)", "IGNORE_SPACE", 0},

    // ANSI_QUOTES
    {"select \"abc\" \"def\" 'ghi''\\n\\Z\\z'", "", 0},            // Double + single quoted text concatenated.
    {"select \"abc\" \"def\" as 'ghi\\n\\Z\\z'", "", 0},           // Double quoted text concatenated + alias.
    {"select \"abc\" \"def\" 'ghi''\\n\\Z\\z'", "ANSI_QUOTES", 2}, // column ref + alias + invalid single quoted text.

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

  using P = MySQLParser;

  const std::vector<std::vector<size_t>> sqlModeTestResults = {
    {P::CREATE_SYMBOL, P::TABLE_SYMBOL, P::IDENTIFIER, P::OPEN_PAR_SYMBOL, P::IDENTIFIER, P::INT_SYMBOL, P::CLOSE_PAR_SYMBOL, Token::EOF},
    {P::CREATE_SYMBOL, P::TABLE_SYMBOL, P::COUNT_SYMBOL, P::OPEN_PAR_SYMBOL, P::IDENTIFIER, P::INT_SYMBOL, P::CLOSE_PAR_SYMBOL, Token::EOF},
    {P::CREATE_SYMBOL, P::TABLE_SYMBOL, P::OPEN_PAR_SYMBOL, P::IDENTIFIER, P::INT_SYMBOL, P::CLOSE_PAR_SYMBOL},
    {P::CREATE_SYMBOL, P::TABLE_SYMBOL, P::COUNT_SYMBOL, P::OPEN_PAR_SYMBOL, P::IDENTIFIER, P::INT_SYMBOL, P::CLOSE_PAR_SYMBOL, Token::EOF},
    {P::CREATE_SYMBOL, P::TABLE_SYMBOL, P::IDENTIFIER, P::OPEN_PAR_SYMBOL, P::IDENTIFIER, P::INT_SYMBOL, P::CLOSE_PAR_SYMBOL, Token::EOF},
    {P::CREATE_SYMBOL, P::TABLE_SYMBOL, P::IDENTIFIER, P::OPEN_PAR_SYMBOL, P::IDENTIFIER, P::INT_SYMBOL, P::CLOSE_PAR_SYMBOL, Token::EOF},
    {P::CREATE_SYMBOL, P::TABLE_SYMBOL, P::IDENTIFIER, P::OPEN_PAR_SYMBOL, P::IDENTIFIER, P::INT_SYMBOL, P::CLOSE_PAR_SYMBOL, Token::EOF},
    {P::CREATE_SYMBOL, P::TABLE_SYMBOL, P::IDENTIFIER, P::OPEN_PAR_SYMBOL, P::IDENTIFIER, P::INT_SYMBOL, P::CLOSE_PAR_SYMBOL, Token::EOF},

    {P::SELECT_SYMBOL, P::DOUBLE_QUOTED_TEXT, P::DOUBLE_QUOTED_TEXT, P::SINGLE_QUOTED_TEXT, Token::EOF},
    {P::SELECT_SYMBOL, P::DOUBLE_QUOTED_TEXT, P::DOUBLE_QUOTED_TEXT, P::AS_SYMBOL, P::SINGLE_QUOTED_TEXT, Token::EOF},
    {P::SELECT_SYMBOL, P::DOUBLE_QUOTED_TEXT, P::DOUBLE_QUOTED_TEXT, P::SINGLE_QUOTED_TEXT},

    {P::SELECT_SYMBOL, P::DOUBLE_QUOTED_TEXT, P::LOGICAL_OR_OPERATOR, P::DOUBLE_QUOTED_TEXT, Token::EOF},
    {P::SELECT_SYMBOL, P::DOUBLE_QUOTED_TEXT, P::CONCAT_PIPES_SYMBOL, P::DOUBLE_QUOTED_TEXT, Token::EOF},

    {P::SELECT_SYMBOL, P::NOT_SYMBOL, P::INT_NUMBER, P::BETWEEN_SYMBOL, P::MINUS_OPERATOR, P::INT_NUMBER, P::AND_SYMBOL, P::INT_NUMBER, Token::EOF},
    {P::SELECT_SYMBOL, P::NOT2_SYMBOL, P::INT_NUMBER, P::BETWEEN_SYMBOL, P::MINUS_OPERATOR, P::INT_NUMBER, P::AND_SYMBOL, P::INT_NUMBER, Token::EOF},

    {P::SELECT_SYMBOL, P::DOUBLE_QUOTED_TEXT, Token::EOF},
    {P::SELECT_SYMBOL, P::DOUBLE_QUOTED_TEXT, P::IDENTIFIER, Token::EOF},
  };

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

  const std::vector<VersionTestData> versionTestResults = {
    VersionTestData(50600, "grant all privileges on a to mike", 0U),
    VersionTestData(50600, "grant all privileges on a to mike identified by 'blah'", 0U),
    VersionTestData(50600, "grant all privileges on a to mike identified by password 'blah'", 0U),
    VersionTestData(50600, "grant all privileges on a to mike identified by password 'blah'", 0U),
    VersionTestData(50600, "grant all privileges on a to mike identified by password 'blah'", 0U),
    VersionTestData(50710, "grant all privileges on a to mike identified by password 'blah'", 0U),
    VersionTestData(50600, "grant select on *.* to mike identified with 'blah'", 0U),
    VersionTestData(50600, "grant select on *.* to mike identified with 'blah'", 0U),
    VersionTestData(50600, "grant select on *.* to mike identified with blah as 'blubb'", 0U),
    VersionTestData(50600, "grant select on *.* to mike identified with blah as 'blubb'", 0U),
    VersionTestData(50600, "grant select on *.* to mike identified with blah by 'blubb'", 1U),
    VersionTestData(50600, "grant select on *.* to mike identified with blah by 'blubb'", 1U),
    VersionTestData(50706, "grant select on *.* to mike identified with blah by 'blubb'", 0U),
    VersionTestData(80011, "grant select on *.* to mike identified with blah by 'blubb'", 1U),
    VersionTestData(80011, "grant select on *.* to mike, goofy", 0U),
    VersionTestData(80011, "grant select on *.* to mike, goofy", 0U),
    VersionTestData(80011, "grant against, something (a, b, c), fine@'me' to mike, current_user() with admin option", 0U),
    VersionTestData(70011, "grant against, something (a, b, c), fine@'me' to mike, current_user() with admin option", 1U),
    VersionTestData(80011, "grant proxy on me to you identified by 'xyz'", 1U),
    VersionTestData(70011, "grant proxy on me to you identified by 'xyz'", 0U),
  };

  const std::vector<SqlModeTestEntry> numbersTestQueries = {
    { "select 0x;", "", 0 },
    { "select 0xa;", "", 0 },
    { "select 0xx;", "", 0 },
    { "select 0x2111;", "", 0 },
    { "select 0X2111x;", "", 0 },
    { "select x'2111';", "", 0 },
    { "select x'2111x';", "", 0 },

    { "select 0b;", "", 0 },
    { "select 0b0;", "", 0 },
    { "select 0b2;", "", 0 },
    { "select 0b111;", "", 0 },
    { "select 0b2111;", "", 0 },
    { "select b'0111';", "", 0 },
    { "select b'2111';", "", 0 },

    { "select .1union select 2;", "", 0 },
    { "select 1 from dual where 1e1=1e1union select 2;", "", 0 },

    { "select 1;", "", 0 },
    { "select 1.1;", "", 0 },
    { "select 1.1e1;", "", 0 },
    { "select 1.1a1;", "", 0 },
    { "select .1a1;", "", 0 },
    { "select .1e2a1;", "", 0 },
    { "select 1e;", "", 0 },
    { "select 1f;", "", 0 },
    { "select .a from b;", "", 0 },
    { "select 1e-2;", "", 0 },
    { "select 1e-a;", "", 0 },
  };

  const std::vector<std::vector<size_t>> numbersTestResults = {
    {P::SELECT_SYMBOL, P::IDENTIFIER, P::SEMICOLON_SYMBOL, Token::EOF},
    {P::SELECT_SYMBOL, P::HEX_NUMBER, P::SEMICOLON_SYMBOL, Token::EOF},
    {P::SELECT_SYMBOL, P::IDENTIFIER, P::SEMICOLON_SYMBOL, Token::EOF},
    {P::SELECT_SYMBOL, P::HEX_NUMBER, P::SEMICOLON_SYMBOL, Token::EOF},
    {P::SELECT_SYMBOL, P::IDENTIFIER, P::SEMICOLON_SYMBOL, Token::EOF},
    {P::SELECT_SYMBOL, P::HEX_NUMBER, P::SEMICOLON_SYMBOL, Token::EOF},
    {P::SELECT_SYMBOL, P::IDENTIFIER, P::SINGLE_QUOTED_TEXT, P::SEMICOLON_SYMBOL, Token::EOF},

    {P::SELECT_SYMBOL, P::IDENTIFIER, P::SEMICOLON_SYMBOL, Token::EOF},
    {P::SELECT_SYMBOL, P::BIN_NUMBER, P::SEMICOLON_SYMBOL, Token::EOF},
    {P::SELECT_SYMBOL, P::IDENTIFIER, P::SEMICOLON_SYMBOL, Token::EOF},
    {P::SELECT_SYMBOL, P::BIN_NUMBER, P::SEMICOLON_SYMBOL, Token::EOF},
    {P::SELECT_SYMBOL, P::IDENTIFIER, P::SEMICOLON_SYMBOL, Token::EOF},
    {P::SELECT_SYMBOL, P::BIN_NUMBER, P::SEMICOLON_SYMBOL, Token::EOF},
    {P::SELECT_SYMBOL, P::IDENTIFIER, P::SINGLE_QUOTED_TEXT, P::SEMICOLON_SYMBOL, Token::EOF},

    {P::SELECT_SYMBOL, P::DECIMAL_NUMBER, P::UNION_SYMBOL, P::SELECT_SYMBOL, P::INT_NUMBER, P::SEMICOLON_SYMBOL,
      Token::EOF},
    {P::SELECT_SYMBOL, P::INT_NUMBER, P::FROM_SYMBOL, P::DUAL_SYMBOL, P::WHERE_SYMBOL, P::FLOAT_NUMBER, P::EQUAL_OPERATOR,
      P::FLOAT_NUMBER, P::UNION_SYMBOL, P::SELECT_SYMBOL, P::INT_NUMBER, P::SEMICOLON_SYMBOL, Token::EOF},

    {P::SELECT_SYMBOL, P::INT_NUMBER, P::SEMICOLON_SYMBOL, Token::EOF},
    {P::SELECT_SYMBOL, P::DECIMAL_NUMBER, P::SEMICOLON_SYMBOL, Token::EOF},
    {P::SELECT_SYMBOL, P::FLOAT_NUMBER, P::SEMICOLON_SYMBOL, Token::EOF},
    {P::SELECT_SYMBOL, P::DECIMAL_NUMBER, P::IDENTIFIER, P::SEMICOLON_SYMBOL, Token::EOF},
    {P::SELECT_SYMBOL, P::DECIMAL_NUMBER, P::IDENTIFIER, P::SEMICOLON_SYMBOL, Token::EOF},
    {P::SELECT_SYMBOL, P::FLOAT_NUMBER, P::IDENTIFIER, P::SEMICOLON_SYMBOL, Token::EOF},
    {P::SELECT_SYMBOL, P::IDENTIFIER, P::SEMICOLON_SYMBOL, Token::EOF},
    {P::SELECT_SYMBOL, P::IDENTIFIER, P::SEMICOLON_SYMBOL, Token::EOF},
    {P::SELECT_SYMBOL, P::DOT_SYMBOL, P::IDENTIFIER, P::FROM_SYMBOL, P::IDENTIFIER, P::SEMICOLON_SYMBOL, Token::EOF},
    {P::SELECT_SYMBOL, P::FLOAT_NUMBER, P::SEMICOLON_SYMBOL, Token::EOF},
    {P::SELECT_SYMBOL, P::IDENTIFIER, P::MINUS_OPERATOR, P::IDENTIFIER, P::SEMICOLON_SYMBOL, Token::EOF},

  };

  std::string dataDir = casmine::CasmineContext::get()->tmpDataDir();

  std::unique_ptr<WorkbenchTester> tester;
  std::set<std::string> charsets;

  ANTLRInputStream input;
  MySQLLexer lexer;
  CommonTokenStream tokens;
  MySQLParser parser;
  Ref<BailErrorStrategy> bailOutErrorStrategy = std::make_shared<BailErrorStrategy>();
  Ref<DefaultErrorStrategy> defaultErrorStrategy = std::make_shared<DefaultErrorStrategy>();
  TestErrorListener errorListener;
  MySQLParser::QueryContext *lastParseTree;

  MySQLParserServices *services;

  // Use the app's version also as the latest/current server version for tests.
  long currentVersion = APP_MAJOR_NUMBER * 10000 + APP_MINOR_NUMBER * 100 + APP_BUILD_NUMBER;

  //--------------------------------------------------------------------------------------------------------------------

  /**
   * Parses the given string and returns the number of errors found.
   */
  std::pair<std::size_t, std::string> parse(const std::string sql, long version, const std::string &mode) {
    parser.serverVersion = version;
    lexer.serverVersion = version;
    parser.sqlModeFromString(mode);
    lexer.sqlModeFromString(mode);

    input.load(sql);
    lexer.reset();
    lexer.setInputStream(&input); // Not just reset(), which only rewinds the current position.
    tokens.setTokenSource(&lexer);

    parser.reset();
    errorListener.lastErrors.clear();
    parser.removeErrorListeners();
    parser.setErrorHandler(bailOutErrorStrategy); // Bail out at the first found error.
    parser.getInterpreter<ParserATNSimulator>()->setPredictionMode(PredictionMode::SLL);

    try {
      tokens.fill();
    } catch (IllegalStateException &e) {
      return { 1, e.what() };
    }

    try {
      lastParseTree = parser.query();
    } catch (ParseCancellationException &) {
      // If parsing was cancelled we either really have a syntax error or we need to do a second step,
      // now with the default strategy and LL parsing.
      tokens.reset();
      parser.reset();
      parser.setErrorHandler(defaultErrorStrategy);
      parser.getInterpreter<ParserATNSimulator>()->setPredictionMode(PredictionMode::LL);
      parser.addErrorListener(&errorListener);
      errorListener.lastErrors.clear();
      lastParseTree = parser.query();
    }

    return { lexer.getNumberOfSyntaxErrors() + parser.getNumberOfSyntaxErrors(), errorListener.lastErrors };
  }

  //--------------------------------------------------------------------------------------------------------------------

  void collectTokenTypes(RuleContext *context, std::vector<size_t> &list) {
    for (size_t index = 0; index < context->children.size(); ++index) {
      tree::ParseTree *child = context->children[index];
      if (antlrcpp::is<RuleContext *>(child))
        collectTokenTypes(dynamic_cast<RuleContext *>(child), list);
      else {
        // A terminal node.
        tree::TerminalNode *node = dynamic_cast<tree::TerminalNode *>(child);
        antlr4::Token *token = node->getSymbol();
        list.push_back(token->getType());
      }
    }
  }

  //--------------------------------------------------------------------------------------------------------------------

  /**
   * Parses the given string and checks the built AST. Returns true if no error occurred, otherwise false.
   */
  std::pair<bool, std::string> parseAndCompare(const std::string &sql, long version, const std::string &mode,
                                               std::vector<size_t> expected, size_t expectedErrorCount = 0) {
    auto result = parse(sql, version, mode);
    if (result.first != expectedErrorCount)
      return { false, result.second };

    // Walk the tokens stored in the parse tree produced by the parse call above and match exactly the given
    // list of token types.
    std::vector<size_t> tokens;
    collectTokenTypes(lastParseTree, tokens);

    return { tokens == expected, "Found token differences" };
  }

  //--------------------------------------------------------------------------------------------------------------------

  TestData(): lexer(&input), tokens(&lexer), parser(&tokens) {}
};

$describe("MySQL parser test suite (ANTLR)") {

  //--------------------------------------------------------------------------------------------------------------------

  $beforeAll([this]() {
    data->tester.reset(new WorkbenchTester());
    data->tester->initializeRuntime();

    // The charset list contains also the 3 charsets that were introduced in 5.5.3.
    grt::ListRef<db_CharacterSet> list = data->tester->getRdbms()->characterSets();
    for (size_t i = 0; i < list->count(); i++)
      data->charsets.insert("_" + base::tolower(*list[i]->name()));

    data->lexer.charsets = data->charsets;
    data->lexer.removeErrorListeners();
    data->parser.removeErrorListeners();

    data->services = MySQLParserServices::get();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Statement splitter test", [this]() {
    std::string filename = data->dataDir + "/db/sakila-db/sakila-data.sql";
    std::string statement_filename = data->dataDir + "/db/sakila-db/single_statement.sql";

    std::ifstream stream(filename, std::ios::binary);
    $expect(stream.good()).toBeTrue("Error loading sql file");
    std::string sql((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

    std::vector<StatementRange> ranges;
    data->services->determineStatementRanges(sql.c_str(), sql.size(), ";", ranges);

    $expect(ranges.size()).toBe(57U, "Unexpected number of statements returned from splitter");

    std::string s1(sql, ranges[0].start, ranges[0].length);
    $expect(s1).toBe("SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0", "Wrong statement");

    std::string s3(sql, ranges[56].start, ranges[56].length);
    $expect(s3).toBe("SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS", "Wrong statement");

    std::string s2(sql, ranges[30].start, ranges[30].length);

    stream.close();
    stream.open(statement_filename, std::ios::binary);
    $expect(stream.good()).toBeTrue("Error loading result file");

    sql = std::string((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    $expect(s2).toBe(sql, "Wrong statement");
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Parse a number of files with various statements", [this]() {
    std::size_t count = 0;
    for (auto entry : testFiles) {
      std::string fileName = data->dataDir + entry.name;

#ifdef _MSC_VER
      std::ifstream stream(base::string_to_wstring(fileName), std::ios::binary);
#else
      std::ifstream stream(fileName, std::ios::binary);
#endif
      $expect(stream.good()).toBeTrue("Error loading sql file: " + fileName);
      std::string sql((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

      std::vector<StatementRange> ranges;
      data->services->determineStatementRanges(sql.c_str(), sql.size(), entry.initial_delmiter, ranges,
                                               entry.line_break);
      count += ranges.size();

      for (auto &range : ranges) {
        std::string statement(sql.c_str() + range.start, range.length);

        if (versionMatches(statement, 50620)) {
          auto result = data->parse(statement, 50620, "ANSI_QUOTES");
          if (result.first > 0U) {
            $fail("This query failed to parse (5.6.20):\n" + statement + "\n with error: " + result.second);
          }
        } else if (versionMatches(statement, 50720)) {
          auto result = data->parse(statement, 50720, "ANSI_QUOTES");
          if (result.first > 0U) {
            $fail("This query failed to parse (5.7.20):\n" + statement + "\n with error: " + result.second);
          }
        } else if (versionMatches(statement, 80021)) {
          auto result = data->parse(statement, 80021, "ANSI_QUOTES");
          if (result.first > 0U) {
            $fail("This query failed to parse (8.0.21):\n" + statement + "\n with error: " + result.second);
          }
        } else
          $fail("Invalid version number found in query: " + statement);
      }

    }
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Queries with function names as identifiers", [this]() {
    for (const char *name : data->functions) {
      std::string query = base::strfmt(data->query1, name);
      auto result = data->parse(query, 50530, "ANSI_QUOTES");
      $expect(result.first).toBe(0U, "Query: " + query + " failed to parse with error: " + result.second);

      query = base::strfmt(data->query2, name, name);
      result = data->parse(query, 50530, "ANSI_QUOTES");
      $expect(result.first).toBe(0U, "Query: " + query + " failed to parse with error: " + result.second);
    }
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Operator precedence tests", [this]() {
    // This file is an unmodified copy from the server parser test suite.
    const std::string filename = data->dataDir + "/parser/parser_precedence.result";

    std::ifstream stream(filename, std::ios::binary);
    $expect(stream.good()).toBeTrue("25.0 Could not open precedence test file");

    std::string line;

    // Start with skipping over some lines that contain results which require a real server (querying from a table).
    while (!std::getline(stream, line).eof()) {
      if (line == "drop table t1_30237_bool;")
        break;
    }

    int skip = 0; // Used to skip not relevant tests while fixing a test.
    int counter = 1;
    while (!std::getline(stream, line).eof()) {
      if (base::trim(line).empty())
        continue;

      // Start of a new test. The test description is optional.
      std::string sql;
      if (base::hasPrefix(line, "Testing ")) {
        $expect(std::getline(stream, sql).eof()).toBeFalse("Invalid test file format");
      } else
        sql = line;
      $expect(base::hasPrefix(sql, "select")).toBeTrue("Invalid test file format");

      // The next line either repeats (parts of) the query or contains a server error.
      $expect(std::getline(stream, line).eof()).toBeFalse("Invalid test file format");

      bool expectError = false;
      if (base::hasPrefix(line, "ERROR "))
        expectError = true;

      std::vector<EvalValue> expectedResults;
      if (!expectError) { // No results to compare in an error case.
        $expect(std::getline(stream, line).eof()).toBeFalse("Invalid test file format");
        std::string temp;
        std::stringstream stream(line);
        while (stream >> temp) {
          if (base::same_string(temp, "true"))
            expectedResults.push_back(EvalValue::fromBool(true));
          if (base::same_string(temp, "false"))
            expectedResults.push_back(EvalValue::fromBool(true));
          if (base::same_string(temp, "null"))
            expectedResults.push_back(EvalValue::fromNull());
          expectedResults.push_back(EvalValue::fromNumber(std::atof(temp.c_str())));
        }
      }

      if (--skip >= 0) {
        ++counter;
        continue;
      }

      auto result = data->parse(sql, 80012, "");
      $expect(result.first == 0).toBe(!expectError,
        "Error status is unexpected for query (" + std::to_string(counter) + "): \n" + sql + "\n");
      if (expectError) {
        ++counter;
        continue;
      }

      EvalParseVisitor evaluator;
      try {
        evaluator.visit(data->lastParseTree);
      } catch (std::bad_cast &) {
        std::cout << "Query failed to evaluate: \"\n" + sql + "\"\n";
        std::cout << "Parse tree: " << data->lastParseTree->toStringTree(&data->parser) << std::endl;
        throw;
      }

      $expect(evaluator.results.size()).toBe(expectedResults.size(),
        "Result counts differ for query (" + std::to_string(counter) + "): \n\"" + sql + "\"\n");

      static std::string dataTypes[] = {"FLOAT", "INT", "NULL", "NOT NULL"};
      for (size_t i = 0; i < expectedResults.size(); ++i) {
        // We have no int type in expected results. So we make float and int being the same.
        EvalValue::ValueType type = evaluator.results[i].type;
        if (type == EvalValue::Int)
          type = EvalValue::Float;
        EvalValue::ValueType expectedType = expectedResults[i].type;

        $expect(dataTypes[type]).toBe(dataTypes[expectedType],
          "Result type " + std::to_string(i) + " differs for query (" + std::to_string(counter) + "): \n\"" + sql + "\"\n");
        if (!expectedResults[i].isNullType())
          $expect(evaluator.results[i].number).toBe(expectedResults[i].number,
            "Result " + std::to_string(i) + " differs for query (" + std::to_string(counter) + "): \n\"" + sql + "\"\n");
      }
      ++counter;
    }
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("SQL mode dependent parsing", [this]() {
    for (size_t i = 0; i < data->sqlModeTestQueries.size(); i++) {
      auto &entry = data->sqlModeTestQueries[i];
      auto result = data->parseAndCompare(entry.query, 80012, entry.sqlMode, data->sqlModeTestResults[i], entry.errors);
      if (!result.first) {
        $fail("SQL mode test " + std::to_string(i) + " failed: " + entry.query + "\nwith error: " + result.second);
      }
    }
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Tests the parser's string concatenation feature", [this]() {
    class TestListener : public MySQLParserBaseListener {
    public:
      std::string text;

      virtual void exitTextLiteral(MySQLParser::TextLiteralContext *ctx) override {
        text = MySQLParser::getText(ctx, true);
      }
    };

    auto result = data->parse("select \"abc\" \"def\" 'ghi''\\n\\z'", 80012, "");
    $expect(result.first).toBe(0U, "String concatenation");

    TestListener listener;
    tree::ParseTreeWalker::DEFAULT.walk(&listener, data->lastParseTree);
    $expect(listener.text).toBe("abcdefghi'\nz", "String concatenation");
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Version dependent parts of GRANT", [this]() {
    for (size_t i = 0; i < data->versionTestResults.size(); ++i) {
      auto &entry = data->versionTestResults[i];
      auto result = data->parse(entry.sql, entry.version, "");
      $expect(result.first).toBe(entry.errorCount, "GRANT parsing failed (" +
        std::to_string(i) + "): ");
    }
  });

//----------------------------------------------------------------------------------------------------------------------

  $it("Hex, binary, float, decimal and int number handling", [this]() {
    for (size_t i = 0; i < data->numbersTestQueries.size(); i++) {
      auto &entry = data->numbersTestQueries[i];
      auto result = data->parseAndCompare(entry.query, 80012, entry.sqlMode, data->numbersTestResults[i], entry.errors);
      if (!result.first) {
        $fail("Number test (" + std::to_string(i) + ") failed: " + entry.query + "\nwith error: " + result.second);
      }
    }
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Parsing of subparts of the MySQL language", []() {
    // Restricted content parsing (e.g. routines only, views only etc.).

    $pending("this must be implemented yet");
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Bug #30449796", [this]() {
    auto result = data->parse("ANALYZE TABLE emp UPDATE HISTOGRAM ON job WITH 5 BUCKETS;", 50720, "");
    $expect(result.first).toEqual(1U);
    $expect(result.second).toEqual("line 1:18 no viable alternative at input 'UPDATE'");
    result = data->parse("ANALYZE TABLE emp UPDATE HISTOGRAM ON job WITH 5 BUCKETS;", 80010, "");
    $expect(result.first).toEqual(0U);
    $expect(result.second).toBeEmpty();
  });

  //--------------------------------------------------------------------------------------------------------------------

}
}
