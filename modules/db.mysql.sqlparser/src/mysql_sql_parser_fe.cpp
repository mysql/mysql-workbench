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

#include <glib.h>
#include <cctype>

#if defined(_MSC_VER)
#include <windows.h>
#endif

#include <sstream>
#include <vector>
#include <list>
#include <memory>
#include <boost/scoped_array.hpp>
#include <boost/optional.hpp>
#include <boost/cstdint.hpp>

#include "grtdb/charset_utils.h"
#include "base/string_utilities.h"
#include "base/threading.h"

#include "mysql_sql_parser_fe.h"
#include "myx_statement_parser.h"

#define MYSQL_LEX 1
#define MYSQL_SERVER

extern "C" int pthread_dummy(int) {
  return 0;
}

typedef void *YYSTYPE;
#define YYSTYPE_IS_DECLARED

// to stop complaint from compiler about a certain ATTRIBUTE_FORMAT in my_dbug.h
#define DBUG_OFF 1

#include "myx_lex_helpers.h"
#include "sql_lex.h"
#include "my_sys.h"
#include "myx_sql_parser.tab.hh"

const char *MYSQL_DEFAULT_CHARSET = "utf8_bin";

namespace mysql_parser {
  CHARSET_INFO *get_sql_charset_by_name(const char *cs_name, int /*myf*/ flags) {
    return get_charset_by_name(cs_name, flags);
  }
}

struct Context {
  Mysql_sql_parser_fe *sql_parser_fe;
  Mysql_sql_parser_fe::fe_process_sql_statement_callback cb;
  void *data;
  int err_count;
  bool ignore_dml;
  bool is_ast_generation_enabled;
  size_t max_insert_statement_size;
  bool processing_create_statements;
  bool processing_alter_statements;
  bool processing_drop_statements;
  Mysql_sql_parser_fe::SqlMode sql_mode;
};

class Lex_helper {
public:
  static CHARSET_INFO *charset() {
    return get_charset_by_name(MYSQL_DEFAULT_CHARSET, MYF(0));
  }
  Lex_helper(const char *statement, const Mysql_sql_parser_fe::SqlMode &sql_mode, bool is_ast_generation_enabled) {
    lex_start(&_lex, reinterpret_cast<const unsigned char *>(statement), (unsigned int)strlen(statement));
    _lex.first_item = NULL;
    _lex.last_item = NULL;
    _lex.charset = charset();
    lex_args.arg1 = &_yystype;
    lex_args.arg2 = &_lex;
    myx_set_parser_source(statement);

    _lex.sql_mode.MODE_ANSI_QUOTES = sql_mode.MODE_ANSI_QUOTES;
    _lex.sql_mode.MODE_HIGH_NOT_PRECEDENCE = sql_mode.MODE_HIGH_NOT_PRECEDENCE;
    _lex.sql_mode.MODE_PIPES_AS_CONCAT = sql_mode.MODE_PIPES_AS_CONCAT;
    _lex.sql_mode.MODE_NO_BACKSLASH_ESCAPES = sql_mode.MODE_NO_BACKSLASH_ESCAPES;
    _lex.sql_mode.MODE_IGNORE_SPACE = sql_mode.MODE_IGNORE_SPACE;
    _lex.ignore_space = _lex.sql_mode.MODE_IGNORE_SPACE;

    SqlAstStatics::sql_statement(statement);
    SqlAstStatics::is_ast_generation_enabled = is_ast_generation_enabled != 0;
  }
  ~Lex_helper() {
    myx_free_parser_source();
  }
  LEX *lex() {
    return &_lex;
  }

private:
  YYSTYPE _yystype;
  LEX _lex;
};

#define LEX_HELPER(statement, sql_mode, is_ast_generation_enabled) \
  Lex_helper _lex_helper(statement, sql_mode, is_ast_generation_enabled);

std::string get_first_sql_token(const char *statement, Mysql_sql_parser_fe::SqlMode sql_mode, int *first_token_pos) {
  LEX_HELPER(statement, sql_mode, true)

  void *token = NULL;
  yylex(&token);
  if (token) {
    const SqlAstNode *item = static_cast<const SqlAstNode *>(token);
    if (item && item->value_length()) {
      *first_token_pos = item->stmt_boffset();
      return base::toupper(item->value());
    }
  }

  *first_token_pos = -1;
  return "";
}

bool is_statement_relevant(const char *statement, const Context *context) {
  int first_token_pos;
  std::string token = get_first_sql_token(statement, context->sql_mode, &first_token_pos);

  bool relevant = (("USE" == token) || ("BEGIN" == token));

  bool irrelevant =
    ((!context->processing_create_statements && ("CREATE" == token)) ||
     (!context->processing_alter_statements && ("ALTER" == token)) ||
     (!context->processing_drop_statements && ("DROP" == token)) || (context->ignore_dml && ("SELECT" == token)) ||
     (context->ignore_dml && ("INSERT" == token)) ||
     (context->is_ast_generation_enabled && ("INSERT" == token) && (context->max_insert_statement_size != 0) &&
      (strlen(statement) > context->max_insert_statement_size)) ||
     (context->ignore_dml && ("DELETE" == token)) || (context->ignore_dml && ("UPDATE" == token)));
  return (relevant || !irrelevant);
}

// this function removes comment braces of form /*!NUMBER */
// making their contents a part of the query itself

void remove_versioning_comments(const std::string &sql, std::string &effective_sql, CHARSET_INFO *cs,
                                bool *ignore_statement, int *first_versioning_comment_pos) {
  *first_versioning_comment_pos = -1;

  const char *begin = sql.c_str();
  const char *ptr = begin;
  const char *endptr = ptr + sql.length();

  while (1) {
    for (; (ptr < endptr) && (*ptr != '/'); ptr += max(my_mbcharlen(cs, *ptr), 1))
      ;

    if ((ptr + 3) >= endptr)
      break;

    if ((ptr[1] != '*') || (ptr[2] != '!')) {
      ptr += my_mbcharlen(cs, *ptr);
      continue;
    }

    const char *start_start = ptr;

    int digit_count = 0;
    for (ptr = ptr + 3; (ptr < endptr) && (my_isdigit(cs, *ptr)); ptr += max(my_mbcharlen(cs, *ptr), 1), digit_count++)
      ;

    if (digit_count == 0)
      continue;

    const char *start_end = ptr;

    // in case of parsing mysqldump files, there are 'create table' statements embraced with comments.
    // they are to be ignored, because they relate to views, not tables.
    if (ignore_statement)
      *ignore_statement = (0 == strncmp(ptr, " CREATE TABLE", 13));

    {
      bool quoted = false;
      bool escaped = false;
      bool commented = false;
      int nested_comments_count = 1;
      char quot_sym = 0;

      for (; ptr < endptr - 1; ++ptr) {
        escaped = (!commented && quoted && ('\\' == ptr[0])) ? !escaped : false;

        switch (ptr[0]) {
          case '#':
            if ((1 == nested_comments_count) && !quoted)
              commented = true;
            break;

          case '\r':
          case '\n':
            commented = false;
            break;

          case '/':
            if (!commented && !quoted && ('*' == ptr[1]))
              ++nested_comments_count;
            break;

          case '*':
            if (!commented && !quoted && ('/' == ptr[1])) {
              if (0 == --nested_comments_count)
                goto end;
            }
            break;

          case '"':
          case '\'':
            if (!escaped && !commented) {
              if (quoted) {
                if (ptr[0] == quot_sym) {
                  quoted = false;
                  quot_sym = 0;
                }
              } else {
                quoted = true;
                quot_sym = ptr[0];
              }
            }
            break;
        }
      }
    end:;
    }

    if (ptr >= endptr)
      break;

    if (effective_sql.empty()) {
      *first_versioning_comment_pos = (int)(start_start - begin);
      effective_sql = sql;
    }

    // replace comments in-place with spaces
    effective_sql.replace(start_start - begin, start_end - start_start, start_end - start_start, ' ');
    effective_sql.replace(ptr - begin, 2, 2, ' ');

    ptr += 2;
  }
}

Mysql_sql_parser_fe::SqlMode::SqlMode() {
  reset();
}

void Mysql_sql_parser_fe::SqlMode::reset() {
  MODE_ANSI_QUOTES = false;
  MODE_HIGH_NOT_PRECEDENCE = false;
  MODE_PIPES_AS_CONCAT = false;
  MODE_NO_BACKSLASH_ESCAPES = false;
  MODE_IGNORE_SPACE = false;
}

void Mysql_sql_parser_fe::SqlMode::parse(const std::string &text_value) {
  reset();

  std::string sql_mode_string = base::toupper(text_value);
  std::istringstream iss(sql_mode_string);
  std::string mode;
  while (std::getline(iss, mode, ',')) {
    if (mode == "ANSI" || mode == "DB2" || mode == "MSSQL" || mode == "ORACLE" || mode == "POSTGRESQL") {
      MODE_ANSI_QUOTES = true;
      MODE_PIPES_AS_CONCAT = true;
      MODE_IGNORE_SPACE = true;
    } else if (mode == "ANSI_QUOTES")
      MODE_ANSI_QUOTES = true;
    else if (mode == "PIPES_AS_CONCAT")
      MODE_PIPES_AS_CONCAT = true;
    else if (mode == "NO_BACKSLASH_ESCAPES")
      MODE_NO_BACKSLASH_ESCAPES = true;
    else if (mode == "IGNORE_SPACE")
      MODE_IGNORE_SPACE = true;
  }
}

Mysql_sql_parser_fe::Mysql_sql_parser_fe(const std::string &sql_mode_)
  : ignore_dml(true),
    max_insert_statement_size(0),
    processing_create_statements(true),
    processing_alter_statements(true),
    processing_drop_statements(true),
    is_ast_generation_enabled(true),
    max_err_count(-1) {
  sql_mode.parse(sql_mode_);
}

std::shared_ptr<base::Mutex> _parser_fe_critical_section(new base::Mutex);

void Mysql_sql_parser_fe::reset() {
  SqlAstStatics::tree(NULL);
  ::parser_is_stopped = false;

  static bool initialized = false;
  if (!initialized) {
    initialized = true;
    lex_init();
  }
}

int Mysql_sql_parser_fe::stop() {
  return ::parser_is_stopped = true;
}

void Mysql_sql_parser_fe::parse_sql_mode(const std::string &sql_mode_string) {
  sql_mode.parse(sql_mode_string);
}

int Mysql_sql_parser_fe::parse_sql_script(const char *sql, fe_process_sql_statement_callback cb, void *user_data) {
  base::MutexLock parser_fe_critical_section(*_parser_fe_critical_section);
  reset();
  Context context = {this,
                     cb,
                     user_data,
                     0,
                     ignore_dml,
                     is_ast_generation_enabled,
                     max_insert_statement_size,
                     processing_create_statements,
                     processing_alter_statements,
                     processing_drop_statements,
                     sql_mode};
  myx_process_sql_statements(sql, Lex_helper::charset(), &process_sql_statement_cb, &context, MYX_SPM_NORMAL_MODE);
  return context.err_count;
}

int Mysql_sql_parser_fe::parse_sql_script_file(const std::string &filename, fe_process_sql_statement_callback cb,
                                               void *user_data) {
  base::MutexLock parser_fe_critical_section(*_parser_fe_critical_section);
  reset();
  Context context = {this,
                     cb,
                     user_data,
                     0,
                     ignore_dml,
                     is_ast_generation_enabled,
                     max_insert_statement_size,
                     processing_create_statements,
                     processing_alter_statements,
                     processing_drop_statements,
                     sql_mode};
  myx_process_sql_statements_from_file(filename.c_str(), Lex_helper::charset(), &process_sql_statement_cb, &context,
                                       MYX_SPM_NORMAL_MODE /*MYX_SPM_DELIMS_REQUIRED*/);
  return context.err_count;
}

int Mysql_sql_parser_fe::escape_string(const std::string &in_text, std::string &out_text) {
  boost::scoped_array<char> out(new char[in_text.size() * 2 + 1]);
  int res = escape_string(out.get(), 0, in_text.c_str(), (unsigned long)in_text.size());
  out_text = out.get();
  return res;
}

int Mysql_sql_parser_fe::escape_string(char *out, unsigned long out_size, const char *in, unsigned long in_size) {
  static CHARSET_INFO *cs = get_charset_by_name(MYSQL_DEFAULT_CHARSET, MYF(0));
  return (int)mysql_parser::escape_string_for_mysql(cs, out, out_size, in, in_size);
}

int Mysql_sql_parser_fe::process_sql_statement_cb(const MyxStatementParser *splitter, const char *statement,
                                                  void *context_ptr) {
  // possible values for result:
  // -1 - statement was ignored
  // 0 - statement was successfully processed
  // 1 - error occurred during statement processing
  int result = 0;

  if (::parser_is_stopped)
    return -1;

  Context *context = reinterpret_cast<Context *>(context_ptr);

  if (!context || !context->cb)
    return -1;

  // check if statement is in utf8 encoding
  if (!g_utf8_validate(statement, -1, NULL)) {
    int stmt_lc = 1;
    {
      const char *c = statement - 1;
      while (c) {
        if (base::EolHelpers::is_eol(++c))
          ++stmt_lc;
        else
          c = NULL;
      }
    }
    std::string err_msg = "SQL statement starting from pointed line contains non UTF8 characters";
    context->cb(context->data, splitter, statement, NULL, 0, 0, stmt_lc, 0, stmt_lc, 0, 0, err_msg);
    context->err_count++;
    return 1;
  }

  // stripe comments before further statement processing because
  // mysqldump puts the whole DDL in comments e.g. for triggers
  std::string orig_sql(statement);
  std::string effective_sql;
  bool ignore_statement = false;
  int first_versioning_comment_pos;
  remove_versioning_comments(orig_sql, effective_sql, Lex_helper::charset(), &ignore_statement,
                             &first_versioning_comment_pos);
  const std::string &sql = effective_sql.empty() ? orig_sql : effective_sql;

  // filter inappropriate statements
  if (ignore_statement || !is_statement_relevant(sql.c_str(), context))
    return -1; // ignored

  // parse/generate AST
  LEX_HELPER(sql.c_str(), context->sql_mode, context->is_ast_generation_enabled)
  myx_parse();
  const SqlAstNode *tree = SqlAstStatics::tree();

  // in case of syntax error extend err message with context
  std::string err_msg = myx_get_err_msg();
  int err_tok_line_pos = 0;
  int err_tok_len = 0;
  int err_tok_lineno = _lex_helper.lex()->yylineno;
  if (!tree) {
    if (err_msg.empty()) {
      if (!_lex_helper.lex()->last_item || (_lex_helper.lex()->first_item->value_length() == -1)) {
        // empty statement
        return result;
      }
    } else if ("syntax error" == err_msg) {
      // Simple style error messages.
      if (const SqlAstNode *item = _lex_helper.lex()->last_item) {
        static const size_t MAX_SQL_CONTEXT_SIZE = 80;
        std::string statement_ = statement;
        size_t boffset = item->stmt_boffset();
        std::string err_context =
          statement_.substr(boffset, std::min<size_t>(statement_.size() - boffset, MAX_SQL_CONTEXT_SIZE));
        err_msg.clear();
        err_msg.append("SQL syntax error near '").append(err_context).append("'");

        determine_token_position(item, splitter, statement, err_tok_lineno, err_tok_line_pos, err_tok_len);
      }
    } else {
      // General error message style.
      if (const SqlAstNode *item = _lex_helper.lex()->last_item)
        determine_token_position(item, splitter, statement, err_tok_lineno, err_tok_line_pos, err_tok_len);
    }
  }

  int stmt_begin_lineno = -1;
  int stmt_begin_line_pos = -1;
  if (const SqlAstNode *first_item = _lex_helper.lex()->first_item) {
    stmt_begin_lineno = first_item->stmt_lineno();
    stmt_begin_line_pos = 0;
    int tok_len = 0;
    determine_token_position(first_item, splitter, statement, stmt_begin_lineno, stmt_begin_line_pos, tok_len);
  }

  int stmt_end_lineno = -1;
  int stmt_end_line_pos = -1;
  if (const SqlAstNode *last_item = _lex_helper.lex()->last_item) {
    stmt_end_lineno = last_item->stmt_lineno();
    stmt_end_line_pos = 0;
    int tok_len = 0;
    bool is_tok_multiline = false;
    int alt_stmt_end_line_pos = 0;

    determine_token_position(last_item, splitter, statement, stmt_end_lineno, stmt_end_line_pos, tok_len);

    for (const char *c = (statement + last_item->stmt_boffset()),
                    *end = (statement + last_item->stmt_boffset() + tok_len);
         c < end; ++c) {
      if (base::EolHelpers::is_eol(c)) {
        ++stmt_end_lineno;
        // in case of multi-line token the line position of its end needs adjustment
        is_tok_multiline = true;
        alt_stmt_end_line_pos = 0;
      } else {
        ++alt_stmt_end_line_pos;
      }
    }
    if (is_tok_multiline)
      stmt_end_line_pos = alt_stmt_end_line_pos;
    else
      stmt_end_line_pos += tok_len;

    // closing quote possible which must be included into the range
    int statement_non_token_ending_len = 0;
    switch (*(statement + last_item->stmt_boffset() + tok_len)) {
      case '\'':
      case '"':
      case '`':
        ++statement_non_token_ending_len;
        break;
    }
    stmt_end_line_pos += statement_non_token_ending_len;
  }

  // call callback function to process generated AST or syntax error
  result = context->cb(context->data, splitter, orig_sql.c_str(), tree, stmt_begin_lineno, stmt_begin_line_pos,
                       stmt_end_lineno, stmt_end_line_pos, err_tok_lineno, err_tok_line_pos, err_tok_len, err_msg);
  if (0 != result)
    context->err_count++;

  if ((context->sql_parser_fe->max_err_count > 0) && (context->sql_parser_fe->max_err_count <= context->err_count))
    context->sql_parser_fe->stop();

  return result;
}

void Mysql_sql_parser_fe::determine_token_position(const SqlAstNode *item, const MyxStatementParser *splitter,
                                                   const char *statement, int &lineno, int &token_line_pos,
                                                   int &token_len) {
  lineno = item->stmt_lineno();
  const char *tokenbeg = statement + item->stmt_boffset();
  const char *tokenend = statement + item->stmt_eoffset();

  // fix token end
  {
    bool initial_token_end_was_eol = false;

    if (std::isspace((unsigned char)*(tokenend - 1))) {
      // this is a case when token end is set to the beginning of the subsequent token
      --tokenend;
    } else if (base::EolHelpers::is_eol(tokenend)) {
      // if initial token end is set to EOL then this EOL doesn't change effective token lineno
      --tokenend;
      initial_token_end_was_eol = true;
    }

    if (std::isspace((unsigned char)*tokenend) && (tokenend - 1 > tokenbeg)) {
      // there can be multiple trailing token delimiters
      while (std::isspace((unsigned char)*tokenend) && (tokenend > tokenbeg))
        --tokenend;
      ++tokenend;
    } else if (initial_token_end_was_eol)
      ++tokenend;
  }

  // find beginning of the line
  const char *linebeg = tokenbeg;
  for (; (linebeg > statement) && ('\n' != *linebeg) && ('\r' != *linebeg); --linebeg) {
  }
  if (('\n' == *linebeg) || ('\r' == *linebeg))
    ++linebeg;

  static CHARSET_INFO *cs = get_charset_by_name(MYSQL_DEFAULT_CHARSET, MYF(0));

  // translate boffset/eoffset into position within the line
  // taking into account active encoding
  const char *ptr = linebeg;
  while (ptr < tokenbeg) {
    ptr += max(my_mbcharlen(cs, *ptr), 1);
    ++token_line_pos;
  }

  // calc token length
  // taking into account active encoding
  while (ptr < tokenend) {
    ++token_len;
    ptr += max(my_mbcharlen(cs, *ptr), 1);
  }

  // first line is special because it may contain part of previous statement ending with statements delimiter
  // while token position is specified relative to current statement
  if (1 == lineno)
    token_line_pos += splitter->statement_first_line_first_symbol_pos();
}

std::string Mysql_sql_parser_fe::get_first_sql_token(const std::string &sql,
                                                     const std::string &versioning_comment_subst_token) {
  base::MutexLock parser_fe_critical_section(*_parser_fe_critical_section);
  reset();
  static Mysql_sql_parser_fe::SqlMode sql_mode;

  std::string effective_sql;
  bool ignore_statement = false;
  int first_versioning_comment_pos;
  remove_versioning_comments(sql, effective_sql, Lex_helper::charset(), &ignore_statement,
                             &first_versioning_comment_pos);
  const std::string &sql_ = effective_sql.empty() ? sql : effective_sql;

  int first_token_pos;
  std::string token = ::get_first_sql_token(sql_.c_str(), sql_mode, &first_token_pos);

  if ((first_versioning_comment_pos > -1) && (first_token_pos > -1) &&
      (first_versioning_comment_pos < first_token_pos) && !versioning_comment_subst_token.empty())
    return versioning_comment_subst_token;
  else
    return token;
}
