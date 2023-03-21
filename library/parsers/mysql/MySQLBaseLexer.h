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

#include <list>
#include "Lexer.h"
#include "MySQLRecognizerCommon.h"
#include "mysql-recognition-types.h"

namespace antlr4 {
  class PARSERS_PUBLIC_TYPE Lexer;
}

namespace parsers {

  // The base lexer class provides a number of function needed in actions in the lexer (grammar).
  class PARSERS_PUBLIC_TYPE MySQLBaseLexer : public antlr4::Lexer, public MySQLRecognizerCommon {
  public:
    std::set<std::string> charsets; // Used to check repertoires.
    bool inVersionComment;

    MySQLBaseLexer(antlr4::CharStream *input);

    virtual void reset() override;

    bool isIdentifier(size_t type) const;
    size_t keywordFromText(std::string const& name);

    // Scans from the current token position to find out which query type we are dealing with in the input.
    MySQLQueryType determineQueryType();

    static bool isRelation(size_t type);
    static bool isNumber(size_t type);
    static bool isOperator(size_t type);

    virtual std::unique_ptr<antlr4::Token> nextToken() override;

  protected:
    // Checks if the version number, given by the token, is less than or equal to the current server version.
    // Returns true if so, otherwise false.
    bool checkVersion(const std::string &text);

    // Called when a keyword was consumed that represents an internal MySQL function and checks if that
    // keyword is followed by an open parenthesis. If not then it is not considered a keyword but
    // treated like a normal identifier.
    size_t determineFunction(size_t proposed);

    // Checks the given text and determines the smallest number type from it. Code has been taken from sql_lex.cc.
    size_t determineNumericType(const std::string &text);

    // Checks if the given text corresponds to a charset defined in the server (text is preceded by an underscore).
    // Returns UNDERSCORE_CHARSET if so, otherwise IDENTIFIER.
    size_t checkCharset(const std::string &text);

    void emitDot();

  private:
    std::list<std::unique_ptr<antlr4::Token>> _pendingTokens;
    std::map<std::string, size_t> _symbols; // A list of all defined symbols for lookup.

    std::unique_ptr<antlr4::Token> nextDefaultChannelToken();
    bool skipDefiner(std::unique_ptr<antlr4::Token> &token);
  };

} // namespace parsers
