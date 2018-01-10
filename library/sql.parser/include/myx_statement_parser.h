/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#ifndef statement_parser_h
#define statement_parser_h

#include "mysql_sql_parser_public_interface.h"
#include "myx_sql_parser_public_interface.h"
#include <string>

namespace mysql_parser
{

typedef struct charset_info_st CHARSET_INFO;

class MYSQL_SQL_PARSER_PUBLIC_FUNC MyxStatementParser
{
  static const int CHAR_BUFFER_SIZE= 32768; // required to be >= 4

  enum ParserState { start, stmt, str, comment1, comment2, mlcomment, delimtok, delimkwd, eos };

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif

  std::string delim;
  CHARSET_INFO *cs;
  
  char *char_buffer;
  char *char_buffer_b;
  char *char_buffer_e;
  bool eof_hit;
  int _stmt_boffset;
  int _stmt_first_line_first_symbol_pos;
  int _symbols_since_newline;
  int _total_lc;

#ifdef _MSC_VER
#pragma warning(pop)
#endif

  int fill_buffer(std::istream& is);
  int buffer_eof(std::istream& is);

  int get_next_char(std::istream& is, int *len, int count_lines= 1);
  int peek_next_char(std::istream& is, int *len);
  void add_char_to_buffer(std::string& buffer, int c, int len) const;

public:
  MyxStatementParser(CHARSET_INFO *charset);
  virtual ~MyxStatementParser();

  void process(std::istream& is, process_sql_statement_callback, void *arg, int mode);
  const std::string & delimiter() const { return delim; }
  int statement_boffset() const { return _stmt_boffset; }
  int statement_first_line_first_symbol_pos() const { return _stmt_first_line_first_symbol_pos; }
  int total_line_count() const { return _total_lc; }
};

} // namespace mysql_parser

#endif // _STATEMENT_PARSER_H_
