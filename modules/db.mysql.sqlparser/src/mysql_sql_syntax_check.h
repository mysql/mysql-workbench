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

#pragma once

#include "mysql_sql_parser_base.h"
#include "grtsqlparser/sql_syntax_check.h"

/** Implements DBMS specifics.
 *
 * @ingroup sqlparser
 */
class MYSQL_SQL_PARSER_PUBLIC_FUNC Mysql_sql_syntax_check : virtual protected Mysql_sql_parser_base,
                                                            virtual public Sql_syntax_check {
public:
  typedef std::shared_ptr<Mysql_sql_syntax_check> Ref;
  static Ref create() {
    return Ref(new Mysql_sql_syntax_check());
  }
  virtual ~Mysql_sql_syntax_check() {
  }

protected:
  Mysql_sql_syntax_check();

public:
  Statement_type determine_statement_type(const std::string &sql);

public:
  int check_sql(const char *sql);
  int check_trigger(const char *sql);
  int check_view(const char *sql);
  int check_routine(const char *sql);

protected:
  typedef boost::function<Parse_result(const SqlAstNode *)> Check_sql_statement;
  Check_sql_statement _check_sql_statement;

  int process_sql_statement(const SqlAstNode *tree, ObjectType object_type);
  int check_sql_statement(const char *sql, Check_sql_statement check_sql_statement, ObjectType object_type);

  Parse_result do_check_sql(const SqlAstNode *tree);
  Parse_result do_check_trigger(const SqlAstNode *tree);
  Parse_result do_check_view(const SqlAstNode *tree);
  Parse_result do_check_routine(const SqlAstNode *tree);

  virtual Parse_result check_sql(const SqlAstNode *tree);
  virtual Parse_result check_trigger(const SqlAstNode *tree, const SqlAstNode *trigger_tail);
  virtual Parse_result check_view(const SqlAstNode *tree, const SqlAstNode *view_tail);
  virtual Parse_result check_routine(const SqlAstNode *tree, const SqlAstNode *routine_tail);

  bool _use_delimiter;

  class Null_state_keeper : Mysql_sql_parser_base::Null_state_keeper {
  public:
    Null_state_keeper(Mysql_sql_syntax_check *sql_parser)
      : Mysql_sql_parser_base::Null_state_keeper(sql_parser), _sql_parser(sql_parser) {
    }
    ~Null_state_keeper();

  private:
    Mysql_sql_syntax_check *_sql_parser;
  };
};
