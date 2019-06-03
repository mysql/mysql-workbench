/*
 * Copyright (c) 2012, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <regex>

#include "wb_helpers.h"

#include "mysql/MySQLLexer.h"
#include "mysql/MySQLParser.h"
#include "mysql/MySQLParserBaseListener.h"
#include "mysql/MySQLParserBaseVisitor.h"

#include "grtsqlparser/mysql_parser_services.h"

// This file contains unit tests for the statement splitter and the ANTLR based parser.
// These are low level tests. There's another set of high level tests (see test_mysql_sql_parser.cpp).

#define VERBOSE_OUTPUT 0

using namespace parsers;
using namespace antlr4;
using namespace antlr4::atn;
using namespace antlr4::tree;

//----------------------------------------------------------------------------------------------------------------------

BEGIN_TEST_DATA_CLASS(mysql_parser_tests)
protected:
  WBTester *_tester;
  std::set<std::string> _charsets;

  ANTLRInputStream _input;
  MySQLLexer _lexer;
  CommonTokenStream _tokens;
  MySQLParser _parser;
  Ref<BailErrorStrategy> _bailOutErrorStrategy = std::make_shared<BailErrorStrategy>();
  Ref<DefaultErrorStrategy> _defaultErrorStrategy = std::make_shared<DefaultErrorStrategy>();
  MySQLParser::QueryContext *_lastParseTree;

  MySQLParserServices *_services;

  size_t parse(const std::string sql, long version, const std::string &mode);
  bool parseAndCompare(const std::string &sql, long version, const std::string &mode, std::vector<size_t> tokens,
                       size_t expectedErrorCount = 0);

TEST_DATA_CONSTRUCTOR(mysql_parser_tests) : _lexer(&_input), _tokens(&_lexer), _parser(&_tokens) {
  _tester = new WBTester();
  // init datatypes
  populate_grt(*_tester);

  // The charset list contains also the 3 charsets that were introduced in 5.5.3.
  grt::ListRef<db_CharacterSet> list = _tester->get_rdbms()->characterSets();
  for (size_t i = 0; i < list->count(); i++)
    _charsets.insert("_" + base::tolower(*list[i]->name()));

  _lexer.charsets = _charsets;
  _lexer.removeErrorListeners();
  _parser.removeErrorListeners();

  _services = MySQLParserServices::get();
}
END_TEST_DATA_CLASS

TEST_MODULE(mysql_parser_tests, "MySQL parser test suite (ANTLR)");

//----------------------------------------------------------------------------------------------------------------------

/**
 * Parses the given string and returns the number of errors found.
 */
size_t Test_object_base<mysql_parser_tests>::parse(const std::string sql, long version, const std::string &mode) {
  _parser.serverVersion = version;
  _lexer.serverVersion = version;
  _parser.sqlModeFromString(mode);
  _lexer.sqlModeFromString(mode);

  _input.load(sql);
  _lexer.reset();
  _lexer.setInputStream(&_input); // Not just reset(), which only rewinds the current position.
  _tokens.setTokenSource(&_lexer);

  _parser.reset();
  _parser.setErrorHandler(_bailOutErrorStrategy); // Bail out at the first found error.
  _parser.getInterpreter<ParserATNSimulator>()->setPredictionMode(PredictionMode::SLL);

  try {
    _tokens.fill();
  } catch (IllegalStateException &) {
    return 1;
  }

  try {
    _lastParseTree = _parser.query();
  } catch (ParseCancellationException &) {
    // If parsing was cancelled we either really have a syntax error or we need to do a second step,
    // now with the default strategy and LL parsing.
    _tokens.reset();
    _parser.reset();
    _parser.setErrorHandler(_defaultErrorStrategy);
    _parser.getInterpreter<ParserATNSimulator>()->setPredictionMode(PredictionMode::LL);
    _lastParseTree = _parser.query();
  }

  return _lexer.getNumberOfSyntaxErrors() + _parser.getNumberOfSyntaxErrors();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 *  Statement splitter test.
 */
TEST_FUNCTION(5) {
  std::string filename = "data/db/sakila-db/sakila-data.sql";
  std::string statement_filename = "data/db/sakila-db/single_statement.sql";

  std::ifstream stream(filename, std::ios::binary);
  ensure("Error loading sql file", stream.good());
  std::string sql((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

#if VERBOSE_OUTPUT
  test_time_point t1;
#endif

  std::vector<StatementRange> ranges;
  _services->determineStatementRanges(sql.c_str(), sql.size(), ";", ranges);

#if VERBOSE_OUTPUT
  test_time_point t2;

  float time_rate = 1000.0f / (t2 - t1).get_ticks();
  float size_per_sec = size * time_rate / 1024.0f / 1024.0f;
  std::cout << "Splitter performance test (no parsing): " << std::endl
            << "sakila-data.sql was processed in " << (t2 - t1) << " [" << size_per_sec << " MB/sec]" << std::endl;
#endif

  ensure("Unexpected number of statements returned from splitter", ranges.size() == 57);

  std::string s1(sql, ranges[0].start, ranges[0].length);
  ensure("Wrong statement", s1 == "SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0");

  std::string s3(sql, ranges[56].start, ranges[56].length);
  ensure("Wrong statement", s3 == "SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS");

  std::string s2(sql, ranges[30].start, ranges[30].length);

  stream.close();
  stream.open(statement_filename, std::ios::binary);
  ensure("Error loading result file", stream.good());

  sql = std::string((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
  ensure_equals("Wrong statement", s2, sql);
}

//----------------------------------------------------------------------------------------------------------------------

struct TestFile {
  std::string name;
  const char *line_break;
  const char *initial_delmiter;
};

static const TestFile test_files[] = {
  // Large set of all possible query types in different combinations and versions.
  {"data/db/statements.txt", "\n", "$$"},

  // A file with a number of create tables statements that stresses the use
  // of the grammar (e.g. using weird but still valid object names including \n, long
  // list of indices, all possible data types + the default values etc.).
  // Note: it is essential to use \r\n as normal line break in the file to allow usage of \n
  //       in object names.
  {"data/db/nasty_tables.sql", "\r\n", ";"},

#ifndef WITH_VALGRIND
  // Not so many, but some very long insert statements.
  {"data/db/sakila-db/sakila-data.sql", "\n", ";"}
#endif
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

/**
 * Parse a number of files with various statements.
 */
TEST_FUNCTION(10) {
#if VERBOSE_OUTPUT
  test_time_point t1;
#endif

  size_t count = 0;
  for (size_t i = 0; i < sizeof(test_files) / sizeof(test_files[0]); ++i) {
    std::cout << std::endl << "Test file: " << i << std::endl;

#ifdef _MSC_VER
    std::ifstream stream(base::string_to_wstring(test_files[i].name), std::ios::binary);
#else
    std::ifstream stream(test_files[i].name, std::ios::binary);
#endif
    ensure("Error loading sql file: " + test_files[i].name, stream.good());
    std::string sql((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

    std::vector<StatementRange> ranges;
    _services->determineStatementRanges(sql.c_str(), sql.size(), test_files[i].initial_delmiter, ranges,
                                        test_files[i].line_break);
    count += ranges.size();

    // Two loops here, one for an older server and one for the current release.
    size_t j = 0;
    for (auto &range : ranges) {
      std::cout << "." << std::flush; // Need a progress indicator or pb2 might kill our test process.
      if (++j % 50 == 0)
        std::cout << std::endl;

      std::string statement(sql.c_str() + range.start, range.length);
      if (versionMatches(statement, 50610)) {
        if (parse(statement, 50610, "ANSI_QUOTES") > 0U) {
          std::string query(sql.c_str() + range.start, range.length);
          ensure("This query failed to parse:\n" + query, false);
        }
      }
    }
    std::cout << std::endl;

    j = 0;
    for (auto &range : ranges) {
      std::cout << "." << std::flush;
      if (++j % 50 == 0)
        std::cout << std::endl;

      std::string statement(sql.c_str() + range.start, range.length);
      if (versionMatches(statement, 80011)) {
        if (parse(statement, 80011, "ANSI_QUOTES") > 0U) {
          std::string query(sql.c_str() + range.start, range.length);
          ensure("This query failed to parse:\n" + query, false);
        }
      }
    }
    std::cout << std::endl;
  }

#if VERBOSE_OUTPUT
  test_time_point t2;

  std::cout << count << " queries parsed in " << (t2 - t1).get_ticks() / 1000.0 << " s" << std::endl;
#endif
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * This test generates queries with many (all?) MySQL function names used in foreign key creation
 * (parser bug #21114). Taken from the server test suite.
 */

static const char *functions[] = {
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

//----------------------------------------------------------------------------------------------------------------------

TEST_FUNCTION(20) {
  int count = sizeof(functions) / sizeof(functions[0]);
  for (int i = 0; i < count; i++) {
    std::string query = base::strfmt(query1, functions[i]);
    ensure_equals("A statement failed to parse", parse(query, 50530, "ANSI_QUOTES"), 0U);

    query = base::strfmt(query2, functions[i], functions[i]);
    ensure_equals("A statement failed to parse", parse(query, 50530, "ANSI_QUOTES"), 0U);
  }
}

//----------------------------------------------------------------------------------------------------------------------

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

//----------------------------------------------------------------------------------------------------------------------

/**
 * Parses the given string and checks the built AST. Returns true if no error occurred, otherwise false.
 */
bool Test_object_base<mysql_parser_tests>::parseAndCompare(const std::string &sql, long version,
                                                           const std::string &mode, std::vector<size_t> expected,
                                                           size_t expectedErrorCount) {
  if (parse(sql, version, mode) != expectedErrorCount)
    return false;

  // Walk the tokens stored in the parse tree produced by the parse call above and match exactly the given list of token
  // types.
  std::vector<size_t> tokens;
  collectTokenTypes(_lastParseTree, tokens);

  return tokens == expected;
}

//----------------------------------------------------------------------------------------------------------------------

using namespace antlrcpp;

// This evaluator only implements the necessary parts for this test. It's not a full blown evaluator.
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
    results.push_back(result.as<EvalValue>());
    return result;
  }

  virtual Any visitExprNot(MySQLParser::ExprNotContext *context) override {
    EvalValue value = visit(context->expr());
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
    EvalValue left = visit(context->expr(0));
    EvalValue right = visit(context->expr(1));

    if (left.isNullType() || right.isNullType())
      return EvalValue::fromNull();
    return EvalValue::fromBool(asBool(left) && asBool(right));

    return visitChildren(context);
  }

  virtual Any visitExprXor(MySQLParser::ExprXorContext *context) override {
    EvalValue left = visit(context->expr(0));
    EvalValue right = visit(context->expr(1));

    if (left.isNullType() || right.isNullType())
      return EvalValue::fromNull();
    return EvalValue::fromBool(asBool(left) != asBool(right));
  }

  virtual Any visitExprOr(MySQLParser::ExprOrContext *context) override {
    EvalValue left = visit(context->expr(0));
    EvalValue right = visit(context->expr(1));

    if (left.isNullType() || right.isNullType())
      return EvalValue::fromNull();
    return EvalValue::fromBool(asBool(left) || asBool(right));
  }

  virtual Any visitExprIs(MySQLParser::ExprIsContext *context) override {
    EvalValue value = visit(context->boolPri());
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
    EvalValue value = visit(context->boolPri());
    if (context->notRule() == nullptr)
      return EvalValue::fromBool(value.type == EvalValue::Null);
    return EvalValue::fromBool(value.type != EvalValue::Null);
  }

  virtual Any visitPrimaryExprCompare(MySQLParser::PrimaryExprCompareContext *context) override {
    EvalValue left = visit(context->boolPri());
    EvalValue right = visit(context->predicate());

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

    EvalValue left = visit(context->bitExpr(0));
    EvalValue right = visit(context->bitExpr(1));

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
    EvalValue value = visit(context->simpleExpr());
    if (value.isNullType())
      return EvalValue::fromNull();
    return EvalValue::fromBool(!asBool(value));
  }
};

//----------------------------------------------------------------------------------------------------------------------

/**
 * Operator precedence tests.
 */
TEST_FUNCTION(25) {
  // This file is an unmodified copy from the server parser test suite.
  const std::string filename = "data/parser/parser_precedence.result";

  std::ifstream stream(filename, std::ios::binary);
  ensure("25.0 Could not open precedence test file", stream.good());

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
      ensure("25.1 - invalid test file format", !std::getline(stream, sql).eof());
    } else
      sql = line;
    ensure("25.2 - invalid test file format", base::hasPrefix(sql, "select"));

    // The next line either repeats (parts of) the query or contains a server error.
    ensure("25.3 - invalid test file format", !std::getline(stream, line).eof());

    bool expectError = false;
    if (base::hasPrefix(line, "ERROR "))
      expectError = true;

    std::vector<EvalValue> expectedResults;
    if (!expectError) { // No results to compare in an error case.
      ensure("25.5 - invalid test file format", !std::getline(stream, line).eof());
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

    ensure_equals("25.4 - error status is unexpected for query (" + std::to_string(counter) + "): \n" + sql + "\n",
                  (parse(sql, 80012, "") == 0), !expectError);
    if (expectError) {
      ++counter;
      continue;
    }

    EvalParseVisitor evaluator;
    try {
      evaluator.visit(_lastParseTree);
    } catch (std::bad_cast &) {
      std::cout << "25.6 Query failed to evaluate: \"\n" + sql + "\"\n";
      std::cout << "25.7 Parse tree: " << _lastParseTree->toStringTree(&_parser) << std::endl;
      throw;
    }

    ensure_equals("25.8 - result counts differ for query (" + std::to_string(counter) + "): \n\"" + sql + "\"\n",
                  evaluator.results.size(), expectedResults.size());

    static std::string dataTypes[] = {"FLOAT", "INT", "NULL", "NOT NULL"};
    for (size_t i = 0; i < expectedResults.size(); ++i) {
      // We have no int type in expected results. So we make float and int being the same.
      EvalValue::ValueType type = evaluator.results[i].type;
      if (type == EvalValue::Int)
        type = EvalValue::Float;
      EvalValue::ValueType expectedType = expectedResults[i].type;

      ensure_equals("25.9 - result type " + std::to_string(i) + " differs for query (" + std::to_string(counter) +
                      "): \n\"" + sql + "\"\n",
                    dataTypes[type], dataTypes[expectedType]);
      if (!expectedResults[i].isNullType())
        ensure_equals("25.10 - result " + std::to_string(i) + " differs for query (" + std::to_string(counter) +
                        "): \n\"" + sql + "\"\n",
                      evaluator.results[i].number, expectedResults[i].number);
    }

    ++counter;
  }
}

//----------------------------------------------------------------------------------------------------------------------

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

static const std::vector<std::vector<size_t>> sqlModeTestResults = {
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

//----------------------------------------------------------------------------------------------------------------------

TEST_FUNCTION(30) {
  for (size_t i = 0; i < sqlModeTestQueries.size(); i++)
    if (!parseAndCompare(sqlModeTestQueries[i].query, 80012, sqlModeTestQueries[i].sqlMode, sqlModeTestResults[i],
                         sqlModeTestQueries[i].errors)) {
      fail("30." + std::to_string(i) + ": SQL mode test - query failed: " + sqlModeTestQueries[i].query);
    }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Tests the parser's string concatenation feature.
 */
TEST_FUNCTION(35) {
  class TestListener : public MySQLParserBaseListener {
  public:
    std::string text;

    virtual void exitTextLiteral(MySQLParser::TextLiteralContext *ctx) override {
      text = MySQLParser::getText(ctx, true);
    }
  };

  ensure_equals("35.1 String concatenation", parse("select \"abc\" \"def\" 'ghi''\\n\\z'", 80012, ""), 0U);
  TestListener listener;
  tree::ParseTreeWalker::DEFAULT.walk(&listener, _lastParseTree);
  ensure_equals("35.2 String concatenation", listener.text, "abcdefghi'\nz");
}

//----------------------------------------------------------------------------------------------------------------------

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
  VersionTestData(50500, "grant all privileges on a to mike identified by password 'blah'", 0U),
  VersionTestData(50710, "grant all privileges on a to mike identified by password 'blah'", 0U),
  VersionTestData(50100, "grant select on *.* to mike identified with 'blah'", 1U),
  VersionTestData(50600, "grant select on *.* to mike identified with 'blah'", 0U),
  VersionTestData(50600, "grant select on *.* to mike identified with blah as 'blubb'", 0U),
  VersionTestData(50100, "grant select on *.* to mike identified with blah by 'blubb'", 1U),
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

// TODO: create tests for all server version dependent features.
// Will be obsolete if we support versions in the statements test file (or similar file).

//----------------------------------------------------------------------------------------------------------------------

TEST_FUNCTION(40) {
  // Version dependent parts of GRANT.
  for (size_t i = 0; i < versionTestResults.size(); ++i) {
    ensure_equals("40." + std::to_string(i) + " grant",
                  parse(versionTestResults[i].sql, versionTestResults[i].version, ""),
                  versionTestResults[i].errorCount);
  }
}

//----------------------------------------------------------------------------------------------------------------------

// TODO: create tests for restricted content parsing (e.g. routines only, views only etc.).

// Test hex, binary, float, decimal and int number handling.
static const std::vector<SqlModeTestEntry> numbersTestQueries = {
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

static const std::vector<std::vector<size_t>> numbersTestResults = {
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

//----------------------------------------------------------------------------------------------------------------------

TEST_FUNCTION(45) {
  for (size_t i = 0; i < numbersTestQueries.size(); i++)
    if (!parseAndCompare(numbersTestQueries[i].query, 80012, numbersTestQueries[i].sqlMode, numbersTestResults[i],
                         numbersTestQueries[i].errors)) {
      fail("45." + std::to_string(i) + ": number test - query failed: " + numbersTestQueries[i].query);
    }
}

//----------------------------------------------------------------------------------------------------------------------

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  delete _tester;
}

END_TESTS;

//----------------------------------------------------------------------------------------------------------------------
