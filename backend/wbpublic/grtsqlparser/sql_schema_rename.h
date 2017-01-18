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
