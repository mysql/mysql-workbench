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

#include "TokenStream.h"
#include "ParserRuleContext.h"
#include "MySQLParser.h"

#include "MySQLBaseRecognizer.h"

using namespace antlr4;
using namespace parsers;

MySQLBaseRecognizer::MySQLBaseRecognizer(TokenStream *input) : Parser(input)
{
  removeErrorListeners();
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLBaseRecognizer::reset()
{
  Parser::reset();
}

//----------------------------------------------------------------------------------------------------------------------

std::string MySQLBaseRecognizer::getText(RuleContext *context, bool convertEscapes)
{
  if (antlrcpp::is<MySQLParser::TextLiteralContext *>(context))
  {
    // TODO: take the optional repertoire prefix into account.
    std::string result;
    auto list = ((MySQLParser::TextLiteralContext *)context)->textStringLiteral();

    size_t lastType = Token::INVALID_TYPE;
    size_t lastIndex = INVALID_INDEX;
    for (auto entry : list)
    {
      Token *token = entry->value;
      switch (token->getType())
      {
        case MySQLParser::DOUBLE_QUOTED_TEXT:
        case MySQLParser::SINGLE_QUOTED_TEXT:
        {
          std::string text = token->getText();
          char quoteChar = text[0];
          std::string doubledQuoteChar(2, text[0]);

          // If the previous token was of the same text type and both follow directly each other
          // add a single instance of the quote char (for text like: 'abc''de' -> abc'de).
          if (lastType == token->getType() && lastIndex + 1 == token->getTokenIndex())
            result += quoteChar;
          lastType = token->getType();
          lastIndex = token->getTokenIndex();

          text = text.substr(1, text.size() - 2); // Remove outer quotes.
          size_t position = 0;
          while (true)
          {
            position = text.find(doubledQuoteChar, position);
            if (position == std::string::npos)
              break;
            text.replace(position, 2, &quoteChar);
          }
          result += text;
          break;
        }
      }
    }

    if (convertEscapes)
    {
      std::string temp = result;
      result = "";
      result.reserve(temp.size());

      bool pendingEscape = false;
      for (auto c: temp)
      {
        if (pendingEscape)
        {
          pendingEscape = false;
          switch (c)
          {
            case 'n': c = '\n'; break;
            case 't': c = '\t'; break;
            case 'r': c = '\r'; break;
            case 'b': c = '\b'; break;
            case '0': c = 0; break;         // ASCII null
            case 'Z': c = '\032'; break;    // Win32 end of file
          }
        }
        else
          if (c == '\\')
          {
            pendingEscape = true;
            continue;
          }
        result.push_back(c);
      }
      
      if (pendingEscape)
        result.push_back('\\');
    }

    return result;
  }

  return context->getText();
}

bool MySQLBaseRecognizer::look(ssize_t position, size_t expected)
{
  return _input->LA(position) == expected;
}

bool MySQLBaseRecognizer::containsLinebreak(const std::string &text) const
{
  return text.find_first_of("\r\n") != std::string::npos;
}
