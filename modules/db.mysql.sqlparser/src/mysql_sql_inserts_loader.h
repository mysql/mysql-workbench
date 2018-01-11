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
#include "grtsqlparser/sql_inserts_loader.h"

/** Implements DBMS specifics.
 *
 * @ingroup sqlparser
 */
class MYSQL_SQL_PARSER_PUBLIC_FUNC Mysql_sql_inserts_loader : protected Mysql_sql_parser_base,
                                                              public Sql_inserts_loader {
public:
  typedef std::shared_ptr<Mysql_sql_inserts_loader> Ref;
  static Ref create() {
    return Ref(new Mysql_sql_inserts_loader());
  }
  virtual ~Mysql_sql_inserts_loader() {
  }

protected:
  Mysql_sql_inserts_loader();

public:
  void load(const std::string &sql, const std::string &schema_name);

protected:
  // higher level
  int process_sql_statement(const SqlAstNode *tree);

  // parse tree core
  Parse_result process_insert_statement(const SqlAstNode *tree);

  // context
  std::string _schema_name;

  class Null_state_keeper : Mysql_sql_parser_base::Null_state_keeper {
  public:
    Null_state_keeper(Mysql_sql_inserts_loader *sql_parser)
      : Mysql_sql_parser_base::Null_state_keeper(sql_parser), _sql_parser(sql_parser) {
    }
    ~Null_state_keeper() {
      _sql_parser->_schema_name = std::string();
    }

  private:
    Mysql_sql_inserts_loader *_sql_parser;
  };
  friend class Null_state_keeper;
};
