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

#ifndef _SQL_SCHEMA_RENAME_H_
#define _SQL_SCHEMA_RENAME_H_

#include "wbpublic_public_interface.h"
#include "sql_parser_base.h"

/**
 * Defines interface to rename schema within SQL statement/script.
 *
 * @ingroup sqlparser
 */
class WBPUBLICBACKEND_PUBLIC_FUNC Sql_schema_rename : virtual public Sql_parser_base {
public:
  typedef std::shared_ptr<Sql_schema_rename> Ref;

protected:
  Sql_schema_rename() {
  }

public:
  virtual int rename_schema_references(db_CatalogRef catalog, const std::string &old_schema_name,
                                       const std::string &new_schema_name) = 0;
  virtual int rename_schema_references(std::string &sql, const std::string &old_schema_name,
                                       const std::string &new_schema_name) = 0;
};

#endif // _SQL_SCHEMA_RENAME_H_
