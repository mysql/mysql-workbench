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
#include "grtsqlparser/sql_schema_rename.h"

/** Searches catalog objects' statements for occurences of old schema name and replaces them with new one.
 *
 * @ingroup sqlparser
 */
class Mysql_sql_schema_rename : protected Mysql_sql_parser_base, virtual public Sql_schema_rename {
public:
  typedef std::shared_ptr<Mysql_sql_schema_rename> Ref;
  static Ref create() {
    return Ref(new Mysql_sql_schema_rename());
  }
  virtual ~Mysql_sql_schema_rename() {
  }

protected:
  Mysql_sql_schema_rename();

public:
  int rename_schema_references(db_CatalogRef catalog, const std::string &old_schema_name,
                               const std::string &new_schema_name);
  int rename_schema_references(std::string &sql, const std::string &old_schema_name,
                               const std::string &new_schema_name);

protected:
  template <typename T>
  void rename_schema_references(grt::ListRef<T> obj_list, grt::StringRef (T::*sql_text_prop_r)() const,
                                void (T::*sql_text_prop_w)(const grt::StringRef &), int delim_wrapping,
                                Mysql_sql_parser_fe &parser_fe);

  int process_sql_statement(const SqlAstNode *tree);
  void process_sql_statement_item(const SqlAstNode *item);
  void process_schema_reference_candidate(const SqlAstNode *item, int dot_count);
  bool rename_schema_references(std::string &sql_text, Mysql_sql_parser_fe &sql_parser_fe, int delim_wrapping);
  bool rename_schema_references(std::string &sql_text);

  std::string _old_schema_name;
  std::string _new_schema_name;
  std::list<int> _schema_names_offsets;

  class Null_state_keeper : Mysql_sql_parser_base::Null_state_keeper {
  public:
    Null_state_keeper(Mysql_sql_schema_rename *sql_parser)
      : Mysql_sql_parser_base::Null_state_keeper(sql_parser), _sql_parser(sql_parser) {
    }
    ~Null_state_keeper();

  private:
    Mysql_sql_schema_rename *_sql_parser;
  };
  friend class Null_state_keeper;
};
