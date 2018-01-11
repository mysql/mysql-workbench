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

#ifndef _SQL_PARSER_H_
#define _SQL_PARSER_H_

#include "wbpublic_public_interface.h"
#include "sql_parser_base.h"

/**
 * Defines interface to fill provided catalog with schemata/tables/views/routines/etc. parsed from the given DDL script.
 * Serves as a base class for all parsers transforming DDL script into GRT schema objects.
 *
 * @ingroup sqlparser
 */
class WBPUBLICBACKEND_PUBLIC_FUNC Sql_parser : virtual public Sql_parser_base {
public:
  typedef std::shared_ptr<Sql_parser> Ref;

protected:
  Sql_parser() {
  }

public:
  virtual int parse_sql_script(db_CatalogRef catalog, const std::string &sql, grt::DictRef options) = 0;
  virtual int parse_sql_script_file(db_CatalogRef catalog, const std::string &sql, grt::DictRef options) = 0;
};

#endif // _SQL_PARSER_H_
