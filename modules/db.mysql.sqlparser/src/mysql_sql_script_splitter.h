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
