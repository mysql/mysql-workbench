/* 
 * Copyright (c) 2014,2015,  Oracle and/or its affiliates. All rights reserved.
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
 * C++ interface for the ANTLR based MySQL syntax checker. It's similar to the MySQL parser
 * but does not create a syntax tree and hence operates significantly faster.
 */
class MYSQL_PARSER_PUBLIC_FUNC MySQLSyntaxChecker : public MySQLRecognitionBase
{
public:
  MySQLSyntaxChecker(long server_version, const std::string &sql_mode, const std::set<std::string> &charsets);
  virtual ~MySQLSyntaxChecker();
  
  void parse(const char *text, size_t length, bool is_utf8, MySQLQueryType parse_unit);

  virtual void set_sql_mode(const std::string &new_mode);
  virtual void set_server_version(long new_version);
  virtual std::string text();
  virtual const char* lineStart();
  long server_version();

protected:

private:
  class Private;
  Private *d;
};
