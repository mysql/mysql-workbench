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
};

MySQLScanner::MySQLScanner(const char *text, size_t length, bool is_utf8, long server_version,
  const std::string &sql_mode, const std::set<std::string> &charsets)
  : MySQLRecognitionBase(charsets)
{
  d = new Private();

  d->_text = text;
  d->_text_length = length;
  d->_context.version = server_version;
  d->_context.payload = this;
  set_sql_mode(sql_mode);

  // If the text is not using utf-8 (which it should) then we interpret as 8bit encoding
  // (everything requiring only one byte per char as Latin1, ASCII and similar).
  d->_input_encoding = is_utf8 ? ANTLR3_ENC_UTF8 : ANTLR3_ENC_8BIT;
  setup();
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
  d->_lexer->reset(d->_lexer);
}

//--------------------------------------------------------------------------------------------------

MySQLToken MySQLScanner::next_token()
{
  pANTLR3_COMMON_TOKEN token = d->_token_source->nextToken(d->_token_source);
  MySQLToken result = {token->type, token->line, token->charPosition, token->index, token->channel,
                       (char*)token->lineStart, reinterpret_cast<char*>(token->start), reinterpret_cast<char*>(token->stop)};
  pANTLR3_STRING text = token->getText(token);
  result.text = (const char*)text->chars;

  return result;
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
void* MySQLScanner::input_start()
{
  return (void *)d->_text;
}

//--------------------------------------------------------------------------------------------------
