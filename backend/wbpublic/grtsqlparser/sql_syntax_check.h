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

#pragma once

#include "wbpublic_public_interface.h"
#include "sql_parser_base.h"

/**
 * Defines interface to check syntax of provided SQL statement/script.
 *
 * @ingroup sqlparser
 */
class WBPUBLICBACKEND_PUBLIC_FUNC Sql_syntax_check : virtual public Sql_parser_base {
public:
  typedef std::shared_ptr<Sql_syntax_check> Ref;

protected:
  Sql_syntax_check() : _object_type(ot_none) {
  }

public:
  enum Statement_type {
    sql_unknown,
    sql_empty,
    sql_create,
    sql_alter,
    sql_drop,
    sql_insert,
    sql_delete,
    sql_update,
    sql_select,
    sql_describe,
    sql_show,
    sql_use,
    sql_load,
    sql_set
  };
  virtual Statement_type determine_statement_type(const std::string& sql) = 0;

public:
  enum ObjectType { ot_none, ot_trigger, ot_view, ot_routine };
  void only_object_type_of(ObjectType ot) {
    _object_type = ot;
  }

protected:
  ObjectType _object_type;

public:
  virtual int check_sql(const char* sql) = 0;
  virtual int check_trigger(const char* sql) = 0;
  virtual int check_view(const char* sql) = 0;
  virtual int check_routine(const char* sql) = 0;
};
