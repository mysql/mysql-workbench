/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

#ifndef HAVE_PRECOMPILED_HEADERS
#include <sstream>
#include <algorithm>
#include <string>
#include <set>
#include <vector>

#include <antlr3.h>
#endif

#include "ECMALexer.h"
#include "ECMAParser.h"

#include "base/log.h"

#include "ecma-parser.h"

#include "base/string_utilities.h"

DEFAULT_LOG_DOMAIN("ECMA parser")

//--------------------------------------------------------------------------------------------------

static std::string getTokenName(pANTLR3_UINT8 *tokenNames, ANTLR3_UINT32 token)
{
  // Transform a selection of tokens to nice strings. All others just take the token name.
  switch (token)
  {
    case ANTLR3_TOKEN_EOF:
      return "end of statement";
    case 1:
    case 2:
    case 3:
    case 4:
      return "<invalid token>";
    case LEFT_PAR_SYMBOL:
      return "opening parenthesis";
    case RIGHT_PAR_SYMBOL:
      return "closing parenthesis";
    case LEFT_BRACE_SYMBOL:
      return "opening curly brace";
    case RIGHT_BRACE_SYMBOL:
      return "closing curly brace";
    case EOL:
      return "line break";

    default:
      std::string result = base::tolower((char *)tokenNames[token]);
      std::string::size_type position = result.find("_symbol");
      if (position != std::string::npos)
        result = result.substr(0, position);

      base::replaceStringInplace(result, "_", " ");
      return result;
  }
}

//--------------------------------------------------------------------------------------------------

static bool handleLexerError(pANTLR3_BASE_RECOGNIZER recognizer, pANTLR3_EXCEPTION exception,
  ANTLR3_MARKER &start, ANTLR3_MARKER &length, std::string &message)
{
  pANTLR3_LEXER lexer = (pANTLR3_LEXER)(recognizer->super);
  start = recognizer->state->tokenStartCharIndex;

  // For debugging use the native error message that contains the grammar line.
  // std::string native = (pANTLR3_UINT8)(exception->message);
  length = ANTLR3_UINT32_CAST(((pANTLR3_UINT8)(lexer->input->data) + (lexer->input->size(lexer->input))) - (pANTLR3_UINT8)(exception->index));

  if (length <= 0)
  {
    message = "unexpected end of input (unfinished string or quoted identifier)";
    length = ANTLR3_UINT32_CAST(((pANTLR3_UINT8)(lexer->input->data) + (lexer->input->size(lexer->input))) - (pANTLR3_UINT8)(lexer->rec->state->tokenStartCharIndex));
  }
  else
  {
    switch (exception->type)
    {
      case ANTLR3_RECOGNITION_EXCEPTION:
        // Invalid character. Can currently never appear because any input from the Unicode BMP
        // is acceptable input for our parser. We cannot parse input beyond the BMP.
        message += "'";
        if (isprint(exception->c))
          message += (char)exception->c;
        else
          message += (ANTLR3_UINT8)(exception->c);

        message += "' is not valid input";
        break;

      case ANTLR3_NO_VIABLE_ALT_EXCEPTION:
        message += (ANTLR3_UINT8)(exception->c);
        message += " is not valid input";
        break;
    }
  }

  return true;
}

//--------------------------------------------------------------------------------------------------

static bool handleParserError(pANTLR3_BASE_RECOGNIZER recognizer, pANTLR3_EXCEPTION exception,
  pANTLR3_UINT8 *tokenNames, ANTLR3_MARKER &start, ANTLR3_MARKER &length, std::string &message)
{
  std::ostringstream error;

  pANTLR3_PARSER parser = (pANTLR3_PARSER) (recognizer->super);
  pANTLR3_COMMON_TOKEN error_token = (pANTLR3_COMMON_TOKEN)(exception->token);

  std::string token_text = (char *)error_token->getText(error_token)->chars;
  if (token_text[0] != '"' && token_text[0] != '\'' && token_text[0] != '`')
    token_text = "'" + token_text + "'";

  std::string token_name;

  bool eoi = error_token->type == ANTLR3_TOKEN_EOF;
  if (eoi)
  {
    // We are at the end of the input. Seek back one token to have a meaningful error indicator.
    // If we cannot get a previous token then issue a generic eoi error.
    pANTLR3_COMMON_TOKEN previous_token = parser->tstream->_LT(parser->tstream, -1);
    if (previous_token != NULL)
      error_token = previous_token;
  }
  else
    token_name = getTokenName(tokenNames, error_token->type);

  start = error_token->start;
  switch (exception->type)
  {
    case ANTLR3_RECOGNITION_EXCEPTION:
      // Unpredicted input.
      error << token_text << " (" << token_name << ") is not valid input at this position";
      break;

    case ANTLR3_MISMATCHED_TOKEN_EXCEPTION:
      // We were expecting to see one thing and got another. This is the
      // most common error if we could not detect a missing or unwanted token.
      if (exception->expecting == ANTLR3_TOKEN_EOF)
        error << "expected end of statement but found " << token_text << " (" << token_name << ")";
      else
        error << "expected '" << tokenNames[exception->expecting] << "' but found " << token_text << " (" << token_name << ")";
      break;

    case ANTLR3_NO_VIABLE_ALT_EXCEPTION:
      // No alternative to choose from here.
      if (eoi)
        error << "unexpected end of input";
      else
        error << "unexpected " << token_text << " (" << token_name << ")";

      break;

    case ANTLR3_MISMATCHED_SET_EXCEPTION:
    {
      // One out of a set of tokens was expected but hasn't been found.
      pANTLR3_BITSET errBits = antlr3BitsetLoad(exception->expectingSet);
      ANTLR3_UINT32 numbits = errBits->numBits(errBits);
      ANTLR3_UINT32 size = errBits->size(errBits);

      if (size == 0)
      {
        // No information about expected tokens available.
        error << "unexpected " << token_text << " (" << token_name << ")";
      }
      else
      {
        // I'd expect only a few set members here, but this case is hard to test. So
        // just to be sure not to show a huge list of expected tokens we limit the number here.
        // TODO: find a query that triggers this error branch.
        error << "wrong input, expected one of: ";
        for (ANTLR3_UINT32 bit = 1; bit < numbits && bit <= 20 && bit < size; ++bit)
        {
          if  (errBits->isMember(errBits, bit))
            error << (bit > 1 ? ", " : "") << getTokenName(tokenNames, bit);
        }
      }
    }
      break;

    case ANTLR3_EARLY_EXIT_EXCEPTION:
      // We entered a loop requiring a number of token sequences but found a token that ended that
      // sequence earlier than we should have done.
      // Unfortunately, there's no expecting set for this exception which would have made the message
      // very useful.
      error << "missing sub clause or other elements before " << token_text << " (" << token_name << ")";
      break;

    case ANTLR3_FAILED_PREDICATE_EXCEPTION:
      // Appears when a gated semantic predicate is used in the grammar, but not for predicting an alternative,
      // e.g. ... some_rule {condition}? => some_rule.
      // If the condition does not match we get a failed predicate error. So it's more like a grammar error
      // unless this is by intention (which is never the case in the MySQL grammar where predicates are only used
      // to guide the parser).
      error << "failed predicate";
      break;

    case ANTLR3_MISMATCHED_TREE_NODE_EXCEPTION:
      // This is very likely a tree parser error and hence not relevant here (no info in ANTLR docs).
      error << "unexpected parser error type (" << exception->type << "), please file a bug report!";
      break;

    case ANTLR3_REWRITE_EARLY_EXCEPTION:
      // ANTLR docs say: No elements within a (...)+ in a rewrite rule
      // so this seems to be an error only raised if there was a grammar bug -> internal error.
      error << "internal parser error type (" << exception->type << "), please file a bug report!";
      break;

    case ANTLR3_UNWANTED_TOKEN_EXCEPTION:
      // Indicates that the recognizer was fed a token which seems to be spurious input. We can detect
      // this when the token that follows this unwanted token would normally be part of the
      // syntactically correct stream.
      if	(exception->expecting == ANTLR3_TOKEN_EOF)
        error << "extraneous input found - expected end of input";
      else
        error << "extraneous input found - expected '" << getTokenName(tokenNames, exception->expecting) << "'";
      break;

    case ANTLR3_MISSING_TOKEN_EXCEPTION:
    {
      // Indicates that the recognizer detected that the token we just
      // hit would be valid syntactically if preceded by a particular
      // token. Perhaps a missing ';' at line end or a missing ',' in an
      // expression list, and such like.
      if (tokenNames == NULL)
        error << "missing token " << exception->expecting; // Will very likely never occur.
      else
      {
        if (exception->expecting == ANTLR3_TOKEN_EOF)
          // Will probably not occur since ANTLR3_UNWANTED_TOKEN_EXCEPTION will kick in instead.
          error << "expected end of input";
        else
          error << "missing '" << getTokenName(tokenNames, exception->expecting) << "'";
      }

      // The error token for a missing token does not contain much information to display.
      // If we reach this error case then the token after the missing token has been consumed already
      // (which is how the parser found out about the missing one), so by going back to that token
      // we can get good start and length information (showing so the error at this following token instead).
      error_token = parser->tstream->_LT(parser->tstream, -1);
      if (error_token == NULL)
        return false;

      start = error_token->start;
      break;
    }

    default:
      error << "unexpected parser error type (" << exception->type << "), please file a bug report!";
      break;

  }

  if (length == 0)
  {
    if (error_token != NULL)
      length = (int)error_token->stop - (int)error_token->start + 1;
    else
      length = 1;
  }

  message = error.str();
  return true;
}

//--------------------------------------------------------------------------------------------------

extern "C" {

  /**
   * Error report function which is set in the parser (see MySQL.g where this is done).
   */
  void onECMAParseError(pANTLR3_BASE_RECOGNIZER recognizer, pANTLR3_UINT8 *tokenNames)
  {
    pANTLR3_EXCEPTION exception = recognizer->state->exception;

    // Only the take the current exception into account. There's a linked list of all exceptions we could walk
    // but that only contains what we've seen anyway.
    if (exception != NULL)
    {
      // Token position and length for error marker.
      ANTLR3_MARKER length = 0;
      ANTLR3_MARKER start = 0;
      std::string message;
      switch (recognizer->type)
      {
        case ANTLR3_TYPE_LEXER:
          if (!handleLexerError(recognizer, exception, start, length, message))
            return;
          break;

        case ANTLR3_TYPE_PARSER:
          if (!handleParserError(recognizer, exception, tokenNames, start, length, message))
            return;
          break;
      }

      pANTLR3_COMMON_TOKEN errorToken = (pANTLR3_COMMON_TOKEN)(exception->token);
      ECMARecognizer *checker = (ECMARecognizer*)recognizer->state->userp;
      checker->addError("Syntax error: " + message, (errorToken == nullptr) ? 0 : errorToken->type,
        start, exception->line, exception->charPositionInLine, length);
    }
  }
  
} // extern "C"

//--------------------------------------------------------------------------------------------------

ECMARecognizer::ECMARecognizer()
{
  _input = NULL;
  _lexer = NULL;
  _tokens = NULL;
  _parser = NULL;
}

//--------------------------------------------------------------------------------------------------

ECMARecognizer::~ECMARecognizer()
{
  if (_parser != NULL)
    _parser->free(_parser);
  if (_tokens != NULL)
    _tokens ->free(_tokens);
  if (_lexer != NULL)
    _lexer->free(_lexer);
  if (_input != NULL)
    _input->close(_input);
}

//--------------------------------------------------------------------------------------------------

/**
 * Starts parsing with new input but keeps everything else in place.
 *
 * @param text The text to parse.
 * @param length The length of the text.
 * @param isUtf8 True if text is utf-8 encoded. If false we assume ASCII encoding.
 *
 * Note: only a few types are supported, everything else is just parsed as a query.
 */
void ECMARecognizer::parse(const char *text, std::size_t length, bool isUtf8)
{
  logDebug3("Running ECMA syntax check...\n");

  ANTLR3_UINT32 encoding = isUtf8 ? ANTLR3_ENC_UTF8 : ANTLR3_ENC_8BIT;

  _text = text;
  _textLength = length;
  _errorInfo.clear();

  if (_input == nullptr)
  {
    // Input and depending structures are only created once. If there's no input stream yet we need the full setup.
    _input = antlr3StringStreamNew((pANTLR3_UINT8)_text, encoding, (ANTLR3_UINT32)_textLength, (pANTLR3_UINT8)"ecma syntax check");
    _lexer = ECMALexerNew(_input);
    _lexer->pLexer->rec->state->userp = this;

    _tokens = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(_lexer));
    _parser = ECMAParserNew(_tokens);
    _parser->pParser->rec->state->userp = this;
  }
  else
  {
    _input->reuse(_input, (pANTLR3_UINT8)_text, (ANTLR3_UINT32)_textLength, (pANTLR3_UINT8)"ecma syntax check");
    _tokens->reset(_tokens);
    _lexer->reset(_lexer);
    _parser->reset(_parser);
  }

  _ast = _parser->program(_parser).tree;

  logDebug3("ECMA syntax check done\n");
}

//--------------------------------------------------------------------------------------------------

void ECMARecognizer::addError(const std::string &message, ANTLR3_UINT32 token,
  ANTLR3_MARKER token_start, ANTLR3_UINT32 line, ANTLR3_UINT32 offset_in_line, ANTLR3_MARKER length)
{
  ParserErrorInfo info = { message, token, (size_t)(token_start - (ANTLR3_MARKER)lineStart()),
    line, offset_in_line, (size_t)length};
  _errorInfo.push_back(info);
};

//--------------------------------------------------------------------------------------------------

const std::vector<ParserErrorInfo>& ECMARecognizer::errorInfo() const
{
  return _errorInfo;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns position and length of all top level statements (no nested statements, e.g. in functions).
 */
std::vector<std::pair<std::size_t, std::size_t>> ECMARecognizer::statementRanges() const
{
  std::vector<std::pair<std::size_t, std::size_t>> result;

  uint32_t count = _ast->getChildCount(_ast);
  if (count == 0)
    return result;

  --count;  // There must always be one child (EOF) if there's an AST at all.

  // Iterate over all children of the top level node and get their start end end positions.
  for (uint32_t i = 0; i < count; ++i)
  {
    pANTLR3_BASE_TREE topLevelNode = (pANTLR3_BASE_TREE)_ast->getChild(_ast, i);

    // Find the start of the first real node (no virtual nodes).
    pANTLR3_BASE_TREE child = topLevelNode;
    while (child->getChildCount(child) > 0)
      child = (pANTLR3_BASE_TREE)child->getChild(child, 0);

    pANTLR3_COMMON_TOKEN startToken = child->getToken(child);
    if (startToken->type == ANTLR3_TOKEN_INVALID)
      continue;

    // Similar for the last node.
    child = topLevelNode;
    while (child->getChildCount(child) > 0)
      child = (pANTLR3_BASE_TREE)child->getChild(child, child->getChildCount(child) - 1);

    // The grammar is designed in a way that ensures we always have a terminating token.
    // Either semicolon (if it was given in the text) or EOL (which is a whitespace but gets
    // promoted to a default channel token in that case).
    pANTLR3_COMMON_TOKEN stopToken = child->getToken(child);

    uint64_t temp = stopToken->stop - startToken->start + 1;
    if (stopToken->type == ANTLR3_TOKEN_INVALID)
      continue;

    result.push_back({ (uint64_t)startToken->start - (uint64_t)_text, stopToken->stop - startToken->start + 1});
  }
  return result;
}

//--------------------------------------------------------------------------------------------------

bool ECMARecognizer::hasErrors() const
{
  return !_errorInfo.empty();
}

//--------------------------------------------------------------------------------------------------

std::string ECMARecognizer::text() const
{
  return std::string(_text, _textLength);
}

//--------------------------------------------------------------------------------------------------

const char* ECMARecognizer::lineStart() const
{
  return _text;
}

//--------------------------------------------------------------------------------------------------

std::string ECMARecognizer::dumpTree()
{
  logDebug2("Generating parse tree\n");

  return dumpTree(_ast, "");
}

//--------------------------------------------------------------------------------------------------

std::string ECMARecognizer::dumpTree(pANTLR3_BASE_TREE tree, const std::string &indentation)
{
  std::string result;

  pANTLR3_RECOGNIZER_SHARED_STATE state = _parser->pParser->rec->state;
  ANTLR3_UINT32 char_pos = tree->getCharPositionInLine(tree);
  ANTLR3_UINT32 line = tree->getLine(tree);
  pANTLR3_STRING token_text = tree->getText(tree);

  pANTLR3_COMMON_TOKEN token = tree->getToken(tree);
  const char* utf8 = (const char*)token_text->chars;
  if (token != NULL)
  {
    ANTLR3_UINT32 token_type = token->getType(token);

    pANTLR3_UINT8 token_name;
    if (token_type == EOF)
      token_name = (pANTLR3_UINT8)"EOF";
    else
      token_name = state->tokenNames[token_type];

#ifdef  ANTLR3_USE_64BIT
    result = base::strfmt("%s(line: %i, offset: %i, length: %" PRId64 ", index: %" PRId64 ", %s[%i])    %s\n",
                          indentation.c_str(), line, char_pos, token->stop - token->start + 1, token->index, token_name,
                          token_type, utf8);
#else
    result = base::strfmt("%s(line: %i, offset: %i, length: %i, index: %i, %s[%i])    %s\n",
                          indentation.c_str(), line, char_pos, token->stop - token->start + 1, token->index, token_name,
                          token_type, utf8);
#endif

  }
  else
  {
    result = base::strfmt("%s(line: %i, offset: %i, nil)    %s\n", indentation.c_str(), line, char_pos, utf8);
  }

  for (ANTLR3_UINT32 index = 0; index < tree->getChildCount(tree); index++)
  {
    pANTLR3_BASE_TREE child = (pANTLR3_BASE_TREE)tree->getChild(tree, index);
    std::string child_text = dumpTree(child, indentation + "\t");
    result += child_text;
  }
  return result;
}

//--------------------------------------------------------------------------------------------------

