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

#pragma once

#ifdef _MSC_VER
  #ifdef PARSERS_EXPORTS
    #define PARSERS_PUBLIC_TYPE __declspec(dllexport)
  #else
    #define PARSERS_PUBLIC_TYPE __declspec(dllimport)
  #endif
#else
  #define PARSERS_PUBLIC_TYPE
#endif

#include <string>
#include <stack>
#include <vector>
#include <limits>

// Generally used type by the recognizers/scanners, as well as their consumers.
#undef EOF

// Same as in antlr4-common.h.
#define INVALID_INDEX std::numeric_limits<size_t>::max()

// Identifiers for images used in auto completion lists.
#define AC_KEYWORD_IMAGE        1
#define AC_SCHEMA_IMAGE         2
#define AC_TABLE_IMAGE          3
#define AC_ROUTINE_IMAGE        4 // For MySQL stored procedures + functions.
#define AC_FUNCTION_IMAGE       5 // For MySQL library (runtime) functions.
#define AC_VIEW_IMAGE           6
#define AC_COLUMN_IMAGE         7
#define AC_OPERATOR_IMAGE       8
#define AC_ENGINE_IMAGE         9
#define AC_TRIGGER_IMAGE       10
#define AC_LOGFILE_GROUP_IMAGE 11
#define AC_USER_VAR_IMAGE      12
#define AC_SYSTEM_VAR_IMAGE    13
#define AC_TABLESPACE_IMAGE    14
#define AC_EVENT_IMAGE         15
#define AC_INDEX_IMAGE         16
#define AC_USER_IMAGE          17
#define AC_CHARSET_IMAGE       18
#define AC_COLLATION_IMAGE     19

namespace antlr4 {
  class Token;
  class BufferedTokenStream;
}

namespace parsers {

  struct PARSERS_PUBLIC_TYPE ParserErrorInfo {
    std::string message;
    size_t tokenType;
    size_t charOffset; // Offset (in bytes) from the beginning of the input to the error position.
    size_t line;       // Error line.
    size_t offset;     // Byte offset in the error line to the error start position.
    size_t length;
  };

  // A token struct to abstract from antlr4 Token class.
  struct PARSERS_PUBLIC_TYPE ParserToken {
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

    ParserToken() {
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

  // A grammar independent class to work directly with ANTLR token streams.
  // The original token stream is not modified. We maintain a copy of all token references and act on that.
  class PARSERS_PUBLIC_TYPE Scanner {
  public:
    Scanner(antlr4::BufferedTokenStream *input);

    // Standard navigation.
    bool next(bool skipHidden = true);
    bool previous(bool skipHidden = true);

    // Advanced navigation.
    bool advanceToPosition(size_t line, size_t offset);
    bool advanceToType(size_t type);
    bool skipTokenSequence(std::initializer_list<size_t> sequence);

    size_t lookAhead(bool skipHidden = true);
    size_t lookBack(bool skipHidden = true);

    void seek(size_t index);

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
    size_t tokenChannel() const;

    std::string tokenSubText() const;
  private:
    std::vector<antlr4::Token *> _tokens; // Only valid so long as the input stream passed in to the c-tor is alive.
    std::stack<size_t> _tokenStack;
    size_t _index;
  };

} // namespace parsers
