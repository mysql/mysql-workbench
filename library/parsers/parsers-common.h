/*
 * Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#ifdef _WIN32
  #ifdef PARSERS_EXPORTS
    #define PARSERS_PUBLIC_TYPE __declspec(dllexport)
  #else
    #define PARSERS_PUBLIC_TYPE __declspec(dllimport)
  #endif
#else
  #define PARSERS_PUBLIC_TYPE
#endif

#ifndef HAVE_PRECOMPILED_HEADERS
#include <string>
#endif

// Generally used types by the recognizers/scanners, as well as their consumers.
#undef EOF

// Same as in antlr4-common.h.
#define INVALID_INDEX (size_t)-1

namespace antlr4 {
  class Token;
  class BufferedTokenStream;
}

namespace parsers {

  struct PARSERS_PUBLIC_TYPE ParserErrorInfo
  {
    std::string message;
    size_t tokenType;
    size_t charOffset; // Offset (in bytes) from the beginning of the input to the error position.
    size_t line;       // Error line.
    size_t offset;     // Byte offset in the error line to the error start position.
    size_t length;
  };

  // A token struct to abstract from antlr4 Token class.
  struct PARSERS_PUBLIC_TYPE ParserToken
  {
    // Same as in antlr4::Token.
    static const size_t INVALID_TYPE = 0;
    static const size_t DEFAULT_CHANNEL = 0;
    static const size_t HIDDEN_CHANNEL = 1;
    static const size_t EOF = (size_t)-1;

    size_t type;      // The type as defined in the grammar.
    size_t line;      // One-based line number of this token.
    size_t position;  // Zero-based position in the line.
    size_t index;     // The index of the token in the input.
    size_t channel;   // One of the channel constants.

    char *lineStart;  // Pointer into the input to the beginning of the line where this token is located.
    char *start;      // Points to the start of the token in the input.
    char *stop;       // Points to the last character of the token.

    std::string text; // The text of the token.

    ParserToken()
    {
      type = INVALID_TYPE;
      line = 0;
      position = 0;
      index = INVALID_INDEX;
      channel = DEFAULT_CHANNEL;
      lineStart = nullptr;
      start = nullptr;
      stop = nullptr;
    }
  };

  class PARSERS_PUBLIC_TYPE Scanner
  {
  public:
    Scanner(antlr4::BufferedTokenStream *input);

    //void printToken(pANTLR3_BASE_TREE tree);

    // Standard navigation.
    bool next(std::size_t count = 1);
    bool previous();

    // Advanced navigation.
    bool advanceToPosition(size_t line, size_t offset);
    bool advanceToType(size_t type);
    bool skipTokenSequence(size_t startToken, ...);
    bool skipIf(size_t token, size_t count = 1);

    size_t lookAhead();
    size_t parentType();
    size_t previousType();

    // Stacking.
    void reset();
    void push();
    bool pop();
    void removeTos();

    // Properties of current token.
    bool is(size_t type) const;

    std::string tokenText(bool keepQuotes = false) const;
    size_t tokenType() const;
    size_t tokenLine() const;
    size_t tokenStart() const;
    size_t tokenIndex() const;
    size_t tokenOffset() const;
    size_t tokenLength() const;

  private:
    std::vector<antlr4::Token *> _tokens; // Only valid so long as the input stream passed in to the c-tor is alive.
    std::stack<size_t> _tokenStack;
    size_t _index;
  };

} // namespace parsers
