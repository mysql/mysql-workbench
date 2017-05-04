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

#include <regex>
#include <thread>

#include "base/log.h"
#include "base/string_utilities.h"
#include "base/file_utilities.h"
#include "base/jsonparser.h"

#include "mforms/app.h"
#include "mforms/code_editor.h"

#include "grtdb/db_helpers.h"
#include "workbench/wb_backend_public_interface.h"
#include "wb_sql_editor_help.h"

#include "mysql/mysql-recognition-types.h"
#include "mysql/MySQLRecognizerCommon.h"
#include "mysql/MySQLLexer.h"
#include "mysql/MySQLParser.h"

DEFAULT_LOG_DOMAIN("Context help")

using namespace parsers;
using namespace antlr4;

using namespace help;

class HelpContext::Private {
public:
  Private() : lexer(&input), tokens(&lexer), parser(&tokens) {
  }

  ANTLRInputStream input;
  MySQLLexer lexer;
  CommonTokenStream tokens;
  MySQLParser parser;

  ParserRuleContext *parse(const std::string &query) {
    input.load(query);
    lexer.reset();
    lexer.setInputStream(&input);
    tokens.setTokenSource(&lexer);

    parser.reset();
    return parser.query();
  }
};

//----------------- HelpContext ----------------------------------------------------------------------------------------

HelpContext::HelpContext(GrtCharacterSetsRef charsets, const std::string &sqlMode, long serverVersion) {
  _d = new Private();
  std::set<std::string> filteredCharsets;
  for (size_t i = 0; i < charsets->count(); i++)
    filteredCharsets.insert("_" + base::tolower(*charsets[i]->name()));

  if (_d->lexer.serverVersion < 50503) {
    filteredCharsets.erase("_utf8mb4");
    filteredCharsets.erase("_utf16");
    filteredCharsets.erase("_utf32");
  } else {
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
}

//----------------------------------------------------------------------------------------------------------------------

HelpContext::~HelpContext() {
  delete _d;
}

//----------------------------------------------------------------------------------------------------------------------

long HelpContext::serverVersion() const {
  return _d->lexer.serverVersion;
}

//----------------- DbSqlEditorContextHelp -----------------------------------------------------------------------------

DbSqlEditorContextHelp *DbSqlEditorContextHelp::get() {
  static DbSqlEditorContextHelp instance;
  return &instance;
}

//----------------------------------------------------------------------------------------------------------------------

static bool helpDataReady = false; // Set once the loader thread is done.

/**
 * Required if you need to sync loading and use (e.g. in tests).
 */
bool DbSqlEditorContextHelp::helpReady() {
  return helpDataReady;
}

//----------------------------------------------------------------------------------------------------------------------

static std::map<long, std::set<std::string>> helpTopics; // Quick lookup for help topics per server version.
static std::map<long, std::map<std::string, std::string>> helpContent; // Help text from a topic (also per version).

static std::string helpStyleSheet = "<style>\n"
  "body { color: #404040; spacing: 5px; }\n"
  "literal { font-family: monospace; background-color: rgba(0, 0, 0, 0.07); color: black; }\n"
  "literal[role='stmt'] { font-weight: 600; background-color: rgba(0, 0, 0, 0); }\n"
  "literal[role='func'] { font-weight: 600; background-color: rgba(0, 0, 0, 0); }\n"
  "literal[role='cfunc'] { color: #ba0099; background-color: rgba(0, 0, 0, 0); }\n"
  "replaceable { font-style: italic; font-weight: 600; color: black; }\n"
  "indexterm { display: none; }\n"
  "userinput { color: #004480; font-weight: 600; }\n"
  "li { style-position: outside; list-style-image: none; list-style-type: square; line-height: 120%; }\n"
  "pre { background-color: #ddd; border: 1px solid #c9c9c9; margin-top: 10px; margin-botton: 10px; margin-left: 6px;"
    " padding: 3px 8px; line-height: 1.5; }\n"
  "</style>";

std::string convertXRef(long version, std::string const &source) {
  static std::regex pattern("<xref linkend=\"([^\"]+)\" />");

  if (source.find("xref") == std::string::npos)
    return source;

  // We can have more than one occurence of a link, so a normal string format won't help.
  std::string result = std::regex_replace(source, pattern, "<a href='http://dev.mysql.com/doc/refman/{0}.{1}/en/$1.html'>$1</a>");
  result = base::replaceString(result, "{0}", std::to_string(version / 100));
  result = base::replaceString(result, "{1}", std::to_string(version % 10));
  return result;
}

std::string convertInternalLinks(std::string const &source) {
  static std::regex pattern("<literal role=\"stmt\">([^<]+)</literal>");

  if (source.find("role=\"stmt\"") == std::string::npos)
    return source;

  return std::regex_replace(source, pattern, "<a href='local:$1'>$1</a>");
}

std::string convertList(long version, JsonParser::JsonArray const &list) {
  std::string result;
  for (JsonParser::JsonObject const &entry: list) {
    auto iterator = entry.find("para");
    if (iterator != entry.end()) {
      std::string text = "<p>" + convertInternalLinks(iterator->second) +  "</p>";
      result += convertXRef(version, text);
    } else {
      auto iterator = entry.find("programlisting");
      if (iterator != entry.end()) {
        std::string text = convertInternalLinks(iterator->second);
        result += "<pre>" + text + "</pre>";
      } else {
        auto iterator = entry.find("itemizedlist"); // Convert to bullet list.
        if (iterator != entry.end()) {
          result = "<ul>";
          JsonParser::JsonArray const &itemizedList = iterator->second;
          for (JsonParser::JsonArray const &listentry: itemizedList) {
            result += "<li>" + convertList(version, listentry) + "</li>";
          }
          result += "</ul>";
        }
      }
    }
  }

  return result;
}

/**
 * Creates the HTML formatted help text from the object that's passed in.
 */
std::string createHelpTextFromJson(long version, JsonParser::JsonObject const &json) {
  std::string result = "<html><head>" + helpStyleSheet + "</head><body>";

  std::string id = json.get("id");
  result += "<h3>" + id + " Syntax:</h3>";

  // Syntax (the summary), often in a code block.
  JsonParser::JsonArray const &syntax = json.get("syntax");
  for (JsonParser::JsonObject const &entry: syntax) {
    // There are different variants for syntax descriptions. Usually it's encapsulated in a program listing,
    // but e.g. for functions in a list the syntax is a paragraph.
    auto iterator = entry.find("programlisting");
    if (iterator != entry.end()) {
      result += "<pre>" + convertInternalLinks(iterator->second) + "</pre>";
    } else {
      auto iterator = entry.find("para");
      if (iterator != entry.end()) {
        result += "<p>" + convertInternalLinks(iterator->second) + "</p>";
      }
    }
  }

  // The full description, plain text with code examples, lists and more.
  JsonParser::JsonArray const &description = json.get("description");
  for (JsonParser::JsonObject const &entry: description) {
    auto iterator = entry.find("para");
    if (iterator != entry.end()) {
      std::string text = "<p>" + convertInternalLinks(iterator->second) + "</p>";
      result += convertXRef(version, text);
    } else {
      auto iterator = entry.find("programlisting");
      if (iterator != entry.end()) {
        std::string text = convertInternalLinks(iterator->second);
        result += "<pre>" + text + "</pre>";
      } else {
        auto iterator = entry.find("itemizedlist"); // Convert to bullet list.
        if (iterator != entry.end()) {
          result += "<ul>";
          JsonParser::JsonArray const &itemizedList = iterator->second;
          for (JsonParser::JsonArray const &listentry: itemizedList) {
            result += "<li>" + convertList(version, listentry) + "</li>";
          }
          result += "</ul>";
        }
      }
    }
  }

  std::string page = base::replaceString(base::tolower(id), " ", "-");
  std::string url = base::strfmt("http://dev.mysql.com/doc/refman/%ld.%ld/en/%s.html", version / 100, version % 10, page.c_str());
  result += "<b>See also: </>: <a href='" + url + "'>Online help " + page + "</a><br /><br />";
  return result + "</body></html>";
}

DbSqlEditorContextHelp::DbSqlEditorContextHelp() {

  std::thread([]() {
    std::string dataDir = base::makePath(mforms::App::get()->baseDir(), "modules/data/sqlide");
    for (long version : { 800, 507, 506, 505 }) {
      std::string fileName = "help-" + std::to_string(version / 100) + "." + std::to_string(version % 10) + ".json";
      std::string path = base::makePath(dataDir, fileName);
      if (!base::file_exists(path)) {
        logError("Help file not found (%s)\n", path.c_str());
        continue;
      }

      try {
        JsonParser::JsonValue document;
        JsonParser::JsonReader::readFromFile(path, document);

        std::set<std::string> topics;
        JsonParser::JsonObject &topicRoot = document;
        JsonParser::JsonArray &topicList = topicRoot.get("topics");
        for (JsonParser::JsonObject &topic: topicList) {
          std::string id = base::toupper(topic.get("id"));
          topics.insert(id);
          helpContent[version][id] = createHelpTextFromJson(version, topic);
        }
        helpTopics[version] = topics;
      } catch (JsonParser::ParserException &e) {
        logError("Could not read help text file (%s)\nError message: %s\n", fileName.c_str(), e.what());
      } catch (std::bad_cast &e) {
        logError("Unexpected file format (%s)\nError message: %s\n", fileName.c_str(), e.what());
      }
      
    }
    helpDataReady = true;
  }).detach();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * A quick lookup if the help topic exists actually, without retrieving help text.
 */
bool DbSqlEditorContextHelp::topicExists(long serverVersion, const std::string &topic) {
  if (!helpDataReady)
    return false;

  auto iterator = helpTopics.find(serverVersion / 100);
  if (iterator == helpTopics.end())
    return false;
  return iterator->second.count(topic) > 0;
};

//----------------------------------------------------------------------------------------------------------------------

bool DbSqlEditorContextHelp::helpTextForTopic(HelpContext *context, const std::string &topic, std::string &text) {
  logDebug2("Looking up help topic: %s\n", topic.c_str());

  if (helpDataReady && !topic.empty()) {
    auto iterator = helpContent.find(context->serverVersion() / 100);
    if (iterator == helpContent.end())
      return false;

    text = iterator->second[topic];
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

// Determines if the given tree is a terminal node and if so, if it is of the given type.
bool isToken(tree::ParseTree *tree, size_t type) {
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
bool isToken(Token *token, size_t type) {
  return token->getType() == type;
}

//----------------------------------------------------------------------------------------------------------------------

// Determines if the parent of the given tree is a specific context.
bool isParentContext(tree::ParseTree *tree, size_t type) {
  auto parent = dynamic_cast<ParserRuleContext *>(tree->parent);
  return parent->getRuleIndex() == type;
}

//----------------------------------------------------------------------------------------------------------------------

static std::map<std::string, std::string> functionSynonyms = {
  {"ST_ASWKB", "ASBINARY"},
  {"ASWKB", "ASBINARY"},
  {"ST_ASWKT", "ASTEXT"},
  {"ASWKT", "ASTEXT"},
  {"ST_CROSSES", "CROSSES"},
  {"GEOMETRYFROMTEXT", "GEOMFROMTEXT"},
  {"GEOMETRYFROMWKB", "GEOMFROMWKB"},
};

std::string functionTopicForContext(ParserRuleContext *context) {
  std::string topic;

  Token *nameToken = nullptr;
  size_t rule = context->getRuleIndex();
  switch (rule) {
    case MySQLParser::RuleFunctionCall: {
      auto functionContext = dynamic_cast<MySQLParser::FunctionCallContext *>(context);

      // We only consider global functions here, hence there should not be any qualifier.
      if (functionContext->pureIdentifier() != nullptr)
        nameToken = functionContext->pureIdentifier()->start;
      else if (functionContext->qualifiedIdentifier() != nullptr)
        nameToken = functionContext->qualifiedIdentifier()->start;

      break;
    }

    case MySQLParser::RuleRuntimeFunctionCall: {
      auto functionContext = dynamic_cast<MySQLParser::RuntimeFunctionCallContext *>(context);
      if (functionContext->name != nullptr) {
        switch (functionContext->name->getType()) {
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

    case MySQLParser::RuleSumExpr: {
      auto exprContext = dynamic_cast<MySQLParser::SumExprContext *>(context);
      if (exprContext->COUNT_SYMBOL() != nullptr && exprContext->DISTINCT_SYMBOL() != nullptr)
        return "COUNT DISTINCT";
      nameToken = exprContext->name;

      break;
    }

    case MySQLParser::RuleGeometryFunction: {
      auto functionContext = dynamic_cast<MySQLParser::GeometryFunctionContext *>(context);
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
  {MySQLLexer::EQUAL_OPERATOR, "ASSIGN-EQUAL"},
  {MySQLLexer::ASSIGN_OPERATOR, "ASSIGN-VALUE"},
  {MySQLLexer::LOGICAL_AND_OPERATOR, "AND"},
  {MySQLLexer::LOGICAL_OR_OPERATOR, "||"},
  {MySQLLexer::BIT_AND_SYMBOL, "BIT_AND"},
  {MySQLLexer::BIT_OR_SYMBOL, "BIT_OR"},
  {MySQLLexer::BIT_XOR_SYMBOL, "BIT_XOR"},
  {MySQLLexer::LOGICAL_NOT_OPERATOR, "!"},
  {MySQLLexer::NOT_EQUAL_OPERATOR, "!="},
  {MySQLLexer::MOD_OPERATOR, "%"},
  {MySQLLexer::BITWISE_AND_OPERATOR, "&"},
  {MySQLLexer::MULT_OPERATOR, "*"},
  {MySQLLexer::PLUS_OPERATOR, "+"},
  {MySQLLexer::JSON_SEPARATOR_SYMBOL, "->"},
  {MySQLLexer::JSON_UNQUOTED_SEPARATOR_SYMBOL, "->>"},
  {MySQLLexer::DIV_OPERATOR, "/"},
  {MySQLLexer::LESS_THAN_OPERATOR, "<"},
  {MySQLLexer::SHIFT_LEFT_OPERATOR, "<<"},
  {MySQLLexer::NULL_SAFE_EQUAL_OPERATOR, "<=>"},
  {MySQLLexer::GREATER_THAN_OPERATOR, ">"},
  {MySQLLexer::GREATER_OR_EQUAL_OPERATOR, ">="},
  {MySQLLexer::LESS_OR_EQUAL_OPERATOR, "<="},
  {MySQLLexer::SHIFT_RIGHT_OPERATOR, ">>"},
  {MySQLLexer::BITWISE_XOR_OPERATOR, "^"},
  {MySQLLexer::BITWISE_OR_OPERATOR, "|"},
  {MySQLLexer::BITWISE_NOT_OPERATOR, "~"},

  {MySQLLexer::AUTO_INCREMENT_SYMBOL, "AUTO_INCREMENT"},
  {MySQLLexer::CALL_SYMBOL, "CALL"},
  {MySQLLexer::CAST_SYMBOL, "CAST"},
  {MySQLLexer::DIV_SYMBOL, "DIV"},
  {MySQLLexer::MOD_SYMBOL, "MOD"},
  {MySQLLexer::OR_SYMBOL, "OR"},
  {MySQLLexer::SPATIAL_SYMBOL, "SPATIAL"},
  {MySQLLexer::UNION_SYMBOL, "UNION"},
  {MySQLLexer::XOR_SYMBOL, "XOR"},
};

// Simple token -> topic matches, only used in certain contexts and only if there is no trivial token -> topic
// translation.
static std::unordered_map<size_t, std::string> tokenToTopic = {
  {MySQLLexer::AUTHORS_SYMBOL, "SHOW AUTHORS"},
  {MySQLLexer::BINLOG_SYMBOL, "SHOW BINLOG EVENTS"},
  {MySQLLexer::COLLATION_SYMBOL, "SHOW COLLATION"},
  {MySQLLexer::COLUMNS_SYMBOL, "SHOW COLUMNS"},
  {MySQLLexer::CONTRIBUTORS_SYMBOL, "SHOW CONTRIBUTORS"},
  {MySQLLexer::DATABASES_SYMBOL, "SHOW databases"},
  {MySQLLexer::ENGINE_SYMBOL, "SHOW ENGINE"},
  {MySQLLexer::ENGINES_SYMBOL, "SHOW ENGINES"},
  {MySQLLexer::ERRORS_SYMBOL, "SHOW ERRORS"},
  {MySQLLexer::EVENTS_SYMBOL, "SHOW EVENTS"},
  {MySQLLexer::GRANTS_SYMBOL, "SHOW GRANTS"},
  {MySQLLexer::INDEX_SYMBOL, "SHOW INDEX"},
  {MySQLLexer::INDEXES_SYMBOL, "SHOW INDEX"},
  {MySQLLexer::INNODB_SYMBOL, "SHOW INNODB STATUS"},
  {MySQLLexer::INSTALL_SYMBOL, "INSTALL PLUGIN"},
  {MySQLLexer::KEYS_SYMBOL, "SHOW INDEX"},
  {MySQLLexer::LOGS_SYMBOL, "SHOW BINARY LOGS"},
  {MySQLLexer::MASTER_SYMBOL, "SHOW MASTER STATUS"},
  {MySQLLexer::OPEN_SYMBOL, "SHOW OPEN TABLES"},
  {MySQLLexer::PLUGIN_SYMBOL, "SHOW PLUGIN"},
  {MySQLLexer::PLUGINS_SYMBOL, "SHOW PLUGINS"},
  {MySQLLexer::PRIVILEGES_SYMBOL, "SHOW PRIVILEGES"},
  {MySQLLexer::PROCESSLIST_SYMBOL, "SHOW PROCESSLIST"},
  {MySQLLexer::PROFILE_SYMBOL, "SHOW PROFILE"},
  {MySQLLexer::PROFILES_SYMBOL, "SHOW PROFILES"},
  {MySQLLexer::RELAYLOG_SYMBOL, "SHOW RELAYLOG EVENTS"},
  {MySQLLexer::STATUS_SYMBOL, "SHOW STATUS"},
  {MySQLLexer::TABLES_SYMBOL, "SHOW TABLES"},
  {MySQLLexer::TRIGGERS_SYMBOL, "SHOW TRIGGERS"},
  {MySQLLexer::VARIABLES_SYMBOL, "SHOW VARIABLES"},
  {MySQLLexer::WARNINGS_SYMBOL, "SHOW WARNINGS"},

  {MySQLLexer::ANALYZE_SYMBOL, "ANALYZE TABLE"},
  {MySQLLexer::CHECKSUM_SYMBOL, "CHECKSUM TABLE"},
  {MySQLLexer::CACHE_SYMBOL, "CACHE INDEX"},
  {MySQLLexer::CHECK_SYMBOL, "CHECK TABLE"},
  {MySQLLexer::FLUSH_SYMBOL, "FLUSH"},
  {MySQLLexer::KILL_SYMBOL, "KILL"},
  {MySQLLexer::LOAD_SYMBOL, "LOAD INDEX"},
  {MySQLLexer::OPTIMIZE_SYMBOL, "OPTIMIZE TABLE"},
  {MySQLLexer::REPAIR_SYMBOL, "REPAIR TABLE"},
  {MySQLLexer::SHUTDOWN_SYMBOL, "SHUTDOWN"},
  {MySQLLexer::UNINSTALL_SYMBOL, "UNINSTALL PLUGIN"},

};

static std::unordered_map<size_t, std::string> contextToTopic = {
  {MySQLParser::RuleCallStatement, "CALL"},
  {MySQLParser::RuleCreateDatabase, "CREATE DATABASE"},
  {MySQLParser::RuleCreateEvent, "CREATE EVENT"},
  {MySQLParser::RuleCreateFunction, "CREATE FUNCTION"},
  {MySQLParser::RuleCreateUdf, "CREATE FUNCTION UDF"},
  {MySQLParser::RuleCreateIndex, "CREATE INDEX"},
  {MySQLParser::RuleCreateProcedure, "CREATE PROCEDURE"},
  {MySQLParser::RuleCreateServer, "CREATE SERVER"},
  {MySQLParser::RuleCreateTable, "CREATE TABLE"},
  {MySQLParser::RuleCreateTablespace, "CREATE TABLESPACE"},
  {MySQLParser::RuleCreateTrigger, "CREATE TRIGGER"},
  {MySQLParser::RuleCreateUser, "CREATE USER"},
  {MySQLParser::RuleCreateView, "CREATE VIEW"},
  {MySQLParser::RuleDeleteStatement, "DELETE"},
  {MySQLParser::RuleDoStatement, "DO"},
  {MySQLParser::RuleDropUser, "DROP USER"},
  {MySQLParser::RuleExecuteStatement, "EXECUTE STATEMENT"},
  {MySQLParser::RuleDescribeCommand, "EXPLAIN"},
  {MySQLParser::RuleGrant, "GRANT"},
  {MySQLParser::RuleHandlerStatement, "HANDLER"},
  {MySQLParser::RuleHandlerDeclaration, "DECLARE HANDLER"},
  {MySQLParser::RuleHelpCommand, "HELP COMMAND"},
  {MySQLParser::RuleIfStatement, "IF STATEMENT"},
  {MySQLParser::RuleIterateStatement, "ITERATE"},
  {MySQLParser::RuleJoinedTable, "JOIN"},
  {MySQLParser::RuleLabel, "LABELS"},
  {MySQLParser::RuleLeaveStatement, "LEAVE"},
  {MySQLParser::RuleLockStatement, "LOCK"},
  {MySQLParser::RuleLoopBlock, "LOOP"},
  {MySQLParser::RuleCursorOpen, "OPEN"},
  {MySQLParser::RuleCursorClose, "CLOSE"},
  {MySQLParser::RuleCursorFetch, "FETCH"},
  {MySQLParser::RuleProcedureAnalyseClause, "PROCEDURE ANALYSE"},
  {MySQLParser::RuleRenameTableStatement, "RENAME TABLE"},
  {MySQLParser::RuleRenameUser, "RENAME USER"},
  {MySQLParser::RuleRepeatUntilBlock, "REPEAT LOOP"},
  {MySQLParser::RuleReplaceStatement, "REPLACE"},
  {MySQLParser::RuleResignalStatement, "RESIGNAL"},
  {MySQLParser::RuleReturnStatement, "RETURN"},
  {MySQLParser::RuleRevoke, "REVOKE"},
  {MySQLParser::RuleSavepointStatement, "SAVEPOINT"},
  {MySQLParser::RuleSelectStatement, "SELECT"},
  {MySQLParser::RuleSetPassword, "SET PASSWORD"},
  {MySQLParser::RuleTransactionStatement, "START TRANSACTION"},
  {MySQLParser::RuleTruncateTableStatement, "TRUNCATE TABLE"},
  {MySQLParser::RuleUpdateStatement, "UPDATE"},
  {MySQLParser::RuleUseCommand, "USE"},
  {MySQLParser::RuleWhileDoBlock, "WHILE"},
  {MySQLParser::RuleXaStatement, "XA"},
  {MySQLParser::RuleVariableDeclaration, "DECLARE VARIABLE"},
  {MySQLParser::RuleConditionDeclaration, "DECLARE CONDITION"},
  {MySQLParser::RuleHandlerDeclaration, "DECLARE HANDLER"},
  {MySQLParser::RuleCursorDeclaration, "DECLARE CURSOR"},
  {MySQLParser::RuleGetDiagnostics, "GET DIAGNOSTICS"},
  {MySQLParser::RuleSignalStatement, "SIGNAL"},
  {MySQLParser::RuleCursorFetch, "FETCH"},
  {MySQLParser::RuleLeaveStatement, "LEAVE"},
  {MySQLParser::RuleAlterUser, "ALTER USER"},
  {MySQLParser::RuleCaseStatement, "CASE STATEMENT"},
  {MySQLParser::RuleChangeMaster, "CHANGE MASTER TO"},

  {MySQLParser::RuleDropDatabase, "DROP DATABASE"},
  {MySQLParser::RuleDropEvent, "DROP EVENT"},
  {MySQLParser::RuleDropFunction, "DROP FUNCTION"},
  {MySQLParser::RuleDropProcedure, "DROP PROCEDURE"},
  {MySQLParser::RuleDropIndex, "DROP INDEX"},
  {MySQLParser::RuleDropLogfileGroup, "DROP LOGFILEGROUP"},
  {MySQLParser::RuleDropServer, "DROP SERVER"},
  {MySQLParser::RuleDropTable, "DROP TABLE"},
  {MySQLParser::RuleDropTableSpace, "DROP TABLESPACE"},
  {MySQLParser::RuleDropTrigger, "DROP TRIGGER"},
  {MySQLParser::RuleDropView, "DROP VIEW"},

};

// Words which are part of a multi word topic or can produce wrong topics if used alone, and hence need further
// examination.
static std::unordered_set<std::string> specialWords = {
  "CHAR", "COUNT",    "DATE",    "DOUBLE",    "REPLACE", "TIME",   "TIMESTAMP", "YEAR", "DATABASE",
  "USER", "INSERT",   "PREPARE", "HANDLER",   "FLUSH",   "IS",     "IN",        "LIKE", "REGEXP",
  "SET",  "PASSWORD", "SHOW",    "COLLATION", "OPEN",    "UPDATE", "DELETE"};

//----------------------------------------------------------------------------------------------------------------------

/**
 * Determines a help topic from the given query at the given position (given as column/row pair).
 */
std::string DbSqlEditorContextHelp::helpTopicFromPosition(HelpContext *helpContext, const std::string &query,
                                                          std::pair<size_t, size_t> caret) {
  logDebug2("Finding help topic\n");

  // We are not interested in validity here. We simply parse in default mode (LL) and examine the returned parse tree.
  // This usually will give us a good result, except in cases where the query has an error before the caret such that
  // we cannot predict the path through the rules.
  ParserRuleContext *parseTree = helpContext->_d->parse(query);
  ++caret.second; // ANTLR lines are one-based.
  tree::ParseTree *tree = MySQLRecognizerCommon::contextFromPosition(parseTree, caret);

  if (tree == nullptr)
    return "";

  if (antlrcpp::is<tree::TerminalNode *>(tree)) // Should always be the case at this point.
  {
    tree::TerminalNode *node = dynamic_cast<tree::TerminalNode *>(tree);
    size_t token = node->getSymbol()->getType();
    if (token == MySQLLexer::SEMICOLON_SYMBOL) {
      tree = MySQLRecognizerCommon::getPrevious(tree);
      node = dynamic_cast<tree::TerminalNode *>(tree);
      token = node->getSymbol()->getType();
    }

    // First check if we can get a topic for this single token, either from our topic table or by lookup.
    // This is a double-edged sword. It will help in incomplete statements where we do not get a good parse tree
    // but might also show help topics for unrelated stuff (e.g. returns "SAVEPOINT" for "select a := savepoint(c);".
    if (supportedOperatorsAndKeywords.count(token) > 0)
      return supportedOperatorsAndKeywords[token];

    switch (token) {
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
        if (specialWords.count(s) == 0 && topicExists(helpContext->serverVersion(), s))
          return s;

        // No specific help topic for the given terminal. Jump to the token's parent and start the
        // context search then.
        tree = tree->parent;
        break;
    }
  }

  // See if we have a help topic for the given tree. If not walk up the parent chain until we find something.
  while (true) {
    if (tree == nullptr)
      return "";

    // We deliberately don't check if the given tree is actually a parse rule context - there is no other possibility.
    ParserRuleContext *context = dynamic_cast<ParserRuleContext *>(tree);
    size_t ruleIndex = context->getRuleIndex();
    if (contextToTopic.count(ruleIndex) > 0)
      return contextToTopic[ruleIndex];

    // Topics from function names
    std::string functionTopic = functionTopicForContext(context);
    if (!functionTopic.empty() && topicExists(helpContext->serverVersion(), functionTopic))
      return functionTopic;

    switch (ruleIndex) {
      case MySQLParser::RulePredicateOperations: {
        if (!context->children.empty()) {
          // Some keyword topics have variants with a leading NOT.
          auto parent = dynamic_cast<MySQLParser::PredicateContext *>(context->parent);
          bool isNot = parent->notRule() != nullptr;

          // IN, BETWEEN (with special help topic name), LIKE, REGEXP
          auto predicateContext = dynamic_cast<MySQLParser::PredicateOperationsContext *>(context);
          if (isToken(predicateContext->children[0], MySQLLexer::BETWEEN_SYMBOL)) {
            if (isNot)
              return "NOT BETWEEN";
            return "BETWEEN AND";
          }
          std::string topic = isNot ? "NOT " : "";
          return topic + base::toupper(predicateContext->children[0]->getText());
        }
        break;
      }

      case MySQLParser::RuleOtherAdministrativeStatement: {
        // See if we only have a single flush command.
        auto adminContext = dynamic_cast<MySQLParser::OtherAdministrativeStatementContext *>(context);
        if (adminContext->type->getType() == MySQLLexer::FLUSH_SYMBOL && adminContext->flushOption().size() == 1 &&
            adminContext->flushOption(0)->option->getType() == MySQLLexer::QUERY_SYMBOL)
          return "FLUSH QUERY CACHE";

        if (adminContext->type != nullptr)
          return tokenToTopic[adminContext->type->getType()];
        break;
      }

      case MySQLParser::RuleInsertStatement: {
        auto insertContext = dynamic_cast<MySQLParser::InsertStatementContext *>(context);
        if (insertContext->insertQueryExpression() != nullptr)
          return "INSERT SELECT";
        if (insertContext->insertLockOption() != nullptr &&
            insertContext->insertLockOption()->DELAYED_SYMBOL() != nullptr)
          return "INSERT DELAYED";
        return "INSERT";
      }

      case MySQLParser::RuleInstallUninstallStatment: {
        auto pluginContext = dynamic_cast<MySQLParser::InstallUninstallStatmentContext *>(context);
        if (pluginContext->action != nullptr)
          return tokenToTopic[pluginContext->action->getType()];
        break;
      }

      case MySQLParser::RuleExpr: {
        auto exprContext = dynamic_cast<MySQLParser::ExprContext *>(context);
        if (exprContext->children.size() > 2 && isToken(exprContext->children[1], MySQLLexer::IS_SYMBOL)) {
          if (isToken(exprContext->children[2], MySQLLexer::NOT_SYMBOL) ||
              isToken(exprContext->children[2], MySQLLexer::NOT2_SYMBOL))
            return "IS NOT";
          return "IS";
        }
        break;
      }

      case MySQLParser::RuleBoolPri: {
        if (antlrcpp::is<MySQLParser::PrimaryExprIsNullContext *>(context)) {
          auto primaryExprIsNullContext = dynamic_cast<MySQLParser::PrimaryExprIsNullContext *>(context);
          if (primaryExprIsNullContext->notRule() == nullptr)
            return "IS NULL";
          return "IS NOT NULL";
        }
        break;
      }

      case MySQLParser::RuleSetStatement: {
        auto setStatementContext = dynamic_cast<MySQLParser::SetStatementContext *>(context);
        if (setStatementContext->TRANSACTION_SYMBOL() != nullptr)
          return "ISOLATION";

        ParserRuleContext *variableName = nullptr;
        if (setStatementContext->optionValueFollowingOptionType() != nullptr)
          variableName = setStatementContext->optionValueFollowingOptionType()->internalVariableName();
        else if (setStatementContext->optionValueNoOptionType() != nullptr)
          variableName = setStatementContext->optionValueNoOptionType()->internalVariableName();
        if (variableName != nullptr) {
          std::string option = base::toupper(variableName->getText());
          if (option == "SQL_SLAVE_SKIP_COUNTER")
            return "SET GLOBAL SQL_SLAVE_SKIP_COUNTER";
          if (option == "SQL_LOG_BIN")
            return "SET SQL_LOG_BIN";
        }
        return "SET";
      }

      case MySQLParser::RuleLoadStatement: {
        auto loadStatementContext = dynamic_cast<MySQLParser::LoadStatementContext *>(context);
        if (loadStatementContext->dataOrXml()->DATA_SYMBOL() != nullptr)
          return "LOAD DATA";
        return "LOAD XML";
      }

      case MySQLParser::RulePredicate:
        if (context->children.size() > 2) {
          if (isToken(context->children[1], MySQLLexer::NOT_SYMBOL) ||
              isToken(context->children[1], MySQLLexer::NOT2_SYMBOL)) {
            // For NOT BETWEEN, NOT LIKE, NOT IN, NOT REGEXP.
            auto predicateContext = dynamic_cast<MySQLParser::PredicateOperationsContext *>(context->children[2]);
            return "NOT " + base::toupper(predicateContext->children[0]->getText());
          }
          if (isToken(context->children[1], MySQLLexer::SOUNDS_SYMBOL))
            return "SOUNDS LIKE";
        }
        break;

      case MySQLParser::RuleTableAdministrationStatement: {
        auto adminStatementContext = dynamic_cast<MySQLParser::TableAdministrationStatementContext *>(context);
        if (adminStatementContext->type != nullptr)
          return tokenToTopic[adminStatementContext->type->getType()];
        break;
      }

      case MySQLParser::RulePreparedStatement: {
        auto preparedContext = dynamic_cast<MySQLParser::PreparedStatementContext *>(context);
        size_t type = 0;
        if (preparedContext->type != nullptr)
          type = preparedContext->type->getType();
        if (type == MySQLLexer::PREPARE_SYMBOL)
          return "PREPARE";
        if (type == MySQLLexer::DEALLOCATE_SYMBOL || type == MySQLLexer::DROP_SYMBOL)
          return "DEALLOCATE PREPARE";
        break;
      }

      case MySQLParser::RuleReplicationStatement: {
        auto replicationContext = dynamic_cast<MySQLParser::ReplicationStatementContext *>(context);
        if (replicationContext->PURGE_SYMBOL() != nullptr)
          return "PURGE BINARY LOGS";
        if (replicationContext->RESET_SYMBOL() != nullptr &&
            (replicationContext->resetOption().empty() || replicationContext->resetOption()[0]->option == nullptr))
          return "RESET";
        break;
      }

      case MySQLParser::RuleResetOption: {
        auto optionContext = dynamic_cast<MySQLParser::ResetOptionContext *>(context);
        if (isToken(optionContext->option, MySQLLexer::MASTER_SYMBOL))
          return "RESET MASTER";
        if (isToken(optionContext->option, MySQLLexer::SLAVE_SYMBOL))
          return "RESET SLAVE";
        return "RESET";
      }

      case MySQLParser::RuleShowStatement: {
        auto showContext = dynamic_cast<MySQLParser::ShowStatementContext *>(context);
        if (showContext->value == nullptr) {
          if (showContext->charset() != nullptr)
            return "SHOW CHARACTER SET";
          return "SHOW";
        }

        switch (showContext->value->getType()) {
          case MySQLLexer::TABLE_SYMBOL:
            return "SHOW TABLE STATUS";
          case MySQLLexer::SLAVE_SYMBOL:
            if (showContext->HOSTS_SYMBOL() != nullptr)
              return "SHOW SLAVE HOSTS";
            if (showContext->STATUS_SYMBOL() != nullptr)
              return "SHOW SLAVE STATUS";
            break;
          case MySQLLexer::CREATE_SYMBOL: {
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

      case MySQLParser::RuleTableConstraintDef: {
        auto definitionContext = dynamic_cast<MySQLParser::TableConstraintDefContext *>(context);
        if (definitionContext->type->getType() == MySQLLexer::FOREIGN_SYMBOL)
          return "CONSTRAINT";
        break;
      }

      case MySQLParser::RuleHelpCommand:
        return "HELP COMMAND";

      case MySQLParser::RuleSimpleExpr:
        if (!context->children.empty() && antlrcpp::is<tree::TerminalNode *>(context->children[0])) {
          size_t type = dynamic_cast<tree::TerminalNode *>(context->children[0])->getSymbol()->getType();
          switch (type) {
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

      case MySQLParser::RuleEngineRef: {
        std::string engine = base::tolower(context->getText());
        if (engine == "merge" || engine == "mrg_myisam")
          return "MERGE";
        break;
      }

      case MySQLParser::RuleSlave:
        if (!context->children.empty())
          return base::toupper(context->children[0]->getText()) + " SLAVE";
        break;

      case MySQLParser::RuleDataType: {
        auto typeContext = dynamic_cast<MySQLParser::DataTypeContext *>(context);
        if (typeContext->nchar() != nullptr)
          return "CHAR";

        std::string topic;
        switch (typeContext->type->getType()) {
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
            if (typeContext->charsetWithOptBinary() != nullptr &&
                typeContext->charsetWithOptBinary()->BYTE_SYMBOL() != nullptr)
              return "CHAR BYTE";
            return "CHAR";

          default:
            topic = base::toupper(typeContext->type->getText());
            break;
        }

        if (topicExists(helpContext->serverVersion(), topic))
          return topic;

        break; // Not all data types have an own topic.
      }

      case MySQLParser::RuleFromClause: {
        auto keylistContext = dynamic_cast<MySQLParser::FromClauseContext *>(context);
        if (keylistContext->DUAL_SYMBOL() != nullptr)
          return "DUAL";
        break;
      }

      case MySQLParser::RuleSetTransactionCharacteristic: {
        auto characteristicsContext = dynamic_cast<MySQLParser::SetTransactionCharacteristicContext *>(context);
        if (characteristicsContext->ISOLATION_SYMBOL() != nullptr)
          return "ISOLATION";
        break;
      }

      case MySQLParser::RuleSubstringFunction: {
        // A case where we might have a synonym, so we need to check the text actually.
        auto substringContext = dynamic_cast<MySQLParser::SubstringFunctionContext *>(context);
        return base::toupper(substringContext->SUBSTRING_SYMBOL()->getText());

        break;
      }

      case MySQLParser::RuleAlterStatement: {
        auto alterContext = dynamic_cast<MySQLParser::AlterStatementContext *>(context);
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
