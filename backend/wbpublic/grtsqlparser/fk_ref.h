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

#ifndef _FK_REF_H
#define _FK_REF_H

#include "sql_facade.h"
#include "grtdb/db_object_helpers.h"
#include "grts/structs.db.h"
#include <list>

/** Stores information required to create foreign key between two tables.
 * Used to accumulate info on FK to create them on last step of reverse-engineering,
 * as workaround for cases when table being referenced is not parsed yet by time of parsing
 * the referencing FK.
 *
 * @ingroup sqlparser
 */
class Fk_ref {
public:
  typedef std::list<std::string> String_collection;

private:
  db_ForeignKeyRef _fk;
  std::string _ref_schema_name;
  std::string _ref_table_name;
  String_collection _ref_columns_names;

public:
  Fk_ref(db_ForeignKeyRef fk) : _fk(fk) {
  }

  inline operator db_ForeignKeyRef &() {
    return _fk;
  }
  inline db_TableRef owner_table() {
    return db_TableRef::cast_from(_fk->owner());
  }

  inline std::string &ref_schema_name() {
    return _ref_schema_name;
  }
  inline std::string &ref_table_name() {
    return _ref_table_name;
  }
  inline String_collection &ref_column_names() {
    return _ref_columns_names;
  }

  inline void ref_schema_name(const std::string &schema_name) {
    _ref_schema_name = schema_name;
  }
  inline void ref_table_name(const std::string &table_name) {
    _ref_table_name = table_name;
  }
};

typedef std::list<Fk_ref> Fk_ref_collection;

#endif // _FK_REF_H
