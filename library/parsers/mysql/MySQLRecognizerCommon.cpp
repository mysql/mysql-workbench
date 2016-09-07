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
#include "base/string_utilities.h"

using namespace parsers;
using namespace antlr4;

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
  if (keepQuotes)
    return result;
  return base::unquote(result);
}

//----------------------------------------------------------------------------------------------------------------------
