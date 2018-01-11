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

#include <glib.h>
#include <boost/signals2.hpp>

#include "mysql_sql_semantic_check.h"
#include "mysql_sql_parser_fe.h"
#include "base/string_utilities.h"

using namespace grt;
using namespace base;

Mysql_sql_semantic_check::Null_state_keeper::~Null_state_keeper() {
}
#define NULL_STATE_KEEPER Null_state_keeper _nsk(this);

Mysql_sql_semantic_check::Mysql_sql_semantic_check() {
  NULL_STATE_KEEPER
}

Mysql_sql_parser_base::Parse_result Mysql_sql_semantic_check::check_sql(const SqlAstNode *tree) {
  return pr_processed;
}

Mysql_sql_parser_base::Parse_result Mysql_sql_semantic_check::check_trigger(const SqlAstNode *tree,
                                                                            const SqlAstNode *trigger_tail) {
  if (!_context_table.is_valid())
    return pr_processed;

  const SqlAstNode *table_ident = trigger_tail->subitem(sql::_table_ident);
  std::string schema_name;
  std::string table_name;

  Mysql_sql_parser_base::process_obj_full_name_item(table_ident, schema_name, table_name);

  if (schema_name.empty())
    schema_name = _active_schema.is_valid() ? _active_schema->name() : _context_table->owner()->name();

  if ((!schema_name.empty() &&
       !are_strings_eq(_context_table->owner()->name(), schema_name, _case_sensitive_identifiers)) ||
      !are_strings_eq(_context_table->name(), table_name, _case_sensitive_identifiers)) {
    std::string err_msg =
      strfmt("Wrong table: `%s`.`%s`, while `%s`.`%s` expected", schema_name.c_str(), table_name.c_str(),
             _context_table->owner()->name().c_str(), _context_table->name().c_str());
    report_semantic_error(table_ident, err_msg, 2);
    return pr_invalid;
  }

  return pr_processed;
}

Mysql_sql_parser_base::Parse_result Mysql_sql_semantic_check::check_view(const SqlAstNode *tree,
                                                                         const SqlAstNode *view_tail) {
  return pr_processed;
}

Mysql_sql_parser_base::Parse_result Mysql_sql_semantic_check::check_routine(const SqlAstNode *tree,
                                                                            const SqlAstNode *routine_tail) {
  return pr_processed;
}
