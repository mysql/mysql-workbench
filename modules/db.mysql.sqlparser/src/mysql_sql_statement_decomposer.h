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
#include "grtsqlparser/sql_statement_decomposer.h"

/** Implements DBMS specifics.
 *
 * @ingroup sqlparser
 */
class MYSQL_SQL_PARSER_PUBLIC_FUNC Mysql_sql_statement_decomposer : protected Mysql_sql_parser_base,
                                                                    public Sql_statement_decomposer {
public:
  typedef std::shared_ptr<Mysql_sql_statement_decomposer> Ref;
  static Ref create(grt::DictRef db_opts = grt::DictRef()) {
    Ref decomposer(new Mysql_sql_statement_decomposer);
    decomposer->set_options(db_opts);
    return decomposer;
  }
  virtual ~Mysql_sql_statement_decomposer() {
  }

protected:
  Mysql_sql_statement_decomposer();
  void set_options(const grt::DictRef &opts);
  int decompose_query(const std::string &sql, SelectStatement::Ref select_statement);
  int decompose_view(const std::string &ddl, SelectStatement::Ref select_statement);
  int decompose_view(db_ViewRef view, SelectStatement::Ref select_statement);

protected:
  typedef boost::function<Parse_result(const SqlAstNode *)> ProcessSqlStatement;
  int process_sql_statement(const std::string &sql, SelectStatement::Ref select_statement,
                            ProcessSqlStatement do_process_sql_statement_cb);
  int process_sql_statement(const std::string &sql, SelectStatement::Ref select_statement,
                            Mysql_sql_parser_fe &sql_parser_fe);
  int do_process_sql_statement(const SqlAstNode *tree);
  ProcessSqlStatement _do_process_sql_statement;

protected:
  Parse_result decompose_query(const SqlAstNode *select_init);
  Parse_result do_decompose_query(const SqlAstNode *tree);
  SelectStatement::Ref _select_statement;

protected:
  Parse_result do_decompose_view(const SqlAstNode *tree);
  void expand_wildcards(SelectStatement::Ref select_statement, db_SchemaRef &db_schema,
                        grt::ListRef<db_Schema> &db_schemata);
  std::list<std::string> _view_columns_names;

protected:
  class Null_state_keeper : Mysql_sql_parser_base::Null_state_keeper {
  public:
    Null_state_keeper(Mysql_sql_statement_decomposer *sql_parser)
      : Mysql_sql_parser_base::Null_state_keeper(sql_parser), _sql_parser(sql_parser) {
    }
    ~Null_state_keeper();

  private:
    Mysql_sql_statement_decomposer *_sql_parser;
  };
};
