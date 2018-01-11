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

#ifndef _MYSQL_SQL_PARSER_BASE_H_
#define _MYSQL_SQL_PARSER_BASE_H_

#include "mysql_sql_parser_public_interface.h"
#include "grtsqlparser/sql_parser_base.h"
#include "grts/structs.db.mysql.h"

class Mysql_sql_parser_fe;
namespace mysql_parser {
  class MyxStatementParser;
  class SqlAstNode;
}
using namespace mysql_parser;

#ifdef _WIN32
#pragma warning(disable : 4250)
// warning C4250: 'Mysql_sql_parser' : inherits 'Mysql_sql_parser_base::Mysql_sql_parser_base::total_line_count' via
// dominance
#endif

/** Provides DBMS specific functionality for processing of SQL statements/scripts.
 *
 * @ingroup sqlparser
 */
class MYSQL_SQL_PARSER_PUBLIC_FUNC Mysql_sql_parser_base : virtual public Sql_parser_base {
private:
  static int process_sql_statement(void *sql_parser_ptr, const MyxStatementParser *splitter, const char *sql_statement,
                                   const SqlAstNode *tree, int stmt_begin_lineno, int stmt_begin_line_pos,
                                   int stmt_end_lineno, int stmt_end_line_pos, int err_tok_lineno, int err_tok_line_pos,
                                   int err_tok_len, const std::string &err_msg);
  friend int process_sql_statement(void *, const MyxStatementParser *, const char *, const SqlAstNode *, int,
                                   const std::string &);

protected:
  Mysql_sql_parser_base();
  virtual ~Mysql_sql_parser_base() {
  }

  virtual void sql_mode(const std::string &value);

  // prepare/clear routines
  void set_options(const grt::DictRef &options);

  // parse tree helpers
  void process_obj_full_name_item(const SqlAstNode *item, std::string &schema_name, std::string &obj_name);

  // error reporting
  void report_semantic_error(const SqlAstNode *item, const std::string &err_msg, int entry_type);

  // misc
  virtual int total_line_count();

  // aux types
  typedef boost::function<int(const SqlAstNode *)> Process_sql_statement;

  // basic functionality
  int parse_sql_script(Mysql_sql_parser_fe &sql_parser_fe, const char *sql);
  int parse_sql_script_file(Mysql_sql_parser_fe &sql_parser_fe, const std::string &filename);

  // data members
  std::string _non_std_sql_delimiter;
  const MyxStatementParser *_splitter;
  Process_sql_statement _process_sql_statement;
  db_mysql_CatalogRef _catalog;
  db_mysql_SchemaRef _active_schema;
  int _stmt_begin_lineno;
  int _stmt_begin_line_pos;
  int _stmt_end_lineno;
  int _stmt_end_line_pos;
  int _err_tok_lineno;
  int _err_tok_line_pos;
  int _err_tok_len;
  std::string _err_msg;
  bool _override_sql_mode;
  std::string _sql_mode;

  bool on_stop(Mysql_sql_parser_fe *sql_parser_fe);

  class Null_state_keeper : public Sql_parser_base::Null_state_keeper {
  public:
    Null_state_keeper(Mysql_sql_parser_base *sql_parser)
      : Sql_parser_base::Null_state_keeper(sql_parser), _sql_parser(sql_parser) {
    }
    virtual ~Null_state_keeper();

  private:
    Mysql_sql_parser_base *_sql_parser;
  };
  friend class Null_state_keeper;
};

#endif // _MYSQL_SQL_PARSER_BASE_H_
