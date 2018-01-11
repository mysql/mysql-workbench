/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MYSQL_SQL_PARSER_FE_H_
#define _MYSQL_SQL_PARSER_FE_H_

#include "myx_sql_tree_item.h"
#include "mysql_sql_parser_public_interface.h"

using namespace mysql_parser;

namespace mysql_parser {
  MYSQL_SQL_PARSER_PUBLIC_FUNC CHARSET_INFO *get_sql_charset_by_name(const char *cs_name, int /*myf*/ flags);
}

/** Splits provided SQL script into statements and parses them in turn providing generated AST to given callback.
 *
 * @ingroup sqlparser
 */
class MYSQL_SQL_PARSER_PUBLIC_FUNC Mysql_sql_parser_fe {
public:
  static int escape_string(const std::string &in_text, std::string &out_text);
  static int escape_string(char *out, unsigned long out_size, const char *in, unsigned long in_size);

public:
  Mysql_sql_parser_fe(const std::string &sql_mode_);

  int stop();
  typedef int (*fe_process_sql_statement_callback)(void *user_data, const MyxStatementParser *splitter, const char *sql,
                                                   const SqlAstNode *tree, int stmt_begin_lineno,
                                                   int stmt_begin_line_pos, int stmt_end_lineno, int stmt_end_line_pos,
                                                   int err_tok_lineno, int err_tok_line_pos, int err_tok_len,
                                                   const std::string &err_msg);
  int parse_sql_script(const char *sql, fe_process_sql_statement_callback cb, void *user_data);
  int parse_sql_script_file(const std::string &filename, fe_process_sql_statement_callback cb, void *user_data);
  std::string get_first_sql_token(const std::string &sql, const std::string &versioning_comment_subst_token = "");

public:
  bool ignore_dml;
  size_t max_insert_statement_size;
  bool processing_create_statements;
  bool processing_alter_statements;
  bool processing_drop_statements;
  struct MYSQL_SQL_PARSER_PUBLIC_FUNC SqlMode {
    SqlMode();
    void reset();
    void parse(const std::string &text_value);

    bool MODE_ANSI_QUOTES;
    bool MODE_HIGH_NOT_PRECEDENCE;
    bool MODE_PIPES_AS_CONCAT;
    bool MODE_NO_BACKSLASH_ESCAPES;
    bool MODE_IGNORE_SPACE;
  };
  SqlMode sql_mode;
  void parse_sql_mode(const std::string &sql_mode_string);

public:
  bool is_ast_generation_enabled;

public:
  static void determine_token_position(const SqlAstNode *item, const MyxStatementParser *splitter,
                                       const char *statement, int &lineno, int &token_line_pos, int &token_len);

private:
  static int process_sql_statement_cb(const MyxStatementParser *splitter, const char *sql, void *context);
  void reset();

public:
  int max_err_count;
};

#endif // _MYSQL_SQL_PARSER_FE_H_
