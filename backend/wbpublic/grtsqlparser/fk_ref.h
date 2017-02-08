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
