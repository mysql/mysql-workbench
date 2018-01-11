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

struct SelectItem;
typedef std::list<SelectItem> SelectItems;
struct FromItem;
typedef std::list<FromItem> FromItems;

struct WBPUBLICBACKEND_PUBLIC_FUNC SelectStatement {
  typedef std::shared_ptr<SelectStatement> Ref;
  Ref parent;
  SelectItems select_items;
  FromItems from_items;
};
WBPUBLICBACKEND_PUBLIC_FUNC std::ostream &operator<<(std::ostream &os, SelectStatement &s);

struct WBPUBLICBACKEND_PUBLIC_FUNC SelectItem {
  SelectItem() : wildcard(false) {
  }
  std::string schema;
  std::string table;
  std::string field;
  std::string expr;
  std::string alias;
  bool wildcard;
  std::string effective_alias() const;
  std::string state_as_string() const;
};

struct WBPUBLICBACKEND_PUBLIC_FUNC FromItem {
  FromItem() {
  }
  std::string schema;
  std::string table;
  std::string alias;
  std::string subquery;
  SelectStatement::Ref statement;
};

/**
 * Defines interface to extract some information from provided SQL statement.
 *
 * @ingroup sqlparser
 */
class WBPUBLICBACKEND_PUBLIC_FUNC Sql_statement_decomposer : virtual public Sql_parser_base {
public:
  typedef std::shared_ptr<Sql_statement_decomposer> Ref;

protected:
  Sql_statement_decomposer();

public:
  virtual int decompose_query(const std::string &sql, SelectStatement::Ref select_statement) = 0;
  virtual int decompose_view(const std::string &ddl, SelectStatement::Ref select_statement) = 0;
  virtual int decompose_view(db_ViewRef view, SelectStatement::Ref select_statement) = 0;
};
