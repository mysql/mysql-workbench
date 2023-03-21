/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "MySQLLexer.h"
#include "MySQLParser.h"

#include "SymbolTable.h"

#include "MySQLRecognizerCommon.h"

using namespace base;
using namespace parsers;

using namespace antlr4;
using namespace antlr4::tree;
using namespace antlr4::dfa;

//---------------------------------------------------------------------------------------------------------------------

static void replaceStringInplace(std::string &value, const std::string &search, const std::string &replacement) {
  std::string::size_type next;

  for (next = value.find(search); next != std::string::npos; next = value.find(search, next)) {
    value.replace(next, search.length(), replacement);
    next += replacement.length();
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool MySQLRecognizerCommon::isSqlModeActive(size_t mode) {
  return (sqlMode & mode) != 0;
}

//----------------------------------------------------------------------------------------------------------------------

static void trim(std::string &s) {
  std::locale locale;
  s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), [&locale](char c) { return std::isspace(c, locale); }));
  s.erase(std::find_if_not(s.rbegin(), s.rend(), [&locale](char c) { return std::isspace(c, locale); }).base(),
          s.end());
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLRecognizerCommon::sqlModeFromString(std::string modes) {
  sqlMode = NoMode;

  for (auto &c : modes)
    c = toupper(c);
  std::istringstream iss(modes);
  std::string mode;
  while (std::getline(iss, mode, ',')) {
    trim(mode);
    if (mode == "ANSI" || mode == "DB2" || mode == "MAXDB" || mode == "MSSQL" || mode == "ORACLE" ||
        mode == "POSTGRESQL")
      sqlMode = SqlMode(sqlMode | AnsiQuotes | PipesAsConcat | IgnoreSpace);
    else if (mode == "ANSI_QUOTES")
      sqlMode = SqlMode(sqlMode | AnsiQuotes);
    else if (mode == "PIPES_AS_CONCAT")
      sqlMode = SqlMode(sqlMode | PipesAsConcat);
    else if (mode == "NO_BACKSLASH_ESCAPES")
      sqlMode = SqlMode(sqlMode | NoBackslashEscapes);
    else if (mode == "IGNORE_SPACE")
      sqlMode = SqlMode(sqlMode | IgnoreSpace);
    else if (mode == "HIGH_NOT_PRECEDENCE" || mode == "MYSQL323" || mode == "MYSQL40")
      sqlMode = SqlMode(sqlMode | HighNotPrecedence);
  }
}

//----------------------------------------------------------------------------------------------------------------------

static std::string dumpTree(RuleContext *context, const Vocabulary &vocabulary, const std::string &indentation) {
  std::stringstream stream;

  for (size_t index = 0; index < context->children.size(); ++index) {
    ParseTree *child = context->children[index];
    if (antlrcpp::is<RuleContext *>(child)) {
      auto ruleContext = dynamic_cast<RuleContext *>(child);
      if (antlrcpp::is<MySQLParser::TextLiteralContext *>(child)) {
        misc::Interval interval = ruleContext->getSourceInterval();
        stream << indentation << "(index range: " << interval.a << ".." << interval.b << ", string literal) "
               << MySQLParser::getText(ruleContext, true) << std::endl;
      } else {
        stream << dumpTree(ruleContext, vocabulary, indentation.size() < 100 ? indentation + " " : indentation);
      }
    } else {
      // A terminal node.
      stream << indentation;

      TerminalNode *node = dynamic_cast<TerminalNode *>(child);
      if (antlrcpp::is<tree::ErrorNode *>(child))
        stream << "Syntax Error: ";

      antlr4::Token *token = node->getSymbol();

      std::size_t type = token->getType();
      std::string tokenName = type == Token::EOF ? "<EOF>" : vocabulary.getSymbolicName(token->getType()).data();
      stream << "(line: " << token->getLine() << ", offset: " << token->getCharPositionInLine()
             << ", index: " << token->getTokenIndex() << ", " << tokenName << " [" << token->getType() << "]) "
             << token->getText() << std::endl;
    }
  }

  return stream.str();
}

//----------------------------------------------------------------------------------------------------------------------

std::string MySQLRecognizerCommon::dumpTree(RuleContext *context, const antlr4::dfa::Vocabulary &vocabulary) {
  return ::dumpTree(context, vocabulary, "");
}

//----------------------------------------------------------------------------------------------------------------------

std::string MySQLRecognizerCommon::sourceTextForContext(ParserRuleContext *ctx, bool keepQuotes) {
  return sourceTextForRange(ctx->start, ctx->stop, keepQuotes);
}

//----------------------------------------------------------------------------------------------------------------------

std::string MySQLRecognizerCommon::sourceTextForRange(tree::ParseTree *start, tree::ParseTree *stop, bool keepQuotes) {
  Token *startToken = antlrcpp::is<tree::TerminalNode *>(start) ? dynamic_cast<tree::TerminalNode *>(start)->getSymbol()
                                                                : dynamic_cast<ParserRuleContext *>(start)->start;
  Token *stopToken = antlrcpp::is<tree::TerminalNode *>(stop) ? dynamic_cast<tree::TerminalNode *>(start)->getSymbol()
                                                              : dynamic_cast<ParserRuleContext *>(stop)->stop;
  return sourceTextForRange(startToken, stopToken, keepQuotes);
}

//----------------------------------------------------------------------------------------------------------------------

std::string MySQLRecognizerCommon::sourceTextForRange(Token *start, Token *stop, bool keepQuotes) {
  CharStream *cs = start->getTokenSource()->getInputStream();
  size_t stopIndex = stop != nullptr ? stop->getStopIndex() : std::numeric_limits<size_t>::max();
  std::string result = cs->getText(misc::Interval(start->getStartIndex(), stopIndex));
  if (keepQuotes || result.size() < 2)
    return result;

  char quoteChar = result[0];
  if ((quoteChar == '"' || quoteChar == '`' || quoteChar == '\'') && quoteChar == result.back()) {
    if (quoteChar == '"' || quoteChar == '\'') {
      // Replace any double occurence of the quote char by a single one.
      replaceStringInplace(result, std::string(2, quoteChar), std::string(1, quoteChar));
    }

    return result.substr(1, result.size() - 2);
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the previous sibling of the given tree, which could be a
 * non-terminal. Requires that tree has a valid parent.
 */
ParseTree *getPreviousSibling(ParseTree *tree) {
  ParseTree *parent = tree->parent;
  if (parent == nullptr)
    return nullptr;

  if (parent->children.front() == tree)
    return nullptr;

  for (auto iterator = parent->children.begin(); iterator != parent->children.end(); ++iterator)
    if (*iterator == tree)
      return *(--iterator); // We know we have another node before this,
                            // because of the test above.

  return nullptr; // We actually never arrive here, but compilers want to be silenced.
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the terminal node right before the given position. Parameter tree can
 * be a terminal or non-terminal node.
 * Returns nullptr if there is no such node.
 */
ParseTree *MySQLRecognizerCommon::getPrevious(ParseTree *tree) {
  do {
    ParseTree *sibling = getPreviousSibling(tree);
    if (sibling != nullptr) {
      if (antlrcpp::is<tree::TerminalNode *>(sibling))
        return sibling;

      tree = sibling;
      while (!tree->children.empty())
        tree = tree->children.back();
      if (antlrcpp::is<tree::TerminalNode *>(tree))
        return tree;
    } else
      tree = tree->parent;
  } while (tree != nullptr);

  return nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the next sibling of the given tree, which could be a non-terminal.
 * Requires that tree has a valid parent.
 */
ParseTree *getNextSibling(ParseTree *tree) {
  ParseTree *parent = tree->parent;
  if (parent == nullptr)
    return nullptr;

  if (parent->children.back() == tree)
    return nullptr;

  for (auto iterator = parent->children.begin(); iterator != parent->children.end(); ++iterator)
    if (*iterator == tree)
      return *(++iterator); // We know we have another node after this, because
                            // of the test above.

  return nullptr; // We actually never arrive here, but compilers want to be silenced.
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the terminal node right after the given position. Parameter tree can
 * be a terminal or non-terminal node.
 * Returns nullptr if there is no such node.
 */
ParseTree *MySQLRecognizerCommon::getNext(ParseTree *tree) {
  // If we have children return the first one.
  if (!tree->children.empty()) {
    do {
      tree = tree->children.front();
    } while (!tree->children.empty());
    return tree;
  }

  // No children, so try our next sibling (or that of our parent(s)).
  do {
    ParseTree *sibling = getNextSibling(tree);
    if (sibling != nullptr) {
      if (antlrcpp::is<tree::TerminalNode *>(sibling))
        return sibling;
      return getNext(sibling);
    }
    tree = tree->parent;
  } while (tree != nullptr);

  return nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the terminal node at the given position. If there is no terminal node at the given position (or if it is EOF)
 * then the previous terminal node is returned instead. If there is none then the one after the position is returned
 * instead (which could also be EOF).
 * Note: the line is one-based.
 */
ParseTree* MySQLRecognizerCommon::terminalFromPosition(ParseTree *root, std::pair<size_t, size_t> position) {
  do {
    root = getNext(root);
    if (antlrcpp::is<TerminalNode *>(root)) {
      Token *token = ((TerminalNode *)root)->getSymbol();
      if (token->getType() == Token::EOF)
        return getPrevious(root);

      // If we reached a position after the given one then we found a situation
      // where that position is between two terminals. Return the previous one in this case.
      if (position.second < token->getLine())
        return getPrevious(root);
      if (position.second == token->getLine() && position.first < token->getCharPositionInLine())
        return getPrevious(root);

      size_t length = token->getStopIndex() - token->getStartIndex() + 1;
      if (position.second == token->getLine() && (position.first < token->getCharPositionInLine() + length))
        return root;
    }
  } while (root != nullptr);

  return nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

static bool treeContainsPosition(ParseTree *node, size_t position) {
  auto terminal = dynamic_cast<TerminalNode *>(node);
  if (terminal != nullptr) {
    return terminal->getSymbol()->getStartIndex() <= position && position <= terminal->getSymbol()->getStopIndex();
  }

  auto context = dynamic_cast<ParserRuleContext *>(node);
  if (context == nullptr)
    return false;

  return context->start->getStartIndex() <= position && position <= context->stop->getStopIndex();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the parser context at the given character index position or nullptr if there's none.
 */
ParseTree* MySQLRecognizerCommon::contextFromPosition(ParseTree *root, size_t position) {
  if (!treeContainsPosition(root, position))
    return nullptr;

  for (auto child : root->children) {
    auto result = contextFromPosition(child, position);
    if (result != nullptr)
      return result;
  }

  // No child contains the given position, so it must be in whitespaces between them. Return the root for that case.
  return root;
}

//----------------------------------------------------------------------------------------------------------------------

SymbolTable *parsers::functionSymbolsForVersion(MySQLVersion version) {
  static std::map<MySQLVersion, SymbolTable> functionSymbols;

  if (functionSymbols.count(version) == 0) {
    auto &functions = MySQLSymbolInfo::systemFunctionsForVersion(version);
    SymbolTable &symbolTable = functionSymbols[version]; // Creates the new symbol table.

    for (auto function : functions) {
      symbolTable.addNewSymbol<RoutineSymbol>(nullptr, function, nullptr);
    }
  }
  return &functionSymbols[version];
}

//----------------------------------------------------------------------------------------------------------------------
