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

#include "base/log.h"
#include "base/string_utilities.h"

#include "mforms/code_editor.h"

#include "grtdb/db_helpers.h"
#include "workbench/wb_backend_public_interface.h"
#include "wb_sql_editor_help.h"

#include "mysql-recognition-types.h"
#include "MySQLLexer.h"
#include "MySQLParser.h"

DEFAULT_LOG_DOMAIN("Context help")

using namespace parsers;
using namespace antlr4;

using namespace help;

class HelpContext::Private
{
public:
  Private() : lexer(&input), tokens(&lexer), parser(&tokens) {}

  ANTLRInputStream input;
  MySQLLexer lexer;
  CommonTokenStream tokens;
  MySQLParser parser;

  std::unordered_set<std::string> _functionNames;

  ParserRuleContext* parse(const std::string &query)
  {
    input.load(query);
    lexer.reset();
    lexer.setInputStream(&input);
    tokens.setTokenSource(&lexer);

    parser.reset();
    return parser.query();
  }
};

//----------------- HelpContext ----------------------------------------------------------------------------------------

HelpContext::HelpContext(GrtCharacterSetsRef charsets, const std::string &sqlMode, long serverVersion)
{
  _d = new Private();
  std::set<std::string> filteredCharsets;
  for (size_t i = 0; i < charsets->count(); i++)
    filteredCharsets.insert("_" + base::tolower(*charsets[i]->name()));

  if (_d->lexer.serverVersion < 50503)
  {
    filteredCharsets.erase("_utf8mb4");
    filteredCharsets.erase("_utf16");
    filteredCharsets.erase("_utf32");
  }
  else
  {
    filteredCharsets.insert("_utf8mb4");
    filteredCharsets.insert("_utf16");
    filteredCharsets.insert("_utf32");
  }
  _d->lexer.charsets = filteredCharsets;
  _d->lexer.serverVersion = serverVersion;

  _d->lexer.sqlModeFromString(sqlMode);
  _d->parser.sqlMode = _d->lexer.sqlMode;

  _d->parser.serverVersion = serverVersion;
  _d->parser.removeParseListeners();
  _d->parser.removeErrorListeners();

  mforms::SyntaxHighlighterLanguage language = mforms::LanguageMySQL;
  switch (serverVersion / 10000)
  {
    case 5:
      switch ((serverVersion / 100) % 100)
    {
      case 0: language = mforms::LanguageMySQL50; break;
      case 1: language = mforms::LanguageMySQL51; break;
      case 5: language = mforms::LanguageMySQL55; break;
      case 6: language = mforms::LanguageMySQL56; break;
      case 7: language = mforms::LanguageMySQL57; break;
    }
      break;

    case 8:
      switch ((serverVersion / 100) % 100)
    {
      case 0: language = mforms::LanguageMySQL80; break;
    }
      break;
  }

  mforms::CodeEditorConfig config(language);
  std::vector<std::string> names = base::split(config.get_keywords()["Functions"], " ");
  _d->_functionNames.insert(names.begin(), names.end());
}

//----------------------------------------------------------------------------------------------------------------------

HelpContext::~HelpContext()
{
  delete _d;
}

//----------------- DbSqlEditorContextHelp -----------------------------------------------------------------------------

DbSqlEditorContextHelp* DbSqlEditorContextHelp::get()
{
  static DbSqlEditorContextHelp instance;
  return &instance;
}

//----------------------------------------------------------------------------------------------------------------------

static const std::unordered_set<std::string> availableTopics = {
  "!",
  "!=",
  "%",
  "&",
  "*",
  "+",
  "- BINARY",
  "- UNARY",
  "->",
  "/",
  "<",
  "<<",
  "<=",
  "<=>",
  "=",
  ">",
  ">=",
  ">>",
  "ABS",
  "ACOS",
  "ADDDATE",
  "ADDTIME",
  "AES_DECRYPT",
  "AES_ENCRYPT",
  "ALTER DATABASE",
  "ALTER EVENT",
  "ALTER FUNCTION",
  "ALTER PROCEDURE",
  "ALTER SERVER",
  "ALTER TABLE",
  "ALTER USER",
  "ALTER VIEW",
  "ANALYZE TABLE",
  "AND",
  "ANY_VALUE",
  "AREA",
  "ASBINARY",
  "ASCII",
  "ASIN",
  "ASSIGN-EQUAL",
  "ASSIGN-VALUE",
  "ASTEXT",
  "ASYMMETRIC_DECRYPT",
  "ASYMMETRIC_DERIVE",
  "ASYMMETRIC_ENCRYPT",
  "ASYMMETRIC_SIGN",
  "ASYMMETRIC_VERIFY",
  "ATAN",
  "ATAN2",
  "AUTO_INCREMENT",
  "AVG",
  "BEGIN END",
  "BENCHMARK",
  "BETWEEN AND",
  "BIGINT",
  "BIN",
  "BINARY",
  "BINARY OPERATOR",
  "BINLOG",
  "BIT",
  "BIT_AND",
  "BIT_COUNT",
  "BIT_LENGTH",
  "BIT_OR",
  "BIT_XOR",
  "BLOB",
  "BLOB DATA TYPE",
  "BOOLEAN",
  "BUFFER",
  "CACHE INDEX",
  "CALL",
  "CASE OPERATOR",
  "CASE STATEMENT",
  "CAST",
  "CEIL",
  "CEILING",
  "CENTROID",
  "CHANGE MASTER TO",
  "CHANGE REPLICATION FILTER",
  "CHAR",
  "CHAR BYTE",
  "CHAR FUNCTION",
  "CHARACTER_LENGTH",
  "CHARSET",
  "CHAR_LENGTH",
  "CHECK TABLE",
  "CHECKSUM TABLE",
  "CLOSE",
  "COALESCE",
  "COERCIBILITY",
  "COLLATION",
  "COMPRESS",
  "CONCAT",
  "CONCAT_WS",
  "CONNECTION_ID",
  "CONSTRAINT",
  "CONTAINS",
  "CONV",
  "CONVERT",
  "CONVERT_TZ",
  "CONVEXHULL",
  "COS",
  "COT",
  "COUNT",
  "COUNT DISTINCT",
  "CRC32",
  "CREATE DATABASE",
  "CREATE EVENT",
  "CREATE FUNCTION",
  "CREATE FUNCTION UDF",
  "CREATE INDEX",
  "CREATE PROCEDURE",
  "CREATE SERVER",
  "CREATE TABLE",
  "CREATE TABLESPACE",
  "CREATE TRIGGER",
  "CREATE USER",
  "CREATE VIEW",
  "CREATE_ASYMMETRIC_PRIV_KEY",
  "CREATE_ASYMMETRIC_PUB_KEY",
  "CREATE_DH_PARAMETERS",
  "CREATE_DIGEST",
  "CROSSES",
  "CURDATE",
  "CURRENT_DATE",
  "CURRENT_TIME",
  "CURRENT_TIMESTAMP",
  "CURRENT_USER",
  "CURTIME",
  "DATABASE",
  "DATE",
  "DATE FUNCTION",
  "DATEDIFF",
  "DATETIME",
  "DATE_ADD",
  "DATE_FORMAT",
  "DATE_SUB",
  "DAY",
  "DAYNAME",
  "DAYOFMONTH",
  "DAYOFWEEK",
  "DAYOFYEAR",
  "DEALLOCATE PREPARE",
  "DEC",
  "DECIMAL",
  "DECLARE CONDITION",
  "DECLARE CURSOR",
  "DECLARE HANDLER",
  "DECLARE VARIABLE",
  "DECODE",
  "DEFAULT",
  "DEGREES",
  "DELETE",
  "DES_DECRYPT",
  "DES_ENCRYPT",
  "DIMENSION",
  "DISJOINT",
  "DISTANCE",
  "DIV",
  "DO",
  "DOUBLE",
  "DOUBLE PRECISION",
  "DROP DATABASE",
  "DROP EVENT",
  "DROP FUNCTION",
  "DROP FUNCTION UDF",
  "DROP INDEX",
  "DROP PROCEDURE",
  "DROP SERVER",
  "DROP TABLE",
  "DROP TABLESPACE",
  "DROP TRIGGER",
  "DROP USER",
  "DROP VIEW",
  "DUAL",
  "ELT",
  "ENCODE",
  "ENCRYPT",
  "ENDPOINT",
  "ENUM",
  "ENVELOPE",
  "EQUALS",
  "EXECUTE STATEMENT",
  "EXP",
  "EXPLAIN",
  "EXPORT_SET",
  "EXTERIORRING",
  "EXTRACT",
  "EXTRACTVALUE",
  "FETCH",
  "FIELD",
  "FIND_IN_SET",
  "FLOAT",
  "FLOOR",
  "FLUSH",
  "FLUSH QUERY CACHE",
  "FORMAT",
  "FOUND_ROWS",
  "FROM_BASE64",
  "FROM_DAYS",
  "FROM_UNIXTIME",
  "GEOMCOLLFROMTEXT",
  "GEOMCOLLFROMWKB",
  "GEOMETRY",
  "GEOMETRY HIERARCHY",
  "GEOMETRYCOLLECTION",
  "GEOMETRYN",
  "GEOMETRYTYPE",
  "GEOMFROMTEXT",
  "GEOMFROMWKB",
  "GET DIAGNOSTICS",
  "GET_FORMAT",
  "GET_LOCK",
  "GLENGTH",
  "GRANT",
  "GREATEST",
  "GROUP_CONCAT",
  "GTID_SUBSET",
  "GTID_SUBTRACT",
  "HANDLER",
  "HELP COMMAND",
  "HELP STATEMENT",
  "HELP_DATE",
  "HELP_VERSION",
  "HEX",
  "HOUR",
  "IF FUNCTION",
  "IF STATEMENT",
  "IFNULL",
  "IN",
  "INET6_ATON",
  "INET6_NTOA",
  "INET_ATON",
  "INET_NTOA",
  "INSERT",
  "INSERT DELAYED",
  "INSERT FUNCTION",
  "INSERT SELECT",
  "INSTALL PLUGIN",
  "INSTR",
  "INT",
  "INTEGER",
  "INTERIORRINGN",
  "INTERSECTS",
  "INTERVAL",
  "IS",
  "IS NOT",
  "IS NOT NULL",
  "IS NULL",
  "ISCLOSED",
  "ISEMPTY",
  "ISNULL",
  "ISOLATION",
  "ISSIMPLE",
  "IS_FREE_LOCK",
  "IS_IPV4",
  "IS_IPV4_COMPAT",
  "IS_IPV4_MAPPED",
  "IS_IPV6",
  "IS_USED_LOCK",
  "ITERATE",
  "JOIN",
  "JSON_APPEND",
  "JSON_ARRAY",
  "JSON_ARRAY_APPEND",
  "JSON_ARRAY_INSERT",
  "JSON_CONTAINS",
  "JSON_CONTAINS_PATH",
  "JSON_DEPTH",
  "JSON_EXTRACT",
  "JSON_INSERT",
  "JSON_KEYS",
  "JSON_LENGTH",
  "JSON_MERGE",
  "JSON_OBJECT",
  "JSON_QUOTE",
  "JSON_REMOVE",
  "JSON_REPLACE",
  "JSON_SEARCH",
  "JSON_SET",
  "JSON_TYPE",
  "JSON_UNQUOTE",
  "JSON_VALID",
  "KILL",
  "LABELS",
  "LAST_DAY",
  "LAST_INSERT_ID",
  "LCASE",
  "LEAST",
  "LEAVE",
  "LEFT",
  "LENGTH",
  "LIKE",
  "LINEFROMTEXT",
  "LINEFROMWKB",
  "LINESTRING",
  "LN",
  "LOAD DATA",
  "LOAD INDEX",
  "LOAD XML",
  "LOAD_FILE",
  "LOCALTIME",
  "LOCALTIMESTAMP",
  "LOCATE",
  "LOCK",
  "LOG",
  "LOG10",
  "LOG2",
  "LONGBLOB",
  "LONGTEXT",
  "LOOP",
  "LOWER",
  "LPAD",
  "LTRIM",
  "MAKEDATE",
  "MAKETIME",
  "MAKE_SET",
  "MASTER_POS_WAIT",
  "MATCH AGAINST",
  "MAX",
  "MBR DEFINITION",
  "MBRCONTAINS",
  "MBRCOVEREDBY",
  "MBRCOVERS",
  "MBRDISJOINT",
  "MBREQUAL",
  "MBREQUALS",
  "MBRINTERSECTS",
  "MBROVERLAPS",
  "MBRTOUCHES",
  "MBRWITHIN",
  "MD5",
  "MEDIUMBLOB",
  "MEDIUMINT",
  "MEDIUMTEXT",
  "MERGE",
  "MICROSECOND",
  "MID",
  "MIN",
  "MINUTE",
  "MLINEFROMTEXT",
  "MLINEFROMWKB",
  "MOD",
  "MONTH",
  "MONTHNAME",
  "MPOINTFROMTEXT",
  "MPOINTFROMWKB",
  "MPOLYFROMTEXT",
  "MPOLYFROMWKB",
  "MULTILINESTRING",
  "MULTIPOINT",
  "MULTIPOLYGON",
  "NAME_CONST",
  "NOT BETWEEN",
  "NOT IN",
  "NOT LIKE",
  "NOT REGEXP",
  "NOW",
  "NULLIF",
  "NUMGEOMETRIES",
  "NUMINTERIORRINGS",
  "NUMPOINTS",
  "OCT",
  "OCTET_LENGTH",
  "OLD_PASSWORD",
  "OPEN",
  "OPTIMIZE TABLE",
  "OR",
  "ORD",
  "OVERLAPS",
  "PASSWORD",
  "PERIOD_ADD",
  "PERIOD_DIFF",
  "PI",
  "POINT",
  "POINTFROMTEXT",
  "POINTFROMWKB",
  "POINTN",
  "POLYFROMTEXT",
  "POLYFROMWKB",
  "POLYGON",
  "POSITION",
  "POW",
  "POWER",
  "PREPARE",
  "PROCEDURE ANALYSE",
  "PURGE BINARY LOGS",
  "QUARTER",
  "QUOTE",
  "RADIANS",
  "RAND",
  "RANDOM_BYTES",
  "REGEXP",
  "RELEASE_ALL_LOCKS",
  "RELEASE_LOCK",
  "RENAME TABLE",
  "RENAME USER",
  "REPAIR TABLE",
  "REPEAT FUNCTION",
  "REPEAT LOOP",
  "REPLACE",
  "REPLACE FUNCTION",
  "RESET",
  "RESET MASTER",
  "RESET SLAVE",
  "RESIGNAL",
  "RETURN",
  "REVERSE",
  "REVOKE",
  "RIGHT",
  "ROUND",
  "ROW_COUNT",
  "RPAD",
  "RTRIM",
  "SAVEPOINT",
  "SCHEMA",
  "SECOND",
  "SEC_TO_TIME",
  "SELECT",
  "SESSION_USER",
  "SET",
  "SET DATA TYPE",
  "SET GLOBAL SQL_SLAVE_SKIP_COUNTER",
  "SET PASSWORD",
  "SET SQL_LOG_BIN",
  "SHA1",
  "SHA2",
  "SHOW",
  "SHOW BINARY LOGS",
  "SHOW BINLOG EVENTS",
  "SHOW CHARACTER SET",
  "SHOW COLLATION",
  "SHOW COLUMNS",
  "SHOW CREATE DATABASE",
  "SHOW CREATE EVENT",
  "SHOW CREATE FUNCTION",
  "SHOW CREATE PROCEDURE",
  "SHOW CREATE TABLE",
  "SHOW CREATE TRIGGER",
  "SHOW CREATE USER",
  "SHOW CREATE VIEW",
  "SHOW DATABASES",
  "SHOW ENGINE",
  "SHOW ENGINES",
  "SHOW ERRORS",
  "SHOW EVENTS",
  "SHOW FUNCTION CODE",
  "SHOW FUNCTION STATUS",
  "SHOW GRANTS",
  "SHOW INDEX",
  "SHOW MASTER STATUS",
  "SHOW OPEN TABLES",
  "SHOW PLUGINS",
  "SHOW PRIVILEGES",
  "SHOW PROCEDURE CODE",
  "SHOW PROCEDURE STATUS",
  "SHOW PROCESSLIST",
  "SHOW PROFILE",
  "SHOW PROFILES",
  "SHOW RELAYLOG EVENTS",
  "SHOW SLAVE HOSTS",
  "SHOW SLAVE STATUS",
  "SHOW STATUS",
  "SHOW TABLE STATUS",
  "SHOW TABLES",
  "SHOW TRIGGERS",
  "SHOW VARIABLES",
  "SHOW WARNINGS",
  "SHUTDOWN",
  "SIGN",
  "SIGNAL",
  "SIN",
  "SLEEP",
  "SMALLINT",
  "SOUNDEX",
  "SOUNDS LIKE",
  "SPACE",
  "SPATIAL",
  "SQRT",
  "SRID",
  "START SLAVE",
  "START TRANSACTION",
  "STARTPOINT",
  "STD",
  "STDDEV",
  "STDDEV_POP",
  "STDDEV_SAMP",
  "STOP SLAVE",
  "STRCMP",
  "STR_TO_DATE",
  "ST_AREA",
  "ST_ASBINARY",
  "ST_ASGEOJSON",
  "ST_ASTEXT",
  "ST_BUFFER",
  "ST_BUFFER_STRATEGY",
  "ST_CENTROID",
  "ST_CONTAINS",
  "ST_CONVEXHULL",
  "ST_CROSSES",
  "ST_DIFFERENCE",
  "ST_DIMENSION",
  "ST_DISJOINT",
  "ST_DISTANCE",
  "ST_DISTANCE_SPHERE",
  "ST_ENDPOINT",
  "ST_ENVELOPE",
  "ST_EQUALS",
  "ST_EXTERIORRING",
  "ST_GEOHASH",
  "ST_GEOMCOLLFROMTEXT",
  "ST_GEOMCOLLFROMWKB",
  "ST_GEOMETRYN",
  "ST_GEOMETRYTYPE",
  "ST_GEOMFROMGEOJSON",
  "ST_GEOMFROMTEXT",
  "ST_GEOMFROMWKB",
  "ST_INTERIORRINGN",
  "ST_INTERSECTION",
  "ST_INTERSECTS",
  "ST_ISCLOSED",
  "ST_ISEMPTY",
  "ST_ISSIMPLE",
  "ST_ISVALID",
  "ST_LATFROMGEOHASH",
  "ST_LENGTH",
  "ST_LINEFROMTEXT",
  "ST_LINEFROMWKB",
  "ST_LONGFROMGEOHASH",
  "ST_MAKEENVELOPE",
  "ST_MLINEFROMTEXT",
  "ST_MLINEFROMWKB",
  "ST_MPOINTFROMTEXT",
  "ST_MPOINTFROMWKB",
  "ST_MPOLYFROMTEXT",
  "ST_MPOLYFROMWKB",
  "ST_NUMGEOMETRIES",
  "ST_NUMINTERIORRINGS",
  "ST_NUMPOINTS",
  "ST_OVERLAPS",
  "ST_POINTFROMGEOHASH",
  "ST_POINTFROMTEXT",
  "ST_POINTFROMWKB",
  "ST_POINTN",
  "ST_POLYFROMTEXT",
  "ST_POLYFROMWKB",
  "ST_SIMPLIFY",
  "ST_SRID",
  "ST_STARTPOINT",
  "ST_SYMDIFFERENCE",
  "ST_TOUCHES",
  "ST_UNION",
  "ST_VALIDATE",
  "ST_WITHIN",
  "ST_X",
  "ST_Y",
  "SUBDATE",
  "SUBSTR",
  "SUBSTRING",
  "SUBSTRING_INDEX",
  "SUBTIME",
  "SUM",
  "SYSDATE",
  "SYSTEM_USER",
  "TAN",
  "TEXT",
  "TIME",
  "TIME FUNCTION",
  "TIMEDIFF",
  "TIMESTAMP",
  "TIMESTAMP FUNCTION",
  "TIMESTAMPADD",
  "TIMESTAMPDIFF",
  "TIME_FORMAT",
  "TIME_TO_SEC",
  "TINYBLOB",
  "TINYINT",
  "TINYTEXT",
  "TOUCHES",
  "TO_BASE64",
  "TO_DAYS",
  "TO_SECONDS",
  "TRIM",
  "TRUE FALSE",
  "TRUNCATE",
  "TRUNCATE TABLE",
  "UCASE",
  "UNCOMPRESS",
  "UNCOMPRESSED_LENGTH",
  "UNHEX",
  "UNINSTALL PLUGIN",
  "UNION",
  "UNIX_TIMESTAMP",
  "UPDATE",
  "UPDATEXML",
  "UPPER",
  "USE",
  "USER",
  "UTC_DATE",
  "UTC_TIME",
  "UTC_TIMESTAMP",
  "UUID",
  "UUID_SHORT",
  "VALIDATE_PASSWORD_STRENGTH",
  "VALUES",
  "VARBINARY",
  "VARCHAR",
  "VARIANCE",
  "VAR_POP",
  "VAR_SAMP",
  "VERSION",
  "WAIT_FOR_EXECUTED_GTID_SET",
  "WAIT_UNTIL_SQL_THREAD_AFTER_GTIDS",
  "WEEK",
  "WEEKDAY",
  "WEEKOFYEAR",
  "WEIGHT_STRING",
  "WHILE",
  "WITHIN",
  "WKT DEFINITION",
  "X",
  "XA",
  "XOR",
  "Y",
  "YEAR",
  "YEAR DATA TYPE",
  "YEARWEEK",
  "^",
  "|",
  "~",
};

/**
 * A quick lookup if the help topic exists actually, without retrieving help text.
 */
bool DbSqlEditorContextHelp::topicExists(const std::string &topic)
{
  return availableTopics.count(topic) > 0;
};

//----------------------------------------------------------------------------------------------------------------------

bool DbSqlEditorContextHelp::helpTextForTopic(const std::string &topic, std::string &title, std::string &text)
{
  logDebug2("Looking up help topic: %s\n", topic.c_str());
  
  if (!topic.empty())
  {
    try
    {/*
      sql::Dbc_connection_handler::Ref conn;
      base::RecMutexLock aux_dbc_conn_mutex(form->ensure_valid_aux_connection(conn));

      // % is interpreted as a wildcard, so we have to escape it. However, we don't use wildcards
      // in any other topic (nor %), so a simple check is enough.
      base::sqlstring query = base::sqlstring("help ?", 0) << (topic == "%" ? "\\%" : topic);
      std::auto_ptr<sql::ResultSet> rs(conn->ref->createStatement()->executeQuery(std::string(query)));
      if (rs->rowsCount() > 0)
      {
        rs->next();
        title = rs->getString(1);
        text = rs->getString(2);
        return true;
      }*/
    }
    catch (...)
    {
      logDebug2("Exception caught while looking up help text\n");
    }
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

// Determines if the given tree is a terminal node and if so, if it is of the given type.
bool isToken(tree::ParseTree *tree, size_t type)
{
  auto terminal = dynamic_cast<tree::TerminalNode *>(tree);
  if (terminal != nullptr)
    return terminal->getSymbol()->getType() == type;

  auto token = dynamic_cast<ParserRuleContext *>(tree)->start;
  if (token == nullptr)
    return false;
  return token->getType() == type;
}

//----------------------------------------------------------------------------------------------------------------------

// Determines if the given is of the given type.
bool isToken(Token *token, size_t type)
{
  return token->getType() == type;
}

//----------------------------------------------------------------------------------------------------------------------

// Determines if the parent of the given tree is a specific context.
bool isParentContext(tree::ParseTree *tree, size_t type)
{
  auto parent = (ParserRuleContext *)(tree->parent);
  return parent->getRuleIndex() == type;
}

//----------------------------------------------------------------------------------------------------------------------

static std::map<std::string, std::string> functionSynonyms = {
  { "ST_ASWKB", "ASBINARY" },
  { "ASWKB", "ASBINARY" },
  { "ST_ASWKT", "ASTEXT" },
  { "ASWKT", "ASTEXT" },
  { "ST_CROSSES", "CROSSES" },
  { "GEOMETRYFROMTEXT", "GEOMFROMTEXT" },
  { "GEOMETRYFROMWKB", "GEOMFROMWKB" },
};

std::string functionTopicForContext(ParserRuleContext *context)
{
  std::string topic;

  Token *nameToken = nullptr;
  size_t rule = context->getRuleIndex();
  switch (rule)
  {
    case MySQLParser::RuleFunctionCall:
    {
      auto functionContext = (MySQLParser::FunctionCallContext *)context;

      // We only consider global functions here, hence there should not be any qualifier.
      if (functionContext->pureIdentifier() != nullptr)
        nameToken = functionContext->pureIdentifier()->start;
      else if (functionContext->qualifiedIdentifier() != nullptr)
        nameToken = functionContext->qualifiedIdentifier()->start;

      break;
    }

    case MySQLParser::RuleRuntimeFunctionCall:
    {
      auto functionContext = (MySQLParser::RuntimeFunctionCallContext *)context;
      if (functionContext->name != nullptr)
      {
        switch (functionContext->name->getType())
        {
            // Function names that are also keywords.
          case MySQLLexer::IF_SYMBOL:
          case MySQLLexer::REPEAT_SYMBOL:
          case MySQLLexer::REPLACE_SYMBOL:
          case MySQLLexer::TIME_SYMBOL:
          case MySQLLexer::TIMESTAMP_SYMBOL:
          case MySQLLexer::CHAR_SYMBOL:
          case MySQLLexer::DATE_SYMBOL:
          case MySQLLexer::INSERT_SYMBOL:
            return base::toupper(functionContext->name->getText()) + " FUNCTION";

          case MySQLLexer::COLLATION_SYMBOL:
            return "COLLATION";

          default:
            nameToken = functionContext->name;
        }
      }
      break;
    }

    case MySQLParser::RuleSumExpr:
    {
      auto exprContext = (MySQLParser::SumExprContext *)context;
      if (exprContext->COUNT_SYMBOL() != nullptr && exprContext->DISTINCT_SYMBOL() != nullptr)
        return "COUNT DISTINCT";
      nameToken = exprContext->name;

      break;
    }

    case MySQLParser::RuleGeometryFunction:
    {
      auto functionContext = (MySQLParser::GeometryFunctionContext *)context;
      nameToken = functionContext->name;
      break;
    }
  }

  if (nameToken != nullptr)
    topic = base::toupper(nameToken->getText());
  if (functionSynonyms.count(topic) > 0)
    topic = functionSynonyms[topic];

  return topic;
}

//----------------------------------------------------------------------------------------------------------------------

static std::unordered_map<size_t, std::string> supportedOperatorsAndKeywords = {
  { MySQLLexer::EQUAL_OPERATOR, "ASSIGN-EQUAL" },
  { MySQLLexer::ASSIGN_OPERATOR, "ASSIGN-VALUE" },
  { MySQLLexer::LOGICAL_AND_OPERATOR, "AND" },
  { MySQLLexer::LOGICAL_OR_OPERATOR, "||" },
  { MySQLLexer::BIT_AND_SYMBOL, "BIT_AND" },
  { MySQLLexer::BIT_OR_SYMBOL, "BIT_OR" },
  { MySQLLexer::BIT_XOR_SYMBOL, "BIT_XOR" },
  { MySQLLexer::LOGICAL_NOT_OPERATOR, "!" },
  { MySQLLexer::NOT_EQUAL_OPERATOR, "!=" },
  { MySQLLexer::MOD_OPERATOR, "%" },
  { MySQLLexer::BITWISE_AND_OPERATOR, "&" },
  { MySQLLexer::MULT_OPERATOR, "*" },
  { MySQLLexer::PLUS_OPERATOR, "+" },
  { MySQLLexer::JSON_SEPARATOR_SYMBOL, "->" },
  { MySQLLexer::JSON_UNQUOTED_SEPARATOR_SYMBOL, "->>" },
  { MySQLLexer::DIV_OPERATOR, "/" },
  { MySQLLexer::LESS_THAN_OPERATOR, "<" },
  { MySQLLexer::SHIFT_LEFT_OPERATOR, "<<" },
  { MySQLLexer::NULL_SAFE_EQUAL_OPERATOR, "<=>" },
  { MySQLLexer::GREATER_THAN_OPERATOR, ">" },
  { MySQLLexer::GREATER_OR_EQUAL_OPERATOR, ">=" },
  { MySQLLexer::LESS_OR_EQUAL_OPERATOR, "<=" },
  { MySQLLexer::SHIFT_RIGHT_OPERATOR, ">>" },
  { MySQLLexer::BITWISE_XOR_OPERATOR, "^" },
  { MySQLLexer::BITWISE_OR_OPERATOR, "|" },
  { MySQLLexer::BITWISE_NOT_OPERATOR, "~" },

  { MySQLLexer::AUTO_INCREMENT_SYMBOL, "AUTO_INCREMENT" },
  { MySQLLexer::CALL_SYMBOL, "CALL" },
  { MySQLLexer::CAST_SYMBOL, "CAST" },
  { MySQLLexer::SPATIAL_SYMBOL, "SPATIAL" },
  { MySQLLexer::DIV_SYMBOL, "DIV" },
  { MySQLLexer::OR_SYMBOL, "OR" },
  { MySQLLexer::XOR_SYMBOL, "XOR" },
  { MySQLLexer::MOD_SYMBOL, "MOD" },
};

// Simple token -> topic matches, only used in certain contexts and only if there is no trivial token -> topic translation.
static std::unordered_map<size_t, std::string> tokenToTopic = {
  { MySQLLexer::AUTHORS_SYMBOL, "SHOW AUTHORS" },
  { MySQLLexer::BINLOG_SYMBOL, "SHOW BINLOG EVENTS" },
  { MySQLLexer::COLLATION_SYMBOL, "SHOW COLLATION" },
  { MySQLLexer::COLUMNS_SYMBOL, "SHOW COLUMNS" },
  { MySQLLexer::CONTRIBUTORS_SYMBOL, "SHOW CONTRIBUTORS" },
  { MySQLLexer::DATABASES_SYMBOL, "SHOW databases" },
  { MySQLLexer::ENGINE_SYMBOL, "SHOW ENGINE" },
  { MySQLLexer::ENGINES_SYMBOL, "SHOW ENGINES" },
  { MySQLLexer::ERRORS_SYMBOL, "SHOW ERRORS" },
  { MySQLLexer::EVENTS_SYMBOL, "SHOW EVENTS" },
  { MySQLLexer::GRANTS_SYMBOL, "SHOW GRANTS" },
  { MySQLLexer::INDEX_SYMBOL, "SHOW INDEX" },
  { MySQLLexer::INDEXES_SYMBOL, "SHOW INDEX" },
  { MySQLLexer::INNODB_SYMBOL, "SHOW INNODB STATUS" },
  { MySQLLexer::INSTALL_SYMBOL, "INSTALL PLUGIN" },
  { MySQLLexer::KEYS_SYMBOL, "SHOW INDEX" },
  { MySQLLexer::LOGS_SYMBOL, "SHOW BINARY LOGS" },
  { MySQLLexer::MASTER_SYMBOL, "SHOW MASTER STATUS" },
  { MySQLLexer::OPEN_SYMBOL, "SHOW OPEN TABLES" },
  { MySQLLexer::PLUGIN_SYMBOL, "SHOW PLUGIN" },
  { MySQLLexer::PLUGINS_SYMBOL, "SHOW PLUGINS" },
  { MySQLLexer::PRIVILEGES_SYMBOL, "SHOW PRIVILEGES" },
  { MySQLLexer::PROCESSLIST_SYMBOL, "SHOW PROCESSLIST" },
  { MySQLLexer::PROFILE_SYMBOL, "SHOW PROFILE" },
  { MySQLLexer::PROFILES_SYMBOL, "SHOW PROFILES" },
  { MySQLLexer::RELAYLOG_SYMBOL, "SHOW RELAYLOG EVENTS" },
  { MySQLLexer::STATUS_SYMBOL, "SHOW STATUS" },
  { MySQLLexer::TABLES_SYMBOL, "SHOW TABLES" },
  { MySQLLexer::TRIGGERS_SYMBOL, "SHOW TRIGGERS" },
  { MySQLLexer::VARIABLES_SYMBOL, "SHOW VARIABLES" },
  { MySQLLexer::WARNINGS_SYMBOL, "SHOW WARNINGS" },

  { MySQLLexer::ANALYZE_SYMBOL, "ANALYZE TABLE" },
  { MySQLLexer::CHECKSUM_SYMBOL, "CHECKSUM TABLE" },
  { MySQLLexer::CACHE_SYMBOL, "CACHE INDEX" },
  { MySQLLexer::CHECK_SYMBOL, "CHECK TABLE" },
  { MySQLLexer::FLUSH_SYMBOL, "FLUSH" },
  { MySQLLexer::KILL_SYMBOL, "KILL" },
  { MySQLLexer::LOAD_SYMBOL, "LOAD INDEX" },
  { MySQLLexer::OPTIMIZE_SYMBOL, "OPTIMIZE TABLE" },
  { MySQLLexer::REPAIR_SYMBOL, "REPAIR TABLE" },
  { MySQLLexer::SHUTDOWN_SYMBOL, "SHUTDOWN" },
  { MySQLLexer::UNINSTALL_SYMBOL, "UNINSTALL PLUGIN" },

};

static std::unordered_map<size_t, std::string> contextToTopic = {
  { MySQLParser::RuleCreateDatabase, "CREATE DATABASE" },
  { MySQLParser::RuleCreateEvent, "CREATE EVENT" },
  { MySQLParser::RuleCreateFunction, "CREATE FUNCTION" },
  { MySQLParser::RuleCreateUdf, "CREATE FUNCTION UDF" },
  { MySQLParser::RuleCreateIndex, "CREATE INDEX" },
  { MySQLParser::RuleCreateProcedure, "CREATE PROCEDURE" },
  { MySQLParser::RuleCreateServer, "CREATE SERVER" },
  { MySQLParser::RuleCreateTable, "CREATE TABLE" },
  { MySQLParser::RuleCreateTablespace, "CREATE TABLESPACE" },
  { MySQLParser::RuleCreateTrigger, "CREATE TRIGGER" },
  { MySQLParser::RuleCreateUser, "CREATE USER" },
  { MySQLParser::RuleCreateView, "CREATE VIEW" },
  { MySQLParser::RuleDeleteStatement, "DELETE" },
  { MySQLParser::RuleDoStatement, "DO" },
  { MySQLParser::RuleDropUser, "DROP USER" },
  { MySQLParser::RuleExecuteStatement, "EXECUTE STATEMENT" },
  { MySQLParser::RuleDescribeCommand, "EXPLAIN" },
  { MySQLParser::RuleGrant, "GRANT" },
  { MySQLParser::RuleHandlerStatement, "HANDLER" },
  { MySQLParser::RuleHandlerDeclaration, "DECLARE HANDLER" },
  { MySQLParser::RuleHelpCommand, "HELP COMMAND" },
  { MySQLParser::RuleIfStatement, "IF STATEMENT" },
  { MySQLParser::RuleIterateStatement, "ITERATE" },
  { MySQLParser::RuleJoinTable, "JOIN" },
  { MySQLParser::RuleLabel, "LABELS" },
  { MySQLParser::RuleLeaveStatement, "LEAVE" },
  { MySQLParser::RuleLockStatement, "LOCK" },
  { MySQLParser::RuleLoopBlock, "LOOP" },
  { MySQLParser::RuleCursorOpen, "OPEN" },
  { MySQLParser::RuleCursorClose, "CLOSE" },
  { MySQLParser::RuleCursorFetch, "FETCH" },
  { MySQLParser::RuleProcedureAnalyseClause, "PROCEDURE ANALYSE" },
  { MySQLParser::RuleRenameTableStatement, "RENAME TABLE" },
  { MySQLParser::RuleRenameUser, "RENAME USER" },
  { MySQLParser::RuleRepeatUntilBlock, "REPEAT LOOP" },
  { MySQLParser::RuleReplaceStatement, "REPLACE" },
  { MySQLParser::RuleResignalStatement, "RESIGNAL" },
  { MySQLParser::RuleReturnStatement, "RETURN" },
  { MySQLParser::RuleRevokeStatement, "REVOKE" },
  { MySQLParser::RuleSavepointStatement, "SAVEPOINT" },
  { MySQLParser::RuleSelectStatement, "SELECT" },
  { MySQLParser::RuleSetPassword, "SET PASSWORD" },
  { MySQLParser::RuleTransactionStatement, "START TRANSACTION" },
  { MySQLParser::RuleTruncateTableStatement, "TRUNCATE TABLE" },
  { MySQLParser::RuleUnionClause, "UNION" },
  { MySQLParser::RuleUpdateStatement, "UPDATE" },
  { MySQLParser::RuleUseCommand, "USE" },
  { MySQLParser::RuleWhileDoBlock, "WHILE" },
  { MySQLParser::RuleXaStatement, "XA" },
  { MySQLParser::RuleVariableDeclaration, "DECLARE VARIABLE" },
  { MySQLParser::RuleConditionDeclaration, "DECLARE CONDITION" },
  { MySQLParser::RuleHandlerDeclaration, "DECLARE HANDLER" },
  { MySQLParser::RuleCursorDeclaration, "DECLARE CURSOR" },
  { MySQLParser::RuleGetDiagnostics, "GET DIAGNOSTICS" },
  { MySQLParser::RuleSignalStatement, "SIGNAL" },
  { MySQLParser::RuleCursorFetch, "FETCH" },
  { MySQLParser::RuleLeaveStatement, "LEAVE" },
  { MySQLParser::RuleUseCommand, "USE" },
  { MySQLParser::RuleAlterUser, "ALTER USER" },
  { MySQLParser::RuleCaseStatement, "CASE STATEMENT" },
  { MySQLParser::RuleChangeMaster, "CHANGE MASTER TO" },
};

// Words which are part of a multi word topic or can produce wrong topics if used alone, and hence need further examination.
static std::unordered_set<std::string> specialWords = {
  "CHAR", "COUNT", "DATE", "DOUBLE", "REPLACE", "TIME", "TIMESTAMP", "YEAR", "DATABASE", "USER", "INSERT", "PREPARE",
  "HANDLER", "FLUSH", "IS", "IN", "LIKE", "REGEXP", "SET", "PASSWORD", "SHOW", "COLLATION", "OPEN", "UPDATE", "DELETE"
};

//----------------------------------------------------------------------------------------------------------------------

/**
 * Determines a help topic from the given query at the given position (given as column/row pair).
 */
std::string DbSqlEditorContextHelp::helpTopicFromPosition(HelpContext *context, const std::string &query,
  std::pair<size_t, size_t> caret)
{
  logDebug2("Finding help topic\n");
  
  // We are not interested in validity here. We simply parse in default mode (LL) and examine the returned parse tree.
  // This usually will give us a good result, except in cases where the query has an error before the caret such that
  // we cannot predict the path through the rules.
  ParserRuleContext *parseTree = context->_d->parse(query);
  ++caret.second; // ANTLR lines are one-based.
  tree::ParseTree *tree = MySQLRecognizerCommon::contextFromPosition(parseTree, caret);

  if (tree == nullptr)
    return "";

  if (antlrcpp::is<tree::TerminalNode *>(tree)) // Should always be the case at this point.
  {
    tree::TerminalNode *node = (tree::TerminalNode *)tree;
    size_t token = node->getSymbol()->getType();
    if (token == MySQLLexer::SEMICOLON_SYMBOL)
    {
      tree = MySQLRecognizerCommon::getPrevious(tree);
      node = (tree::TerminalNode *)tree;
      token = node->getSymbol()->getType();
    }

    // First check if we can get a topic for this single token, either from our topic table or by lookup.
    // This is a double-edged sword. It will help in incomplete statements where we do not get a good parse tree
    // but might also show help topics for unrelated stuff (e.g. returns "SAVEPOINT" for "select a := savepoint(c);".
    if (supportedOperatorsAndKeywords.count(token) > 0)
      return supportedOperatorsAndKeywords[token];

    switch (token)
    {
      case MySQLLexer::MINUS_OPERATOR:
        if (isParentContext(tree, MySQLParser::RuleSimpleExpr))
          return "- UNARY";
        return "- BINARY";

      case MySQLLexer::BINARY_SYMBOL:
        if (isParentContext(tree, MySQLParser::RuleDataType))
          return "BINARY";
        if (isParentContext(tree, MySQLParser::RuleSimpleExpr) || isParentContext(tree, MySQLParser::RuleCastType))
          return "BINARY OPERATOR";

        tree = tree->parent;
        break;

      case MySQLLexer::CHANGE_SYMBOL:
        if (isParentContext(tree, MySQLParser::RuleChangeMaster))
          return "CHANGE MASTER TO";
        if (isParentContext(tree, MySQLParser::RuleChangeReplication))
          return "CHANGE REPLICATION FILTER";

        tree = tree->parent;
        break;

      // Other keywords connected to topics.
      case MySQLLexer::BEGIN_SYMBOL:
      case MySQLLexer::END_SYMBOL:
        return "BEGIN END";

      case MySQLLexer::TRUE_SYMBOL:
      case MySQLLexer::FALSE_SYMBOL:
        return "TRUE FALSE";

      case MySQLLexer::BINLOG_SYMBOL:
        if (isParentContext(tree, MySQLParser::RuleOtherAdministrativeStatement))
          return "BINLOG";
        if (isParentContext(tree, MySQLParser::RuleShowStatement))
          return "SHOW BINLOG EVENTS";

        tree = tree->parent;
        break;

      default:
        std::string s = base::toupper(node->getText());
        if (specialWords.count(s) == 0 && topicExists(s))
          return s;
        
        // No specific help topic for the given terminal. Jump to the token's parent and start the
        // context search then.
        tree = tree->parent;
        break;
    }
  }

  // See if we have a help topic for the given tree. If not walk up the parent chain until we find something.
  while (true)
  {
    if (tree == nullptr)
      return "";

    // We deliberately don't check if the given tree is actually a parse rule context - there is no other possibility.
    ParserRuleContext *context = (ParserRuleContext *)tree;
    size_t ruleIndex = context->getRuleIndex();
    if (contextToTopic.count(ruleIndex) > 0)
      return contextToTopic[ruleIndex];

    // Topics from function names
    std::string functionTopic = functionTopicForContext(context);
    if (!functionTopic.empty() && topicExists(functionTopic))
      return functionTopic;

    switch (ruleIndex)
    {
      case MySQLParser::RulePredicateOperations:
      {
        if (!context->children.empty())
        {
          // Some keyword topics have variants with a leading NOT.
          auto parent = dynamic_cast<MySQLParser::PredicateContext *>(context->parent);
          bool isNot = parent->notRule() != nullptr;

          // IN, BETWEEN (with special help topic name), LIKE, REGEXP
          auto predicateContext = (MySQLParser::PredicateOperationsContext *)context;
          if (isToken(predicateContext->children[0], MySQLLexer::BETWEEN_SYMBOL))
          {
            if (isNot)
              return "NOT BETWEEN";
            return "BETWEEN AND";
          }
          std::string topic = isNot ? "NOT " : "";
          return topic + base::toupper(predicateContext->children[0]->getText());
        }
        break;
      }

      case MySQLParser::RuleDropStatement:
      {
        auto dropContext = (MySQLParser::DropStatementContext *)context;
        if (dropContext->type != nullptr)
        {
          if (dropContext->type->getType() == MySQLLexer::TABLES_SYMBOL)
            return "DROP TABLE"; // Extra handling to avoid "DROP TABLES".
          if (dropContext->type->getType() == MySQLLexer::DATABASE_SYMBOL)
            return "DROP DATABASE";
          return "DROP " + base::toupper(dropContext->type->getText());
        }

        // online/offline is version dependent, which can cause "type" not to be filled.
        if (dropContext->INDEX_SYMBOL() != nullptr)
          return "DROP INDEX";

        break;
      }

      case MySQLParser::RuleOtherAdministrativeStatement:
      {
        // See if we only have a single flush command.
        auto adminContext = (MySQLParser::OtherAdministrativeStatementContext *)context;
        if (adminContext->type->getType() == MySQLLexer::FLUSH_SYMBOL && adminContext->flushOption().size() == 1
            && adminContext->flushOption(0)->option->getType() == MySQLLexer::QUERY_SYMBOL)
          return "FLUSH QUERY CACHE";

        if (adminContext->type != nullptr)
          return tokenToTopic[adminContext->type->getType()];
        break;
      }

      case MySQLParser::RuleInsertStatement:
      {
        auto insertContext = (MySQLParser::InsertStatementContext *)context;
        if (insertContext->insertFieldSpec()->insertQueryExpression() != nullptr)
          return "INSERT SELECT";
        if (insertContext->insertLockOption() != nullptr && insertContext->insertLockOption()->DELAYED_SYMBOL() != nullptr)
          return "INSERT DELAYED";
        return "INSERT";
      }

      case MySQLParser::RuleInstallUninstallStatment:
      {
        auto pluginContext = (MySQLParser::InstallUninstallStatmentContext *)context;
        if (pluginContext->action != nullptr)
          return tokenToTopic[pluginContext->action->getType()];
        break;
      }

      case MySQLParser::RuleExpr:
      {
        auto exprContext = (MySQLParser::ExprContext *)context;
        if (exprContext->children.size() > 2 && isToken(exprContext->children[1], MySQLLexer::IS_SYMBOL))
        {
          if (isToken(exprContext->children[2], MySQLLexer::NOT_SYMBOL)
            || isToken(exprContext->children[2], MySQLLexer::NOT2_SYMBOL))
            return "IS NOT";
          return "IS";
        }
        break;
      }

      case MySQLParser::RuleBoolPri:
      {
        if (antlrcpp::is<MySQLParser::PrimaryExprIsNullContext *>(context))
        {
          auto primaryExprIsNullContext = (MySQLParser::PrimaryExprIsNullContext *)context;
          if (primaryExprIsNullContext->notRule() == nullptr)
            return "IS NULL";
          return "IS NOT NULL";
        }
        break;
      }

      case MySQLParser::RuleSetStatement:
      {
        auto setStatementContext = (MySQLParser::SetStatementContext *)context;
        if (setStatementContext->TRANSACTION_SYMBOL() != nullptr)
          return "ISOLATION";

        ParserRuleContext *variableName = nullptr;
        if (setStatementContext->optionValueFollowingOptionType() != nullptr)
          variableName = setStatementContext->optionValueFollowingOptionType()->variableName();
        else if (setStatementContext->optionValueNoOptionType() != nullptr)
          variableName = setStatementContext->optionValueNoOptionType()->variableName();
        if (variableName != nullptr)
        {
          std::string option = base::toupper(variableName->getText());
          if (option == "SQL_SLAVE_SKIP_COUNTER")
            return "SET GLOBAL SQL_SLAVE_SKIP_COUNTER";
          if (option == "SQL_LOG_BIN")
            return "SET SQL_LOG_BIN";
        }
        return "SET";
      }

      case MySQLParser::RuleLoadStatement:
      {
        auto loadStatementContext = (MySQLParser::LoadStatementContext *)context;
        if (loadStatementContext->dataOrXml()->DATA_SYMBOL() != nullptr)
          return "LOAD DATA";
        return "LOAD XML";
      }
        
      case MySQLParser::RulePredicate:
        if (context->children.size() > 2)
        {
          if (isToken(context->children[1], MySQLLexer::NOT_SYMBOL) || isToken(context->children[1], MySQLLexer::NOT2_SYMBOL))
          {
            // For NOT BETWEEN, NOT LIKE, NOT IN, NOT REGEXP.
            auto predicateContext = dynamic_cast<MySQLParser::PredicateOperationsContext *>(context->children[2]);
            return "NOT " + base::toupper(predicateContext->children[0]->getText());
          }
          if (isToken(context->children[1], MySQLLexer::SOUNDS_SYMBOL))
            return "SOUNDS LIKE";
        }
        break;

      case MySQLParser::RuleTableAdministrationStatement:
      {
        auto adminStatementContext = (MySQLParser::TableAdministrationStatementContext *)context;
        if (adminStatementContext->type != nullptr)
          return tokenToTopic[adminStatementContext->type->getType()];
        break;
      }

      case MySQLParser::RulePreparedStatement:
      {
        auto preparedContext = (MySQLParser::PreparedStatementContext *)context;
        int type = 0;
        if (preparedContext->type != nullptr)
          type = preparedContext->type->getType();
        if (type == MySQLLexer::PREPARE_SYMBOL)
          return "PREPARE";
        if (type == MySQLLexer::DEALLOCATE_SYMBOL || type == MySQLLexer::DROP_SYMBOL)
          return "DEALLOCATE PREPARE";
        break;
      }

      case MySQLParser::RuleReplicationStatement:
      {
        auto replicationContext = (MySQLParser::ReplicationStatementContext *)context;
        if (replicationContext->PURGE_SYMBOL() != nullptr)
          return "PURGE BINARY LOGS";
        if (replicationContext->RESET_SYMBOL() != nullptr
          && (replicationContext->resetOption().empty() || replicationContext->resetOption()[0]->option == nullptr))
          return "RESET";
        break;
      }

      case MySQLParser::RuleResetOption:
      {
        auto optionContext = (MySQLParser::ResetOptionContext *)context;
        if (isToken(optionContext->option, MySQLLexer::MASTER_SYMBOL))
          return "RESET MASTER";
        if (isToken(optionContext->option, MySQLLexer::SLAVE_SYMBOL))
          return "RESET SLAVE";
        return "RESET";
      }

      case MySQLParser::RuleVariableName: // Special var names in different contexts.
      {
        auto variableContext = (MySQLParser::VariableNameContext *)context;
        std::string name = base::tolower(MySQLParser::getText(variableContext, true));
        if (name == "sql_slave_skip_counter")
          return "SET GLOBAL SQL_SLAVE_SKIP_COUNTER";
        if (name == "sql_log_bin")
          return "SET SQL_LOG_BIN";
        break;
      }

      case MySQLParser::RuleShowStatement:
      {
        auto showContext = (MySQLParser::ShowStatementContext *)context;
        if (showContext->value == nullptr)
        {
          if (showContext->charset() != nullptr)
            return "SHOW CHARACTER SET";
          return "SHOW";
        }

        switch (showContext->value->getType())
        {
          case MySQLLexer::TABLE_SYMBOL:
            return "SHOW TABLE STATUS";
          case MySQLLexer::SLAVE_SYMBOL:
            if (showContext->HOSTS_SYMBOL() != nullptr)
              return "SHOW SLAVE HOSTS";
            if (showContext->STATUS_SYMBOL() != nullptr)
              return "SHOW SLAVE STATUS";
            break;
          case MySQLLexer::CREATE_SYMBOL:
          {
            if (showContext->object != nullptr)
              return "SHOW CREATE " + base::toupper(showContext->object->getText());
            break;
          }
          case MySQLLexer::PROCEDURE_SYMBOL:
            if (showContext->STATUS_SYMBOL() != nullptr)
              return "SHOW PROCEDURE STATUS";
            if (showContext->CODE_SYMBOL() != nullptr)
              return "SHOW PROCEDURE CODE";
            break;
          case MySQLLexer::FUNCTION_SYMBOL:
            if (showContext->STATUS_SYMBOL() != nullptr)
              return "SHOW FUNCTION STATUS";
            if (showContext->CODE_SYMBOL() != nullptr)
              return "SHOW FUNCTION CODE";
            break;

          default:
            return tokenToTopic[showContext->value->getType()];
        }
      }

      case MySQLParser::RuleKeyDefinition:
      {
        auto definitionContext = (MySQLParser::KeyDefinitionContext *)context;
        if (definitionContext->type->getType() == MySQLLexer::FOREIGN_SYMBOL)
          return "CONSTRAINT";
        break;
      }

      case MySQLParser::RuleHelpCommand:
        return "HELP COMMAND";

      case MySQLParser::RuleSimpleExpr:
        if (!context->children.empty() && antlrcpp::is<tree::TerminalNode *>(context->children[0]))
        {
          size_t type = dynamic_cast<tree::TerminalNode *>(context->children[0])->getSymbol()->getType();
          switch (type)
          {
            case MySQLLexer::MATCH_SYMBOL:
              return "MATCH AGAINST";
            case MySQLLexer::CONVERT_SYMBOL:
              return "CONVERT";
            case MySQLLexer::DEFAULT_SYMBOL:
              return "DEFAULT";
            case MySQLLexer::CASE_SYMBOL:
              return "CASE OPERATOR";
          }
        }
        break;

      case MySQLParser::RuleEngineRef:
      {
        std::string engine = base::tolower(context->getText());
        if (engine == "merge" || engine == "mrg_myisam")
          return "MERGE";
        break;
      }

      case MySQLParser::RuleSlave:
        if (!context->children.empty())
        {
          return base::toupper(context->children[0]->getText()) + " SLAVE";
        }

      case MySQLParser::RuleDataType:
      {
        auto typeContext = (MySQLParser::DataTypeContext *)context;
        std::string topic;
        switch (typeContext->type->getType())
        {
          case MySQLLexer::DOUBLE_SYMBOL:
            if (typeContext->PRECISION_SYMBOL() != nullptr)
              return "DOUBLE PRECISION";
            return "DOUBLE";

          case MySQLLexer::SET_SYMBOL:
            return "SET DATA TYPE";

          case MySQLLexer::YEAR_SYMBOL:
            return "YEAR DATA TYPE";

          case MySQLLexer::BOOL_SYMBOL:
            return "BOOLEAN";

          case MySQLLexer::CHAR_SYMBOL:
          case MySQLLexer::NCHAR_SYMBOL:
          case MySQLLexer::NATIONAL_SYMBOL:
          case MySQLLexer::VARCHAR_SYMBOL:
          case MySQLLexer::NVARCHAR_SYMBOL:
          case MySQLLexer::VARYING_SYMBOL:
            if (typeContext->VARYING_SYMBOL() != nullptr || typeContext->VARCHAR_SYMBOL() != nullptr)
              return "VARCHAR";
            if (typeContext->stringBinary() != nullptr && typeContext->stringBinary()->BYTE_SYMBOL() != nullptr)
              return "CHAR BYTE";
            return "CHAR";

          default:
            topic = base::toupper(typeContext->type->getText());
            break;
        }

        if (topicExists(topic))
          return topic;

        break; // Not all data types have an own topic.
      }

      case MySQLParser::RuleTablekeyList:
      {
        auto keylistContext = (MySQLParser::TablekeyListContext *)context;
        if (keylistContext->DUAL_SYMBOL() != nullptr)
          return "DUAL";
        break;
      }

      case MySQLParser::RuleSetTransactionCharacteristic:
      {
        auto characteristicsContext = (MySQLParser::SetTransactionCharacteristicContext *)context;
        if (characteristicsContext->ISOLATION_SYMBOL() != nullptr)
          return "ISOLATION";
        break;
      }

      case MySQLParser::RuleSubstringFunction:
      {
        // A case where we might have a synonym, so we need to check the text actually.
        auto substringContext = (MySQLParser::SubstringFunctionContext *)context;
        return base::toupper(substringContext->SUBSTRING_SYMBOL()->getText());

        break;
      }

      case MySQLParser::RuleAlterStatement:
      {
        auto alterContext = (MySQLParser::AlterStatementContext *)context;
        std::string topic = "ALTER ";
        if (alterContext->alterTable() != nullptr)
          return "ALTER TABLE";
        if (alterContext->alterDatabase() != nullptr)
          return "ALTER DATABASE";
        if (alterContext->PROCEDURE_SYMBOL() != nullptr)
          return "ALTER PROCEDURE";
        if (alterContext->FUNCTION_SYMBOL() != nullptr)
          return "ALTER FUNCTION";
        if (alterContext->alterView() != nullptr)
          return "ALTER VIEW";
        if (alterContext->alterEvent() != nullptr)
          return "ALTER EVENT";
        // No tablespace or logfile group topics.
        if (alterContext->alterServer() != nullptr)
          return "ALTER SERVER";
        // No alter instance topic.

        break;
      }

      default:
        if (contextToTopic.count(ruleIndex) > 0)
          return contextToTopic[ruleIndex];
        break;
    }

    tree = tree->parent;
  }

  return "";
}

//----------------------------------------------------------------------------------------------------------------------
