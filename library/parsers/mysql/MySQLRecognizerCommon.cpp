/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "MySQLRecognizerCommon.h"

using namespace parsers;
using namespace antlr4;
using namespace antlr4::tree;

//----------------------------------------------------------------------------------------------------------------------

bool MySQLRecognizerCommon::isSqlModeActive(size_t mode)
{
  return (sqlMode & mode) != 0;
}

//----------------------------------------------------------------------------------------------------------------------

static void trim(std::string &s) {
  s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), [] (char c) { return std::isspace(c); }));
  s.erase(std::find_if_not(s.rbegin(), s.rend(), [] (char c) { return std::isspace(c); }).base(), s.end());
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLRecognizerCommon::sqlModeFromString(std::string modes)
{
  sqlMode = NoMode;

  for (auto & c: modes) c = toupper(c);
  std::istringstream iss(modes);
  std::string mode;
  while (std::getline(iss, mode, ','))
  {
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

std::string MySQLRecognizerCommon::dumpTree(Ref<RuleContext> context, Parser &parser, const std::string &indentation)
{
  std::stringstream stream;

  const std::vector<std::string>& tokenNames = parser.getTokenNames();

  for (size_t index = 0; index < context->children.size(); ++index)
  {
    Ref<tree::Tree> child = context->children[index];
    if (antlrcpp::is<RuleContext>(child))
      stream << dumpTree(std::dynamic_pointer_cast<RuleContext>(child), parser, indentation + "\t");
    else {
      // A terminal node.
      misc::Interval interval = context->getSourceInterval();
      Ref<tree::TerminalNode> node = std::dynamic_pointer_cast<tree::TerminalNode>(child);
      antlr4::Token *token = node->getSymbol();

      stream << "(line: " << token->getLine() << ", offset: " << token->getCharPositionInLine()
      << ", length: " << token->getStopIndex() - token->getStopIndex() + 1 << ", index: " << token->getTokenIndex()
      << ", " << tokenNames[token->getType()] << " [" << token->getType() << "]) " << token->getText() << std::endl;
    }
  }

  return stream.str();
}

//----------------------------------------------------------------------------------------------------------------------

std::string MySQLRecognizerCommon::sourceTextForContext(ParserRuleContext *ctx, bool keepQuotes)
{
  CharStream *cs = ctx->start->getTokenSource()->getInputStream();
  std::string result = cs->getText(misc::Interval(ctx->start->getStartIndex(), ctx->stop->getStopIndex()));
  if (keepQuotes || result.size() < 2)
    return result;

  char quoteChar = result[0];
  if ((quoteChar == '"' || quoteChar == '`' || quoteChar == '\'') && quoteChar == result.back())
    return result.substr(1, result.size() - 2);
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the previous sibling of the given tree, which could be a non-terminal. Requires that tree has a valid parent.
 */
Tree* getPreviousSibling(Tree *tree)
{
  Tree *parent = tree->parent.lock().get();
  if (parent == nullptr)
    return nullptr;

  if (parent->children.front().get() == tree)
    return nullptr;

  for (auto iterator = parent->children.begin(); iterator != parent->children.end(); ++iterator)
    if (iterator->get() == tree)
      return (--iterator)->get(); // We know we have another node before this, because of the test above.

  return nullptr; // We actually never arrive here, but compilers want to be silenced.
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the terminal node right before the given position. Parameter tree can be a terminal or non-terminal node.
 * Returns nullptr if there is no such node.
 */
Tree* MySQLRecognizerCommon::getPrevious(Tree *tree)
{
  do {
    Tree *sibling = getPreviousSibling(tree);
    if (sibling != nullptr)
    {
      if (antlrcpp::is<tree::TerminalNode *>(sibling))
        return sibling;

      tree = sibling;
      while (!tree->children.empty())
        tree = tree->children.back().get();
      if (antlrcpp::is<tree::TerminalNode *>(tree))
        return tree;
    }
    else
      tree = tree->parent.lock().get();
  } while (tree != nullptr);

  return nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the next sibling of the given tree, which could be a non-terminal. Requires that tree has a valid parent.
 */
Tree* getNextSibling(Tree *tree)
{
  Tree *parent = tree->parent.lock().get();
  if (parent == nullptr)
    return nullptr;

  if (parent->children.back().get() == tree)
    return nullptr;

  for (auto iterator = parent->children.begin(); iterator != parent->children.end(); ++iterator)
    if (iterator->get() == tree)
      return (++iterator)->get(); // We know we have another node after this, because of the test above.

  return nullptr; // We actually never arrive here, but compilers want to be silenced.
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the terminal node right after the given position. Parameter tree can be a terminal or non-terminal node.
 * Returns nullptr if there is no such node.
 */
Tree* MySQLRecognizerCommon::getNext(Tree *tree)
{
  // If we have children return the first one.
  if (!tree->children.empty())
  {
    do { tree = tree->children.front().get(); } while (!tree->children.empty());
    return tree;
  }

  // No children, so try our next sibling (or that of our parent(s)).
  do {
    Tree *sibling = getNextSibling(tree);
    if (sibling != nullptr)
    {
      if (antlrcpp::is<tree::TerminalNode *>(sibling))
        return sibling;
      return getNext(sibling);
    }
    tree = tree->parent.lock().get();
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
Tree* MySQLRecognizerCommon::contextFromPosition(Tree *root, std::pair<size_t, size_t> position)
{
  do {
    root = getNext(root);
    if (antlrcpp::is<TerminalNode *>(root))
    {
      Token *token = ((TerminalNode *)root)->getSymbol();
      if (token->getType() == Token::EOF)
        return getPrevious(root);

      // If we reached a position after the given one then we found a situation where that position is
      // between two terminals. Return the previous one in this case.
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
