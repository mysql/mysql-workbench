/* 
 * Copyright (c) 2012, 2016, Oracle and/or its affiliates. All rights reserved.
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
#include "MySQLParser.h"
#include "MySQLParserBaseListener.h"
#include "MySQLParserBaseVisitor.h"

#include "grtsqlparser/mysql_parser_services.h"

// This file contains unit tests for the statement splitter and the ANTLR based parser.
// These are low level tests. There's another set of high level tests (see test_mysql_sql_parser.cpp).

#define VERBOSE_OUTPUT 0

using namespace parsers;
using namespace antlr4;
using namespace antlr4::atn;
using namespace antlr4::tree;

//--------------------------------------------------------------------------------------------------

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
  Ref<MySQLParser::QueryContext> _lastParseTree;

  MySQLParserServices *_services;

  size_t parse(const std::string sql, long version, const std::string &mode);
  bool parseAndCompare(const std::string &sql, long version, const std::string &mode, std::vector<int> tokens,
                       size_t expectedErrorCount = 0);

TEST_DATA_CONSTRUCTOR(mysql_parser_tests) : _lexer(&_input), _tokens(&_lexer), _parser(&_tokens)
{
  _tester = new WBTester();
  // init datatypes
  populate_grt(*_tester);

  // The charset list contains also the 3 charsets that were introduced in 5.5.3.
  grt::ListRef<db_CharacterSet> list= _tester->get_rdbms()->characterSets();
  for (size_t i = 0; i < list->count(); i++)
    _charsets.insert("_" + base::tolower(*list[i]->name()));

  _lexer.charsets = _charsets;
  _lexer.removeErrorListeners();
  _parser.removeErrorListeners();

  _services = MySQLParserServices::get();
}
END_TEST_DATA_CLASS

TEST_MODULE(mysql_parser_tests, "MySQL parser test suite (ANTLR)");

//--------------------------------------------------------------------------------------------------

/**
 * Parses the given string and returns the number of errors found.
 */
size_t Test_object_base<mysql_parser_tests>::parse(const std::string sql, long version, const std::string &mode)
{
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
  } catch (ParseCancellationException &pce) {
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

//--------------------------------------------------------------------------------------------------

/**
 *  Statement splitter test.
 */
TEST_FUNCTION(5)
{
  std::string filename = "data/db/sakila-db/sakila-data.sql";
  std::string statement_filename = "data/db/sakila-db/single_statement.sql";

  std::ifstream stream(filename, std::ios::binary);
  ensure("Error loading sql file", stream.good());
  std::string sql((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

#if VERBOSE_OUTPUT
  test_time_point t1;
#endif

  std::vector<std::pair<size_t, size_t>> ranges;
  _services->determineStatementRanges(sql.c_str(), sql.size(), ";", ranges);
 
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

  stream.close();
  stream.open(statement_filename, std::ios::binary);
  ensure("Error loading result file", stream.good());

  sql = std::string((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
  ensure_equals("Wrong statement", s2, sql);
}

struct TestFile 
{
  std::string name;
  const char *line_break;
  const char *initial_delmiter;
};

static const TestFile test_files[] = {
  // Large set of all possible query types in different combinations.
  {"data/db/statements.txt", "\n", "$$"},

  // A file with a number of create tables statements that stresses the use
  // of the grammar (e.g. using weird but still valid object names including \n, long
  // list of indices, all possible data types + the default values etc.).
  // Note: it is essential to use \r\n as normal line break in the file to allow usage of \n
  //       in object names.
  {"data/db/nasty_tables.sql", "\r\n", ";"},

  // Not so many, but some very long insert statements.
  {"data/db/sakila-db/sakila-data.sql", "\n", ";"}
};

/**
 * Parse a number of files with various statements.
 */
TEST_FUNCTION(10)
{
#if VERBOSE_OUTPUT
  test_time_point t1;
#endif

  size_t count = 0;
  for (size_t i = 0; i < sizeof(test_files) / sizeof(test_files[0]); ++i)
  {
#ifdef _WIN32
    std::ifstream stream(base::string_to_wstring(test_files[i].name), std::ios::binary);
#else
    std::ifstream stream(test_files[i].name, std::ios::binary);
#endif
    ensure("Error loading sql file: " + test_files[i].name, stream.good());
    std::string sql((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

    std::vector<std::pair<size_t, size_t>> ranges;
    _services->determineStatementRanges(sql.c_str(), sql.size(), test_files[i].initial_delmiter, ranges, test_files[i].line_break);
    count += ranges.size();

    for (auto &range : ranges)
    {
      if (parse(std::string(sql.c_str() + range.first, range.second), 50610, "ANSI_QUOTES") > 0U)
      {
        std::string query(sql.c_str() + range.first, range.second);
        parse(query, 50610, "ANSI_QUOTES");
        ensure("This query failed to parse:\n" + query, false);
      }
    }
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
    ensure_equals("A statement failed to parse", parse(query, 50530, "ANSI_QUOTES"), 0U);

    query = base::strfmt(query2, functions[i], functions[i]);
    ensure_equals("A statement failed to parse", parse(query, 50530, "ANSI_QUOTES"), 0U);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void collectTokenTypes(Ref<RuleContext> context, std::vector<int> &list)
{
  for (size_t index = 0; index < context->getChildCount(); ++index)
  {
    Ref<tree::ParseTree> child = context->getChild(index);
    if (antlrcpp::is<RuleContext>(child))
      collectTokenTypes(std::dynamic_pointer_cast<RuleContext>(child), list);
    else {
      // A terminal node.
      Ref<tree::TerminalNode> node = std::dynamic_pointer_cast<tree::TerminalNode>(child);
      antlr4::Token *token = node->getSymbol();
      list.push_back(token->getType());
    }
  }
}

/**
 * Parses the given string and checks the built AST. Returns true if no error occurred, otherwise false.
 */
bool Test_object_base<mysql_parser_tests>::parseAndCompare(const std::string &sql, long version, const std::string &mode,
  std::vector<int> expected, size_t expectedErrorCount)
{
  if (parse(sql, version, mode) != expectedErrorCount)
    return false;

  // Walk the tokens stored in the parse tree produced by the parse call above and match exactly the given list of token types.
  std::vector<int> tokens;
  collectTokenTypes(_lastParseTree, tokens);

  return tokens == expected;
}

//----------------------------------------------------------------------------------------------------------------------

using namespace antlrcpp;

// This evaluator only implements the necessary parts for this test. It's not a full blown evaluator.
struct EvalValue
{
  enum ValueType { Float, Int, Null, NotNull } type; // NULL and NOT NULL are possible literals, so we need an enum for them.
  double number;

  EvalValue (ValueType aType, double aNumber) : type(aType), number(aNumber) {}
  bool isNullType() { return type == Null || type == NotNull; }

  static EvalValue fromBool(bool value) { return EvalValue(Int, value ? 1 : 0); };
  static EvalValue fromNumber(double number) { return EvalValue(Float, number); };
  static EvalValue fromInt(long long number) { return EvalValue(Int, number); };
  static EvalValue fromNull() { return EvalValue(Null, 0); };
  static EvalValue fromNotNull() { return EvalValue(NotNull, 0); };
};

class EvalParseVisitor : public MySQLParserBaseVisitor
{
public:
  std::vector<EvalValue> results; // One entry for each select item.

  bool asBool(EvalValue in)
  {
    if (!in.isNullType() && in.number != 0)
      return true;
    return false;
  };

  virtual Any visitSelect_item(MySQLParser::Select_itemContext *context) override
  {
    Any result = visitChildren(context);
    results.push_back(result.as<EvalValue>());
    return result;
  }

  virtual Any visitExprNot(MySQLParser::ExprNotContext *context) override
  {
    EvalValue value = visit(context->expr().get());
    switch (value.type)
    {
      case EvalValue::Null:
        return EvalValue::fromNotNull();
      case EvalValue::NotNull:
        return EvalValue::fromNull();
      default:
        return EvalValue::fromBool(!asBool(value));
    }
  }

  virtual Any visitExprAnd(MySQLParser::ExprAndContext *context) override
  {
    EvalValue left = visit(context->expr(0).get());
    EvalValue right = visit(context->expr(1).get());

    if (left.isNullType() || right.isNullType())
      return EvalValue::fromNull();
    return EvalValue::fromBool(asBool(left) && asBool(right));

    return visitChildren(context);
  }
  
  virtual Any visitExprXor(MySQLParser::ExprXorContext *context) override
  {
    EvalValue left = visit(context->expr(0).get());
    EvalValue right = visit(context->expr(1).get());

    if (left.isNullType() || right.isNullType())
      return EvalValue::fromNull();
    return EvalValue::fromBool(asBool(left) != asBool(right));
  }

  virtual Any visitExprOr(MySQLParser::ExprOrContext *context) override
  {
    EvalValue left = visit(context->expr(0).get());
    EvalValue right = visit(context->expr(1).get());

    if (left.isNullType() || right.isNullType())
      return EvalValue::fromNull();
    return EvalValue::fromBool(asBool(left) || asBool(right));
  }
  
  virtual Any visitExprIs(MySQLParser::ExprIsContext *context) override
  {
    EvalValue value = visit(context->bool_pri().get());
    if (context->IS_SYMBOL() == nullptr)
      return value;

    bool result = false;
    switch (context->type->getType())
    {
      case MySQLLexer::FALSE_SYMBOL:
      case MySQLLexer::TRUE_SYMBOL:
        if (!value.isNullType())
          result = asBool(value);
        break;
      default: // Must be UNKOWN.
        result = value.isNullType();
        break;
    }

    if (context->not_rule() != nullptr)
      result = !result;
    return EvalValue::fromBool(result);
  }

  virtual Any visitPrimaryExprIsNull(MySQLParser::PrimaryExprIsNullContext *context) override
  {
    EvalValue value = visit(context->bool_pri().get());
    if (context->not_rule() == nullptr)
      return EvalValue::fromBool(value.type == EvalValue::Null);
    return EvalValue::fromBool(value.type != EvalValue::Null);
  }

  virtual Any visitPrimaryExprCompare(MySQLParser::PrimaryExprCompareContext *context) override
  {
    EvalValue left = visit(context->bool_pri().get());
    EvalValue right = visit(context->predicate().get());

    ssize_t op = context->comp_op()->getStart()->getType();
    if (left.isNullType() || right.isNullType())
    {
      if (op == MySQLLexer::NULL_SAFE_EQUAL_OPERATOR)
        return EvalValue::fromBool(left.type == right.type);
      return EvalValue::fromNull();
    }

    switch (op)
    {
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

  virtual Any visitPredicateExprOperations(MySQLParser::PredicateExprOperationsContext *context) override
  {
    return visit(context->bit_expr().get());
  }

  template <class Op>
  Any evalBitExpression(MySQLParser::Bit_exprContext *context, Op op)
  {
    // Always in the form "bit_expr op bit_expr".
    EvalValue left = visit(context->children[0].get());
    EvalValue right = visit(context->children[2].get());

    if (left.isNullType() || right.isNullType())
      return EvalValue::fromNull();

    return EvalValue::fromNumber(op(left.number, right.number));
  }
  
  virtual Any visitBitExprXor(MySQLParser::BitExprXorContext *context) override
  {
    return evalBitExpression(context, std::bit_xor<int>());
  }
  
  virtual Any visitBitExprMult(MySQLParser::BitExprMultContext *context) override
  {
    switch (context->op->getType())
    {
      case MySQLLexer::MULT_OPERATOR:
        return evalBitExpression(context, std::multiplies<double>());
      case MySQLLexer::DIV_OPERATOR:
        return evalBitExpression(context, std::divides<double>());
      case MySQLLexer::DIV_SYMBOL:
        return evalBitExpression(context, std::divides<long long>());
      case MySQLLexer::MOD_OPERATOR:
      case MySQLLexer::MOD_SYMBOL:
      {
        EvalValue left = visit(context->bit_expr(0).get());
        EvalValue right = visit(context->bit_expr(1).get());

        if (left.isNullType() || right.isNullType())
          return EvalValue::fromNull();

        if (left.type == EvalValue::Int && right.type == EvalValue::Int)
          return EvalValue::fromInt((long long)left.number % (long long)right.number);
        return EvalValue::fromNumber(fmod(left.number, right.number));
      }
    }
    return EvalValue::fromNull();
  }

  virtual Any visitBitExprAdd(MySQLParser::BitExprAddContext *context) override
  {
    if (context->op->getType() == MySQLLexer::PLUS_OPERATOR)
      return evalBitExpression(context, std::plus<double>());
    return evalBitExpression(context, std::minus<double>());
  }

  static unsigned long long shiftLeftWithOverflow(double l, double r)
  {
    // Shift with overflow if r is larger than the data type size of unsigned long long (64)
    // (which would be undefined if using the standard << operator).
    std::bitset<64> bits = (unsigned long long)llround(l);
    bits <<= (unsigned long long)llround(r);
    return bits.to_ullong();
  }

  virtual Any visitBitExprShift(MySQLParser::BitExprShiftContext *context) override
  {
    if (context->op->getType() == MySQLLexer::SHIFT_LEFT_OPERATOR)
      return evalBitExpression(context, [](double l, double r) { return shiftLeftWithOverflow(l, r); });
    return evalBitExpression(context, [](double l, double r) { return (unsigned long long)llround(l) >> (unsigned long long)llround(r); });
  }

  virtual Any visitBitExprAnd(MySQLParser::BitExprAndContext *context) override
  {
    return evalBitExpression(context, std::bit_and<int>());
  }

  virtual Any visitBitExprOr(MySQLParser::BitExprOrContext *context) override
  {
    return evalBitExpression(context, std::bit_or<int>());
  }

  virtual Any visitExpression_list_with_parentheses(MySQLParser::Expression_list_with_parenthesesContext *context) override
  {
    return visit(context->expression_list().get());
  }

  virtual Any visitSimpleExprList(MySQLParser::SimpleExprListContext *context) override
  {
    return visit(context->expression_list().get());
  }

  virtual Any visitSimpleExprLiteral(MySQLParser::SimpleExprLiteralContext *context) override
  {
    switch (context->start->getType())
    {
      case MySQLLexer::TRUE_SYMBOL:
        return EvalValue::fromBool(true);
      case MySQLLexer::FALSE_SYMBOL:
        return EvalValue::fromBool(false);
      case MySQLLexer::NULL_SYMBOL:
        return EvalValue::fromNull();
      case MySQLLexer::HEX_NUMBER:
        return EvalValue::fromInt(std::stoul(context->start->getText(), nullptr, 16));
      case MySQLLexer::BIN_NUMBER:
      {
        std::bitset<64> bits(context->start->getText());
        return EvalValue::fromInt(bits.to_ullong());
      }
      case MySQLLexer::INT_NUMBER:
      {
        std::string text = context->start->getText();
        return EvalValue::fromInt(std::stoul(text, nullptr, 10));
      }
      default:
        return EvalValue::fromNumber(std::atof(context->start->getText().c_str()));
    }
  }

  virtual Any visitSimpleExprNot(MySQLParser::SimpleExprNotContext *context) override
  {
    EvalValue value = visit(context->simple_expr().get());
    if (value.isNullType())
      return EvalValue::fromNull();
    return EvalValue::fromBool(!asBool(value));
  }
};


/**
 * Operator precedence tests.
 */
TEST_FUNCTION(25)
{
  // This file is an unmodified copy from the server parser test suite.
  const std::string filename = "data/parser/parser_precedence.result";

  std::ifstream stream(filename, std::ios::binary);
  ensure("25.0 Could not open precedence test file", stream.good());

  std::string line;

  // Start with skipping over some lines that contain results which require a real server (querying from a table).
  while (!std::getline(stream, line).eof())
  {
    if (line == "drop table t1_30237_bool;")
      break;
  }

  int skip = 0; // Used to skip not relevant tests while fixing a test.
  int counter = 1;
  while (!std::getline(stream, line).eof())
  {
    if (base::trim(line).empty())
      continue;

    // Start of a new test. The test description is optional.
    std::string sql;
    if (base::hasPrefix(line, "Testing "))
    {
      ensure("25.1 - invalid test file format", !std::getline(stream, sql).eof());
    }
    else
      sql = line;
    ensure("25.2 - invalid test file format", base::hasPrefix(sql, "select"));

    // The next line either repeats (parts of) the query or contains a server error.
    ensure("25.3 - invalid test file format", !std::getline(stream, line).eof());

    bool expectError = false;
    if (base::hasPrefix(line, "ERROR "))
      expectError = true;
    ensure_equals("25.4 - error status is unexpected for query: \"\n" + sql + "\"\n", (parse(sql, 50630, "") == 0), !expectError);

    if (expectError) // No results to compare in an error case.
    {
      ++counter;
      --skip;
      continue;
    }

    // Compare results if there was no error.
    ensure("25.5 - invalid test file format", !std::getline(stream, line).eof());
    std::vector<EvalValue> expectedResults;
    std::string temp;
    std::stringstream stream(line);
    while (stream >> temp)
    {
      if (base::same_string(temp, "true"))
        expectedResults.push_back(EvalValue::fromBool(true));
      if (base::same_string(temp, "false"))
        expectedResults.push_back(EvalValue::fromBool(true));
      if (base::same_string(temp, "null"))
        expectedResults.push_back(EvalValue::fromNull());
      expectedResults.push_back(EvalValue::fromNumber(std::atof(temp.c_str())));
    }

    if (--skip >= 0)
    {
      ++counter;
      continue;
    }

    EvalParseVisitor evaluator;
    try
    {
      evaluator.visit(_lastParseTree.get());
    }
    catch (std::bad_cast &)
    {
      std::cout << "25.6 Query failed to evaluate: \"\n" + sql + "\"\n";
      std::cout << "25.7 Parse tree: " << _lastParseTree->toStringTree(&_parser) << std::endl;
      throw;
    }

    ensure_equals("25.8 - result counts differ for query (" + std::to_string(counter) + "): \n\"" + sql + "\"\n",
                  evaluator.results.size(), expectedResults.size());

    static std::string dataTypes[] = { "FLOAT", "INT", "NULL", "NOT NULL" };
    for (size_t i = 0; i < expectedResults.size(); ++i)
    {
      // We have no int type in expected results. So we make float and int being the same.
      EvalValue::ValueType type = evaluator.results[i].type;
      if (type == EvalValue::Int)
        type = EvalValue::Float;
      EvalValue::ValueType expectedType = expectedResults[i].type;
      
      ensure_equals("25.9 - result type " + std::to_string(i) + " differs for query (" + std::to_string(counter) + "): \n\"" + sql + "\"\n",
                    dataTypes[type], dataTypes[expectedType]);
      if (!expectedResults[i].isNullType())
        ensure_equals("25.10 - result " + std::to_string(i) + " differs for query (" + std::to_string(counter) + "): \n\"" + sql + "\"\n",
                      evaluator.results[i].number, expectedResults[i].number);
    }

    ++counter;
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
  { "create table count (id int)", "", 0 },
  { "create table count(id int)", "", 1 },
  { "create table count (id int)", "IGNORE_SPACE", 2 },
  { "create table count(id int)", "IGNORE_SPACE", 1 },
  { "create table xxx (id int)", "", 0 },
  { "create table xxx(id int)", "", 0 },
  { "create table xxx (id int)", "IGNORE_SPACE", 0 },
  { "create table xxx(id int)", "IGNORE_SPACE", 0 },

  // ANSI_QUOTES
  { "select \"abc\" \"def\" 'ghi''\\n\\Z\\z'", "", 0 }, // Double + single quoted text concatenated.
  { "select \"abc\" \"def\" as 'ghi\\n\\Z\\z'", "", 0 }, // Double quoted text concatenated + alias.
  { "select \"abc\" \"def\" 'ghi''\\n\\Z\\z'", "ANSI_QUOTES", 2 }, // column ref + alias + invalid single quoted text.
  
  // PIPES_AS_CONCAT
  { "select \"abc\" || \"def\"", "", 0 },
  { "select \"abc\" || \"def\"", "PIPES_AS_CONCAT", 0 },
  
  // HIGH_NOT_PRECEDENCE
  { "select not 1 between -5 and 5", "", 0 },
  { "select not 1 between -5 and 5", "HIGH_NOT_PRECEDENCE", 0 },
  
  // NO_BACKSLASH_ESCAPES
  { "select \"abc \\\"def\"", "", 0 },
  { "select \"abc \\\"def\"", "NO_BACKSLASH_ESCAPES", 1 },

  // TODO: add tests for sql modes that are synonyms for a combination of the base modes.
};

using P = MySQLParser;

static const std::vector<std::vector<int>> sqlModeTestResults = {
  { P::CREATE_SYMBOL, P::TABLE_SYMBOL, P::IDENTIFIER, P::OPEN_PAR_SYMBOL, P::IDENTIFIER, P::INT_SYMBOL, P::CLOSE_PAR_SYMBOL, Token::EOF },
  { P::CREATE_SYMBOL, P::TABLE_SYMBOL, P::COUNT_SYMBOL, P::OPEN_PAR_SYMBOL, P::IDENTIFIER, P::INT_SYMBOL, P::CLOSE_PAR_SYMBOL, Token::EOF },
  { P::CREATE_SYMBOL, P::TABLE_SYMBOL, P::OPEN_PAR_SYMBOL, P::IDENTIFIER, P::INT_SYMBOL, P::CLOSE_PAR_SYMBOL, Token::EOF },
  { P::CREATE_SYMBOL, P::TABLE_SYMBOL, P::COUNT_SYMBOL, P::OPEN_PAR_SYMBOL, P::IDENTIFIER, P::INT_SYMBOL, P::CLOSE_PAR_SYMBOL, Token::EOF },
  { P::CREATE_SYMBOL, P::TABLE_SYMBOL, P::IDENTIFIER, P::OPEN_PAR_SYMBOL, P::IDENTIFIER, P::INT_SYMBOL, P::CLOSE_PAR_SYMBOL, Token::EOF },
  { P::CREATE_SYMBOL, P::TABLE_SYMBOL, P::IDENTIFIER, P::OPEN_PAR_SYMBOL, P::IDENTIFIER, P::INT_SYMBOL, P::CLOSE_PAR_SYMBOL, Token::EOF },
  { P::CREATE_SYMBOL, P::TABLE_SYMBOL, P::IDENTIFIER, P::OPEN_PAR_SYMBOL, P::IDENTIFIER, P::INT_SYMBOL, P::CLOSE_PAR_SYMBOL, Token::EOF },
  { P::CREATE_SYMBOL, P::TABLE_SYMBOL, P::IDENTIFIER, P::OPEN_PAR_SYMBOL, P::IDENTIFIER, P::INT_SYMBOL, P::CLOSE_PAR_SYMBOL, Token::EOF },

  { P::SELECT_SYMBOL, P::DOUBLE_QUOTED_TEXT, P::DOUBLE_QUOTED_TEXT, P::SINGLE_QUOTED_TEXT, P::SINGLE_QUOTED_TEXT, Token::EOF },
  { P::SELECT_SYMBOL, P::DOUBLE_QUOTED_TEXT, P::DOUBLE_QUOTED_TEXT, P::AS_SYMBOL, P::SINGLE_QUOTED_TEXT, Token::EOF },
  { P::SELECT_SYMBOL, P::DOUBLE_QUOTED_TEXT, P::DOUBLE_QUOTED_TEXT, P::SINGLE_QUOTED_TEXT, P::SINGLE_QUOTED_TEXT, Token::EOF },

  { P::SELECT_SYMBOL, P::DOUBLE_QUOTED_TEXT, P::LOGICAL_OR_OPERATOR, P::DOUBLE_QUOTED_TEXT, Token::EOF },
  { P::SELECT_SYMBOL, P::DOUBLE_QUOTED_TEXT, P::CONCAT_PIPES_SYMBOL, P::DOUBLE_QUOTED_TEXT, Token::EOF },

  { P::SELECT_SYMBOL, P::NOT_SYMBOL, P::INT_NUMBER, P::BETWEEN_SYMBOL, P::MINUS_OPERATOR, P::INT_NUMBER, P::AND_SYMBOL, P::INT_NUMBER, Token::EOF },
  { P::SELECT_SYMBOL, P::NOT2_SYMBOL, P::INT_NUMBER, P::BETWEEN_SYMBOL, P::MINUS_OPERATOR, P::INT_NUMBER, P::AND_SYMBOL, P::INT_NUMBER, Token::EOF },

  { P::SELECT_SYMBOL, P::DOUBLE_QUOTED_TEXT, Token::EOF },
  { P::SELECT_SYMBOL, P::DOUBLE_QUOTED_TEXT, P::IDENTIFIER, Token::EOF },
};

TEST_FUNCTION(30)
{
  for (size_t i = 0; i < sqlModeTestQueries.size(); i++)
    if (!parseAndCompare(sqlModeTestQueries[i].query, 50610, sqlModeTestQueries[i].sqlMode,
      sqlModeTestResults[i], sqlModeTestQueries[i].errors))
    {
      fail("30." + std::to_string(i) + ": SQL mode test - query failed: " + sqlModeTestQueries[i].query);
    }
}

/**
 * Tests the parser's string concatenation feature.
 */
TEST_FUNCTION(35)
{
  class TestListener : public MySQLParserBaseListener {
  public:
    std::string text;

    virtual void exitString_literal(MySQLParser::String_literalContext *ctx) override
    {
      text = MySQLParser::getText(ctx, true);
    }

  };

  ensure_equals("35.1 String concatenation", parse("select \"abc\" \"def\" 'ghi''\\n\\z'", 50610, ""), 0U);
  TestListener listener;
  tree::ParseTreeWalker::DEFAULT.walk(&listener, _lastParseTree);
  ensure_equals("35.2 String concatenation", listener.text, "abcdefghi'\nz");
}

struct VersionTestData
{
  long version;
  std::string sql;
  size_t errorCount;
  VersionTestData(long version_, const std::string &sql_, size_t errors_)
  {
    version = version_;
    sql = sql_;
    errorCount = errors_;
  }

};

const std::vector<VersionTestData> versionTestResults = {
  VersionTestData(50100, "grant all privileges on a to mike", 0U),
  VersionTestData(50100, "grant all privileges on a to mike identified by 'blah'", 0U),
  VersionTestData(50100, "grant all privileges on a to mike identified by password 'blah'", 0U),
  VersionTestData(50100, "grant all privileges on a to mike identified by password 'blah'", 0U),
  VersionTestData(50500, "grant all privileges on a to mike identified by password 'blah'", 0U),
  VersionTestData(50710, "grant all privileges on a to mike identified by password 'blah'", 0U),
  VersionTestData(50100, "grant select on *.* to mike identified with 'blah'", 2U),
  VersionTestData(50600, "grant select on *.* to mike identified with 'blah'", 0U),
  VersionTestData(50100, "grant select on *.* to mike identified with blah as 'blubb'", 2U),
  VersionTestData(50600, "grant select on *.* to mike identified with blah as 'blubb'", 0U),
  VersionTestData(50100, "grant select on *.* to mike identified with blah by 'blubb'", 2U),
  VersionTestData(50600, "grant select on *.* to mike identified with blah by 'blubb'", 1U),
  VersionTestData(50706, "grant select on *.* to mike identified with blah by 'blubb'", 0U)
};

// TODO: create tests for all server version dependent features.
// Will be obsolete if we support versions in the statements test file (or similar file).

TEST_FUNCTION(40)
{
  // Version dependent parts of GRANT.
  for (size_t i = 0; i < versionTestResults.size(); ++i)
  {
    ensure_equals("40." + std::to_string(i) + " grant",
      parse(versionTestResults[i].sql, versionTestResults[i].version, ""), versionTestResults[i].errorCount);
  }
}

// TODO: create tests for restricted content parsing (e.g. routines only, views only etc.).

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99)
{
  delete _tester;
}

END_TESTS;

//----------------------------------------------------------------------------------------------------------------------

