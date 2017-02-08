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

#include "sql_statement_decomposer.h"
#include "base/string_utilities.h"

#include <iomanip>

std::string SelectItem::effective_alias() const {
  if (wildcard)
    return "*";
  if (!alias.empty())
    return alias;
  if (!expr.empty())
    return expr;
  return field;
}

std::string SelectItem::state_as_string() const {
  return base::strfmt("{%s}.{%s}.{%s}.{%s}.{%s}.{%i}", schema.c_str(), table.c_str(), field.c_str(), expr.c_str(),
                      alias.c_str(), wildcard);
}

std::ostream &operator<<(std::ostream &os, SelectStatement &s) {
  int indent = 0;
  for (SelectStatement::Ref parent = s.parent; parent; parent = parent->parent)
    ++indent;
  os << std::setw(indent * 2) << ""
     << "{SELECT\n";
  for (const SelectItem &select_item : s.select_items)
    os << std::setw((indent + 1) * 2) << "" << select_item.state_as_string() << "\n";
  os << std::setw(indent * 2) << ""
     << "FROM\n";
  for (const FromItem &from_item : s.from_items) {
    if (from_item.statement)
      os << *from_item.statement;
    else
      os << std::setw((indent + 1) * 2) << "";
    if (!from_item.schema.empty())
      os << from_item.schema << ".";
    os << from_item.table;
    if (!from_item.alias.empty())
      os << " " << from_item.alias;
    os << "\n";
  }
  os << std::setw(indent * 2) << ""
     << "}";
  return os;
}

Sql_statement_decomposer::Sql_statement_decomposer() {
}
