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

#pragma once

#include "parsers-common.h"

#include "ECMALexer.h"
#include "ECMAParser.h"

/**
 * C++ interface for the ANTLR based ECMA (Javascript) parser.
 */
class PARSERS_PUBLIC_TYPE ECMARecognizer
{
public:
  ECMARecognizer();
  virtual ~ECMARecognizer();

  void parse(const char *text, std::size_t length, bool isUtf8);

  // Internal function called by static callback.
  void addError(const std::string &message, ANTLR3_UINT32 token, ANTLR3_MARKER tokenStart,
                ANTLR3_UINT32 line, ANTLR3_UINT32 offsetInLine, ANTLR3_MARKER length);

  const std::vector<ParserErrorInfo> &errorInfo() const;
  std::vector<std::pair<std::size_t, std::size_t>> statementRanges() const;

  bool hasErrors() const;
  std::string text() const;
  const char* lineStart() const;

  std::string dumpTree();
  
private:
  const char *_text;
  std::size_t _textLength;

  pANTLR3_INPUT_STREAM _input;
  pECMALexer _lexer;
  pANTLR3_COMMON_TOKEN_STREAM _tokens;
  pECMAParser _parser;
  pANTLR3_BASE_TREE _ast;

  std::vector<ParserErrorInfo> _errorInfo;

  std::string dumpTree(pANTLR3_BASE_TREE tree, const std::string &indentation);
};
