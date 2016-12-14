/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "MySQLRecognizerCommon.h"
#include "MySQLBaseLexer.h"
#include "Parser.h"
#include "CommonToken.h"

namespace antlr4 {
  class PARSERS_PUBLIC_TYPE Parser;
}

namespace parsers {

  class PARSERS_PUBLIC_TYPE MySQLBaseRecognizer : public antlr4::Parser, public MySQLRecognizerCommon {
  public:
    MySQLBaseRecognizer(antlr4::TokenStream *input);

    virtual void reset() override;

    // A specialized function to get the text from a given context. This falls back to context->getText() in the general
    // case, but provides special behavior for certain contexts (e.g. the implicit string concatenation used in MySQL).
    static std::string getText(antlr4::RuleContext *context, bool convertEscapes);
    
  protected:
    // Checks the token at the given position relative to the current position, whether it matches the expected value.
    // For positions > 1 this looks ahead, otherwise it looks back.
    // Note: position == 0 is not defined. position == 1 is the current position.
    bool look(ssize_t position, size_t expected);

    // Validation function used to check that a string that is not allowed to contain line breaks really doesn't.
    bool containsLinebreak(const std::string &text) const;
  };

} // namespace parsers
