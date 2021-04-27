/*
 * Copyright (c) 2015, 2021, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation. The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <deque>

#include "antlr4-runtime.h"
#include <glib.h>

#include "base/common.h"
#include "base/log.h"
#include "base/file_utilities.h"
#include "base/string_utilities.h"
#include "base/threading.h"

#include "parsers-common.h"
#include "mysql/MySQLLexer.h"
#include "mysql/MySQLParser.h"
#include "mysql/MySQLParserBaseListener.h"
#include "CodeCompletionCore.h"

#include "SymbolTable.h"

#include "mysql-code-completion.h"

using namespace parsers;
using namespace antlr4;

DEFAULT_LOG_DOMAIN("MySQL code completion");

//----------------------------------------------------------------------------------------------------------------------

struct TableReference {
  std::string schema;
  std::string table;
  std::string alias;
};

// Context structure for code completion results and token info.
struct AutoCompletionContext {
  CandidatesCollection completionCandidates;

  // A hierarchical view of all table references in the code, updated by visiting all relevant FROM clauses after
  // the candidate collection.
  // Organized as stack to be able to easily remove sets of references when changing nesting level.
  // Implemented as deque however, to allow iterating it.
  std::deque<std::vector<TableReference>> referencesStack;

  // A flat list of possible references for easier lookup.
  std::vector<TableReference> references;

  //--------------------------------------------------------------------------------------------------------------------

  void collectCandidates(MySQLParser *parser, Scanner &scanner, size_t caretOffset, size_t caretLine) {
    CodeCompletionCore c3(parser);

    c3.ignoredTokens = {
      MySQLLexer::EOF,
      MySQLLexer::EQUAL_OPERATOR,
      MySQLLexer::ASSIGN_OPERATOR,
      MySQLLexer::NULL_SAFE_EQUAL_OPERATOR,
      MySQLLexer::GREATER_OR_EQUAL_OPERATOR,
      MySQLLexer::GREATER_THAN_OPERATOR,
      MySQLLexer::LESS_OR_EQUAL_OPERATOR,
      MySQLLexer::LESS_THAN_OPERATOR,
      MySQLLexer::NOT_EQUAL_OPERATOR,
      MySQLLexer::NOT_EQUAL2_OPERATOR,
      MySQLLexer::PLUS_OPERATOR,
      MySQLLexer::MINUS_OPERATOR,
      MySQLLexer::MULT_OPERATOR,
      MySQLLexer::DIV_OPERATOR,
      MySQLLexer::MOD_OPERATOR,
      MySQLLexer::LOGICAL_NOT_OPERATOR,
      MySQLLexer::BITWISE_NOT_OPERATOR,
      MySQLLexer::SHIFT_LEFT_OPERATOR,
      MySQLLexer::SHIFT_RIGHT_OPERATOR,
      MySQLLexer::LOGICAL_AND_OPERATOR,
      MySQLLexer::BITWISE_AND_OPERATOR,
      MySQLLexer::BITWISE_XOR_OPERATOR,
      MySQLLexer::LOGICAL_OR_OPERATOR,
      MySQLLexer::BITWISE_OR_OPERATOR,
      MySQLLexer::DOT_SYMBOL,
      MySQLLexer::COMMA_SYMBOL,
      MySQLLexer::SEMICOLON_SYMBOL,
      MySQLLexer::COLON_SYMBOL,
      MySQLLexer::OPEN_PAR_SYMBOL,
      MySQLLexer::CLOSE_PAR_SYMBOL,
      MySQLLexer::OPEN_CURLY_SYMBOL,
      MySQLLexer::CLOSE_CURLY_SYMBOL,
      MySQLLexer::UNDERLINE_SYMBOL,
      MySQLLexer::AT_SIGN_SYMBOL,
      MySQLLexer::AT_AT_SIGN_SYMBOL,
      MySQLLexer::NULL2_SYMBOL,
      MySQLLexer::PARAM_MARKER,
      MySQLLexer::CONCAT_PIPES_SYMBOL,
      MySQLLexer::AT_TEXT_SUFFIX,
      MySQLLexer::BACK_TICK_QUOTED_ID,
      MySQLLexer::SINGLE_QUOTED_TEXT,
      MySQLLexer::DOUBLE_QUOTED_TEXT,
      MySQLLexer::NCHAR_TEXT,
      MySQLLexer::UNDERSCORE_CHARSET,
      MySQLLexer::IDENTIFIER,
      MySQLLexer::INT_NUMBER,
      MySQLLexer::LONG_NUMBER,
      MySQLLexer::ULONGLONG_NUMBER,
      MySQLLexer::DECIMAL_NUMBER,
      MySQLLexer::BIN_NUMBER,
      MySQLLexer::HEX_NUMBER,
    };

    c3.preferredRules = {
      MySQLParser::RuleSchemaRef,

      MySQLParser::RuleTableRef, MySQLParser::RuleTableRefWithWildcard, MySQLParser::RuleFilterTableRef,

      MySQLParser::RuleColumnRef, MySQLParser::RuleColumnInternalRef, MySQLParser::RuleTableWild,

      MySQLParser::RuleFunctionRef, MySQLParser::RuleFunctionCall, MySQLParser::RuleRuntimeFunctionCall,
      MySQLParser::RuleTriggerRef, MySQLParser::RuleViewRef, MySQLParser::RuleProcedureRef,
      MySQLParser::RuleLogfileGroupRef, MySQLParser::RuleTablespaceRef, MySQLParser::RuleEngineRef,
      MySQLParser::RuleCollationName, MySQLParser::RuleCharsetName, MySQLParser::RuleEventRef,
      MySQLParser::RuleServerRef, MySQLParser::RuleUser,

      MySQLParser::RuleUserVariable, MySQLParser::RuleSystemVariable, MySQLParser::RuleLabelRef,
      MySQLParser::RuleSetSystemVariable,

      // For better handling, but will be ignored.
      MySQLParser::RuleParameterName, MySQLParser::RuleProcedureName, MySQLParser::RuleIdentifier,
      MySQLParser::RuleLabelIdentifier,
    };

    static std::set<size_t> noSeparatorRequiredFor = {
      MySQLLexer::EQUAL_OPERATOR,
      MySQLLexer::ASSIGN_OPERATOR,
      MySQLLexer::NULL_SAFE_EQUAL_OPERATOR,
      MySQLLexer::GREATER_OR_EQUAL_OPERATOR,
      MySQLLexer::GREATER_THAN_OPERATOR,
      MySQLLexer::LESS_OR_EQUAL_OPERATOR,
      MySQLLexer::LESS_THAN_OPERATOR,
      MySQLLexer::NOT_EQUAL_OPERATOR,
      MySQLLexer::NOT_EQUAL2_OPERATOR,
      MySQLLexer::PLUS_OPERATOR,
      MySQLLexer::MINUS_OPERATOR,
      MySQLLexer::MULT_OPERATOR,
      MySQLLexer::DIV_OPERATOR,
      MySQLLexer::MOD_OPERATOR,
      MySQLLexer::LOGICAL_NOT_OPERATOR,
      MySQLLexer::BITWISE_NOT_OPERATOR,
      MySQLLexer::SHIFT_LEFT_OPERATOR,
      MySQLLexer::SHIFT_RIGHT_OPERATOR,
      MySQLLexer::LOGICAL_AND_OPERATOR,
      MySQLLexer::BITWISE_AND_OPERATOR,
      MySQLLexer::BITWISE_XOR_OPERATOR,
      MySQLLexer::LOGICAL_OR_OPERATOR,
      MySQLLexer::BITWISE_OR_OPERATOR,
      MySQLLexer::DOT_SYMBOL,
      MySQLLexer::COMMA_SYMBOL,
      MySQLLexer::SEMICOLON_SYMBOL,
      MySQLLexer::COLON_SYMBOL,
      MySQLLexer::OPEN_PAR_SYMBOL,
      MySQLLexer::CLOSE_PAR_SYMBOL,
      MySQLLexer::OPEN_CURLY_SYMBOL,
      MySQLLexer::CLOSE_CURLY_SYMBOL,
      MySQLLexer::PARAM_MARKER,
    };

    // Certain tokens (like identifiers) must be treated as if the char directly following them still belongs to that
    // token (e.g. a whitespace after a name), because visually the caret is placed between that token and the
    // whitespace creating the impression we are still at the identifier (and we should show candidates for this
    // identifier position).
    // Other tokens (like operators) don't need a separator and hence we can take the caret index as is for them.
    size_t caretIndex = scanner.tokenIndex();
    if (caretIndex > 0 && noSeparatorRequiredFor.count(scanner.lookBack()) == 0)
      --caretIndex;

    c3.showResult = false;
    c3.showDebugOutput = false;
    referencesStack.emplace_front(); // For the root level of table references.

    parser->reset();
    ParserRuleContext *context = parser->query();

    completionCandidates = c3.collectCandidates(caretIndex, context);

    // Post processing some entries.
    if (completionCandidates.tokens.count(MySQLLexer::NOT2_SYMBOL) > 0) {
      // NOT2 is a NOT with special meaning in the operator precedence chain.
      // For code completion it's the same as NOT.
      completionCandidates.tokens[MySQLLexer::NOT_SYMBOL] = completionCandidates.tokens[MySQLLexer::NOT2_SYMBOL];
      completionCandidates.tokens.erase(MySQLLexer::NOT2_SYMBOL);
    }

    // If a column reference is required then we have to continue scanning the query for table references.
    for (auto ruleEntry : completionCandidates.rules) {
      if (ruleEntry.first == MySQLParser::RuleColumnRef) {
        collectLeadingTableReferences(parser, scanner, caretIndex, false);
        takeReferencesSnapshot();
        collectRemainingTableReferences(parser, scanner);
        takeReferencesSnapshot();
        break;
      } else if (ruleEntry.first == MySQLParser::RuleColumnInternalRef) {
        // Note:: rule columnInternalRef is not only used for ALTER TABLE, but atm. we only support that here.
        collectLeadingTableReferences(parser, scanner, caretIndex, true);
        takeReferencesSnapshot();
        break;
      }
    }

    return;
  }

  //--------------------------------------------------------------------------------------------------------------------

private:
  // A listener to handle references as we traverse a parse tree.
  // We have two modes here:
  //   fromClauseMode = true: we are not interested in subqueries and don't need to stop at the caret.
  //   otherwise: go down all subqueries and stop when the caret position is reached.
  class TableRefListener : public parsers::MySQLParserBaseListener {
  public:
    TableRefListener(AutoCompletionContext &context, bool fromClauseMode)
    : _context(context), _fromClauseMode(fromClauseMode) {
    }

    virtual void exitTableRef(MySQLParser::TableRefContext *ctx) override {
      if (_done)
        return;

      if (!_fromClauseMode || _level == 0) {
        TableReference reference;
        if (ctx->qualifiedIdentifier() != nullptr) {
          reference.table = base::unquote(ctx->qualifiedIdentifier()->identifier()->getText());
          if (ctx->qualifiedIdentifier()->dotIdentifier() != nullptr) {
            // Full schema.table reference.
            reference.schema = reference.table;
            reference.table = base::unquote(ctx->qualifiedIdentifier()->dotIdentifier()->identifier()->getText());
          }
        } else {
          // No schema reference.
          reference.table = base::unquote(ctx->dotIdentifier()->identifier()->getText());
        }
        _context.referencesStack.front().push_back(reference);
      }
    }

    virtual void exitTableAlias(MySQLParser::TableAliasContext *ctx) override {
      if (_done)
        return;

      if (_level == 0 && !_context.referencesStack.empty() && !_context.referencesStack.front().empty()) {
        // Appears after a single or derived table.
        // Since derived tables can be very complex it is not possible here to determine possible columns for
        // completion, hence we just walk over them and thus have no field where to store the found alias.
        _context.referencesStack.front().back().alias = base::unquote(ctx->identifier()->getText());
      }
    }

    virtual void enterSubquery(MySQLParser::SubqueryContext *ctx) override {
      if (_done)
        return;

      if (_fromClauseMode)
        ++_level;
      else
        _context.referencesStack.emplace_front();
    }

    virtual void exitSubquery(MySQLParser::SubqueryContext *ctx) override {
      if (_done)
        return;

      if (_fromClauseMode)
        --_level;
      else
        _context.referencesStack.pop_front();
    }

  private:
    bool _done = false; // Only used in full mode.
    size_t _level = 0;  // Only used in FROM clause traversal.
    AutoCompletionContext &_context;
    bool _fromClauseMode;
  };

  //--------------------------------------------------------------------------------------------------------------------

  /**
   * Called if one of the candidates is a column reference, for table references *before* the caret.
   * SQL code must be valid up to the caret, so we can check nesting strictly.
   */
  void collectLeadingTableReferences(MySQLParser *parser, Scanner &scanner, size_t caretIndex, bool forTableAlter) {
    scanner.push();

    if (forTableAlter) {
      MySQLLexer *lexer = dynamic_cast<MySQLLexer *>(parser->getTokenStream()->getTokenSource());

      // For ALTER TABLE commands we do a simple backscan (no nesting is allowed) until we find ALTER TABLE.
      while (scanner.previous() && scanner.tokenType() != MySQLLexer::ALTER_SYMBOL)
        ;
      if (scanner.tokenType() == MySQLLexer::ALTER_SYMBOL) {
        scanner.skipTokenSequence({ MySQLLexer::ALTER_SYMBOL, MySQLLexer::TABLE_SYMBOL });

        TableReference reference;
        reference.table = base::unquote(scanner.tokenText());
        if (scanner.next() && scanner.is(MySQLLexer::DOT_SYMBOL)) {
          reference.schema = reference.table;
          scanner.next();
          scanner.next();
          reference.table = base::unquote(scanner.tokenText());
        }
        referencesStack.front().push_back(reference);
      }
    } else {
      scanner.seek(0);

      size_t level = 0;
      while (true) {
        bool found = scanner.tokenType() == MySQLLexer::FROM_SYMBOL;
        while (!found) {
          if (!scanner.next() || scanner.tokenIndex() >= caretIndex)
            break;

          switch (scanner.tokenType()) {
            case MySQLLexer::OPEN_PAR_SYMBOL:
              ++level;
              referencesStack.emplace_front();

              break;

            case MySQLLexer::CLOSE_PAR_SYMBOL:
              if (level == 0) {
                scanner.pop();
                return; // We cannot go above the initial nesting level.
              }

              --level;
              referencesStack.pop_front();

              break;

            case MySQLLexer::FROM_SYMBOL:
              found = true;
              break;

            default:
              break;
          }
        }

        if (!found) {
          scanner.pop();
          return; // No more FROM clause found.
        }

        parseTableReferences(scanner.tokenSubText(), parser);
        if (scanner.tokenType() == MySQLLexer::FROM_SYMBOL)
          scanner.next();
      }
    }

    scanner.pop();
  }

  //--------------------------------------------------------------------------------------------------------------------

  /**
   * Called if one of the candidates is a column reference, for table references *after* the caret.
   * The function attempts to get table references together with aliases where possible. This is the only place
   * where we actually look beyond the caret and hence different rules apply: the query doesn't need to be valid
   * beyond that point. We simply scan forward until we find a FROM keyword and work from there. This makes it much
   * easier to work on incomplete queries, which nonetheless need e.g. columns from table references.
   * Because inner queries can use table references from outer queries we can simply scan for all outer FROM clauses
   * (skip over subqueries).
   */
  void collectRemainingTableReferences(MySQLParser *parser, Scanner &scanner) {
    scanner.push();

    // Continously scan forward to all FROM clauses on the current or any higher nesting level.
    // With certain syntax errors this can lead to a wrong FROM clause (e.g. if parentheses don't match).
    // But that is acceptable.
    size_t level = 0;
    while (true) {
      bool found = scanner.tokenType() == MySQLLexer::FROM_SYMBOL;
      while (!found) {
        if (!scanner.next())
          break;

        switch (scanner.tokenType()) {
          case MySQLLexer::OPEN_PAR_SYMBOL:
            ++level;
            break;

          case MySQLLexer::CLOSE_PAR_SYMBOL:
            if (level > 0)
              --level;
            break;

          case MySQLLexer::FROM_SYMBOL:
            // Open and close parentheses don't need to match, if we come from within a subquery.
            if (level == 0)
              found = true;
            break;

          default:
            break;
        }
      }

      if (!found) {
        scanner.pop();
        return; // No more FROM clause found.
      }

      parseTableReferences(scanner.tokenSubText(), parser);
      if (scanner.tokenType() == MySQLLexer::FROM_SYMBOL)
        scanner.next();
    }
  }

  //--------------------------------------------------------------------------------------------------------------------

  /**
   * Parses the given FROM clause text using a local parser and collects all found table references.
   */
  void parseTableReferences(std::string const& fromClause, MySQLParser *parserTemplate) {
    // We use a local parser just for the FROM clause to avoid messing up tokens on the autocompletion
    // parser (which would affect the processing of the found candidates).
    ANTLRInputStream input(fromClause);
    MySQLLexer lexer(&input);
    CommonTokenStream tokens(&lexer);
    MySQLParser fromParser(&tokens);

    lexer.serverVersion = parserTemplate->serverVersion;
    lexer.sqlMode = parserTemplate->sqlMode;
    fromParser.serverVersion = parserTemplate->serverVersion;
    fromParser.sqlMode = parserTemplate->sqlMode;
    fromParser.setBuildParseTree(true);

    fromParser.removeErrorListeners();
    tree::ParseTree *tree = fromParser.fromClause();

    TableRefListener listener(*this, true);
    tree::ParseTreeWalker::DEFAULT.walk(&listener, tree);
  }

  //--------------------------------------------------------------------------------------------------------------------

  /**
   * Copies the current references stack into the references map.
   */
  void takeReferencesSnapshot() {
    // Don't clear the references map here. Can happen we have to take multiple snapshots.
    // We automatically remove duplicates by using a map.
    for (auto &entry : referencesStack) {
      for (auto &reference : entry)
        references.push_back(reference);
    }
  }

  //--------------------------------------------------------------------------------------------------------------------

};

//----------------------------------------------------------------------------------------------------------------------

enum ObjectFlags {
  // For 3 part identifiers.
  ShowSchemas = 1 << 0,
  ShowTables = 1 << 1,
  ShowColumns = 1 << 2,

  // For 2 part identifiers.
  ShowFirst = 1 << 3,
  ShowSecond = 1 << 4,
};

//----------------------------------------------------------------------------------------------------------------------

/**
 * Determines the qualifier used for a qualified identifier with up to 2 parts (id or id.id).
 * Returns the found qualifier (if any) and a flag indicating what should be shown.
 *
 * Note: it is essential to understand that we do the determination only up to the caret
 *       (or the token following it, solely for getting a terminator). Since we cannot know the user's
 *       intention, we never look forward.
 */
static ObjectFlags determineQualifier(Scanner &scanner, MySQLLexer *lexer, size_t offsetInLine,
                                      std::string &qualifier) {
  // Five possible positions here:
  //   - In the first id (including the position directly after the last char).
  //   - In the space between first id and a dot.
  //   - On a dot (visually directly before the dot).
  //   - In space after the dot, that includes the position directly after the dot.
  //   - In the second id.
  // All parts are optional (though not at the same time). The on-dot position is considered the same
  // as in first id as it visually belongs to the first id.

  size_t position = scanner.tokenIndex();

  if (scanner.tokenChannel() != 0)
    scanner.next(true); // First skip to the next non-hidden token.

  if (!scanner.is(MySQLLexer::DOT_SYMBOL) && !lexer->isIdentifier(scanner.tokenType())) {
    // We are at the end of an incomplete identifier spec. Jump back, so that the other tests succeed.
    scanner.previous(true);
  }

  // Go left until we find something not related to an id or find at most 1 dot.
  if (position > 0) {
    if (lexer->isIdentifier(scanner.tokenType()) && scanner.lookBack() == MySQLLexer::DOT_SYMBOL)
      scanner.previous(true);
    if (scanner.is(MySQLLexer::DOT_SYMBOL) && lexer->isIdentifier(scanner.lookBack()))
      scanner.previous(true);
  }

  // The scanner is now on the leading identifier or dot (if there's no leading id).
  qualifier = "";
  std::string temp;
  if (lexer->isIdentifier(scanner.tokenType())) {
    temp = base::unquote(scanner.tokenText());
    scanner.next(true);
  }

  // Bail out if there is no more id parts or we are already behind the caret position.
  if (!scanner.is(MySQLLexer::DOT_SYMBOL) || position <= scanner.tokenIndex())
    return ObjectFlags(ShowFirst | ShowSecond);
  qualifier = temp;

  return ShowSecond;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Enhanced variant of the previous function that determines schema and table qualifiers for
 * column references (and table_wild in multi table delete, for that matter).
 * Returns a set of flags that indicate what to show for that identifier, as well as schema and table
 * if given.
 * The returned schema can be either for a schema.table situation (which requires to show tables)
 * or a schema.table.column situation. Which one is determined by whether showing columns alone or not.
 */
static ObjectFlags determineSchemaTableQualifier(Scanner &scanner, MySQLLexer *lexer, std::string &schema,
                                                 std::string &table) {
  size_t position = scanner.tokenIndex();
  if (scanner.tokenChannel() != 0)
    scanner.next(true);

  size_t tokenType = scanner.tokenType();
  if (tokenType != MySQLLexer::DOT_SYMBOL && !lexer->isIdentifier(scanner.tokenType())) {
    // Just like in the simpler function. If we have found no identifier or dot then we are at the
    // end of an incomplete definition. Simply seek back to the previous non-hidden token.
    scanner.previous(true);
  }

  // Go left until we find something not related to an id or at most 2 dots.
  if (position > 0) {
    if (lexer->isIdentifier(scanner.tokenType()) && (scanner.lookBack() == MySQLLexer::DOT_SYMBOL))
      scanner.previous(true);
    if (scanner.is(MySQLLexer::DOT_SYMBOL) && lexer->isIdentifier(scanner.lookBack())) {
      scanner.previous(true);

      // And once more.
      if (scanner.lookBack() == MySQLLexer::DOT_SYMBOL) {
        scanner.previous(true);
        if (lexer->isIdentifier(scanner.lookBack()))
          scanner.previous(true);
      }
    }
  }

  // The scanner is now on the leading identifier or dot (if there's no leading id).
  schema = "";
  table = "";

  std::string temp;
  if (lexer->isIdentifier(scanner.tokenType())) {
    temp = base::unquote(scanner.tokenText());
    scanner.next(true);
  }

  // Bail out if there is no more id parts or we are already behind the caret position.
  if (!scanner.is(MySQLLexer::DOT_SYMBOL) || position <= scanner.tokenIndex())
    return ObjectFlags(ShowSchemas | ShowTables | ShowColumns);

  scanner.next(true); // Skip dot.
  table = temp;
  schema = temp;
  if (lexer->isIdentifier(scanner.tokenType())) {
    temp = base::unquote(scanner.tokenText());
    scanner.next(true);

    if (!scanner.is(MySQLLexer::DOT_SYMBOL) || position <= scanner.tokenIndex())
      return ObjectFlags(ShowTables | ShowColumns); // Schema only valid for tables. Columns must use default schema.

    table = temp;
    return ShowColumns;
  }

  return ObjectFlags(ShowTables | ShowColumns); // Schema only valid for tables. Columns must use default schema.
}

//----------------------------------------------------------------------------------------------------------------------

struct CompareAcEntries {
  bool operator()(const std::pair<int, std::string> &lhs, const std::pair<int, std::string> &rhs) const {
    return base::string_compare(lhs.second, rhs.second, false) < 0;
  }
};

typedef std::set<std::pair<int, std::string>, CompareAcEntries> CompletionSet;

//----------------------------------------------------------------------------------------------------------------------

static void insertSchemas(SymbolTable &symbolTable, CompletionSet &set) {
  auto symbols = symbolTable.getSymbolsOfType<SchemaSymbol>();
  for (auto symbol : symbols)
    set.insert({ AC_SCHEMA_IMAGE, symbol->name });
}

//----------------------------------------------------------------------------------------------------------------------

static void insertTables(SymbolTable &symbolTable, CompletionSet &set, std::set<std::string> &schemas) {

  for (auto &schema : schemas) {
    SchemaSymbol *schemaSymbol = dynamic_cast<SchemaSymbol *>(symbolTable.resolve(schema));
    if (schemaSymbol == nullptr)
      continue;

    auto symbols = schemaSymbol->getSymbolsOfType<TableSymbol>();
    for (auto symbol : symbols)
      set.insert({ AC_TABLE_IMAGE, symbol->name });
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void insertViews(SymbolTable &symbolTable, CompletionSet &set, const std::set<std::string> &schemas) {

  for (auto &schema : schemas) {
    Symbol *symbol = symbolTable.resolve(schema);
    SchemaSymbol *schemaSymbol = dynamic_cast<SchemaSymbol *>(symbol);
    if (schemaSymbol == nullptr)
      continue;

    auto symbols = schemaSymbol->getSymbolsOfType<ViewSymbol>();
    for (auto symbol : symbols)
      set.insert({ AC_VIEW_IMAGE, symbol->name });
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void insertRoutines(SymbolTable &symbolTable, CompletionSet &set, std::string const &schema) {

  SchemaSymbol *schemaSymbol = dynamic_cast<SchemaSymbol *>(symbolTable.resolve(schema));
  if (schemaSymbol != nullptr) {
    auto symbols = schemaSymbol->getSymbolsOfType<RoutineSymbol>();
    for (auto symbol : symbols)
      set.insert({ AC_ROUTINE_IMAGE, symbol->name + "()" });
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void insertColumns(SymbolTable &symbolTable, CompletionSet &set, const std::set<std::string> &schemas,
                          const std::set<std::string> &tables) {

  for (auto &schema : schemas) {
    Symbol *symbol = symbolTable.resolve(schema);
    SchemaSymbol *schemaSymbol = dynamic_cast<SchemaSymbol *>(symbol);
    if (schemaSymbol == nullptr)
      continue;

    for (auto &table : tables) {
      symbol = schemaSymbol->resolve(table);
      TableSymbol *tableSymbol = dynamic_cast<TableSymbol *>(symbol);
      if (tableSymbol == nullptr)
        continue;

      auto symbols = tableSymbol->getSymbolsOfType<ColumnSymbol>();
      for (auto symbol : symbols)
        set.insert({ AC_COLUMN_IMAGE, symbol->name });
    }
  }

}

//----------------------------------------------------------------------------------------------------------------------

std::vector<std::pair<int, std::string>> getCodeCompletionList(size_t caretLine, size_t caretOffset,
  const std::string &defaultSchema, bool uppercaseKeywords, MySQLParser *parser, parsers::SymbolTable &symbolTable) {

  logDebug("Invoking code completion\n");

  AutoCompletionContext context;

  // A set for each object type. This will sort the groups alphabetically and avoids duplicates,
  // but allows to add them as groups to the final list.
  CompletionSet schemaEntries;
  CompletionSet tableEntries;
  CompletionSet columnEntries;
  CompletionSet viewEntries;
  CompletionSet functionEntries;
  CompletionSet udfEntries;
  CompletionSet runtimeFunctionEntries;
  CompletionSet procedureEntries;
  CompletionSet triggerEntries;
  CompletionSet engineEntries;
  CompletionSet logfileGroupEntries;
  CompletionSet tablespaceEntries;
  CompletionSet systemVarEntries;
  CompletionSet keywordEntries;
  CompletionSet collationEntries;
  CompletionSet charsetEntries;
  CompletionSet eventEntries;

  // Handled but needs meat yet.
  CompletionSet userVarEntries;

  // To be done yet.
  CompletionSet userEntries;
  CompletionSet indexEntries;
  CompletionSet pluginEntries;
  CompletionSet fkEntries;
  CompletionSet labelEntries;

  static std::map<size_t, std::vector<std::string>> synonyms = {
    { MySQLLexer::CHAR_SYMBOL, { "CHARACTER" }},
    { MySQLLexer::NOW_SYMBOL, { "CURRENT_TIMESTAMP", "LOCALTIME", "LOCALTIMESTAMP" }},
    { MySQLLexer::DAY_SYMBOL, { "DAYOFMONTH" }},
    { MySQLLexer::DECIMAL_SYMBOL, { "DEC" }},
    { MySQLLexer::DISTINCT_SYMBOL, { "DISTINCTROW" }},
    { MySQLLexer::CHAR_SYMBOL, { "CHARACTER" }},
    { MySQLLexer::COLUMNS_SYMBOL, { "FIELDS" }},
    { MySQLLexer::FLOAT_SYMBOL, { "FLOAT4" }},
    { MySQLLexer::DOUBLE_SYMBOL, { "FLOAT8" }},
    { MySQLLexer::INT_SYMBOL, { "INTEGER", "INT4" }},
    { MySQLLexer::RELAY_THREAD_SYMBOL, { "IO_THREAD" }},
    { MySQLLexer::SUBSTRING_SYMBOL, { "MID" }},
    { MySQLLexer::MID_SYMBOL, { "MEDIUMINT" }},
    { MySQLLexer::MEDIUMINT_SYMBOL, { "MIDDLEINT" }},
    { MySQLLexer::NDBCLUSTER_SYMBOL, { "NDB" }},
    { MySQLLexer::REGEXP_SYMBOL, { "RLIKE" }},
    { MySQLLexer::DATABASE_SYMBOL, { "SCHEMA" }},
    { MySQLLexer::DATABASES_SYMBOL, { "SCHEMAS" }},
    { MySQLLexer::USER_SYMBOL, { "SESSION_USER" }},
    { MySQLLexer::STD_SYMBOL, { "STDDEV", "STDDEV" }},
    { MySQLLexer::SUBSTRING_SYMBOL, { "SUBSTR" }},
    { MySQLLexer::VARCHAR_SYMBOL, { "VARCHARACTER" }},
    { MySQLLexer::VARIANCE_SYMBOL, { "VAR_POP" }},
    { MySQLLexer::TINYINT_SYMBOL, { "INT1" }},
    { MySQLLexer::SMALLINT_SYMBOL, { "INT2" }},
    { MySQLLexer::MEDIUMINT_SYMBOL, { "INT3" }},
    { MySQLLexer::BIGINT_SYMBOL, { "INT8" }},
    { MySQLLexer::SECOND_SYMBOL, { "SQL_TSI_SECOND" }},
    { MySQLLexer::MINUTE_SYMBOL, { "SQL_TSI_MINUTE" }},
    { MySQLLexer::HOUR_SYMBOL, { "SQL_TSI_HOUR" }},
    { MySQLLexer::DAY_SYMBOL, { "SQL_TSI_DAY" }},
    { MySQLLexer::WEEK_SYMBOL, { "SQL_TSI_WEEK" }},
    { MySQLLexer::MONTH_SYMBOL, { "SQL_TSI_MONTH" }},
    { MySQLLexer::QUARTER_SYMBOL, { "SQL_TSI_QUARTER" }},
    { MySQLLexer::YEAR_SYMBOL, { "SQL_TSI_YEAR" }},
  };

  std::vector<std::pair<int, std::string>> result;

  // Also create a separate scanner which allows us to easily navigate the tokens
  // without affecting the token stream used by the parser.
  Scanner scanner(dynamic_cast<BufferedTokenStream *>(parser->getTokenStream()));

  // Move to caret position and store that on the scanner stack.
  scanner.advanceToPosition(caretLine + 1, caretOffset);
  scanner.push();

  context.collectCandidates(parser, scanner, caretOffset, caretLine + 1);

  MySQLQueryType queryType = QtUnknown;
  MySQLLexer *lexer = dynamic_cast<MySQLLexer *>(parser->getTokenStream()->getTokenSource());
  if (lexer != nullptr) {
    lexer->reset(); // Set back the input position to the beginning for query type determination.
    queryType = lexer->determineQueryType();
  }

  dfa::Vocabulary const &vocabulary = parser->getVocabulary();

  for (auto &candidate : context.completionCandidates.tokens) {
    std::string entry = vocabulary.getDisplayName(candidate.first);
    if (entry.rfind("_SYMBOL") != std::string::npos)
      entry.resize(entry.size() - 7);
    else
      entry = base::unquote(entry);

    size_t list = 0; // The list where we place the entry in.
    if (!candidate.second.empty()) {
      // A function call?
      if (candidate.second[0] == MySQLLexer::OPEN_PAR_SYMBOL) {
        list = 1;
      } else {
        for (size_t token : candidate.second) {
          std::string subEntry = vocabulary.getDisplayName(token);
          if (subEntry.rfind("_SYMBOL") != std::string::npos)
            subEntry.resize(subEntry.size() - 7);
          else
            subEntry = base::unquote(subEntry);
          entry += " " + subEntry;
        }
      }
    }

    switch (list) {
      case 1:
        runtimeFunctionEntries.insert({ AC_FUNCTION_IMAGE, base::tolower(entry) + "()" });
        break;

      default:
        if (!uppercaseKeywords)
          entry = base::tolower(entry);

        keywordEntries.insert({ AC_KEYWORD_IMAGE, entry });

        // Add also synonyms, if there are any.
        if (synonyms.count(candidate.first) > 0) {
          for (auto synonym : synonyms[candidate.first]) {
            if (!uppercaseKeywords)
              synonym = base::tolower(synonym);

            keywordEntries.insert({ AC_KEYWORD_IMAGE, synonym });
          }
        }
    }
  }

  symbolTable.lock();
  for (auto &candidate : context.completionCandidates.rules) {
    // Restore the scanner position to the caret position and store that value again for the next round.
    scanner.pop();
    scanner.push();

    switch (candidate.first) {
      case MySQLParser::RuleRuntimeFunctionCall: {
        logDebug3("Adding runtime function names\n");

        auto symbols = symbolTable.getSymbolsOfType<RoutineSymbol>();
        for (auto symbol : symbols)
          runtimeFunctionEntries.insert({ AC_FUNCTION_IMAGE, symbol->name + "()" });
        break;
      }

      case MySQLParser::RuleFunctionRef:
      case MySQLParser::RuleFunctionCall: {
        std::string qualifier;
        ObjectFlags flags = determineQualifier(scanner, lexer, caretOffset, qualifier);

        if (qualifier.empty()) {
          logDebug3("Adding user defined function names from cache\n");

          auto symbols = symbolTable.getSymbolsOfType<UdfSymbol>();
          for (auto symbol : symbols)
            runtimeFunctionEntries.insert({ AC_FUNCTION_IMAGE, symbol->name + "()" });
        }

        logDebug3("Adding function names from cache\n");

        if ((flags & ShowFirst) != 0)
          insertSchemas(symbolTable, schemaEntries);

        if ((flags & ShowSecond) != 0) {
          if (qualifier.empty())
            qualifier = defaultSchema;

          insertRoutines(symbolTable, functionEntries, qualifier);
        }

        break;
      }

      case MySQLParser::RuleEngineRef: {
        logDebug3("Adding engine names\n");

        auto symbols = symbolTable.getSymbolsOfType<EngineSymbol>();
        for (auto &symbol : symbols)
          engineEntries.insert({ AC_ENGINE_IMAGE, symbol->name });

        break;
      }

      case MySQLParser::RuleSchemaRef: {
        logDebug3("Adding schema names from cache\n");

        insertSchemas(symbolTable, schemaEntries);
        break;
      }

      case MySQLParser::RuleProcedureRef: {
        logDebug3("Adding procedure names from cache\n");

        std::string qualifier;
        ObjectFlags flags = determineQualifier(scanner, lexer, caretOffset, qualifier);

        if ((flags & ShowFirst) != 0)
          insertSchemas(symbolTable, schemaEntries);

        if ((flags & ShowSecond) != 0) {
          if (qualifier.empty())
            qualifier = defaultSchema;

          insertRoutines(symbolTable, functionEntries, qualifier);
        }
        break;
      }

      case MySQLParser::RuleTableRefWithWildcard: {
        // A special form of table references (id.id.*) used only in multi-table delete.
        // Handling is similar as for column references (just that we have table/view objects instead of column refs).
        logDebug3("Adding table + view names from cache\n");

        std::string schema, table;
        ObjectFlags flags = determineSchemaTableQualifier(scanner, lexer, schema, table);
        if ((flags & ShowSchemas) != 0)
          insertSchemas(symbolTable, schemaEntries);

        std::set<std::string> schemas;
        schemas.insert(schema.empty() ? defaultSchema : schema);
        if ((flags & ShowTables) != 0) {
          insertTables(symbolTable, tableEntries, schemas);
          insertViews(symbolTable, viewEntries, schemas);
        }
        break;
      }

      case MySQLParser::RuleTableRef:
      case MySQLParser::RuleFilterTableRef: {
        logDebug3("Adding table + view names from cache\n");

        // Tables refs - also allow view refs.
        std::string qualifier;
        ObjectFlags flags = determineQualifier(scanner, lexer, caretOffset, qualifier);

        if ((flags & ShowFirst) != 0)
          insertSchemas(symbolTable, schemaEntries);

        if ((flags & ShowSecond) != 0) {
          std::set<std::string> schemas;
          schemas.insert(qualifier.empty() ? defaultSchema : qualifier);

          insertTables(symbolTable, tableEntries, schemas);
          insertViews(symbolTable, viewEntries, schemas);
        }
        break;
      }

      case MySQLParser::RuleTableWild:
      case MySQLParser::RuleColumnRef: {
        logDebug3("Adding column names from cache\n");

        // Try limiting what to show to the smallest set possible.
        // If we have table references show columns only from them.
        // Show columns from the default schema only if there are no _
        std::string schema, table;
        ObjectFlags flags = determineSchemaTableQualifier(scanner, lexer, schema, table);
        if ((flags & ShowSchemas) != 0)
          insertSchemas(symbolTable, schemaEntries);

        // If a schema is given then list only tables + columns from that schema.
        // If no schema is given but we have table references use the schemas from them.
        // Otherwise use the default schema.
        // TODO: case sensitivity.
        std::set<std::string> schemas;

        if (!schema.empty())
          schemas.insert(schema);
        else if (!context.references.empty()) {
          for (size_t i = 0; i < context.references.size(); ++i) {
            if (!context.references[i].schema.empty())
              schemas.insert(context.references[i].schema);
          }
        }

        if (schemas.empty())
          schemas.insert(defaultSchema);

        if ((flags & ShowTables) != 0) {
          insertTables(symbolTable, tableEntries, schemas);
          if (candidate.first == MySQLParser::RuleColumnRef) {
            // Insert also views.
            insertViews(symbolTable, viewEntries, schemas);

            // Insert also tables from our references list.
            for (auto &reference : context.references) {
              // If no schema was specified then allow also tables without a given schema. Otherwise
              // the reference's schema must match any of the specified schemas (which include those from the ref list).
              if ((schema.empty() && reference.schema.empty()) || (schemas.count(reference.schema) > 0))
                tableEntries.insert({ AC_TABLE_IMAGE, reference.alias.empty() ? reference.table : reference.alias});
            }
          }
        }

        if ((flags & ShowColumns) != 0) {
          if (schema == table) // Schema and table are equal if it's not clear if we see a schema or table qualfier.
            schemas.insert(defaultSchema);

          // For the columns we use a similar approach like for the schemas.
          // If a table is given, list only columns from this (use the set of schemas from above).
          // If not and we have table references then show columns from them.
          // Otherwise show no columns.
          std::set<std::string> tables;
          if (!table.empty()) {
            tables.insert(table);

            // Could be an alias.
            for (size_t i = 0; i < context.references.size(); ++i)
              if (base::same_string(table, context.references[i].alias)) {
                tables.insert(context.references[i].table);
                schemas.insert(context.references[i].schema);
                break;
              }
          } else if (!context.references.empty() && candidate.first == MySQLParser::RuleColumnRef) {
            for (size_t i = 0; i < context.references.size(); ++i)
              tables.insert(context.references[i].table);
          }

          if (!tables.empty())
            insertColumns(symbolTable, columnEntries, schemas, tables);

          // Special deal here: triggers. Show columns for the "new" and "old" qualifiers too.
          // Use the first reference in the list, which is the table to which this trigger belongs (there can be more
          // if the trigger body references other tables).
          if (queryType == QtCreateTrigger && !context.references.empty() &&
              (base::same_string(table, "old") || base::same_string(table, "new"))) {
            tables.clear();
            tables.insert(context.references[0].table);
            insertColumns(symbolTable, columnEntries, schemas, tables);
          }
        }

        break;
      }

      case MySQLParser::RuleColumnInternalRef: {
        logDebug3("Adding internal column names from cache\n");

        std::set<std::string> schemas;
        std::set<std::string> tables;
        if (!context.references.empty()) {
          tables.insert(context.references[0].table);
          if (context.references[0].schema.empty())
            schemas.insert(defaultSchema);
          else
            schemas.insert(context.references[0].schema);
        }
        if (!tables.empty())
          insertColumns(symbolTable, columnEntries, schemas, tables);

        break;
      }

      case MySQLParser::RuleTriggerRef: {
        // While triggers are bound to a table they are schema objects and are referenced as "[schema.]trigger"
        // e.g. in DROP TRIGGER.
        logDebug3("Adding trigger names from cache\n");

        std::string qualifier;
        ObjectFlags flags = determineQualifier(scanner, lexer, caretOffset, qualifier);

        if ((flags & ShowFirst) != 0)
          insertSchemas(symbolTable, schemaEntries);

        if ((flags & ShowSecond) != 0) {
          SchemaSymbol *schemaSymbol = dynamic_cast<SchemaSymbol *>(symbolTable.resolve(qualifier));
          if (schemaSymbol != nullptr) {
            auto symbols = schemaSymbol->getSymbolsOfType<TriggerSymbol>();
            for (auto &symbol : symbols)
              triggerEntries.insert({ AC_TRIGGER_IMAGE, symbol->name });
          }
        }
        break;
      }

      case MySQLParser::RuleViewRef: {
        logDebug3("Adding view names from cache\n");

        // View refs only (no table references), e.g. like in DROP VIEW ...
        std::string qualifier;
        ObjectFlags flags = determineQualifier(scanner, lexer, caretOffset, qualifier);

        if ((flags & ShowFirst) != 0)
          insertSchemas(symbolTable, schemaEntries);

        if ((flags & ShowSecond) != 0) {
          std::set<std::string> schemas;
          schemas.insert(qualifier.empty() ? defaultSchema : qualifier);
          insertViews(symbolTable, viewEntries, schemas);
        }
        break;
      }

      case MySQLParser::RuleLogfileGroupRef: {
        logDebug3("Adding logfile group names from cache\n");

        auto symbols = symbolTable.getSymbolsOfType<LogfileGroupSymbol>();
        for (auto &symbol : symbols)
          logfileGroupEntries.insert({ AC_LOGFILE_GROUP_IMAGE, symbol->name });
        break;
      }

      case MySQLParser::RuleTablespaceRef: {
        logDebug3("Adding tablespace names from cache\n");

        auto symbols = symbolTable.getSymbolsOfType<TableSpaceSymbol>();
        for (auto &symbol : symbols)
          tablespaceEntries.insert({ AC_TABLESPACE_IMAGE, symbol->name });
        break;
      }

      case MySQLParser::RuleUserVariable: {
        logDebug3("Adding user variables\n");

        auto symbols = symbolTable.getSymbolsOfType<UserVariableSymbol>();
        for (auto &symbol : symbols)
          userVarEntries.insert({ AC_USER_VAR_IMAGE, symbol->name });
        break;
      }

      case MySQLParser::RuleLabelRef: {
        logDebug3("Adding label references\n");

        //labelEntries.insert({ AC_USER_VAR_IMAGE, "<block labels>" });
        break;
      }

      case MySQLParser::RuleSystemVariable:
      case MySQLParser::RuleSetSystemVariable: {
        logDebug3("Adding system variables\n");

        auto symbols = symbolTable.getSymbolsOfType<SystemVariableSymbol>();
        for (auto &symbol : symbols)
          systemVarEntries.insert({ AC_SYSTEM_VAR_IMAGE, symbol->name });
        break;
      }

      case MySQLParser::RuleCharsetName: {
        logDebug3("Adding charsets\n");

        auto symbols = symbolTable.getSymbolsOfType<CharsetSymbol>();
        for (auto &symbol : symbols)
          charsetEntries.insert({ AC_CHARSET_IMAGE, symbol->name });
        break;
      }

      case MySQLParser::RuleCollationName: {
        logDebug3("Adding collations\n");

        auto symbols = symbolTable.getSymbolsOfType<CollationSymbol>();
        for (auto &symbol : symbols)
          collationEntries.insert({ AC_COLLATION_IMAGE, symbol->name });
        break;
      }

      case MySQLParser::RuleEventRef: {
        logDebug3("Adding events\n");

        std::string qualifier;
        ObjectFlags flags = determineQualifier(scanner, lexer, caretOffset, qualifier);

        if ((flags & ShowFirst) != 0)
          insertSchemas(symbolTable, schemaEntries);

        if ((flags & ShowSecond) != 0) {
          if (qualifier.empty())
            qualifier = defaultSchema;

          auto symbols = symbolTable.getSymbolsOfType<EventSymbol>();
          for (auto &symbol : symbols)
            eventEntries.insert({ AC_EVENT_IMAGE, symbol->name });
        }
        break;
      }

      case MySQLParser::RuleUser: {
        logDebug3("Adding users\n");

        collationEntries.insert({ AC_USER_IMAGE, "<users>" });
        break;
      }
    }
  }

  symbolTable.unlock();
  scanner.pop(); // Clear the scanner stack.

  // Insert the groups "inside out", that is, most likely ones first + most inner first (columns before tables etc).
  std::copy(keywordEntries.begin(), keywordEntries.end(), std::back_inserter(result));
  std::copy(columnEntries.begin(), columnEntries.end(), std::back_inserter(result));
  std::copy(userVarEntries.begin(), userVarEntries.end(), std::back_inserter(result));
  std::copy(labelEntries.begin(), labelEntries.end(), std::back_inserter(result));
  std::copy(tableEntries.begin(), tableEntries.end(), std::back_inserter(result));
  std::copy(viewEntries.begin(), viewEntries.end(), std::back_inserter(result));
  std::copy(schemaEntries.begin(), schemaEntries.end(), std::back_inserter(result));

  // Everything else is significantly less used.
  // TODO: make this configurable.
  // TODO: show an optimized (small) list of candidates on first invocation, a full list on every following.
  std::copy(functionEntries.begin(), functionEntries.end(), std::back_inserter(result));
  std::copy(procedureEntries.begin(), procedureEntries.end(), std::back_inserter(result));
  std::copy(triggerEntries.begin(), triggerEntries.end(), std::back_inserter(result));
  std::copy(indexEntries.begin(), indexEntries.end(), std::back_inserter(result));
  std::copy(eventEntries.begin(), eventEntries.end(), std::back_inserter(result));
  std::copy(userEntries.begin(), userEntries.end(), std::back_inserter(result));
  std::copy(engineEntries.begin(), engineEntries.end(), std::back_inserter(result));
  std::copy(pluginEntries.begin(), pluginEntries.end(), std::back_inserter(result));
  std::copy(logfileGroupEntries.begin(), logfileGroupEntries.end(), std::back_inserter(result));
  std::copy(tablespaceEntries.begin(), tablespaceEntries.end(), std::back_inserter(result));
  std::copy(charsetEntries.begin(), charsetEntries.end(), std::back_inserter(result));
  std::copy(collationEntries.begin(), collationEntries.end(), std::back_inserter(result));
  std::copy(runtimeFunctionEntries.begin(), runtimeFunctionEntries.end(), std::back_inserter(result));
  std::copy(systemVarEntries.begin(), systemVarEntries.end(), std::back_inserter(result));

  return result;
}

//----------------------------------------------------------------------------------------------------------------------
