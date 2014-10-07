/* 
 * Copyright (c) 2013, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include <string>
#include <set>

#include <antlr3.h>

#include "base/log.h"
#include "base/string_utilities.h"

#include "MySQLLexer.h"

#include "mysql-scanner.h"

DEFAULT_LOG_DOMAIN("MySQL parsing")

extern "C" {
  
  /**
   * Error report function placeholder.
   */
  void lexer_error(struct ANTLR3_BASE_lexer_struct *lexer, pANTLR3_UINT8 *tokenNames)
  {
  }

} // extern "C"

//----------------- MySQLScanner -------------------------------------------------------------------

class MySQLScanner::Private
{
public:
  const char *_text;
  size_t _text_length;
  int _input_encoding;
  RecognitionContext _context;

  pANTLR3_INPUT_STREAM _input;
  pMySQLLexer _lexer;
  pANTLR3_TOKEN_SOURCE _token_source;

  // In order to support arbitrary backracking we cache all tokens from the input stream here.
  size_t _token_index;
  std::vector<pANTLR3_COMMON_TOKEN> _tokens;
};

MySQLScanner::MySQLScanner(const char *text, size_t length, bool is_utf8, long server_version,
  const std::string &sql_mode_string, const std::set<std::string> &charsets)
  : MySQLRecognitionBase(charsets)
{
  d = new Private();

  d->_text = text;
  d->_text_length = length;
  d->_context.version = server_version;
  d->_context.payload = this;
  set_sql_mode(sql_mode_string);

  // If the text is not using utf-8 (which it should) then we interpret as 8bit encoding
  // (everything requiring only one byte per char as Latin1, ASCII and similar).
  d->_input_encoding = is_utf8 ? ANTLR3_ENC_UTF8 : ANTLR3_ENC_8BIT;
  setup();

  // Cache the tokens. There's always at least one token: the EOF token.
  // It might seem counter productive to load all tokens upfront, but this makes many
  // things a lot simpler or even possible. The token stream used by a parser does exactly the same.
  d->_token_index = 0;
  while (true)
  {
    pANTLR3_COMMON_TOKEN token = d->_token_source->nextToken(d->_token_source);
    d->_tokens.push_back(token);
    if (token->type == ANTLR3_TOKEN_EOF)
      break;
  }
}

//--------------------------------------------------------------------------------------------------

MySQLScanner::~MySQLScanner()
{
  d->_lexer->free(d->_lexer);
  d->_input->close(d->_input);

  delete d;
}

//--------------------------------------------------------------------------------------------------

void MySQLScanner::reset()
{
  d->_token_index = 0;
}

//--------------------------------------------------------------------------------------------------

/**
 * All token information in one call.
 */
MySQLToken MySQLScanner::token()
{
  pANTLR3_COMMON_TOKEN token = d->_tokens[d->_token_index];

  MySQLToken result;
  if (token != NULL)
  {
    result.type = token->type;
    result.line = token->line;
    result.position = token->charPosition;
    result.index = token->index;
    result.channel = token->channel;
    result.line_start = (char*)token->lineStart;
    result.start = reinterpret_cast<char*>(token->start);
    result.stop = reinterpret_cast<char*>(token->stop);

    pANTLR3_STRING text = token->getText(token);
    result.text = (const char*)text->chars;
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

uint32_t MySQLScanner::token_type()
{
  pANTLR3_COMMON_TOKEN token = d->_tokens[d->_token_index];
  return token->type;
}

//--------------------------------------------------------------------------------------------------

uint32_t MySQLScanner::token_line()
{
  pANTLR3_COMMON_TOKEN token = d->_tokens[d->_token_index];
  return token->line;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the index directly following the last character of the token in the input string.
 */
size_t MySQLScanner::token_end()
{
  pANTLR3_COMMON_TOKEN token = d->_tokens[d->_token_index];
  return token->charPosition + (token->stop - token->start) + 1;
}

//--------------------------------------------------------------------------------------------------

size_t MySQLScanner::token_channel()
{
  pANTLR3_COMMON_TOKEN token = d->_tokens[d->_token_index];
  return token->channel;
}

//--------------------------------------------------------------------------------------------------

std::string MySQLScanner::token_text()
{
  pANTLR3_COMMON_TOKEN token = d->_tokens[d->_token_index];
  pANTLR3_STRING text = token->getText(token);
  return (const char*)text->chars;
}

//--------------------------------------------------------------------------------------------------

void MySQLScanner::next(bool skip_hidden)
{
  pANTLR3_COMMON_TOKEN token;
  while (d->_token_index < d->_tokens.size() - 1)
  {
    ++d->_token_index;
    if (d->_tokens[d->_token_index]->channel == 0 || !skip_hidden)
      break;
  }
}

//--------------------------------------------------------------------------------------------------

void MySQLScanner::previous(bool skip_hidden)
{
  pANTLR3_COMMON_TOKEN token;
  while (d->_token_index > 0)
  {
    --d->_token_index;
    if (d->_tokens[d->_token_index]->channel == 0 || !skip_hidden)
      break;
  }

  // May set the index to a node on another than the default channel, if the very first token is hidden.
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the position in the token vector.
 */
size_t MySQLScanner::position()
{
  return d->_token_index;
}

//--------------------------------------------------------------------------------------------------

/**
 * Sets the position in the token vector to the given index.
 */
void MySQLScanner::seek(size_t position)
{
  d->_token_index = position;
  if (position >= d->_tokens.size())
    d->_token_index = d->_tokens.size() - 1;
}

//--------------------------------------------------------------------------------------------------

/**
 * Scans the token list from the beginning and moves to the token that covers the given 
 * caret position. Line is one-based and offset is the zero-based offset in that line.
 */
void MySQLScanner::seek(size_t line, size_t offset)
{
  // When scanning keep in mind a token can span more than one line (think of multi-line comments).
  // A simple pos + length computation doesn't cut it.
  d->_token_index = 0;
  if ( d->_tokens[d->_token_index]->type == ANTLR3_TOKEN_EOF)
    return;

  while (true)
  {
    // When we reach the input end check if the current token can alone separate from the next token.
    // Some tokens require a whitespace for separation, some do not.
    pANTLR3_COMMON_TOKEN token = d->_tokens[d->_token_index + 1];
    if (token->type == ANTLR3_TOKEN_EOF)
    {
      // At the end of the input. We have no more tokens to make the lookahead work (nothing after EOF).
      // Instead we define: if the last good token is a separator and the caret is at its end
      // we consider the EOF as the current token (a separator is always only 1 char long).
      if (is_separator() &&  (size_t)d->_tokens[d->_token_index]->charPosition < offset)
        ++d->_token_index;
      break;
    }

    if (token->line > line)
      break;

    if (token->line == line && (size_t)token->charPosition > offset)
      break;

    ++d->_token_index;
  };
}

//--------------------------------------------------------------------------------------------------

/**
 * Looks forward (offset > 0) or backward (offset < 0) and returns the token at the given position.
 */
uint32_t MySQLScanner::look_around(int offset, bool ignore_hidden)
{
  if (offset == 0)
    return d->_tokens[d->_token_index]->type;

  if (d->_token_index + offset < 0 || d->_token_index + offset >= d->_tokens.size())
    return ANTLR3_TOKEN_INVALID;

  pANTLR3_COMMON_TOKEN token;
  int index = (int)d->_token_index;
  if (offset < 0)
  {
    while (index > 0 && offset < 0)
    {
      ++offset;
      if (ignore_hidden)
      {
        while (--index >= 0 && d->_tokens[index]->channel != 0)
          ;
      }
      else
        --index;
    }

    if (offset < 0) // Not enough non-hidden entries.
      return ANTLR3_TOKEN_INVALID;
    return d->_tokens[index]->type;
  }
  else
  {
    while (index > 0 && offset > 0)
    {
      --offset;
      if (ignore_hidden)
      {
        while (++index >= 0 && d->_tokens[index]->channel != 0)
          ;
      }
      else
        ++index;
    }

    if (offset > 0) // Not enough non-hidden entries.
      return ANTLR3_TOKEN_INVALID;
    return d->_tokens[index]->type;
  }
}

//--------------------------------------------------------------------------------------------------

bool MySQLScanner::is_keyword()
{
  return MySQLRecognitionBase::is_keyword(d->_tokens[d->_token_index]->type);
}

//--------------------------------------------------------------------------------------------------

bool MySQLScanner::is_relation()
{
  return MySQLRecognitionBase::is_relation(d->_tokens[d->_token_index]->type);
}

//--------------------------------------------------------------------------------------------------

bool MySQLScanner::is_number()
{
  return MySQLRecognitionBase::is_number(d->_tokens[d->_token_index]->type);
}

//--------------------------------------------------------------------------------------------------

bool MySQLScanner::is_operator()
{
  return MySQLRecognitionBase::is_operator(d->_tokens[d->_token_index]->type);
}

//--------------------------------------------------------------------------------------------------

bool MySQLScanner::is_identifier()
{
  return MySQLRecognitionBase::is_identifier(d->_tokens[d->_token_index]->type);
}

//--------------------------------------------------------------------------------------------------

/**
 * Determines if the current token is alone a separator, that is, a character that can separate
 * tokens without whitespaces. Typical tokens are operators (comma, semicolon, parentheses etc.).
 */
bool MySQLScanner::is_separator()
{
  uint32_t type = d->_tokens[d->_token_index]->type;
  switch (type)
  {
    case EQUAL_OPERATOR:
    case ASSIGN_OPERATOR:
    case NULL_SAFE_EQUAL_OPERATOR:
    case GREATER_OR_EQUAL_OPERATOR:
    case GREATER_THAN_OPERATOR:
    case LESS_OR_EQUAL_OPERATOR:
    case LESS_THAN_OPERATOR:
    case NOT_EQUAL_OPERATOR:
    case NOT_EQUAL2_OPERATOR:
    case PLUS_OPERATOR:
    case MINUS_OPERATOR:
    case MULT_OPERATOR:
    case DIV_OPERATOR:
    case MOD_OPERATOR:
    case LOGICAL_NOT_OPERATOR:
    case BITWISE_NOT_OPERATOR:
    case SHIFT_LEFT_OPERATOR:
    case SHIFT_RIGHT_OPERATOR:
    case LOGICAL_AND_OPERATOR:
    case BITWISE_AND_OPERATOR:
    case BITWISE_XOR_OPERATOR:
    case LOGICAL_OR_OPERATOR:
    case BITWISE_OR_OPERATOR:
    case DOT_SYMBOL:
    case COMMA_SYMBOL:
    case SEMICOLON_SYMBOL:
    case COLON_SYMBOL:
    case OPEN_PAR_SYMBOL:
    case CLOSE_PAR_SYMBOL:
    case OPEN_CURLY_SYMBOL:
    case CLOSE_CURLY_SYMBOL:
    case OPEN_BRACKET_SYMBOL:
    case CLOSE_BRACKET_SYMBOL:
    case PARAM_MARKER:
      return true;
  }

  return false;
}

//--------------------------------------------------------------------------------------------------

void MySQLScanner::setup()
{
  log_debug2("Lexer setup\n");
  
  d->_input = antlr3StringStreamNew((pANTLR3_UINT8)d->_text, d->_input_encoding, (ANTLR3_UINT32)d->_text_length,
    (pANTLR3_UINT8)"mysql-script");
  d->_input->setUcaseLA(d->_input, ANTLR3_TRUE); // Make input case-insensitive. String literals must all be upper case in the grammar!
  
  d->_lexer = MySQLLexerNew(d->_input);
  d->_lexer->pLexer->rec->state->userp = &d->_context;
  d->_token_source = TOKENSOURCE(d->_lexer);
  
  log_debug2("Lexer setup ended\n");
}

//--------------------------------------------------------------------------------------------------

const char* MySQLScanner::text()
{
  return d->_text;
}

//--------------------------------------------------------------------------------------------------

void MySQLScanner::set_server_version(long version)
{
  d->_context.version = version;
}

//--------------------------------------------------------------------------------------------------

long MySQLScanner::get_server_version()
{
  return d->_context.version;
}

//--------------------------------------------------------------------------------------------------

void MySQLScanner::set_sql_mode(const std::string &new_mode)
{
  MySQLRecognitionBase::set_sql_mode(new_mode);
  d->_context.sql_mode = sql_mode(); // Parsed SQL mode.
}

//--------------------------------------------------------------------------------------------------

unsigned int MySQLScanner::get_sql_mode_flags()
{
  return d->_context.sql_mode;
}

//--------------------------------------------------------------------------------------------------
