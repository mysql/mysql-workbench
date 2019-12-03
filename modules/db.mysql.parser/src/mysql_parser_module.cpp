/*
 * Copyright (c) 2013, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "base/string_utilities.h"
#include "base/util_functions.h"
#include "base/log.h"

#include "grtpp_util.h"

#include "mysql/mysql-recognition-types.h"

#include "mysql/MySQLLexer.h"
#include "mysql/MySQLParser.h"
#include "mysql/MySQLParserBaseListener.h"

#include "objimpl/wrapper/parser_ContextReference_impl.h"
#include "grtdb/db_object_helpers.h"
#include "code-completion/mysql-code-completion.h"

#include "ObjectListeners.h"

#include "mysql_parser_module.h"

using namespace grt;
using namespace parsers;

using namespace antlr4;
using namespace antlr4::atn;
using namespace antlr4::tree;

using namespace std::string_literals;

DEFAULT_LOG_DOMAIN("parser")

GRT_MODULE_ENTRY_POINT(MySQLParserServicesImpl);

//----------------------------------------------------------------------------------------------------------------------

long shortVersion(const GrtVersionRef &version) {
  ssize_t short_version;
  if (version.is_valid()) {
    short_version = version->majorNumber() * 10000;
    if (version->minorNumber() > -1)
      short_version += version->minorNumber() * 100;
    else
      short_version += 500;
    if (version->releaseNumber() > -1)
      short_version += version->releaseNumber();
  } else
    short_version = 50500; // Assume minimal supported version.

  return (long)short_version;
}

//------------------ MySQLParserContextImpl ----------------------------------------------------------------------------

struct MySQLParserContextImpl;

class LexerErrorListener : public BaseErrorListener {
public:
  MySQLParserContextImpl *owner;

  LexerErrorListener(MySQLParserContextImpl *aOwner) : owner(aOwner) {}

  virtual void syntaxError(Recognizer *recognizer, Token *offendingSymbol, size_t line, size_t charPositionInLine,
                           const std::string &msg, std::exception_ptr e) override;
 };

class ParserErrorListener : public BaseErrorListener {
public:
  MySQLParserContextImpl *owner;

  ParserErrorListener(MySQLParserContextImpl *aOwner) : owner(aOwner) {}

  virtual void syntaxError(Recognizer *recognizer, Token *offendingSymbol, size_t line, size_t charPositionInLine,
                           const std::string &msg, std::exception_ptr e) override;
};
      
//----------------------------------------------------------------------------------------------------------------------

struct MySQLParserContextImpl : public MySQLParserContext {
  ANTLRInputStream input;
  MySQLLexer lexer;
  CommonTokenStream tokens;
  MySQLParser parser;
  LexerErrorListener lexerErrorListener;
  ParserErrorListener parserErrorListener;

  GrtVersionRef version;
  std::string mode;

  bool caseSensitive;
  std::vector<ParserErrorInfo> errors;

  MySQLParserContextImpl(GrtCharacterSetsRef charsets, GrtVersionRef version_, bool caseSensitive)
    : lexer(&input), tokens(&lexer), parser(&tokens), lexerErrorListener(this), parserErrorListener(this),
    caseSensitive(caseSensitive) {

    std::set<std::string> filteredCharsets;
    for (size_t i = 0; i < charsets->count(); i++)
      filteredCharsets.insert("_" + base::tolower(*charsets[i]->name()));
    lexer.charsets = filteredCharsets;
    updateServerVersion(version_);

    lexer.removeErrorListeners();
    lexer.addErrorListener(&lexerErrorListener);

    parser.removeParseListeners();
    parser.removeErrorListeners();
    parser.addErrorListener(&parserErrorListener);
  }

  virtual bool isCaseSensitive() override {
    return caseSensitive;
  }

  virtual void updateServerVersion(GrtVersionRef newVersion) override {
    if (version != newVersion) {
      version = newVersion;
      lexer.serverVersion = shortVersion(version);
      parser.serverVersion = lexer.serverVersion;

      if (lexer.serverVersion < 50503) {
        lexer.charsets.erase("_utf8mb4");
        lexer.charsets.erase("_utf16");
        lexer.charsets.erase("_utf32");
      } else {
        // Duplicates are automatically ignored.
        lexer.charsets.insert("_utf8mb3");
        lexer.charsets.insert("_utf8mb4");
        lexer.charsets.insert("_utf16");
        lexer.charsets.insert("_utf32");
      }
    }
  }

  virtual void updateSqlMode(const std::string &mode) override {
    this->mode = mode;
    lexer.sqlModeFromString(mode);
    parser.sqlMode = lexer.sqlMode;
  }

  virtual GrtVersionRef serverVersion() const override {
    return version;
  }

  virtual std::string sqlMode() const override {
    return mode;
  }

  void addError(std::string const& message, size_t tokenType, size_t startIndex, size_t line, size_t column, size_t length) {
    if (length == 0)
      length = 1;
    errors.push_back({ message, tokenType, startIndex, line, column, length });
  }

  /**
   * Returns a collection of errors from the last parser run. The start position is offset by the given
   * value (used to adjust error position in a larger context).
   */
  virtual std::vector<ParserErrorInfo> errorsWithOffset(size_t offset) const override {
    std::vector<ParserErrorInfo> result = errors;

    for (auto &entry : result)
      entry.charOffset += offset;

    return result;
  }

  virtual Scanner createScanner() override {
    tokens.reset();
    Scanner scanner(&tokens);
    return scanner;
  }

  virtual bool isIdentifier(size_t type) const override {
    return lexer.isIdentifier(type);
  }

  ParseTree *parse(const std::string &text, MySQLParseUnit unit) {
    input.load(text);
    return startParsing(false, unit);
  }

  bool errorCheck(const std::string &text, MySQLParseUnit unit) {
    parser.removeParseListeners();
    input.load(text);
    startParsing(true, unit);
    return errors.empty();
  }

  MySQLQueryType determineQueryType(const std::string &text) {
    // Important: this call invalidates any previous parse result (because we have to reset lexer and parser to avoid
    //            dangling token references).
    parser.reset();
    errors.clear();
    
    input.load(text);
    lexer.setInputStream(&input);
    tokens.setTokenSource(&lexer);

    return lexer.determineQueryType();
  }

  std::vector<std::pair<int, std::string>> getCodeCompletionCandidates(
    std::pair<size_t, size_t> caret, std::string const &sql, std::string const &defaultSchema, bool uppercaseKeywords,
    parsers::SymbolTable &symbolTable) {

    parser.reset();
    errors.clear();

    input.load(sql);
    lexer.setInputStream(&input);
    tokens.setTokenSource(&lexer);
    return getCodeCompletionList(caret.second, caret.first, defaultSchema, uppercaseKeywords, &parser, symbolTable);
  }

private:
  ParseTree *parseUnit(MySQLParseUnit unit) {
    switch (unit) {
      case MySQLParseUnit::PuCreateRoutine:
        return parser.createRoutine();

      case MySQLParseUnit::PuDataType:
        return parser.dataTypeDefinition();

      default:
        return parser.query();
    }
  }

  ParseTree *startParsing(bool fast, MySQLParseUnit unit) {
    errors.clear();
    lexer.reset();
    lexer.setInputStream(&input); // Not just reset(), which only rewinds the current position.
    tokens.setTokenSource(&lexer);

    parser.reset();
    parser.setBuildParseTree(!fast);

    // First parse with the bail error strategy to get quick feedback for correct queries.
    parser.setErrorHandler(std::shared_ptr<BailErrorStrategy>(new BailErrorStrategy()));
    parser.getInterpreter<ParserATNSimulator>()->setPredictionMode(PredictionMode::SLL);

    ParseTree *tree;
    try {
      tree = parseUnit(unit);
    } catch (ParseCancellationException &) {
      // Even in fast mode we have to do a second run if we got no error yet (BailErrorStrategy
      // does not do full processing).
      if (fast && !errors.empty())
        tree = nullptr;
      else {
        // If parsing was canceled we either really have a syntax error or we need to do a second step,
        // now with the default strategy and LL parsing.
        tokens.reset();
        parser.reset();
        errors.clear();
        parser.setErrorHandler(std::make_shared<DefaultErrorStrategy>());
        parser.getInterpreter<ParserATNSimulator>()->setPredictionMode(PredictionMode::LL);
        tree = parseUnit(unit);
      }
    }

    return tree;
  }

  /**
   * Debugging helper that prints all tokens recognized by the lexer with the current input.
   */
  void dumpTokens() {
    tokens.fill();
    for (auto token : tokens.getTokens())
      std::cout << token->toString() << std::endl;
  }
};

//----------------------------------------------------------------------------------------------------------------------

void LexerErrorListener::syntaxError(Recognizer *recognizer, Token *, size_t line,
                                     size_t charPositionInLine, const std::string &, std::exception_ptr ep) {
  // The passed in string is the ANTLR generated error message which we want to improve here.
  // The token reference is always null in a lexer error.
  std::string message;
  try {
    std::rethrow_exception(ep);
  } catch (LexerNoViableAltException &) {
    Lexer *lexer = dynamic_cast<Lexer *>(recognizer);
    CharStream *input = lexer->getInputStream();
    std::string text = lexer->getErrorDisplay(input->getText(misc::Interval(lexer->tokenStartCharIndex, input->index())));
    if (text.empty())
      text = " "; // Should never happen.

    switch (text[0]) {
      case '/':
        message = "Unfinished multiline comment";
        break;
      case '"':
        message = "Unfinished double quoted string literal";
        break;
      case '\'':
        message = "Unfinished single quoted string literal";
        break;
      case '`':
        message = "Unfinished back tick quoted string literal";
        break;

      default:
        // Hex or bin string?
        if (text.size() > 1 && text[1] == '\'' && (text[0] == 'x' || text[0] == 'b')) {
          message = std::string("Unfinished ") + (text[0] == 'x' ? "hex" : "binary") + " string literal";
          break;
        }

        // Something else the lexer couldn't make sense of (likely there is no rule that accepts this input).
        message = "\"" + text + "\" is no valid input at all";
        break;
    }
    owner->addError(message, 0, lexer->tokenStartCharIndex, line, charPositionInLine,
                    input->index() - lexer->tokenStartCharIndex);
  }
}

//----------------------------------------------------------------------------------------------------------------------

std::string intervalToString(misc::IntervalSet set, size_t maxCount, dfa::Vocabulary vocabulary) {
  std::vector<ssize_t> symbols = set.toList();

  if (symbols.empty()) {
    return "";
  }

  std::stringstream ss;
  bool firstEntry = true;
  maxCount = std::min(maxCount, symbols.size());
  for (size_t i = 0; i < maxCount; ++i) {
    ssize_t symbol = symbols[i];
    if (!firstEntry)
      ss << ", ";
    firstEntry = false;

    if (symbol < 0) {
      ss << "EOF";
    } else {
      std::string name = vocabulary.getDisplayName(symbol);
      if (name.find("_SYMBOL") != std::string::npos)
        name = name.substr(0, name.size() - 7);
      else if (name.find("_OPERATOR") != std::string::npos)
        name = name.substr(0, name.size() - 9);
      else if (name.find("_NUMBER") != std::string::npos)
        name = name.substr(0, name.size() - 7) + " number";
      else {
        static std::map<std::string, std::string> friendlyDescription = {
          { "BACK_TICK_QUOTED_ID", "`text`" },
          { "DOUBLE_QUOTED_TEXT", "\"text\"" },
          { "SINGLE_QUOTED_TEXT", "'text'" },
        };
        if (friendlyDescription.find(name) != friendlyDescription.end())
          name = friendlyDescription[name];
      }
      ss << name;
    }
  }

  if (maxCount < symbols.size())
    ss << ", ...";

  return ss.str();
}

//----------------------------------------------------------------------------------------------------------------------

void ParserErrorListener::syntaxError(Recognizer *recognizer, Token *offendingSymbol, size_t line,
                                      size_t charPositionInLine, std::string const& msg, std::exception_ptr ep)  {

  std::string message;

  MySQLParser *parser = dynamic_cast<MySQLParser *>(recognizer);
  MySQLBaseLexer *lexer = dynamic_cast<MySQLBaseLexer *>(parser->getTokenStream()->getTokenSource());
  bool isEof = offendingSymbol->getType() == Token::EOF;
  if (isEof) {
    // In order to show a good error marker look at the previous token if we are at the input end.
    Token *previous = parser->getTokenStream()->LT(-1);
    if (previous != nullptr)
      offendingSymbol = previous;
  }

  std::string wrongText = offendingSymbol->getText();

  // getExpectedTokens() ignores predicates, so it might include the token for which we got this syntax error,
  // if that was excluded by a predicate (which in our case is always a version check).
  // That's a good indicator to tell the user that this keyword is not valid *for the current server version*.
  misc::IntervalSet expected = parser->getExpectedTokens();
  bool invalidForVersion = false;
  size_t tokenType = offendingSymbol->getType();
  if (tokenType != MySQLLexer::IDENTIFIER && expected.contains(tokenType))
    invalidForVersion = true;
  else {
    tokenType = lexer->keywordFromText(wrongText);
    if (expected.contains(tokenType))
      invalidForVersion = true;
  }

  if (invalidForVersion) {
    expected.remove(tokenType);
  }

  // Try to find the expected input by examining the current parser context and
  // the expected interval set. The latter often lists useless keywords, especially if they are allowed
  // as identifiers.
  std::string expectedText;

  static std::set<size_t> simpleRules = {
    MySQLParser::RuleIdentifier,
    MySQLParser::RuleQualifiedIdentifier,
  };

  static std::map<size_t, std::string> objectNames = {
    { MySQLParser::RuleColumnName, "column" },
    { MySQLParser::RuleColumnRef, "column" },
    { MySQLParser::RuleColumnInternalRef, "column" },
    { MySQLParser::RuleIndexName, "index" },
    { MySQLParser::RuleIndexRef, "index" },
    { MySQLParser::RuleSchemaName, "schema" },
    { MySQLParser::RuleSchemaRef, "schema" },
    { MySQLParser::RuleProcedureName, "procedure" },
    { MySQLParser::RuleProcedureRef, "procedure" },
    { MySQLParser::RuleFunctionName, "function" },
    { MySQLParser::RuleFunctionRef, "function" },
    { MySQLParser::RuleTriggerName, "trigger" },
    { MySQLParser::RuleTriggerRef, "trigger" },
    { MySQLParser::RuleViewName, "view" },
    { MySQLParser::RuleViewRef, "view" },
    { MySQLParser::RuleTablespaceName, "tablespace" },
    { MySQLParser::RuleTablespaceRef, "tablespace" },
    { MySQLParser::RuleLogfileGroupName, "logfile group" },
    { MySQLParser::RuleLogfileGroupRef, "logfile group" },
    { MySQLParser::RuleEventName, "event" },
    { MySQLParser::RuleEventRef, "event" },
    { MySQLParser::RuleUdfName, "udf" },
    { MySQLParser::RuleServerName, "server" },
    { MySQLParser::RuleServerRef, "server" },
    { MySQLParser::RuleEngineRef, "engine" },
    { MySQLParser::RuleTableName, "table" },
    { MySQLParser::RuleTableRef, "table" },
    { MySQLParser::RuleFilterTableRef, "table" },
    { MySQLParser::RuleTableRefWithWildcard, "table" },
    { MySQLParser::RuleParameterName, "parameter" },
    { MySQLParser::RuleLabelIdentifier, "label" },
    { MySQLParser::RuleLabelRef, "label" },
    { MySQLParser::RuleRoleIdentifier, "role" },
    { MySQLParser::RuleRoleRef, "role" },
    { MySQLParser::RulePluginRef, "plugin" },
    { MySQLParser::RuleComponentRef, "component" },
    { MySQLParser::RuleResourceGroupRef, "resource group" },
    { MySQLParser::RuleWindowName, "window" },
  };

  // Walk up from generic rules to reach something that gives us more context, if needed.
  ParserRuleContext *context = parser->getRuleContext();
  while (simpleRules.find(context->getRuleIndex()) != simpleRules.end())
    context = dynamic_cast<ParserRuleContext *>(context->parent);

  switch (context->getRuleIndex()) {
    case MySQLParser::RuleFunctionCall:
      expectedText =  "a complete function call or other expression";
      break;

    case MySQLParser::RuleExpr:
      expectedText = "an expression";
      break;

    case MySQLParser::RuleColumnName:
    case MySQLParser::RuleIndexName:
    case MySQLParser::RuleSchemaName:
    case MySQLParser::RuleProcedureName:
    case MySQLParser::RuleFunctionName:
    case MySQLParser::RuleTriggerName:
    case MySQLParser::RuleViewName:
    case MySQLParser::RuleTablespaceName:
    case MySQLParser::RuleLogfileGroupName:
    case MySQLParser::RuleEventName:
    case MySQLParser::RuleUdfName:
    case MySQLParser::RuleServerName:
    case MySQLParser::RuleTableName:
    case MySQLParser::RuleParameterName:
    case MySQLParser::RuleLabelIdentifier:
    case MySQLParser::RuleRoleIdentifier:
    case MySQLParser::RuleWindowName: {
      auto iterator = objectNames.find(context->getRuleIndex());
      if (iterator == objectNames.end())
        expectedText = "a new object name";
      else
        expectedText = "a new "s + iterator->second + " name";
      break;
    }

    case MySQLParser::RuleColumnRef:
    case MySQLParser::RuleIndexRef:
    case MySQLParser::RuleSchemaRef:
    case MySQLParser::RuleProcedureRef:
    case MySQLParser::RuleFunctionRef:
    case MySQLParser::RuleTriggerRef:
    case MySQLParser::RuleViewRef:
    case MySQLParser::RuleTablespaceRef:
    case MySQLParser::RuleLogfileGroupRef:
    case MySQLParser::RuleEventRef:
    case MySQLParser::RuleServerRef:
    case MySQLParser::RuleEngineRef:
    case MySQLParser::RuleTableRef:
    case MySQLParser::RuleFilterTableRef:
    case MySQLParser::RuleTableRefWithWildcard:
    case MySQLParser::RuleLabelRef:
    case MySQLParser::RuleRoleRef:
    case MySQLParser::RulePluginRef:
    case MySQLParser::RuleComponentRef:
    case MySQLParser::RuleResourceGroupRef: {
      auto iterator = objectNames.find(context->getRuleIndex());
      if (iterator == objectNames.end())
        expectedText = "the name of an existing object";
      else
        expectedText = "the name of an existing "s + iterator->second;
      break;
    }

    case MySQLParser::RuleColumnInternalRef:
      expectedText = "a column name from this table";
      break;

    default: {
      // If the expected set contains the IDENTIFIER token we likely want an identifier at this position.
      // Due to the fact that MySQL defines a number of keywords as possible identifiers, we get all those
      // whenever an identifier is actually required, bloating so the expected set with irrelevant elements.
      // Hence we check for the identifier entry and assume we *only* want an identifier. This gives an unprecise
      // result if both certain keywords *and* an identifier are expected.
      if (expected.contains(static_cast<ssize_t>(MySQLLexer::IDENTIFIER)))
        expectedText = "an identifier";
      else
        expectedText = intervalToString(expected, 6, parser->getVocabulary());
      break;
    }
  }

  if (wrongText[0] != '"' && wrongText[0] != '\'' && wrongText[0] != '`')
    wrongText = "\"" + wrongText + "\"";

  if (ep == nullptr) {
    // Missing or unwanted tokens.
    if (msg.find("missing") != std::string::npos) {
      if (expected.size() == 1) {
        message = "Missing "s + expectedText;
      }
    } else {
      message = "Extraneous input "s + wrongText + " found, expecting " + expectedText;
    }
  } else {
    try {
      std::rethrow_exception(ep);
    } catch (InputMismatchException &) {
      if (isEof)
        message = "Statement is incomplete";
      else
        message = wrongText + " is not valid at this position";

      if (!expectedText.empty())
        message += ", expecting " + expectedText;
    } catch (FailedPredicateException &e) {
      // For cases like "... | a ({condition}? b)", but not "... | a ({condition}? b)?".
      std::string condition = e.what();
      static std::string prefix = "predicate failed: ";
      condition.erase(0, prefix.size());
      condition.resize(condition.size() - 1); // Remove trailing question mark.

      std::size_t index;
      while ((index = condition.find("serverVersion")) != std::string::npos) {
        condition.replace(index, 13, "server version");
      }

      if ((index = condition.find("&&")) != std::string::npos) {
        condition.replace(index, 2, "and");
      }
      message = wrongText + " is valid only for " + condition;
    } catch (NoViableAltException &) {
      if (isEof)
        message = "Statement is incomplete";
      else {
        message = wrongText + " is not valid at this position";
        if (invalidForVersion)
          message += " for this server version";
      }

      if (!expectedText.empty())
        message += ", expecting " + expectedText;
    }
  }

  owner->addError(message, offendingSymbol->getType(), offendingSymbol->getStartIndex(), line, charPositionInLine,
                  offendingSymbol->getStopIndex() - offendingSymbol->getStartIndex() + 1);
}

//------------------ MySQLParserServicesImpl ---------------------------------------------------------------------------

MySQLParserContext::Ref MySQLParserServicesImpl::createParserContext(GrtCharacterSetsRef charsets,
                                                                     GrtVersionRef version, const std::string &sqlMode,
                                                                     bool caseSensitive) {
  MySQLParserContext::Ref context = std::make_shared<MySQLParserContextImpl>(charsets, version, caseSensitive != 0);
  context->updateSqlMode(sqlMode);
  return context;
}

//----------------------------------------------------------------------------------------------------------------------

parser_ContextReferenceRef MySQLParserServicesImpl::createNewParserContext(GrtCharacterSetsRef charsets,
                                                                           GrtVersionRef version,
                                                                           const std::string &sqlMode,
                                                                           int caseSensitive) {
  MySQLParserContext::Ref context = std::make_shared<MySQLParserContextImpl>(charsets, version, caseSensitive != 0);
  context->updateSqlMode(sqlMode);
  return parser_context_to_grt(context);
}

//----------------------------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::tokenFromString(MySQLParserContext::Ref context, const std::string &token) {
  MySQLParserContextImpl *impl = dynamic_cast<MySQLParserContextImpl *>(context.get());

  return impl->lexer.getTokenType(token);
}

//----------------------------------------------------------------------------------------------------------------------

MySQLQueryType MySQLParserServicesImpl::determineQueryType(MySQLParserContext::Ref context, const std::string &text) {
  MySQLParserContextImpl *impl = dynamic_cast<MySQLParserContextImpl *>(context.get());

  return impl->determineQueryType(text);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 *	Resolves all column/table references we collected before to existing objects.
 *	If any of the references does not point to a valid object, we create a stub object for it.
 */
void resolveReferences(db_mysql_CatalogRef catalog, DbObjectsRefsCache &refCache, bool caseSensitive) {
  grt::ListRef<db_mysql_Schema> schemata = catalog->schemata();

  for (DbObjectsRefsCache::iterator refIt = refCache.begin(); refIt != refCache.end(); ++refIt) {
    DbObjectReferences references = (*refIt);
    // Referenced table. Only used for foreign keys.
    db_mysql_TableRef referencedTable;
    if (references.type != DbObjectReferences::Index) {
      db_mysql_SchemaRef schema = find_named_object_in_list(schemata, references.targetIdentifier.first, caseSensitive);
      if (!schema.is_valid()) // Implicitly create the schema if we reference one not yet created.
        schema = ObjectListener::ensureSchemaExists(catalog, references.targetIdentifier.first, caseSensitive);

      referencedTable = find_named_object_in_list(schema->tables(), references.targetIdentifier.second, caseSensitive);
      if (!referencedTable.is_valid()) {
        // If we don't find a table with the given name we create a stub object to be used instead.
        referencedTable = db_mysql_TableRef(grt::Initialized);
        referencedTable->owner(schema);
        referencedTable->isStub(1);
        referencedTable->name(references.targetIdentifier.second);
        referencedTable->oldName(references.targetIdentifier.second);
        schema->tables().insert(referencedTable);
      }

      if (references.foreignKey.is_valid() && (references.type == DbObjectReferences::Referenced))
        references.foreignKey->referencedTable(referencedTable);

      if (references.table.is_valid() && !references.table->tableEngine().empty() &&
          referencedTable->tableEngine().empty())
        referencedTable->tableEngine(references.table->tableEngine());
    }

    // Resolve columns.
    switch (references.type) {
      case DbObjectReferences::Index: {
        // Filling column references for an index.
        for (grt::ListRef<db_IndexColumn>::const_iterator indexIt = references.index->columns().begin();
             indexIt != references.index->columns().end(); ++indexIt) {
          db_mysql_ColumnRef column = find_named_object_in_list(references.table->columns(), (*indexIt)->name(), false);

          // Reset name field to avoid unnecessary trouble with test code.
          (*indexIt)->name("");
          if (column.is_valid())
            (*indexIt)->referencedColumn(column);
        }
        break;
      }

      case DbObjectReferences::Referencing: {
        // Filling column references for the referencing table.
        for (std::vector<std::string>::iterator nameIt = references.columnNames.begin();
             nameIt != references.columnNames.end(); ++nameIt) {
          db_mysql_ColumnRef column = find_named_object_in_list(references.table->columns(), *nameIt, false);
          if (column.is_valid())
            references.foreignKey->columns().insert(column);
        }
        break;
      }

      case DbObjectReferences::Referenced: {
        // Column references for the referenced table.
        int columnIndex = 0;

        for (std::vector<std::string>::iterator columnNameIt = references.columnNames.begin();
             columnNameIt != references.columnNames.end(); ++columnNameIt) {
          db_mysql_ColumnRef column = find_named_object_in_list(referencedTable->columns(), *columnNameIt,
                                                                false); // MySQL columns are always case-insensitive.

          if (!column.is_valid()) {
            if (referencedTable->isStub()) {
              column = db_mysql_ColumnRef(grt::Initialized);
              column->owner(referencedTable);
              column->name(*columnNameIt);
              column->oldName(*columnNameIt);

              // For the stub column we use all the data type settings from the foreign key column.
              db_mysql_ColumnRef templateColumn =
                db_mysql_ColumnRef::cast_from(references.foreignKey->columns().get(columnIndex));
              column->simpleType(templateColumn->simpleType());
              column->userType(templateColumn->userType());
              column->structuredType(templateColumn->structuredType());
              column->precision(templateColumn->precision());
              column->scale(templateColumn->scale());
              column->length(templateColumn->length());
              column->datatypeExplicitParams(templateColumn->datatypeExplicitParams());
              column->formattedType(templateColumn->formattedType());

              StringListRef templateFlags = templateColumn->flags();
              StringListRef flags = column->flags();

              for (grt::StringListRef::const_iterator flagIt = templateColumn->flags().begin();
                   flagIt != templateColumn->flags().end(); ++flagIt)
                flags.insert(*flagIt);

              column->characterSetName(templateColumn->characterSetName());
              column->collationName(templateColumn->collationName());

              referencedTable->columns().insert(column);
              references.foreignKey->referencedColumns().insert(column);
            } else {
              // Column not found in a non-stub table. We only add stub columns to stub tables.
              references.table->foreignKeys().gremove_value(references.foreignKey);
              break; // No need to check other columns. That FK is done.
            }
          } else
            references.foreignKey->referencedColumns().insert(column);

          ++columnIndex;
        }

        // Once done with adding all referenced columns add an index for the foreign key if it doesn't exist yet.
        //
        // Don't add an index if there are no FK columns, however.
        // TODO: Review this. There is no reason why we shouldn't create an index in this case.
        // Not sure what this decision is based on, but since the index is purely composed of
        // columns of the referencing table (which are known) it doesn't matter if there are FK columns or not.
        // But that's how the old parser did it, so we replicate this for now.
        //
        // Similarly, if a stub column is found the first time (i.e. created) the old parser did not
        // add an index for it either, which seems to be totally wrong. So we deviate here from that
        // behavior and create an index too in such cases.
        if (references.columnNames.empty())
          continue;

        ListRef<db_Column> fkColumns = references.foreignKey->columns();
        db_IndexRef foundIndex;
        for (grt::ListRef<db_mysql_Index>::const_iterator indexIt = references.table->indices().begin();
             indexIt != references.table->indices().end(); ++indexIt) {
          ListRef<db_IndexColumn> indexColumns = (*indexIt)->columns();

          bool indexMatches = true;

          // Go over all FK columns (not the index columns as they might differ).
          // Check that all FK columns are at the beginning of the index, in the same order.
          for (size_t i = 0; i < fkColumns->count(); ++i) {
            if (i >= indexColumns->count() || fkColumns->get(i) != indexColumns.get(i)->referencedColumn()) {
              indexMatches = false;
              break;
            }
          }

          if (indexMatches) {
            foundIndex = *indexIt;
            break;
          }
        }

        if (foundIndex.is_valid()) {
          if ((*foundIndex->indexType()).empty())
            foundIndex->indexType("INDEX");
          references.foreignKey->index(foundIndex);
        } else {
          // No valid index found, so create a new one.
          db_mysql_IndexRef index(grt::Initialized);
          index->owner(references.table);
          index->name(references.foreignKey->name());
          index->oldName(index->name());
          index->indexType("INDEX");
          references.foreignKey->index(index);

          for (ListRef<db_Column>::const_iterator columnIt = fkColumns.begin(); columnIt != fkColumns.end();
               ++columnIt) {
            db_mysql_IndexColumnRef indexColumn(grt::Initialized);
            indexColumn->owner(index);
            indexColumn->referencedColumn(*columnIt);
            index->columns().insert(indexColumn);
          }

          references.table->indices().insert(index);
        }
        break;
      }
      default:
        break;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Parses all values defined by the sql into the given table.
 * In opposition to other parse functions we pass the target object in by reference because it is possible that
 * the sql contains a LIKE clause (e.g. "create table a like b") which requires to duplicate the
 * referenced table and hence replace the inner value of the passed in table reference.
 */
size_t MySQLParserServicesImpl::parseTable(MySQLParserContext::Ref context, db_mysql_TableRef table,
                                           const std::string &sql) {
  logDebug2("Parse table\n");

  assert(table.is_valid());

  table->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  MySQLParserContextImpl *impl = dynamic_cast<MySQLParserContextImpl *>(context.get());
  auto tree = impl->parse(sql, MySQLParseUnit::PuCreateTable);

  if (impl->errors.empty()) {
    db_mysql_CatalogRef catalog;
    db_mysql_SchemaRef schema;
    if (table->owner().is_valid()) {
      schema = db_mysql_SchemaRef::cast_from(table->owner());
      if (schema->owner().is_valid())
        catalog = db_mysql_CatalogRef::cast_from(schema->owner());
    }

    DbObjectsRefsCache refCache;
    TableListener listener(tree, catalog, schema, table, impl->caseSensitive, true, refCache);
    resolveReferences(catalog, refCache, impl->caseSensitive);
  } else {
    // Finished with errors. See if we can get at least the table name out.
    auto tableTree = dynamic_cast<MySQLParser::CreateTableContext *>(tree);
    if (tableTree->tableName() != nullptr) {
      IdentifierListener listener(tableTree->tableName());
      table->name(listener.parts.back() + "_SYNTAX_ERROR");
    }
  }

  return impl->errors.size();
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseTriggerSql(parser_ContextReferenceRef context_ref, db_mysql_TriggerRef trigger,
                                                const std::string &sql) {
  MySQLParserContext::Ref context = parser_context_from_grt(context_ref);
  return parseTrigger(context, trigger, sql);
}

//----------------------------------------------------------------------------------------------------------------------

/**
* Parses the given sql as trigger create script and fills all found details in the given trigger ref.
* If there's an error nothing is changed.
* Returns the number of errors.
*/
size_t MySQLParserServicesImpl::parseTrigger(MySQLParserContext::Ref context, db_mysql_TriggerRef trigger,
                                             const std::string &sql) {
  logDebug2("Parse trigger\n");

  trigger->sqlDefinition(base::trim(sql));
  trigger->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  MySQLParserContextImpl *impl = dynamic_cast<MySQLParserContextImpl *>(context.get());
  auto tree = impl->parse(sql, MySQLParseUnit::PuCreateTrigger);

  db_mysql_TableRef table;
  if (impl->errors.empty()) {
    trigger->modelOnly(0);

    db_mysql_CatalogRef catalog;
    db_mysql_SchemaRef schema;
    if (trigger->owner().is_valid()) {
      table = db_mysql_TableRef::cast_from(trigger->owner());
      if (table->owner().is_valid()) {
        schema = db_mysql_SchemaRef::cast_from(table->owner());
        if (schema->owner().is_valid())
          catalog = db_mysql_CatalogRef::cast_from(schema->owner());
      }
    }

    TriggerListener listener(tree, catalog, schema, trigger, impl->caseSensitive);
    db_mysql_TableRef ownerTable = db_mysql_TableRef::cast_from(trigger->owner());
    if (!base::same_string(table->name(), ownerTable->name(), false)) {
      trigger->name(*trigger->name() + "_WRONG_SCHEMA");
    }
  } else {
    trigger->modelOnly(1);

    // Finished with errors. See if we can get at least the trigger name out.
    auto triggerTree = dynamic_cast<MySQLParser::CreateTriggerContext *>(tree);
    if (triggerTree != nullptr) {
      if (triggerTree->triggerName() != nullptr) {
        IdentifierListener listener(triggerTree->triggerName());
        trigger->name(listener.parts.back() + "_SYNTAX_ERROR");
      }

      // Another attempt: find the ordering as we may need to manipulate this.
      if (triggerTree->triggerFollowsPrecedesClause() != nullptr) {
        trigger->ordering(triggerTree->triggerFollowsPrecedesClause()->ordering->getText());
        trigger->otherTrigger(triggerTree->triggerFollowsPrecedesClause()->textOrIdentifier()->getText());
      }
    }
  }

  if (table.is_valid()) {
    // TODO: this is modeled after the old parser code but doesn't make much sense this way.
    //       There's only one flag for all triggers. So, at least there should be a scan over all triggers
    //       when determining this flag.
    if (!impl->errors.empty())
      table->customData().set("triggerInvalid", grt::IntegerRef(1));
    else
      table->customData().remove("triggerInvalid");
  }
  return impl->errors.size();
}

//----------------------------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseViewSql(parser_ContextReferenceRef context_ref, db_mysql_ViewRef view,
                                             const std::string &sql) {
  MySQLParserContext::Ref context = parser_context_from_grt(context_ref);
  return parseView(context, view, sql);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Parses the given sql as a create view script and fills all found details in the given view ref.
 * If there's an error nothing changes. If the sql contains a schema reference other than that the
 * the view is in the view's name will be changed (adds _WRONG_SCHEMA) to indicate that.
 */
size_t MySQLParserServicesImpl::parseView(MySQLParserContext::Ref context, db_mysql_ViewRef view,
                                          const std::string &sql) {
  logDebug2("Parse view\n");

  view->sqlDefinition(base::trim(sql));
  view->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  MySQLParserContextImpl *impl = dynamic_cast<MySQLParserContextImpl *>(context.get());
  auto tree = impl->parse(sql, MySQLParseUnit::PuCreateView);

  if (impl->errors.empty()) {
    db_mysql_CatalogRef catalog;
    db_mysql_SchemaRef schema;
    if (view->owner().is_valid()) {
      schema = db_mysql_SchemaRef::cast_from(view->owner());
      if (schema->owner().is_valid())
        catalog = db_mysql_CatalogRef::cast_from(schema->owner());
    }

    ViewListener listener(tree, catalog, view, impl->caseSensitive);

    db_mysql_SchemaRef ownerSchema = db_mysql_SchemaRef::cast_from(view->owner());
    if (schema.is_valid() && !base::same_string(schema->name(), ownerSchema->name(), impl->caseSensitive)) {
      view->name(*view->name() + "_WRONG_SCHEMA");
    }
  } else {
    // Finished with errors. See if we can get at least the view name out.
    auto viewTree = dynamic_cast<MySQLParser::CreateViewContext *>(tree);
    if (viewTree != nullptr && viewTree->viewName() != nullptr) {
      IdentifierListener listener(viewTree->viewName());
      view->name(listener.parts.back() + "_SYNTAX_ERROR");
    }
  }

  return impl->errors.size();
}

//----------------------------------------------------------------------------------------------------------------------

std::pair<std::string, std::string> getRoutineNameAndType(ParseTree *context) {
  auto routineTree = (MySQLParser::CreateRoutineContext *)context;
  std::pair<std::string, std::string> result;
  if (routineTree->createProcedure() != nullptr) {
    result.second = "procedure";
    result.first = base::unquote(routineTree->createProcedure()->procedureName()->getText());
  } else if (routineTree->createFunction() != nullptr) {
    result.second = "function";
    result.first = base::unquote(routineTree->createFunction()->functionName()->getText());
  } else if (routineTree->createUdf() != nullptr) {
    result.second = "udf";
    result.first = base::unquote(routineTree->createUdf()->udfName()->getText());
  }
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseRoutineSql(parser_ContextReferenceRef context_ref, db_mysql_RoutineRef routine,
                                                const std::string &sql) {
  MySQLParserContext::Ref context = parser_context_from_grt(context_ref);
  return parseRoutine(context, routine, sql);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Parses the given sql as a create function/procedure script and fills all found details in the given routine ref.
 * If there's an error nothing changes. If the sql contains a schema reference other than that the
 * the routine is in the routine's name will be changed (adds _WRONG_SCHEMA) to indicate that.
 */
size_t MySQLParserServicesImpl::parseRoutine(MySQLParserContext::Ref context, db_mysql_RoutineRef routine,
                                             const std::string &sql) {
  logDebug2("Parse routine\n");

  routine->sqlDefinition(base::trim(sql));
  routine->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  MySQLParserContextImpl *impl = dynamic_cast<MySQLParserContextImpl *>(context.get());
  auto tree = impl->parse(sql, MySQLParseUnit::PuCreateRoutine);

  if (impl->errors.empty()) {
    db_mysql_CatalogRef catalog;
    db_mysql_SchemaRef schema;
    if (routine->owner().is_valid()) {
      schema = db_mysql_SchemaRef::cast_from(routine->owner());
      if (schema->owner().is_valid())
        catalog = db_mysql_CatalogRef::cast_from(schema->owner());
    }

    RoutineListener listener(tree, catalog, routine, impl->caseSensitive);

    db_mysql_SchemaRef ownerSchema = db_mysql_SchemaRef::cast_from(routine->owner());
    if (!base::same_string(schema->name(), ownerSchema->name(), false)) { // Routine names are never case sensitive.
      routine->name(*routine->name() + "_WRONG_SCHEMA");
    }
  } else {
    // Finished with errors. See if we can get at least the routine name and type out.
    auto info = getRoutineNameAndType(tree);
    routine->name(info.first + "_SYNTAX_ERROR");
    routine->routineType(info.second);
  }

  return impl->errors.size();
}

//--------------------------------------------------------------------------------------------------

bool considerAsSameType(std::string type1, std::string type2) {
  if (type1 == type2)
    return true;

  if (type1 == "function" && type2 == "udf")
    return true;

  if (type2 == "function" && type1 == "udf")
    return true;

  return false;
}

//----------------------------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseRoutinesSql(parser_ContextReferenceRef context_ref, db_mysql_RoutineGroupRef group,
                                                 const std::string &sql) {
  MySQLParserContext::Ref context = parser_context_from_grt(context_ref);
  return parseRoutines(context, group, sql);
}

//----------------------------------------------------------------------------------------------------------------------

/**
* Parses the given sql as a list of create function/procedure statements.
* In case of an error handling depends on the error position. We try to get most of the routines out
* of the script.
*
* This process has two parts attached:
*   - Update the sql text + properties for any routine that is in the script in the owning schema.
*   - Update the list of routines in the given routine group to what is in the script.
*/
size_t MySQLParserServicesImpl::parseRoutines(MySQLParserContext::Ref context, db_mysql_RoutineGroupRef group,
                                              const std::string &sql) {
  logDebug2("Parse routine group\n");

  MySQLParserContextImpl *impl = dynamic_cast<MySQLParserContextImpl *>(context.get());

  size_t errorCount = 0;

  std::vector<StatementRange> ranges;
  determineStatementRanges(sql.c_str(), sql.size(), ";", ranges, "\n");

  grt::ListRef<db_Routine> routines = group->routines();
  routines.remove_all();

  db_mysql_SchemaRef schema = db_mysql_SchemaRef::cast_from(group->owner());
  db_mysql_CatalogRef catalog = db_mysql_CatalogRef::cast_from(schema->owner());
  grt::ListRef<db_Routine> schema_routines = schema->routines();

  int syntaxErrorCounter = 1;

  for (auto &range : ranges) {
    std::string routineSQL = sql.substr(range.start, range.length);
    auto tree = impl->parse(routineSQL, MySQLParseUnit::PuCreateRoutine);

    errorCount += impl->errors.size();

    // Before filling a routine we need to know if there's already one with that name in the schema.
    // Hence we first extract the name and act based on that.
    auto info = getRoutineNameAndType(tree);

    // If there's no usable info from parsing preserve at least the code and generate a
    // name for the routine using a counter.
    if (info.first.empty() || info.second.empty()) {
      // Create a new routine instance.
      db_mysql_RoutineRef routine = db_mysql_RoutineRef(grt::Initialized);
      routine->createDate(base::fmttime(0, DATETIME_FMT));
      routine->lastChangeDate(routine->createDate());
      routine->owner(schema);
      schema_routines.insert(routine);

      routine->name(*group->name() + "_SYNTAX_ERROR_" + std::to_string(syntaxErrorCounter++));
      routine->routineType("unknown");
      routine->modelOnly(1);
      routine->sqlDefinition(base::trim(routineSQL));

      routines.insert(routine);
    } else {
      db_mysql_RoutineRef routine;
      for (size_t i = 0; i < schema_routines.count(); ++i) {
        // Stored functions and UDFs share the same namespace.
        // Stored routine names are not case sensitive.
        db_RoutineRef candidate = schema_routines[i];
        std::string name = candidate->name();

        // Remove automatically added appendixes before comparing names.
        if (base::hasSuffix(name, "_WRONG_SCHEMA"))
          name.resize(name.size() - 13);
        if (base::hasSuffix(name, "_SYNTAX_ERROR"))
          name.resize(name.size() - 13);

        if (base::same_string(info.first, name, false) && considerAsSameType(info.second, candidate->routineType())) {
          routine = db_mysql_RoutineRef::cast_from(candidate);
          break;
        }
      }

      if (!routine.is_valid()) {
        // Create a new routine instance.
        routine = db_mysql_RoutineRef(grt::Initialized);
        routine->createDate(base::fmttime(0, DATETIME_FMT));
        routine->owner(schema);
        schema_routines.insert(routine);
      }

      if (impl->errors.empty()) {
        RoutineListener listener(tree, catalog, routine, impl->caseSensitive);
        db_mysql_SchemaRef ownerSchema = db_mysql_SchemaRef::cast_from(routine->owner());
      } else {
        routine->name(info.first + "_SYNTAX_ERROR");
        routine->routineType(info.second);

        routine->modelOnly(1);
      }

      routine->sqlDefinition(base::trim(routineSQL));
      routine->lastChangeDate(base::fmttime(0, DATETIME_FMT));

      // Finally add the routine to the group if it isn't already there.
      bool found = false;
      for (size_t i = 0; i < routines.count(); ++i) {
        if (base::same_string(routine->name(), routines[i]->name(), false)) {
          found = true;
          break;
        }
      }
      if (!found)
        routines.insert(routine);
    }
  }

  return errorCount;
}

//----------------------------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseSchema(MySQLParserContext::Ref context, db_mysql_SchemaRef schema,
                                            const std::string &sql) {
  logDebug2("Parse schema\n");

  MySQLParserContextImpl *impl = dynamic_cast<MySQLParserContextImpl *>(context.get());
  auto tree = impl->parse(sql, MySQLParseUnit::PuCreateSchema);

  schema->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  if (impl->errors.empty())
    SchemaListener(tree, db_mysql_CatalogRef::cast_from(schema->owner()), schema, impl->caseSensitive);
  else {
    // Finished with errors. See if we can get at least the schema name out.
    auto context = dynamic_cast<MySQLParser::QueryContext *>(tree);
    auto createDatabaseContext = context->simpleStatement()->createStatement()->createDatabase();
    if (createDatabaseContext != nullptr && createDatabaseContext->schemaName() != nullptr)
      schema->name(createDatabaseContext->schemaName()->getText() + "_SYNTAX_ERROR");
  }

  return impl->errors.size();
}

//----------------------------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseIndex(MySQLParserContext::Ref context, db_mysql_IndexRef index,
                                           const std::string &sql) {
  logDebug2("Parse index\n");

  index->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  MySQLParserContextImpl *impl = dynamic_cast<MySQLParserContextImpl *>(context.get());
  auto tree = impl->parse(sql, MySQLParseUnit::PuCreateIndex);

  if (impl->errors.empty()) {
    db_mysql_CatalogRef catalog;
    db_mysql_SchemaRef schema;

    if (index->owner().is_valid()) {
      schema = db_mysql_SchemaRef::cast_from(index->owner()->owner());
      catalog = db_mysql_CatalogRef::cast_from(schema->owner());
    }

    DbObjectsRefsCache refCache;
    IndexListener(tree, catalog, schema, index, impl->caseSensitive, refCache);
  } else {
    // Finished with errors. See if we can get at least the index name out.
    auto indexContext = dynamic_cast<MySQLParser::CreateIndexContext *>(tree);
    if (indexContext->indexName() != nullptr)
      index->name(base::unquote(indexContext->indexName()->getText()) + "_SYNTAX_ERROR");
  }

  return 1;
}

//----------------------------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseEvent(MySQLParserContext::Ref context, db_mysql_EventRef event,
                                           const std::string &sql) {
  logDebug2("Parse event\n");

  event->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  MySQLParserContextImpl *impl = dynamic_cast<MySQLParserContextImpl *>(context.get());
  auto tree = impl->parse(sql, MySQLParseUnit::PuCreateEvent);

  if (impl->errors.empty()) {
    db_mysql_CatalogRef catalog;
    if (event->owner().is_valid())
      catalog = db_mysql_CatalogRef::cast_from(event->owner()->owner());

    EventListener(tree, catalog, event, impl->caseSensitive);
  } else {
    // Finished with errors. See if we can get at least the event name out.
    auto eventContext = dynamic_cast<MySQLParser::CreateEventContext *>(tree);
    if (eventContext->eventName() != nullptr)
      event->name(base::unquote(eventContext->eventName()->getText()) + "_SYNTAX_ERROR");
  }

  return impl->errors.size();
}

//----------------------------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseLogfileGroup(MySQLParserContext::Ref context, db_mysql_LogFileGroupRef group,
                                                  const std::string &sql) {
  logDebug2("Parse logfile group\n");

  group->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  MySQLParserContextImpl *impl = dynamic_cast<MySQLParserContextImpl *>(context.get());
  auto tree = impl->parse(sql, MySQLParseUnit::PuCreateLogfileGroup);

  if (impl->errors.empty()) {
    db_mysql_CatalogRef catalog;
    if (group->owner().is_valid()) {
      db_mysql_SchemaRef schema = db_mysql_SchemaRef::cast_from(group->owner());
      if (schema->owner().is_valid())
        catalog = db_mysql_CatalogRef::cast_from(schema->owner());
    }

    LogfileGroupListener(tree, catalog, group, impl->caseSensitive);
  } else {
    // Finished with errors. See if we can get at least the group name out.
    auto groupTree = dynamic_cast<MySQLParser::CreateLogfileGroupContext *>(tree);
    if (groupTree->logfileGroupName() != nullptr) {
      IdentifierListener listener(groupTree->logfileGroupName());
      group->name(listener.parts.back() + "_SYNTAX_ERROR");
    }
  }

  return impl->errors.size();
  ;
}

//----------------------------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseServer(MySQLParserContext::Ref context, db_mysql_ServerLinkRef server,
                                            const std::string &sql) {
  logDebug2("Parse server\n");

  server->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  MySQLParserContextImpl *impl = dynamic_cast<MySQLParserContextImpl *>(context.get());
  auto tree = impl->parse(sql, MySQLParseUnit::PuCreateLogfileGroup);

  if (impl->errors.empty()) {
    db_mysql_CatalogRef catalog;
    if (server->owner().is_valid()) {
      db_mysql_SchemaRef schema = db_mysql_SchemaRef::cast_from(server->owner());
      if (schema->owner().is_valid())
        catalog = db_mysql_CatalogRef::cast_from(schema->owner());
    }

    ServerListener(tree, catalog, server, impl->caseSensitive);
  } else {
    auto serverTree = dynamic_cast<MySQLParser::CreateServerContext *>(tree);
    if (serverTree->serverName() != nullptr)
      server->name(base::unquote(serverTree->serverName()->getText()) + "_SYNTAX_ERROR");
  }

  return impl->errors.size();
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseTablespace(MySQLParserContext::Ref context, db_mysql_TablespaceRef tablespace,
                                                const std::string &sql) {
  logDebug2("Parse tablespace\n");

  tablespace->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  MySQLParserContextImpl *impl = dynamic_cast<MySQLParserContextImpl *>(context.get());
  auto tree = impl->parse(sql, MySQLParseUnit::PuCreateTablespace);

  if (impl->errors.empty()) {
    db_mysql_CatalogRef catalog;
    if (tablespace->owner().is_valid() && tablespace->owner()->owner().is_valid())
      catalog = db_mysql_CatalogRef::cast_from(tablespace->owner()->owner()->owner());

    TablespaceListener(tree, catalog, tablespace, impl->caseSensitive);
  } else {
    auto tablespaceTree = dynamic_cast<MySQLParser::CreateTablespaceContext *>(tree);
    if (tablespaceTree->tablespaceName() != nullptr)
      tablespace->name(base::unquote(tablespaceTree->tablespaceName()->getText()) + "_SYNTAX_ERROR");
  }

  return impl->errors.size();
}

//----------------------------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseSQLIntoCatalogSql(parser_ContextReferenceRef context_ref,
                                                       db_mysql_CatalogRef catalog, const std::string &sql,
                                                       grt::DictRef options) {
  MySQLParserContext::Ref context = parser_context_from_grt(context_ref);
  return parseSQLIntoCatalog(context, catalog, sql, options);
}

//----------------------------------------------------------------------------------------------------------------------

/**
*	Expects the sql to be a single or multi-statement text in utf-8 encoding which is parsed and
*	the details are used to build a grt tree. Existing objects are replaced unless the SQL has
*	an "if not exist" clause (or no "or replace" clause for views).
*	Statements handled are: create, drop and table rename, everything else is ignored.
*
*  Note for case sensitivity: only schema, table and trigger names *can* be case sensitive.
*  This is determined by the case_sensitive() function of the given context. All other objects
*  are searched for case-insensitively.
*
*	@result Returns the number of errors found during parsing.
*/
size_t MySQLParserServicesImpl::parseSQLIntoCatalog(MySQLParserContext::Ref context, db_mysql_CatalogRef catalog,
                                                    const std::string &sql, grt::DictRef options) {
  MySQLParserContextImpl *impl = dynamic_cast<MySQLParserContextImpl *>(context.get());

  static std::set<MySQLQueryType> relevantQueryTypes = {
    QtAlterDatabase,
    QtAlterLogFileGroup,
    QtAlterFunction,
    QtAlterProcedure,
    QtAlterServer,
    QtAlterTable,
    QtAlterTableSpace,
    QtAlterEvent,
    QtAlterView,

    QtCreateTable,
    QtCreateIndex,
    QtCreateDatabase,
    QtCreateEvent,
    QtCreateView,
    QtCreateRoutine,
    QtCreateProcedure,
    QtCreateFunction,
    QtCreateUdf,
    QtCreateTrigger,
    QtCreateLogFileGroup,
    QtCreateServer,
    QtCreateTableSpace,

    QtDropDatabase,
    QtDropEvent,
    QtDropFunction,
    QtDropProcedure,
    QtDropIndex,
    QtDropLogfileGroup,
    QtDropServer,
    QtDropTable,
    QtDropTablespace,
    QtDropTrigger,
    QtDropView,

    QtRenameTable,

    QtUse
  };

  logDebug2("Parse sql into catalog\n");

  bool caseSensitive = impl->caseSensitive;

  std::string startSchema = options.get_string("schema");
  db_mysql_SchemaRef currentSchema;
  if (!startSchema.empty())
    currentSchema = ObjectListener::ensureSchemaExists(catalog, startSchema, caseSensitive);

  bool defaultSchemaCreated = false;
  bool autoGenerateFkNames = options.get_int("gen_fk_names_when_empty") != 0;
  // bool reuseExistingObjects = options.get_int("reuse_existing_objects") != 0;

  if (!currentSchema.is_valid()) {
    currentSchema = db_mysql_SchemaRef::cast_from(catalog->defaultSchema());
    if (!currentSchema.is_valid()) {
      db_SchemaRef df = find_named_object_in_list(catalog->schemata(), "default_schema", caseSensitive);
      if (!df.is_valid())
        defaultSchemaCreated = true;
      currentSchema = ObjectListener::ensureSchemaExists(catalog, "default_schema", caseSensitive);
    }
  }

  size_t errorCount = 0;
  std::vector<StatementRange> ranges;
  determineStatementRanges(sql.c_str(), sql.size(), ";", ranges, "\n");

  grt::ListRef<GrtObject> createdObjects = grt::ListRef<GrtObject>::cast_from(options.get("created_objects"));
  if (!createdObjects.is_valid()) {
    createdObjects = grt::ListRef<GrtObject>(grt::Initialized);
    options.set("created_objects", createdObjects);
  }

  StringListRef errors = StringListRef::cast_from(options.get("errors"));

  // Collect textual FK references into a local cache. At the end this is used
  // to find actual ref tables + columns, when all tables have been parsed.
  DbObjectsRefsCache refCache;
  for (auto &range : ranges) {
    std::string query(sql.c_str() + range.start, range.length);
    MySQLQueryType queryType = impl->determineQueryType(query);

    if (relevantQueryTypes.count(queryType) == 0)
      continue; // Something we are not interested in. Don't bother parsing it.

    auto tree = impl->parse(query, MySQLParseUnit::PuGeneric);
    if (!impl->errors.empty()) {
      errorCount += impl->errors.size();
      if (errors.is_valid()) {
        for (auto &error : impl->errors)
          errors.insert("(" + std::to_string(range.line) + ", " + std::to_string(error.offset) + ") "
                        + error.message);
      }
      continue;
    }

    auto statementContext = dynamic_cast<MySQLParser::QueryContext *>(tree)->simpleStatement();
    switch (queryType) {
      case QtCreateDatabase: {
        db_mysql_SchemaRef schema(grt::Initialized);
        schema->createDate(base::fmttime(0, DATETIME_FMT));
        schema->lastChangeDate(schema->createDate());
        schema->owner(catalog);

        SchemaListener listener(statementContext, catalog, schema, impl->caseSensitive);
        schema->oldName(schema->name());

        db_SchemaRef existing = find_named_object_in_list(catalog->schemata(), schema->name(), caseSensitive);
        if (existing.is_valid()) {
          if (!listener.ignoreIfExists) {
            catalog->schemata()->remove(existing);
            createdObjects.remove_value(existing);

            catalog->schemata().insert(schema);
            createdObjects.insert(schema);
          }
        } else {
          catalog->schemata().insert(schema);
          createdObjects.insert(schema);
        }

        break;
      }

      case QtUse: {
        std::string schemaName =
          base::unquote(statementContext->utilityStatement()->useCommand()->identifier()->getText());
        currentSchema = ObjectListener::ensureSchemaExists(catalog, schemaName, caseSensitive);
        break;
      }

      case QtCreateTable: {
        db_mysql_TableRef table(grt::Initialized);
        table->createDate(base::fmttime(0, DATETIME_FMT));
        table->lastChangeDate(table->createDate());
        table->owner(currentSchema);

        TableListener listener(statementContext, catalog, currentSchema, table, caseSensitive, autoGenerateFkNames,
                               refCache);
        table->oldName(table->name());

        db_mysql_SchemaRef schema =
          db_mysql_SchemaRef::cast_from(table->owner()); // Might be different from current schema.

        // Ignore tables that use a name that is already used for a view (no drop/new-add takes place then).
        db_mysql_ViewRef existingView = find_named_object_in_list(schema->views(), table->name());
        if (!existingView.is_valid()) {
          db_TableRef existingTable = find_named_object_in_list(schema->tables(), table->name());
          if (existingTable.is_valid()) {
            // Ignore if the table exists already?
            if (!listener.ignoreIfExists) {
              schema->tables()->remove(existingTable);
              createdObjects.remove_value(existingTable);

              schema->tables().insert(table);
              createdObjects.insert(table);
            }
          } else {
            schema->tables().insert(table);
            createdObjects.insert(table);
          }
        }

        break;
      }

      case QtCreateIndex: {
        db_mysql_IndexRef index(grt::Initialized);
        index->createDate(base::fmttime(0, DATETIME_FMT));
        index->lastChangeDate(index->createDate());

        IndexListener listener(statementContext, catalog, currentSchema, index, impl->caseSensitive, refCache);
        index->oldName(index->name());

        db_TableRef table = db_TableRef::cast_from(index->owner());
        if (table.is_valid()) {
          db_IndexRef existing = find_named_object_in_list(table->indices(), index->name());
          if (existing.is_valid()) {
            table->indices()->remove(existing);
            createdObjects.remove_value(existing);
          }
          table->indices().insert(index);
          createdObjects.insert(index);
        }

        break;
      }

      case QtCreateEvent: {
        db_mysql_EventRef event(grt::Initialized);
        event->sqlDefinition(base::trim(query));
        event->createDate(base::fmttime(0, DATETIME_FMT));
        event->lastChangeDate(event->createDate());
        event->owner(currentSchema);

        EventListener listener(statementContext, catalog, event, impl->caseSensitive);
        event->oldName(event->name());

        db_mysql_SchemaRef schema =
          db_mysql_SchemaRef::cast_from(event->owner()); // Might be different from current schema.
        db_EventRef existing = find_named_object_in_list(schema->events(), event->name());
        if (existing.is_valid()) {
          if (!listener.ignoreIfExists) // Ignore if exists?
          {
            schema->events()->remove(existing);
            createdObjects.remove_value(existing);

            schema->events().insert(event);
            createdObjects.insert(event);
          }
        } else {
          schema->events().insert(event);
          createdObjects.insert(event);
        }

        break;
      }

      case QtCreateView: {
        db_mysql_ViewRef view(grt::Initialized);
        view->sqlDefinition(base::trim(base::trim(query)));
        view->createDate(base::fmttime(0, DATETIME_FMT));
        view->lastChangeDate(view->createDate());
        view->owner(currentSchema);

        ViewListener listener(statementContext, catalog, view, caseSensitive);
        view->oldName(view->name());

        db_mysql_SchemaRef schema =
          db_mysql_SchemaRef::cast_from(view->owner()); // Might be different from current schema.

        // Ignore views that use a name that is already used for a table (no drop/new-add takes place then).
        db_mysql_TableRef existingTable = find_named_object_in_list(schema->tables(), view->name());
        if (!existingTable.is_valid()) {
          db_mysql_ViewRef existingView = find_named_object_in_list(schema->views(), view->name());
          if (existingView.is_valid()) {
            schema->views()->remove(existingView);
            createdObjects.remove_value(existingView);
          }
          schema->views().insert(view);
          createdObjects.insert(view);
        }

        break;
      }

      case QtCreateProcedure:
      case QtCreateFunction:
      case QtCreateUdf: {
        db_mysql_RoutineRef routine(grt::Initialized);
        routine->owner(currentSchema);
        routine->sqlDefinition(base::trim(query));
        routine->createDate(base::fmttime(0, DATETIME_FMT));
        routine->lastChangeDate(routine->createDate());

        RoutineListener listener(statementContext, catalog, routine, caseSensitive);
        routine->oldName(routine->name());

        db_mysql_SchemaRef schema =
          db_mysql_SchemaRef::cast_from(routine->owner()); // Might be different from current schema.

        db_RoutineRef existing = find_named_object_in_list(schema->routines(), routine->name());
        if (existing.is_valid()) {
          schema->routines()->remove(existing);
          createdObjects.remove_value(existing);
        }
        schema->routines().insert(routine);
        createdObjects.insert(routine);

        break;
      }

      case QtCreateTrigger: {
        db_mysql_TriggerRef trigger(grt::Initialized);
        trigger->sqlDefinition(base::trim(query));
        trigger->createDate(base::fmttime(0, DATETIME_FMT));
        trigger->lastChangeDate(trigger->createDate());

        TriggerListener listener(statementContext, catalog, currentSchema, trigger, caseSensitive);
        trigger->oldName(trigger->name());

        // It could be the listener had to create stub table. We have to add this to our created objects list.
        db_mysql_TableRef table = db_mysql_TableRef::cast_from(trigger->owner());
        if (table->isStub())
          createdObjects.insert(table);

        db_TriggerRef existing = find_named_object_in_list(table->triggers(), trigger->name());
        if (existing.is_valid()) {
          table->triggers()->remove(existing);
          createdObjects.remove_value(existing);
        }
        table->triggers().insert(trigger);
        createdObjects.insert(trigger);

        break;
      }

      case QtCreateLogFileGroup: {
        db_mysql_LogFileGroupRef group(grt::Initialized);
        group->createDate(base::fmttime(0, DATETIME_FMT));
        group->lastChangeDate(group->createDate());
        group->owner(catalog);

        LogfileGroupListener listener(statementContext, catalog, group, impl->caseSensitive);
        group->oldName(group->name());

        db_LogFileGroupRef existing = find_named_object_in_list(catalog->logFileGroups(), group->name());
        if (existing.is_valid()) {
          catalog->logFileGroups()->remove(existing);
          createdObjects.remove_value(existing);
        }

        catalog->logFileGroups().insert(group);
        createdObjects.insert(group);

        break;
      }

      case QtCreateServer: {
        db_mysql_ServerLinkRef server(grt::Initialized);
        server->createDate(base::fmttime(0, DATETIME_FMT));
        server->lastChangeDate(server->createDate());
        server->owner(catalog);

        ServerListener listener(statementContext, catalog, server, impl->caseSensitive);
        server->oldName(server->name());

        db_ServerLinkRef existing = find_named_object_in_list(catalog->serverLinks(), server->name());
        if (existing.is_valid()) {
          catalog->serverLinks()->remove(existing);
          createdObjects.remove_value(existing);
        }
        catalog->serverLinks().insert(server);
        createdObjects.insert(server);

        break;
      }

      case QtCreateTableSpace: {
        db_mysql_TablespaceRef tablespace(grt::Initialized);
        tablespace->createDate(base::fmttime(0, DATETIME_FMT));
        tablespace->lastChangeDate(tablespace->createDate());
        tablespace->owner(catalog);

        TablespaceListener listener(statementContext, catalog, tablespace, impl->caseSensitive);
        tablespace->oldName(tablespace->name());

        db_TablespaceRef existing = find_named_object_in_list(catalog->tablespaces(), tablespace->name());
        if (existing.is_valid()) {
          catalog->tablespaces()->remove(existing);
          createdObjects.remove_value(existing);
        }
        catalog->tablespaces().insert(tablespace);
        createdObjects.insert(tablespace);

        break;
      }

      case QtDropDatabase: {
        std::string name = base::unquote(statementContext->dropStatement()->dropDatabase()->schemaRef()->getText());
        db_SchemaRef schema = find_named_object_in_list(catalog->schemata(), name);
        if (schema.is_valid()) {
          catalog->schemata()->remove(schema);
          createdObjects.remove_value(schema);

          if (catalog->defaultSchema() == schema)
            catalog->defaultSchema(db_mysql_SchemaRef());
          if (currentSchema == schema)
            currentSchema = db_mysql_SchemaRef::cast_from(catalog->defaultSchema());
          if (!currentSchema.is_valid())
            currentSchema = ObjectListener::ensureSchemaExists(catalog, "default_schema", caseSensitive);
        }
        break;
      }

      case QtDropEvent: {
        IdentifierListener listener(statementContext->dropStatement()->dropEvent()->eventRef());

        db_SchemaRef schema = currentSchema;
        if (listener.parts.size() > 1 && !listener.parts[0].empty())
          schema = ObjectListener::ensureSchemaExists(catalog, listener.parts[0], caseSensitive);
        db_EventRef event = find_named_object_in_list(schema->events(), listener.parts.back());
        if (event.is_valid()) {
          schema->events()->remove(event);
          createdObjects.remove_value(event);
        }

        break;
      }

      case QtDropProcedure:
      case QtDropFunction: // Including UDFs.
      {
        tree::ParseTree *nameContext;
        if (queryType == QtDropFunction)
          nameContext = statementContext->dropStatement()->dropFunction()->functionRef();
        else
          nameContext = statementContext->dropStatement()->dropProcedure()->procedureRef();
        IdentifierListener listener(nameContext);

        db_SchemaRef schema = currentSchema;
        if (listener.parts.size() > 1 && !listener.parts[0].empty())
          schema = ObjectListener::ensureSchemaExists(catalog, listener.parts[0], caseSensitive);
        db_RoutineRef routine = find_named_object_in_list(schema->routines(), listener.parts.back());
        if (routine.is_valid()) {
          schema->routines()->remove(routine);
          createdObjects.remove_value(routine);
        }

        break;
      }

      case QtDropIndex: {
        std::string name;
        {
          IdentifierListener listener(statementContext->dropStatement()->dropIndex()->indexRef());
          name = listener.parts.back();
        }

        IdentifierListener listener(statementContext->dropStatement()->dropIndex()->tableRef());

        db_SchemaRef schema = currentSchema;
        if (listener.parts.size() > 1 && !listener.parts[0].empty())
          schema = ObjectListener::ensureSchemaExists(catalog, listener.parts[0], caseSensitive);
        db_TableRef table = find_named_object_in_list(schema->tables(), listener.parts.back());
        if (table.is_valid()) {
          db_IndexRef index = find_named_object_in_list(table->indices(), name);
          if (index.is_valid()) {
            table->indices()->remove(index);
            createdObjects.remove_value(index);
          }
        }
        break;
      }

      case QtDropLogfileGroup: {
        IdentifierListener listener(statementContext->dropStatement()->dropLogfileGroup()->logfileGroupRef());

        db_LogFileGroupRef group = find_named_object_in_list(catalog->logFileGroups(), listener.parts.back());
        if (group.is_valid()) {
          catalog->logFileGroups()->remove(group);
          createdObjects.remove_value(group);
        }

        break;
      }

      case QtDropServer: {
        IdentifierListener listener(statementContext->dropStatement()->dropServer()->serverRef());
        db_ServerLinkRef server = find_named_object_in_list(catalog->serverLinks(), listener.parts.back());
        if (server.is_valid()) {
          catalog->serverLinks()->remove(server);
          createdObjects.remove_value(server);
        }

        break;
      }

      case QtDropTable: {
        // We can have a list of tables to drop here.
        for (auto tableRef : statementContext->dropStatement()->dropTable()->tableRefList()->tableRef()) {
          IdentifierListener listener(tableRef);
          db_SchemaRef schema = currentSchema;
          if (listener.parts.size() > 1 && !listener.parts[0].empty())
            schema = ObjectListener::ensureSchemaExists(catalog, listener.parts[0], caseSensitive);

          db_TableRef table = find_named_object_in_list(schema->tables(), listener.parts.back());
          if (table.is_valid()) {
            schema->tables()->remove(table);
            createdObjects.remove_value(table);
          }
        }

        break;
      }

      case QtDropView: {
        // We can have a list of views to drop here.
        for (auto tableRef : statementContext->dropStatement()->dropView()->viewRefList()->viewRef()) {
          IdentifierListener listener(tableRef);
          db_SchemaRef schema = currentSchema;
          if (listener.parts.size() > 1 && !listener.parts[0].empty())
            schema = ObjectListener::ensureSchemaExists(catalog, listener.parts[0], caseSensitive);

          db_ViewRef view = find_named_object_in_list(schema->views(), listener.parts.back());
          if (view.is_valid()) {
            schema->views()->remove(view);
            createdObjects.remove_value(view);
          }
        }

        break;
      }

      case QtDropTablespace: {
        IdentifierListener listener(statementContext->dropStatement()->dropTableSpace()->tablespaceRef());
        db_TablespaceRef tablespace = find_named_object_in_list(catalog->tablespaces(), listener.parts.back());
        if (tablespace.is_valid()) {
          catalog->tablespaces()->remove(tablespace);
          createdObjects.remove_value(tablespace);
        }

        break;
      }

      case QtDropTrigger: {
        IdentifierListener listener(statementContext->dropStatement()->dropTrigger()->triggerRef());

        // Even though triggers are schema level objects they work on specific tables
        // and that's why we store them under the affected tables, not in the schema object.
        // This however makes it more difficult to find the trigger to delete, as we have to
        // iterate over all tables.
        db_SchemaRef schema = currentSchema;
        if (listener.parts.size() > 1 && !listener.parts[0].empty())
          schema = ObjectListener::ensureSchemaExists(catalog, listener.parts[0], caseSensitive);

        for (auto table : schema->tables()) {
          db_TriggerRef trigger = find_named_object_in_list(table->triggers(), listener.parts.back());
          if (trigger.is_valid()) {
            table->triggers()->remove(trigger);
            createdObjects.remove_value(trigger);

            break; // A trigger can only be assigned to a single table, so we can stop here.
          }
        }
        break;
      }

      case QtRenameTable: {
        // Renaming a table is special as you can use it also to rename a view and to move
        // a table from one schema to the other (not for views, though).
        // Due to the way we store triggers we have an easy life wrt. related triggers.
        for (auto renamePair : statementContext->renameTableStatement()->renamePair()) {
          IdentifierListener sourceListener(renamePair->tableRef());

          db_SchemaRef sourceSchema = currentSchema;
          if (sourceListener.parts.size() > 1 && !sourceListener.parts[0].empty())
            sourceSchema = ObjectListener::ensureSchemaExists(catalog, sourceListener.parts[0], caseSensitive);

          IdentifierListener targetListener(renamePair->tableName());
          db_SchemaRef targetSchema = currentSchema;
          if (targetListener.parts.size() > 1 && !targetListener.parts[0].empty())
            targetSchema = ObjectListener::ensureSchemaExists(catalog, targetListener.parts[0], caseSensitive);

          db_ViewRef view = find_named_object_in_list(sourceSchema->views(), sourceListener.parts.back());
          if (view.is_valid()) {
            // Cannot move between schemas.
            if (sourceSchema == targetSchema)
              view->name(targetListener.parts.back());
          } else {
            // Renaming a table.
            db_TableRef table = find_named_object_in_list(sourceSchema->tables(), sourceListener.parts.back());
            if (table.is_valid()) {
              if (sourceSchema != targetSchema) {
                sourceSchema->tables()->remove(table);
                targetSchema->tables().insert(table);
                createdObjects.insert(table);
              }
              table->name(targetListener.parts.back());
            }
          }
        }

        break;
      }

      // Alter commands. At the moment we only support a limited number of cases as we mostly
      // need SQL-to-GRT conversion for create scripts.
      case QtAlterDatabase: {
        IdentifierListener listener(statementContext->alterStatement()->alterDatabase()->schemaRef());

        db_mysql_SchemaRef schema = ObjectListener::ensureSchemaExists(catalog, listener.parts.back(), caseSensitive);
        schema->lastChangeDate(base::fmttime(0, DATETIME_FMT));

        SchemaListener(statementContext, catalog, schema, impl->caseSensitive);
        break;
      }

      case QtAlterLogFileGroup:
        break;

      case QtAlterFunction:
        break;

      case QtAlterProcedure:
        break;

      case QtAlterServer:
        break;

      case QtAlterTable: { // Alter table only for adding/removing indices and for renames.
        IdentifierListener listener(statementContext->alterStatement()->alterTable()->tableRef());

        db_mysql_SchemaRef schema = currentSchema;
        if (listener.parts.size() > 1 && !listener.parts[0].empty())
          schema = ObjectListener::ensureSchemaExists(catalog, listener.parts[0], caseSensitive);

        db_mysql_TableRef table = find_named_object_in_list(schema->tables(), listener.parts.back(), caseSensitive);
        if (table.is_valid())
          TableAlterListener(statementContext, catalog, table, caseSensitive, autoGenerateFkNames, refCache);
        else {
          db_mysql_ViewRef view = find_named_object_in_list(schema->views(), listener.parts.back(), caseSensitive);
          if (view.is_valid())
            TableAlterListener(statementContext, catalog, view, caseSensitive, autoGenerateFkNames, refCache);
        }
        break;
      }

      case QtAlterTableSpace:
        break;

      case QtAlterEvent:
        break;

      case QtAlterView:
        break;

      default:
        continue; // Ignore anything else.
    }
  }

  resolveReferences(catalog, refCache, context->isCaseSensitive());

  // Remove the default_schema we may have created at the start, if it is empty.
  if (defaultSchemaCreated) {
    currentSchema = ObjectListener::ensureSchemaExists(catalog, "default_schema", caseSensitive);
    if (currentSchema->tables().count() == 0 && currentSchema->views().count() == 0 &&
        currentSchema->routines().count() == 0 && currentSchema->synonyms().count() == 0 &&
        currentSchema->sequences().count() == 0 && currentSchema->events().count() == 0)
      catalog->schemata().remove_value(currentSchema);
  }

  return errorCount;
}

//----------------------------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::doSyntaxCheck(parser_ContextReferenceRef context_ref, const std::string &sql,
                                              const std::string &type) {
  MySQLParserContext::Ref context = parser_context_from_grt(context_ref);
  MySQLParseUnit queryType = MySQLParseUnit::PuGeneric;
  if (type == "view")
    queryType = MySQLParseUnit::PuCreateView;
  else if (type == "function")
    queryType = MySQLParseUnit::PuCreateFunction;
  else if (type == "procedure")
    queryType = MySQLParseUnit::PuCreateProcedure;
  else if (type == "udf")
    queryType = MySQLParseUnit::PuCreateUdf;
  else if (type == "routine")
    queryType = MySQLParseUnit::PuCreateRoutine;
  else if (type == "trigger")
    queryType = MySQLParseUnit::PuCreateTrigger;
  else if (type == "event")
    queryType = MySQLParseUnit::PuCreateEvent;

  return checkSqlSyntax(context, sql.c_str(), sql.size(), queryType);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Parses the given text as a specific query type (see parser for supported types).
 * Returns the error count.
 */
size_t MySQLParserServicesImpl::checkSqlSyntax(MySQLParserContext::Ref context, const char *sql, size_t length,
                                               MySQLParseUnit type) {
  MySQLParserContextImpl *impl = dynamic_cast<MySQLParserContextImpl *>(context.get());
  impl->errorCheck({sql, length}, type);

  return impl->errors.size();
}

//----------------------------------------------------------------------------------------------------------------------

class SchemaReferencesListener : public MySQLParserBaseListener {
public:
  std::list<size_t> offsets;
  std::string oldName;
  bool caseSensitive;

  virtual void exitSchemaName(MySQLParser::SchemaNameContext *ctx) override {
    checkIdentifierContext(ctx->identifier());
  }

  virtual void exitSchemaRef(MySQLParser::SchemaRefContext *ctx) override {
    checkIdentifierContext(ctx->identifier());
  }

  virtual void exitTableWild(MySQLParser::TableWildContext *ctx) override {
    checkIdentifierContext(ctx->identifier()[0]);
  }

  virtual void exitQualifiedIdentifier(MySQLParser::QualifiedIdentifierContext *ctx) override {
    // The handling is the same for all top level objects (tables, views, triggers etc.), so we use their commonon
    // rule structure (qualifiedIdentifier), however we have to make sure not to include field references by this
    // as they require an additional level (and also use the qualifiedIdentifier rule).
    ParserRuleContext *parent = dynamic_cast<ParserRuleContext *>(ctx->parent);
    if (parent->getRuleIndex() != MySQLParser::RuleFieldIdentifier &&
        ctx->dotIdentifier() != nullptr) // Only if we really have 2 id parts.
      checkIdentifierContext(ctx->identifier());
  }

  virtual void exitFieldIdentifier(MySQLParser::FieldIdentifierContext *ctx) override {
    // Includes column + index references.
    // There can only be a schema name if we have a full field identifier.
    if (ctx->dotIdentifier() != nullptr && ctx->qualifiedIdentifier() != nullptr)
      checkIdentifierContext(ctx->qualifiedIdentifier()->identifier());
  }

private:
  void checkIdentifierContext(ParserRuleContext *ctx) {
    std::string name = ctx->getText();
    bool quoted = false;
    if (name[0] == '`' || name[0] == '"' || name[0] == '\'') {
      quoted = true;
      name = base::unquote(name);
    }

    if (base::same_string(name, oldName, caseSensitive)) {
      size_t offset = ctx->start->getStartIndex();
      if (quoted)
        ++offset;
      offsets.push_back(offset);
    }
  }
};

//----------------------------------------------------------------------------------------------------------------------

/**
 * Replace all occurrences of the old by the new name according to the offsets list.
 */
void replaceSchemaNames(std::string &sql, const std::list<size_t> &offsets, size_t length, const std::string new_name) {
  bool remove_schema = new_name.empty();
  for (std::list<size_t>::const_reverse_iterator iterator = offsets.rbegin(); iterator != offsets.rend(); ++iterator) {
    std::string::size_type start = *iterator;
    std::string::size_type replace_length = length;
    if (remove_schema) {
      // Make sure we also remove quotes and the dot.
      if (start > 0 && (sql[start - 1] == '`' || sql[start - 1] == '"')) {
        --start;
        ++replace_length;
      }
      ++replace_length;
    }
    sql.replace(start, replace_length, new_name);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void renameInList(grt::ListRef<db_DatabaseDdlObject> list, MySQLParserContext::Ref context, MySQLParseUnit unit,
                  const std::string oldName, const std::string newName) {
  MySQLParserContextImpl *impl = dynamic_cast<MySQLParserContextImpl *>(context.get());
  SchemaReferencesListener listener;
  listener.oldName = oldName;
  listener.caseSensitive = impl->caseSensitive;

  for (size_t i = 0; i < list.count(); ++i) {
    std::string sql = list[i]->sqlDefinition();

    auto tree = impl->parse(sql, unit);
    if (impl->errors.empty()) {
      listener.offsets.clear();
      tree::ParseTreeWalker::DEFAULT.walk(&listener, tree);

      if (!listener.offsets.empty()) {
        replaceSchemaNames(sql, listener.offsets, oldName.size(), newName);
        list[i]->sqlDefinition(sql);
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::doSchemaRefRename(parser_ContextReferenceRef context_ref, db_mysql_CatalogRef catalog,
                                                  const std::string old_name, const std::string new_name) {
  MySQLParserContext::Ref context = parser_context_from_grt(context_ref);
  return renameSchemaReferences(context, catalog, old_name, new_name);
}

//--------------------------------------------------------------------------------------------------

/**
 * Goes through all schemas in the catalog and changes all views, tables and routines to refer to the new name if they
 * currently refer to the old name. We also iterate non-related schemas in order to have some
 * consolidation/sanitizing in effect where wrong schema references were used.
 */
size_t MySQLParserServicesImpl::renameSchemaReferences(MySQLParserContext::Ref context, db_mysql_CatalogRef catalog,
                                                       const std::string oldName, const std::string newName) {
  logDebug("Rename schema references\n");

  ListRef<db_mysql_Schema> schemas = catalog->schemata();
  for (size_t i = 0; i < schemas.count(); ++i) {
    db_mysql_SchemaRef schema = schemas[i];
    renameInList(schema->views(), context, MySQLParseUnit::PuCreateView, oldName, newName);
    renameInList(schema->routines(), context, MySQLParseUnit::PuCreateRoutine, oldName, newName);

    grt::ListRef<db_mysql_Table> tables = schemas[i]->tables();
    for (grt::ListRef<db_mysql_Table>::const_iterator iterator = tables.begin(); iterator != tables.end(); ++iterator)
      renameInList((*iterator)->triggers(), context, MySQLParseUnit::PuCreateTrigger, oldName, newName);
  }

  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

static const unsigned char *skipLeadingWhitespace(const unsigned char *head, const unsigned char *tail) {
  while (head < tail && *head <= ' ')
    head++;
  return head;
}

//----------------------------------------------------------------------------------------------------------------------

static bool isLineBreak(const unsigned char *head, const unsigned char *line_break) {
  if (*line_break == '\0')
    return false;

  while (*head != '\0' && *line_break != '\0' && *head == *line_break) {
    head++;
    line_break++;
  }
  return *line_break == '\0';
}

//----------------------------------------------------------------------------------------------------------------------

grt::BaseListRef MySQLParserServicesImpl::getSqlStatementRanges(const std::string &sql) {

  std::vector<StatementRange> ranges;
  determineStatementRanges(sql.c_str(), sql.size(), ";", ranges);

  grt::BaseListRef list(true);
  for (auto &range : ranges) {
    grt::BaseListRef item(true);
    item.ginsert(grt::IntegerRef(range.start));
    item.ginsert(grt::IntegerRef(range.length));
    list.ginsert(item);
  }

  return list;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * A statement splitter to take a list of sql statements and split them into individual statements,
 * return their position and length in the original string (instead the copied strings).
 */
size_t MySQLParserServicesImpl::determineStatementRanges(const char *sql, size_t length,
  const std::string &initialDelimiter, std::vector<StatementRange> &ranges, const std::string &lineBreak) {

  static const unsigned char keyword[] = "delimiter";

  std::string delimiter = initialDelimiter.empty() ? ";" : initialDelimiter;
  const unsigned char *delimiterHead = reinterpret_cast<const unsigned char *>(delimiter.c_str());

  const unsigned char *start = reinterpret_cast<const unsigned char *>(sql);
  const unsigned char *head = start;
  const unsigned char *tail = head;
  const unsigned char *end = head + length;
  const unsigned char *newLine = reinterpret_cast<const unsigned char *>(lineBreak.c_str());

  size_t currentLine = 0;
  size_t statementStart = 0;
  bool haveContent = false; // Set when anything else but comments were found for the current statement.

  while (tail < end) {
    switch (*tail) {
      case '/': { // Possible multi line comment or hidden (conditional) command.
        if (*(tail + 1) == '*') {
          tail += 2;
          bool isHiddenCommand = (*tail == '!');
          while (true) {
            while (tail < end && *tail != '*') {
              if (isLineBreak(tail, newLine))
                ++currentLine;
              tail++;
            }

            if (tail == end) // Unfinished comment.
              break;
            else {
              if (*++tail == '/') {
                tail++; // Skip the slash too.
                break;
              }
            }
          }

          if (isHiddenCommand)
            haveContent = true;
          if (!haveContent) {
            head = tail; // Skip over the comment.
            statementStart = currentLine;
          }

        } else
          tail++;

        break;
      }

      case '-': { // Possible single line comment.
        const unsigned char *end_char = tail + 2;
        if (*(tail + 1) == '-' && (*end_char == ' ' || *end_char == '\t' || isLineBreak(end_char, newLine))) {
          // Skip everything until the end of the line.
          tail += 2;
          while (tail < end && !isLineBreak(tail, newLine))
            tail++;

          if (!haveContent) {
            head = tail;
            statementStart = currentLine;
          }
        } else
          tail++;

        break;
      }

      case '#': { // MySQL single line comment.
        while (tail < end && !isLineBreak(tail, newLine))
          tail++;

        if (!haveContent) {
          head = tail;
          statementStart = currentLine;
        }

        break;
      }

      case '"':
      case '\'':
      case '`': { // Quoted string/id. Skip this in a local loop.
        haveContent = true;
        unsigned char quote = *tail++;
        while (tail < end && *tail != quote) {
          // Skip any escaped character too.
          if (*tail == '\\')
            tail++;
          tail++;
        }
        if (*tail == quote)
          tail++; // Skip trailing quote char if one was there.

        break;
      }

      case 'd':
      case 'D': {
        haveContent = true;

        // Possible start of the keyword DELIMITER. Must be at the start of the text or a character,
        // which is not part of a regular MySQL identifier (0-9, A-Z, a-z, _, $, \u0080-\uffff).
        unsigned char previous = tail > start ? *(tail - 1) : 0;
        bool is_identifier_char = previous >= 0x80 || (previous >= '0' && previous <= '9') ||
                                  ((previous | 0x20) >= 'a' && (previous | 0x20) <= 'z') || previous == '$' ||
                                  previous == '_';
        if (tail == start || !is_identifier_char) {
          const unsigned char *run = tail + 1;
          const unsigned char *kw = keyword + 1;
          int count = 9;
          while (count-- > 1 && (*run++ | 0x20) == *kw++)
            ;
          if (count == 0 && *run == ' ') {
            // Delimiter keyword found. Get the new delimiter (everything until the end of the line).
            tail = run++;
            while (run < end && !isLineBreak(run, newLine))
              ++run;
            delimiter = base::trim(std::string(reinterpret_cast<const char *>(tail), run - tail));
            delimiterHead = reinterpret_cast<const unsigned char *>(delimiter.c_str());

            // Skip over the delimiter statement and any following line breaks.
            while (isLineBreak(run, newLine)) {
              ++currentLine;
              ++run;
            }
            tail = run;
            head = tail;
            statementStart = currentLine;
          } else
            ++tail;
        } else
          ++tail;

        break;
      }

      default:
        if (isLineBreak(tail, newLine)) {
          ++currentLine;
          if (!haveContent)
            ++statementStart;
        }

        if (*tail > ' ')
          haveContent = true;
        tail++;
        break;
    }

    if (*tail == *delimiterHead) {
      // Found possible start of the delimiter. Check if it really is.
      size_t count = delimiter.size();
      if (count == 1) {
        // Most common case. Trim the statement and check if it is not empty before adding the range.
        head = skipLeadingWhitespace(head, tail);
        if (head < tail)
          ranges.push_back({ statementStart, static_cast<size_t>(head - start), static_cast<size_t>(tail - head) });
        head = ++tail;
        statementStart = currentLine;
        haveContent = false;
      } else {
        const unsigned char *run = tail + 1;
        const unsigned char *del = delimiterHead + 1;
        while (count-- > 1 && (*run++ == *del++))
          ;

        if (count == 0) {
          // Multi char delimiter is complete. Tail still points to the start of the delimiter.
          // Run points to the first character after the delimiter.
          head = skipLeadingWhitespace(head, tail);
          if (head < tail)
            ranges.push_back({ statementStart, static_cast<size_t>(head - start), static_cast<size_t>(tail - head) });
          tail = run;
          head = run;
          statementStart = currentLine;
          haveContent = false;
        }
      }
    }
  }

  // Add remaining text to the range list.
  head = skipLeadingWhitespace(head, tail);
  if (head < tail)
    ranges.push_back({ statementStart, static_cast<size_t>(head - start), static_cast<size_t>(tail - head) });

  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

class GrantListener : public parsers::MySQLParserBaseListener {
public:
  grt::DictRef data = grt::DictRef(true);

  GrantListener(ParseTree *tree) {
    data.set("privileges", _privileges);
    data.set("users", _users);
    data.set("options", _options);

    tree::ParseTreeWalker::DEFAULT.walk(this, tree);
  }

  virtual void exitGrant(MySQLParser::GrantContext *ctx) override {
    if (ctx->ON_SYMBOL() != nullptr && ctx->PROXY_SYMBOL() == nullptr) {
      // Handle specialities for the classic grant command.
      std::string target;
      if (ctx->aclType() != nullptr)
        target = ctx->aclType()->getText() + " ";
      target += MySQLBaseLexer::sourceTextForContext(ctx->grantIdentifier());
      data.gset("target", target);

      if (ctx->ALL_SYMBOL() != nullptr) {
        std::string priv = ctx->ALL_SYMBOL()->getText();
        if (ctx->PRIVILEGES_SYMBOL() != nullptr)
          priv += " " + ctx->PRIVILEGES_SYMBOL()->getText();
        _privileges.insert(priv);
      }
    }
  }

  virtual void exitRoleOrPrivilege(MySQLParser::RoleOrPrivilegeContext *ctx) override {
    _privileges.insert(MySQLBaseLexer::sourceTextForContext(ctx));
  }

  virtual void enterUser(MySQLParser::UserContext *ctx) override {
    // There are 3 places where a user is parsed: as source user in a proxy grant, in the user list of a
    // roles-to-user grant and the grant list of the classic grant command. The handling is different however.
    // In the first case we keep the user as proxy user reference, while for the other 2 the user goes to the user list.
    _currentUser = grt::DictRef(true);
  }

  /**
   * Exclusively for pre-8.0 servers.
   */
  virtual void exitCreateUserEntry(MySQLParser::CreateUserEntryContext *ctx) override {
    if (ctx->BY_SYMBOL() != nullptr) {
      _currentUser.gset("id_method", "PASSWORD");
      _currentUser.gset("id_string", base::unquote(ctx->textString()->getText()));
    }

    if (ctx->WITH_SYMBOL() != nullptr) {
      _currentUser.gset("id_method", base::unquote(ctx->textOrIdentifier()->getText()));
      if (ctx->textString() != nullptr)
        _currentUser.gset("id_string", base::unquote(ctx->textString()->getText()));
    }
  }

  /**
   * For server 8 and newer. There's no password allowed anymore.
   */
  virtual void exitUserList(MySQLParser::UserListContext *ctx) override {
    _currentUser.gset("id_method", "");
    _currentUser.gset("id_string", "");
  }

  virtual void exitUser(MySQLParser::UserContext *ctx) override {
    auto name = fillUserDetails(ctx, _currentUser);

    // Is the parent the grant rule? If so this is the proxy user, otherwise a user in an assignee list.
    auto parent = dynamic_cast<MySQLParser::GrantContext *>(ctx->parent);
    if (parent != nullptr) {
      if (parent->WITH_SYMBOL())
        _options.gset("GRANT", "");
      data.set("proxyUser", _currentUser);
    } else
      _users.set(name, _currentUser);
  }

  std::string fillUserDetails(MySQLParser::UserContext *ctx, grt::DictRef user) {
    std::string name;
    if (ctx->CURRENT_USER_SYMBOL() != nullptr)
      name = ctx->CURRENT_USER_SYMBOL()->getText();
    else {
      auto userIdContext = ctx->userIdentifierOrText();
      name = MySQLBaseLexer::sourceTextForContext(userIdContext->textOrIdentifier()[0]);

      // Host part.
      if (userIdContext->AT_SIGN_SYMBOL() != nullptr)
        user.gset("host", MySQLBaseLexer::sourceTextForContext(userIdContext->textOrIdentifier()[1]));
      else if (userIdContext->AT_TEXT_SUFFIX() != nullptr)
        user.gset("host", base::unquote(userIdContext->AT_TEXT_SUFFIX()->getText().substr(1))); // Omit the @ char.
    }

    user.gset("user", name);
    return name;
  }

  virtual void exitRequireClause(MySQLParser::RequireClauseContext *ctx) override {
    if (ctx->option != nullptr)
      _requirements.gset(base::unquote(ctx->option->getText()), "");
    data.set("requirements", _requirements);
  }

  virtual void exitRequireListElement(MySQLParser::RequireListElementContext *ctx) override {
    _requirements.gset(ctx->element->getText(), base::unquote(ctx->textString()->getText()));
  }

  virtual void exitGrantOption(MySQLParser::GrantOptionContext *ctx) override {
    std::string value;
    if (ctx->ulong_number() != nullptr)
      value = ctx->ulong_number()->getText();
    _options.gset(ctx->option->getText(), value);
  }

private:
  grt::StringListRef _privileges = grt::StringListRef(grt::Initialized);
  grt::DictRef _users = grt::DictRef(true);
  grt::DictRef _currentUser;
  grt::DictRef _requirements = grt::DictRef(true);
  grt::DictRef _options = grt::DictRef(true);
};

//----------------------------------------------------------------------------------------------------------------------

grt::DictRef MySQLParserServicesImpl::parseStatementDetails(parser_ContextReferenceRef context_ref,
                                                            const std::string &sql) {
  MySQLParserContext::Ref context = parser_context_from_grt(context_ref);
  return parseStatement(context, sql);
}

//----------------------------------------------------------------------------------------------------------------------

grt::DictRef MySQLParserServicesImpl::parseStatement(MySQLParserContext::Ref context, const std::string &sql) {
  // This part can potentially grow very large because of the sheer amount of possible query types.
  // So it should be moved into an own file if it grows beyond a few 100 lines.
  MySQLParserContextImpl *impl = dynamic_cast<MySQLParserContextImpl *>(context.get());

  // First do the query determination, then parse, or we invalidate the tokens from the parse run.
  MySQLQueryType queryType = impl->determineQueryType(sql);

  auto tree = impl->parse(sql, MySQLParseUnit::PuGeneric);
  if (!impl->errors.empty()) {
    // Return the error message in case of syntax errors.
    grt::DictRef result(true);
    result.gset("error", impl->errors[0].message);
    return result;
  }

  switch (queryType) {
    case QtGrant:
    case QtGrantProxy: {
      GrantListener listener(tree);
      return listener.data;
    }

    default: {
      grt::DictRef result(true);
      result.gset("error", "Unsupported query type (" + std::to_string(queryType) + ")");
      return result;
    }
  }
}

//--------------------------------------------------------------------------------------------------

static bool doParseType(const std::string &type, GrtVersionRef targetVersion, SimpleDatatypeListRef typeList,
                        db_SimpleDatatypeRef &simpleType, int &precision, int &scale, int &length,
                        std::string &explicitParams) {
  // No char sets necessary for parsing data types as there's no repertoire necessary/allowed in any
  // data type part. Neither do we need an sql mode (string lists in enum defs only allow
  // single quoted text). Hence we don't require to pass in a parsing context but create a local parser.
  //
  // Note: we parse here more than the pure data type name + precision/scale (namely additional parameters
  // like charsets etc.). That doesn't affect the main task here, however. Additionally stuff
  // is simply ignored for now (but it must be a valid definition).
  ANTLRInputStream input(type);
  MySQLLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  MySQLParser parser(&tokens);

  lexer.serverVersion = bec::version_to_int(targetVersion);
  parser.serverVersion = lexer.serverVersion;
  parser.setBuildParseTree(true);

  parser.removeParseListeners();
  parser.removeErrorListeners();

  parser.setErrorHandler(std::make_shared<BailErrorStrategy>());
  parser.getInterpreter<ParserATNSimulator>()->setPredictionMode(PredictionMode::SLL);

  ParseTree *tree;
  try {
    tree = parser.dataTypeDefinition();
  } catch (ParseCancellationException &) {
    tokens.reset();
    parser.reset();
    parser.setErrorHandler(std::make_shared<DefaultErrorStrategy>());
    parser.getInterpreter<ParserATNSimulator>()->setPredictionMode(PredictionMode::LL);
    tree = parser.dataTypeDefinition();
  }

  if (parser.getNumberOfSyntaxErrors() > 0)
    return false;

  auto dataTypeContext = dynamic_cast<MySQLParser::DataTypeDefinitionContext *>(tree);
  grt::StringListRef flags(grt::Initialized); // Unused.
  DataTypeListener typeListener(dataTypeContext->dataType(), targetVersion, typeList, flags, "");

  simpleType = typeListener.dataType;
  scale = (int)typeListener.scale;
  precision = (int)typeListener.precision;
  length = (int)typeListener.length;
  explicitParams = typeListener.explicitParams;

  return simpleType.is_valid();
}

//----------------------------------------------------------------------------------------------------------------------

bool MySQLParserServicesImpl::parseTypeDefinition(const std::string &typeDefinition, GrtVersionRef targetVersion,
                                                  SimpleDatatypeListRef typeList, UserDatatypeListRef userTypes,
                                                  SimpleDatatypeListRef defaultTypeList,
                                                  db_SimpleDatatypeRef &simpleType, db_UserDatatypeRef &userType,
                                                  int &precision, int &scale, int &length,
                                                  std::string &datatypeExplicitParams) {
  if (userTypes.is_valid()) {
    std::string::size_type argp = typeDefinition.find('(');
    std::string typeName = typeDefinition;

    if (argp != std::string::npos)
      typeName = typeDefinition.substr(0, argp);

    // 1st check if this is a user defined type
    for (size_t c = userTypes.count(), i = 0; i < c; i++) {
      db_UserDatatypeRef utype(userTypes[i]);

      if (base::string_compare(utype->name(), typeName, false) == 0) {
        userType = utype;
        break;
      }
    }
  }

  if (userType.is_valid()) {
    // If the type spec has an argument, we replace the arguments from the type definition
    // with the one provided by the user.
    std::string finalType = userType->sqlDefinition();
    std::string::size_type tp;
    bool overridenArgs = false;

    if ((tp = typeDefinition.find('(')) != std::string::npos) // Are there user specified args?
    {
      std::string::size_type p = finalType.find('(');
      if (p != std::string::npos) // Strip the original args.
        finalType = finalType.substr(0, p);

      // Extract the user specified args and append to the specification.
      finalType.append(typeDefinition.substr(tp));

      overridenArgs = true;
    }

    // Parse user type definition.
    if (!doParseType(finalType, targetVersion, typeList.is_valid() ? typeList : defaultTypeList, simpleType, precision,
                     scale, length, datatypeExplicitParams))
      return false;

    simpleType = db_SimpleDatatypeRef();
    if (!overridenArgs) {
      precision = bec::EMPTY_COLUMN_PRECISION;
      scale = bec::EMPTY_COLUMN_SCALE;
      length = bec::EMPTY_COLUMN_LENGTH;
      datatypeExplicitParams = "";
    }
  } else {
    if (!doParseType(typeDefinition, targetVersion, typeList.is_valid() ? typeList : defaultTypeList, simpleType,
                     precision, scale, length, datatypeExplicitParams))
      return false;

    userType = db_UserDatatypeRef();
  }
  return true;
}

//----------------------------------------------------------------------------------------------------------------------

std::vector<std::pair<int, std::string>> MySQLParserServicesImpl::getCodeCompletionCandidates(
  MySQLParserContext::Ref context, std::pair<size_t, size_t> caret, std::string const &sql,
  std::string const &defaultSchema, bool uppercaseKeywords, parsers::SymbolTable &symbolTable) {
  
  MySQLParserContextImpl *impl = dynamic_cast<MySQLParserContextImpl *>(context.get());
  std::vector<std::pair<int, std::string>> candidates =
    impl->getCodeCompletionCandidates(caret, sql, defaultSchema, uppercaseKeywords, symbolTable);

  return candidates;
}

//----------------------------------------------------------------------------------------------------------------------

