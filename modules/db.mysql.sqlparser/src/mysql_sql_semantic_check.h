/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "mysql_sql_syntax_check.h"
#include "grtsqlparser/sql_semantic_check.h"

/** Implements DBMS specifics.
 *
 * @ingroup sqlparser
 */
class MYSQL_SQL_PARSER_PUBLIC_FUNC Mysql_sql_semantic_check : virtual protected Mysql_sql_syntax_check,
                                                              virtual public Sql_semantic_check {
public:
  typedef std::shared_ptr<Mysql_sql_semantic_check> Ref;
  static Ref create() {
    return Ref(new Mysql_sql_semantic_check());
  }

protected:
  Mysql_sql_semantic_check();

protected:
#ifndef _WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
  virtual Parse_result check_sql(const SqlAstNode *tree);
  virtual Parse_result check_trigger(const SqlAstNode *tree, const SqlAstNode *trigger_tail);
  virtual Parse_result check_view(const SqlAstNode *tree, const SqlAstNode *view_tail);
  virtual Parse_result check_routine(const SqlAstNode *tree, const SqlAstNode *routine_tail);
#ifndef _WIN32
#pragma GCC diagnostic pop
#endif

  class Null_state_keeper : Mysql_sql_syntax_check::Null_state_keeper {
  public:
    Null_state_keeper(Mysql_sql_semantic_check *sql_parser) : Mysql_sql_syntax_check::Null_state_keeper(sql_parser) {
    }
    ~Null_state_keeper();
  };
};
