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

#include "base/symbol-info.h"

#include "parsers-common.h"

namespace antlr4 {
  class RuleContext;
  class ParserRuleContext;

  namespace tree {
    class ParseTree;
  }

  namespace dfa {
    class Vocabulary;
  }
}

namespace parsers {

  class PARSERS_PUBLIC_TYPE MySQLLexer;

  // A class containing definitions and members used by both lexer and parser classes.
  class PARSERS_PUBLIC_TYPE MySQLRecognizerCommon {
  public:
    // SQL modes that control parsing behavior.
    enum SqlMode {
      NoMode             = 0,
      AnsiQuotes         = 1 << 0,
      HighNotPrecedence  = 1 << 1,
      PipesAsConcat      = 1 << 2,
      IgnoreSpace        = 1 << 3,
      NoBackslashEscapes = 1 << 4
    };

    // For parameterizing the parsing process.
    long serverVersion;
    SqlMode sqlMode; // A collection of flags indicating which of relevant SQL modes are active.

    // Returns true if the given mode (one of the enums above) is set.
    bool isSqlModeActive(size_t mode);
    void sqlModeFromString(std::string modes);

    static std::string dumpTree(antlr4::RuleContext *context, const antlr4::dfa::Vocabulary &vocabulary);
    static std::string sourceTextForContext(antlr4::ParserRuleContext *ctx, bool keepQuotes = false);
    static std::string sourceTextForRange(antlr4::Token *start, antlr4::Token *stop, bool keepQuotes = false);
    static std::string sourceTextForRange(antlr4::tree::ParseTree *start, antlr4::tree::ParseTree *stop,
                                          bool keepQuotes = false);

    static antlr4::tree::ParseTree* getPrevious(antlr4::tree::ParseTree *tree);
    static antlr4::tree::ParseTree* getNext(antlr4::tree::ParseTree *tree);
    static antlr4::tree::ParseTree* terminalFromPosition(antlr4::tree::ParseTree *root,
                                                        std::pair<size_t, size_t> position);
    static antlr4::tree::ParseTree* contextFromPosition(antlr4::tree::ParseTree *root, size_t position);
  };

  class SymbolTable;

  // Returns a symbol table for all predefined system functions in MySQL.
  PARSERS_PUBLIC_TYPE SymbolTable* functionSymbolsForVersion(base::MySQLVersion version);
}
