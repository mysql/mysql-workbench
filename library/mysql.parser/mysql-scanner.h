/* 
 * Copyright (c) 2013, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "mysql-parser-common.h"

/**
 * C++ interface for the ANTLR based MySQL lexer.
 * This scanner class is not needed for the MySQLRecognizer class (it uses the raw lexer)
 * but provides tokenizing functionality beside it.
 */

class MYSQL_PARSER_PUBLIC_FUNC MySQLScanner : public MySQLRecognitionBase
{
public:
  MySQLScanner(const char *text, size_t length, bool is_utf8, long server_version, 
    const std::string &sql_mode_string, const std::set<std::string> &charsets);
  virtual ~MySQLScanner();
  
  void reset();
  MySQLToken next_token();

  void set_server_version(long version);
  void set_sql_mode(const std::string &new_mode);
  virtual const char* text();

protected:
  void setup();

private:
  class Private;
  Private *d;
};
