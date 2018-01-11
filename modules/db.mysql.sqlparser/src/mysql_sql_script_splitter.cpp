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

#include <glib.h>
#include <boost/signals2.hpp>

#include "mysql_sql_script_splitter.h"
#ifdef _WIN32
#include "mysql_sql_parser_public_interface.h"
#endif
#include "myx_sql_parser_public_interface.h"
#include "sql_lex.h"
#include "my_sys.h"
#include "myx_statement_parser.h"

Mysql_sql_script_splitter::Mysql_sql_script_splitter() {
  _cs = get_charset_by_name("utf8_bin", MYF(0));
}

Mysql_sql_script_splitter::~Mysql_sql_script_splitter() {
}

int Mysql_sql_script_splitter::process(const std::string &sql, std::list<std::string> &statements) {
  myx_process_sql_statements(sql.c_str(), _cs, &Mysql_sql_script_splitter::process_statement, &statements,
                             MYX_SPM_NORMAL_MODE);
  return 0;
}

int Mysql_sql_script_splitter::process(const char *sql, std::list<std::pair<size_t, size_t> > &ranges) {
  myx_process_sql_statements(sql, _cs, &Mysql_sql_script_splitter::process_statement_ranges, &ranges,
                             MYX_SPM_NORMAL_MODE);
  return 0;
}

int Mysql_sql_script_splitter::process_statement(const MyxStatementParser *splitter, const char *sql, void *userdata) {
  std::list<std::string> *statements = static_cast<std::list<std::string> *>(userdata);
  statements->push_back(sql);
  return 0;
}

int Mysql_sql_script_splitter::process_statement_ranges(const MyxStatementParser *splitter, const char *sql,
                                                        void *userdata) {
  std::list<std::pair<size_t, size_t> > *statements = static_cast<std::list<std::pair<size_t, size_t> > *>(userdata);
  size_t start = splitter->statement_boffset();
  statements->push_back(std::make_pair(start, strlen(sql)));
  return 0;
}
