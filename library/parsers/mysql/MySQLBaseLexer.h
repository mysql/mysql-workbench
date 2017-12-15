/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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
    bool isKeyword(size_t type) const;

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
    std::unique_ptr<antlr4::Token> nextDefaultChannelToken();
    bool skipDefiner(std::unique_ptr<antlr4::Token> &token);
  };

} // namespace parsers
