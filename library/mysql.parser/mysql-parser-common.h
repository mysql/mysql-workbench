/* 
 * Copyright (c) 2013, 2015, Oracle and/or its affiliates. All rights reserved.
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
  #ifdef MYSQL_PARSER_EXPORTS
    #define MYSQL_PARSER_PUBLIC_FUNC __declspec(dllexport)
  #else
    #define MYSQL_PARSER_PUBLIC_FUNC __declspec(dllimport)
  #endif
#else
  #define MYSQL_PARSER_PUBLIC_FUNC
#endif

#include <set>
#include <antlr3.h>
#include <string>
#include <vector>

#include "mysql-recognition-types.h"

extern "C" {
  ANTLR3_UINT32 check_charset(void *payload, pANTLR3_STRING text);
  ANTLR3_UINT32 check_null(pANTLR3_STRING text);
}

class MYSQL_PARSER_PUBLIC_FUNC MySQLRecognitionBase
{
public:
  MySQLRecognitionBase(const std::set<std::string> &charsets);
  virtual ~MySQLRecognitionBase() {};

  // Internal function called by static callback.
  void add_error(const std::string &message, ANTLR3_UINT32 token, ANTLR3_MARKER token_start,
    ANTLR3_UINT32 line, ANTLR3_UINT32 offset_in_line, ANTLR3_MARKER length);

  const std::vector<MySQLParserErrorInfo> &error_info();
  bool has_errors();
  unsigned sql_mode();
  virtual void set_sql_mode(const std::string &sql_mode);
  virtual std::string text() = 0;
  virtual const char* lineStart() = 0;

  bool is_charset(const std::string &s);
  bool is_identifier(ANTLR3_UINT32 type);

  std::string token_text(pANTLR3_BASE_TREE node, bool keepQuotes = false);
  uint32_t get_keyword_token(const std::string &keyword);
  char** get_token_list();

  static bool is_keyword(ANTLR3_UINT32 type);
  static bool is_relation(ANTLR3_UINT32 type);
  static bool is_number(ANTLR3_UINT32 type);
  static bool is_operator(ANTLR3_UINT32 type);
  static bool is_subtree(struct ANTLR3_BASE_TREE_struct *tree);

protected:
  virtual void reset();

private:
  class Private;
  Private *d;
};
