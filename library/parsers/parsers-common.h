/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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
#include <stdint.h>
#include <stack>

#include <antlr3.h>
#endif

// Generally used types by the recognizers/scanners, as well as their consumers.

#define INVALID_TOKEN 0

struct ParserErrorInfo {
  std::string message;
  uint32_t token_type;
  size_t charOffset; // Offset (in bytes) from the beginning of the input to the error position.
  size_t line;       // Error line.
  uint32_t offset;   // Byte offset in the error line to the error start position.
  size_t length;
};

struct ParserToken {
  uint32_t type;    // The type as defined in the grammar.
  uint32_t line;    // One-based line number of this token.
  int32_t position; // Zero-based position in the line.
  uint64_t index;   // The index of the token in the input.
  uint32_t channel; // 0 for normally visible tokens. 99  for the hidden channel (whitespaces, comments).

  char *line_start; // Pointer into the input to the beginning of the line where this token is located.
  char *start;      // Points to the start of the token in the input.
  char *stop;       // Points to the last character of the token.

  std::string text; // The text of the token.

  ParserToken() {
    type = INVALID_TOKEN;
    line = 0;
    position = 0;
    index = -1;
    channel = 0;
    line_start = nullptr;
    start = nullptr;
    stop = nullptr;
  }
};

// Interface for recognizer specific functionality.
class PARSERS_PUBLIC_TYPE IRecognizer {
public:
  virtual std::string text() const = 0;
  virtual const char *lineStart() const = 0;

  virtual std::string tokenText(pANTLR3_BASE_TREE node, bool keepQuotes = false) const = 0;
  virtual std::string textForTree(pANTLR3_BASE_TREE tree) const = 0;

  virtual bool isIdentifier(uint32_t type) const = 0;
  virtual bool isKeyword(uint32_t type) const = 0;

  static std::string dumpTree(pANTLR3_UINT8 *tokenNames, pANTLR3_BASE_TREE tree, const std::string &indentation);
};

#ifdef _WIN32
#pragma warning(disable : 4251) // DLL interface required for std::string member.
#endif

class PARSERS_PUBLIC_TYPE RecognizerTreeWalker {
public:
  RecognizerTreeWalker(IRecognizer *recognizer, pANTLR3_BASE_TREE tree);

  void printToken(pANTLR3_BASE_TREE tree);

  // Standard navigation.
  bool next(std::size_t count = 1);
  bool nextSibling();
  bool previous();
  bool previousByIndex();
  bool previousSibling();
  bool up();

  // Advanced navigation.
  bool advanceToPosition(int line, int offset);
  bool advanceToType(uint32_t type, bool recurse);
  bool skipTokenSequence(uint32_t startToken, ...);
  bool skipIf(uint32_t token, size_t count = 1);
  void skipSubtree();

  uint32_t lookAhead(bool recursive);
  uint32_t parentType();
  uint32_t previousType();

  // Stacking.
  void reset();
  void push();
  bool pop();
  void removeTos();

  // Properties of current token.
  bool is(uint32_t type) const;
  bool isNil() const;
  bool isSubtree() const;
  bool isFirstChild() const;
  bool isIdentifier() const;
  bool isKeyword() const;

  std::string tokenText(bool keepQuotes = false) const;
  uint32_t tokenType() const;
  uint32_t tokenLine() const;
  uint32_t tokenStart() const;
  int64_t tokenIndex() const;
  size_t tokenOffset() const;
  int64_t tokenLength() const;
  std::string textForTree() const;

protected:
  IRecognizer *_recognizer;

  pANTLR3_BASE_TREE _origin;
  pANTLR3_BASE_TREE _tree;
  std::stack<pANTLR3_BASE_TREE> _tokenStack;
  std::vector<pANTLR3_BASE_TREE> _tokenList; // A list of all tokens in incoming order (no hierarchy).

  pANTLR3_BASE_TREE getNext(pANTLR3_BASE_TREE node, bool recurse) const;
  pANTLR3_BASE_TREE getPrevious(pANTLR3_BASE_TREE node, bool recurse) const;
  pANTLR3_BASE_TREE getPreviousByIndex(pANTLR3_BASE_TREE node) const;

  bool isSubtree(struct ANTLR3_BASE_TREE_struct *tree) const;
};
