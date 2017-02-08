/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <sstream>
#include <algorithm>
#include <string>
#include <set>

#include <antlr3.h>

#include "MySQLLexer.h"        // The generated lexer.
#include "MySQLSimpleParser.h" // The generated parser.

#include "base/log.h"

#include "mysql-syntax-check.h"

DEFAULT_LOG_DOMAIN("MySQL parsing")

//--------------------------------------------------------------------------------------------------

class MySQLSyntaxChecker::Private {
public:
  const char *_text;
  size_t _text_length;
  int _input_encoding;
  RecognitionContext _context;

  pANTLR3_INPUT_STREAM _input;
  pMySQLLexer _lexer;
  pANTLR3_COMMON_TOKEN_STREAM _tokens;
  pMySQLSimpleParser _parser;
};

//--------------------------------------------------------------------------------------------------

MySQLSyntaxChecker::MySQLSyntaxChecker(long server_version, const std::string &sql_mode,
                                       const std::set<std::string> &charsets)
  : MySQLRecognitionBase(charsets) {
  d = new Private();
  d->_context.version = server_version;
  d->_context.payload = this;
  set_sql_mode(sql_mode);

  d->_input = NULL;
  d->_lexer = NULL;
  d->_tokens = NULL;
  d->_parser = NULL;
}

//--------------------------------------------------------------------------------------------------

MySQLSyntaxChecker::~MySQLSyntaxChecker() {
  if (d->_parser != NULL)
    d->_parser->free(d->_parser);
  if (d->_tokens != NULL)
    d->_tokens->free(d->_tokens);
  if (d->_lexer != NULL)
    d->_lexer->free(d->_lexer);
  if (d->_input != NULL)
    d->_input->close(d->_input);

  delete d;
}

//--------------------------------------------------------------------------------------------------

/**
 * Starts parsing with new input but keeps everything else in place.
 *
 * @param text The text to parse.
 * @param length The length of the text.
 * @param is_utf8 True if text is utf-8 encoded. If false we assume ASCII encoding.
 * @param parse_unit used to restrict parsing to a particular query type.
 *
 * Note: only a few types are supported, everything else is just parsed as a query.
 */
void MySQLSyntaxChecker::parse(const char *text, size_t length, bool is_utf8, MySQLParseUnit parse_unit) {
  // If the text is not using utf-8 (which it should) then we interpret as 8bit encoding
  // (everything requiring only one byte per char as Latin1, ASCII and similar).
  // TODO: handle the (bad) case that the input encoding changes between parse runs.
  d->_input_encoding = is_utf8 ? ANTLR3_ENC_UTF8 : ANTLR3_ENC_8BIT;

  d->_text = text;
  d->_text_length = length;

  reset();

  if (d->_input == NULL) {
    // Input and depending structures are only created once. If there's no input stream yet we need the full setup.
    d->_input = antlr3StringStreamNew((pANTLR3_UINT8)d->_text, d->_input_encoding, (ANTLR3_UINT32)d->_text_length,
                                      (pANTLR3_UINT8) "");
    d->_input->setUcaseLA(
      d->_input, ANTLR3_TRUE); // Make input case-insensitive. String literals must all be upper case in the grammar!
    d->_lexer = MySQLLexerNew(d->_input);
    d->_lexer->pLexer->rec->state->userp = &d->_context;

    d->_tokens = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(d->_lexer));

    d->_parser = MySQLSimpleParserNew(d->_tokens);
    d->_parser->pParser->rec->state->userp = &d->_context;
  } else {
    d->_input->reuse(d->_input, (pANTLR3_UINT8)d->_text, (ANTLR3_UINT32)d->_text_length, (pANTLR3_UINT8) "");
    d->_tokens->reset(d->_tokens);
    d->_lexer->reset(d->_lexer);
    d->_parser->reset(d->_parser);
  }

  switch (parse_unit) {
    case MySQLParseUnit::PuCreateTrigger:
      d->_parser->create_trigger(d->_parser);
      break;
    case MySQLParseUnit::PuCreateView:
      d->_parser->create_view(d->_parser);
      break;
    case MySQLParseUnit::PuCreateRoutine:
      d->_parser->create_routine(d->_parser);
      break;
    case MySQLParseUnit::PuCreateEvent:
      d->_parser->create_trigger(d->_parser);
    default:
      d->_parser->query(d->_parser);
  }
}

//--------------------------------------------------------------------------------------------------

void MySQLSyntaxChecker::set_sql_mode(const std::string &new_mode) {
  MySQLRecognitionBase::set_sql_mode(new_mode);
  d->_context.sqlMode = sql_mode();
}

//--------------------------------------------------------------------------------------------------

void MySQLSyntaxChecker::set_server_version(long new_version) {
  d->_context.version = new_version;
}

//--------------------------------------------------------------------------------------------------

long MySQLSyntaxChecker::server_version() const {
  return d->_context.version;
}

//--------------------------------------------------------------------------------------------------

std::string MySQLSyntaxChecker::text() const {
  return std::string(d->_text, d->_text_length);
}

//--------------------------------------------------------------------------------------------------

const char *MySQLSyntaxChecker::lineStart() const {
  return d->_text;
}

//--------------------------------------------------------------------------------------------------
