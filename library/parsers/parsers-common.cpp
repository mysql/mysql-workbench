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

#include <antlr4-runtime.h>

#include "base/string_utilities.h"
#include "parsers-common.h"

using namespace parsers;
using namespace antlr4;

//----------------- Scanner --------------------------------------------------------------------------------------------

Scanner::Scanner(BufferedTokenStream *input) {
  _index = 0;
  input->fill();
  _tokens = input->getTokens(); // The tokens are managed by the token stream, hence this must stay alive
                                // at least as long as the scanner class that holds references to the stream's tokens.
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Advances to the next token.
 *
 * @param skipHidden If true ignore hidden tokens.
 * @return False if we hit the last token before we could advance, true otherwise.
 */
bool Scanner::next(bool skipHidden) {
  while (_index < _tokens.size() - 1) {
    ++_index;
    if (_tokens[_index]->getChannel() == Token::DEFAULT_CHANNEL || !skipHidden)
      return true;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns to the previous token.
 *
 * @param skipHidden If true ignore hidden tokens.
 * @return False if we hit the last token before we could fully go back, true otherwise.
 */
bool Scanner::previous(bool skipHidden) {
  while (_index > 0) {
    --_index;
    if (_tokens[_index]->getChannel() == 0 || !skipHidden)
      return true;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Advances to the token that covers the given line and char offset. The line number is one-based
 * while the character offset is zero-based.
 *
 * Note: this function also considers hidden token.
 *
 * @return True if such a node exists, false otherwise (no change performed then).
 */
bool Scanner::advanceToPosition(size_t line, size_t offset) {
  if (_tokens.empty())
    return false;

  size_t i = 0;
  for (; i < _tokens.size(); i++) {
    Token *run = _tokens[i];
    size_t tokenLine = run->getLine();
    if (tokenLine >= line) {
      size_t tokenOffset = run->getCharPositionInLine();
      size_t tokenLength = run->getStopIndex() - run->getStartIndex() + 1;
      if (tokenLine == line && tokenOffset <= offset && offset < tokenOffset + tokenLength) {
        _index = i;
        break;
      }

      if (tokenLine > line || tokenOffset > offset) {
        // We reached a token after the current offset. Take the previous one as result then.
        if (i == 0)
          return false;

        _index = i - 1;
        break;
      }
    }
  }

  if (i == _tokens.size())
    _index = i - 1; // Nothing found, take the last token instead.

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Advances to the next token with the given lexical type.
 *
 * @param type The token type to search.
 * @return True if such a node exists, false otherwise (no change performed then).
 */
bool Scanner::advanceToType(size_t type) {
  for (size_t i = _index; i < _tokens.size(); ++i) {
    if (_tokens[i]->getType() == type) {
      _index = i;
      return true;
    }
  }

  return false;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Steps over a number of tokens and positions.
 * The tokens are traversed in exactly the given order without intermediate tokens. The current token must be
 * startToken. Any non-default channel token is skipped before testing for the next token in the sequence.
 *
 * @return True if all the given tokens were found and there is another token after the last token
 *         in the list, false otherwise. If the token sequence could not be found or there is no more
 *         token the internal state is undefined.
 */
bool Scanner::skipTokenSequence(std::initializer_list<size_t> sequence) {
  if (_index >= _tokens.size())
    return false;

  for (auto token : sequence) {
    if (_tokens[_index]->getType() != token)
      return false;

    while (++_index < _tokens.size() && _tokens[_index]->getChannel() != Token::DEFAULT_CHANNEL)
      ;

    if (_index == _tokens.size())
      return false;
  }
  return true;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the type of the next token without changing the internal state.
 */
size_t Scanner::lookAhead(bool skipHidden) {
  size_t index = _index;
  while (index < _tokens.size() - 1) {
    ++index;
    if (_tokens[index]->getChannel() == Token::DEFAULT_CHANNEL || !skipHidden)
      return _tokens[index]->getType();
  }

  return ParserToken::INVALID_TYPE;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Look back in the stream (physical order) what was before the current token, without
 * modifying the current position.
 */
size_t Scanner::lookBack(bool skipHidden) {
  size_t index = _index;
  while (index > 0) {
    --index;
    if (_tokens[index]->getChannel() == Token::DEFAULT_CHANNEL || !skipHidden)
      return _tokens[index]->getType();
  }

  return ParserToken::INVALID_TYPE;
}

//----------------------------------------------------------------------------------------------------------------------

void Scanner::seek(size_t index) {
  if (index < _tokens.size())
    _index = index;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Resets the walker to be at the original location.
 */
void Scanner::reset() {
  _index = 0;
  while (!_tokenStack.empty())
    _tokenStack.pop();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Store the current node on the stack, so we can easily come back when needed.
 */
void Scanner::push() {
  _tokenStack.push(_index);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns to the location at the top of the token stack (if any).
 */
bool Scanner::pop() {
  if (_tokenStack.empty())
    return false;

  _index = _tokenStack.top();
  _tokenStack.pop();
  return true;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Removes the current top of stack entry without restoring the internal state.
 * Does nothing if the stack is empty.
 */
void Scanner::removeTos() {
  if (!_tokenStack.empty())
    _tokenStack.pop();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns true if the current token is of the given type.
 */
bool Scanner::is(size_t type) const {
  return _tokens[_index]->getType() == type;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the textual expression of the token.
 */
std::string Scanner::tokenText(bool keepQuotes) const {
  return _tokens[_index]->getText();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the type of the current token. Same as the type you can specify in advance_to().
 */
size_t Scanner::tokenType() const {
  return _tokens[_index]->getType();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the (one-base) line number of the token.
 */
size_t Scanner::tokenLine() const {
  return _tokens[_index]->getLine();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the (zero-based) character offset of the token on its line.
 */
size_t Scanner::tokenStart() const {
  return _tokens[_index]->getCharPositionInLine();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the (zero-based) index of the current token within the input.
 */
size_t Scanner::tokenIndex() const {
  return _tokens[_index]->getTokenIndex(); // Usually the same as _index.
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the offset of the token in its source string.
 */
size_t Scanner::tokenOffset() const {
  return _tokens[_index]->getStartIndex();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the length of the token in bytes.
 */
size_t Scanner::tokenLength() const {
  Token *token = _tokens[_index];

  return token->getStopIndex() - token->getStartIndex() + 1;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the channel of the current token.
 */
size_t Scanner::tokenChannel() const {
  return _tokens[_index]->getChannel();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * This is a special purpose function to return all the input text from the current token to the end.
 */
std::string Scanner::tokenSubText() const {
  CharStream *cs = _tokens[_index]->getTokenSource()->getInputStream();
  return cs->getText(misc::Interval((ssize_t)_tokens[_index]->getStartIndex(), std::numeric_limits<ssize_t>::max()));
}

//----------------------------------------------------------------------------------------------------------------------
