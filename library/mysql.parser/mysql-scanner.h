/* 
 * Copyright (c) 2013, 2014, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MYSQL_LEXER_H_
#define _MYSQL_LEXER_H_

#include "mysql-parser-common.h"

/**
 * C++ interface for the ANTLR based MySQL lexer.
 * This scanner class is not needed for the MySQLRecognizer class (it uses the raw lexer)
 * but provides tokenizing functionality beside it.
 */

struct MySQLToken
{
  ANTLR3_UINT32 type;    // The type as defined in the grammar.
  ANTLR3_UINT32 line;    // One-based line number of this token.
  ANTLR3_INT32 position; // Zero-based position in the line.
  ANTLR3_MARKER index;   // The index of the token in the input.
  ANTLR3_UINT32 channel; // 0 for normally visible tokens. 99  for the hidden channel (whitespaces, comments).

  char *line_start;      // Pointer into the input to the beginning of the line where this token is located.
  char *start;           // Points to the start of the token in the input.
  char *stop;            // Points to the last character of the token.

  std::string text;      // The text of the token.
};

class MYSQL_PARSER_PUBLIC_FUNC MySQLScanner : public MySQLParsingBase
{
public:
  MySQLScanner(const char *text, size_t length, bool is_utf8, long server_version, 
    const std::string &sql_mode, const std::set<std::string> &charsets);
  virtual ~MySQLScanner();
  
  void reset();
  MySQLToken next_token();

protected:
  void setup();

private:
  class Private;
  Private *d;
};

#endif // _MYSQL_LEXER_H_
