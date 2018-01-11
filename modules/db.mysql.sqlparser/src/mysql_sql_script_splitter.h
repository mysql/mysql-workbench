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

#ifndef _MYSQL_SQL_SCRIPT_SPLITTER_H_
#define _MYSQL_SQL_SCRIPT_SPLITTER_H_

#include <memory>
#include <list>
#include <string>

namespace mysql_parser {
  class MyxStatementParser;
  typedef struct charset_info_st CHARSET_INFO;
}
using namespace mysql_parser;

/** Splits given SQL script into separate statements.
 *
 * @ingroup sqlparser
 */
class Mysql_sql_script_splitter {
public:
  typedef std::shared_ptr<Mysql_sql_script_splitter> Ref;
  static Ref create() {
    return Ref(new Mysql_sql_script_splitter());
  }
  virtual ~Mysql_sql_script_splitter();

protected:
  Mysql_sql_script_splitter();

public:
  int process(const std::string &sql, std::list<std::string> &statements);
  int process(const char *sql, std::list<std::pair<size_t, size_t> > &ranges);

private:
  static int process_statement(const MyxStatementParser *splitter, const char *sql, void *userdata);
  static int process_statement_ranges(const MyxStatementParser *splitter, const char *sql, void *userdata);
  CHARSET_INFO *_cs;
};

#endif // _MYSQL_SQL_SCRIPT_SPLITTER_H_
