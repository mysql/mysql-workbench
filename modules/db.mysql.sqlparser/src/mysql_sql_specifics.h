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
#include "grtsqlparser/sql_specifics.h"

/** Implements DBMS specifics.
 *
 * @ingroup sqlparser
 */
class MYSQL_SQL_PARSER_PUBLIC_FUNC Mysql_sql_specifics : public Sql_specifics {
public:
  typedef std::shared_ptr<Mysql_sql_specifics> Ref;
  static Ref create() {
    return Ref(new Mysql_sql_specifics());
  }
  virtual ~Mysql_sql_specifics() {
  }

protected:
  Mysql_sql_specifics();

public:
  std::string limit_select_query(const std::string &sql, int *row_count, int *offset);
  void get_connection_startup_script(std::list<std::string> &sql_script);
  std::string query_connection_id();
  std::string query_kill_connection(std::int64_t connection_id);
  std::string query_kill_query(std::int64_t connection_id);
  std::string query_variable(const std::string &name);
  sqlide::QuoteVar::Escape_sql_string escape_sql_string();
  sqlide::QuoteVar::Blob_to_string blob_to_string();
  std::string setting_non_std_sql_delimiter();
  std::string non_std_sql_delimiter();
  std::string setting_ansi_quotes();
};
