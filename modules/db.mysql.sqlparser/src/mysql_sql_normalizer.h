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
#include "grtsqlparser/sql_normalizer.h"

using namespace grt;

/** Implements DBMS specifics.
 *
 * @ingroup sqlparser
 */
class MYSQL_SQL_PARSER_PUBLIC_FUNC Mysql_sql_normalizer : protected Mysql_sql_parser_base, public Sql_normalizer {
public:
  typedef std::shared_ptr<Mysql_sql_normalizer> Ref;
  static Ref create() {
    return Ref(new Mysql_sql_normalizer());
  }
  virtual ~Mysql_sql_normalizer() {
  }

protected:
  Mysql_sql_normalizer();

public:
  std::string normalize(const std::string &sql, const std::string &schema_name);

protected:
  // higher level
  int process_sql_statement(const SqlAstNode *tree);
  Parse_result process_create_statement(const SqlAstNode *tree);

  // parse tree core
  Parse_result process_create_schema_statement(const SqlAstNode *tree);
  Parse_result process_create_table_statement(const SqlAstNode *tree);
  Parse_result process_create_index_statement(const SqlAstNode *tree);
  Parse_result process_create_view_statement(const SqlAstNode *tree);
  Parse_result process_create_trigger_statement(const SqlAstNode *tree);
  Parse_result process_create_routine_statement(const SqlAstNode *tree);
  Parse_result process_create_server_link_statement(const SqlAstNode *tree);
  Parse_result process_create_tablespace_statement(const SqlAstNode *tree);
  Parse_result process_create_logfile_group_statement(const SqlAstNode *tree);
  Parse_result process_insert_statement(const SqlAstNode *tree);

  // parse tree helpers
  void qualify_obj_ident(const SqlAstNode *sp_name);
  void append_stmt_to_script(const std::string &stmt);

  // context
  std::string _schema_name;
  std::string _norm_stmt;
  std::string _norm_script;
  std::string _common_sql;
  int _cut_sym_count;

  class Null_state_keeper : Mysql_sql_parser_base::Null_state_keeper {
  public:
    Null_state_keeper(Mysql_sql_normalizer *sql_parser)
      : Mysql_sql_parser_base::Null_state_keeper(sql_parser), _sql_parser(sql_parser) {
    }
    ~Null_state_keeper() {
      _sql_parser->_schema_name = std::string();
      _sql_parser->_norm_stmt = std::string();
      _sql_parser->_norm_script = std::string();
      _sql_parser->_common_sql = std::string();
    }

  private:
    Mysql_sql_normalizer *_sql_parser;
  };
  friend class Null_state_keeper;
};
