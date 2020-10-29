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
#include <cctype>

#include "mysql_sql_inserts_loader.h"
#include "mysql_sql_parser_utils.h"
#include <boost/foreach.hpp>

using namespace grt;

#define NULL_STATE_KEEPER Null_state_keeper _nsk(this);

Mysql_sql_inserts_loader::Mysql_sql_inserts_loader() {
  NULL_STATE_KEEPER
}

void Mysql_sql_inserts_loader::load(const std::string &sql, const std::string &schema_name) {
  NULL_STATE_KEEPER

  _schema_name = schema_name;
  _process_sql_statement = boost::bind(&Mysql_sql_inserts_loader::process_sql_statement, this, boost::placeholders::_1);

  Mysql_sql_parser_fe sql_parser_fe(bec::GRTManager::get()->get_app_option_string("SqlMode"));
  sql_parser_fe.ignore_dml = false;
  Mysql_sql_parser_base::parse_sql_script(sql_parser_fe, sql.c_str());
}

int Mysql_sql_inserts_loader::process_sql_statement(const SqlAstNode *tree) {
  if (tree) {
    if (const SqlAstNode *item = tree->subitem(sql::_statement, sql::_insert))
      process_insert_statement(item);
  }

  return 0; // error count
}

Mysql_sql_inserts_loader::Parse_result Mysql_sql_inserts_loader::process_insert_statement(const SqlAstNode *tree) {
  std::string schema_name = _schema_name;
  std::string table_name;
  Strings fields_names;
  Strings fields_values;
  std::vector<bool> null_fields;

  const SqlAstNode *insert_field_spec = tree->subitem(sql::_insert_field_spec);
  if (insert_field_spec) {
    // schema & table name
    {
      const SqlAstNode *table_ident = tree->subitem(sql::_insert2, sql::_insert_table, sql::_table_ident);
      process_obj_full_name_item(table_ident, schema_name, table_name);
    }

    // fields names
    {
      const SqlAstNode *fields = insert_field_spec->subitem(sql::_fields);
      if (fields) {
        for (const SqlAstNode *item : *fields->subitems())
          if (item->name_equals(sql::_insert_ident))
            fields_names.push_back(item->restore_sql_text(_sql_statement));
      }
    }

    // fields values
    {
      fields_values.reserve(fields_names.size());
      null_fields.reserve(fields_names.size());

      const SqlAstNode *insert_values = insert_field_spec->subitem(sql::_insert_values, sql::_values_list);
      for (const SqlAstNode *item : *insert_values->subitems()) {
        if (item->name_equals(sql::_no_braces)) {
          fields_values.clear();
          null_fields.clear();

          const SqlAstNode *values = item->subitem(sql::_opt_values, sql::_values);
          for (const SqlAstNode *item : *values->subitems()) {
            if (item->name_equals(sql::_expr_or_default)) {
              std::string value;
              bool is_field_null = false;
              if (const SqlAstNode *item1 = item->subitem(sql::_expr))
                if (1 == item1->subitems()->size())
                  if (const SqlAstNode *item2 = item1->subitem(sql::_bool_pri))
                    if (1 == item2->subitems()->size())
                      if (const SqlAstNode *item3 = item2->subitem(sql::_predicate))
                        if (1 == item3->subitems()->size())
                          if (const SqlAstNode *item4 = item3->subitem(sql::_bit_expr))
                            if (1 == item4->subitems()->size())
                              if (const SqlAstNode *item5 = item4->subitem(sql::_simple_expr))
                                if (1 == item5->subitems()->size())
                                  if (const SqlAstNode *item6 = item5->subitem(sql::_literal))
                                    if (1 == item6->subitems()->size())
                                      if (/*const SqlAstNode *item7=*/item6->subitem(sql::_NULL_SYM))
                                        is_field_null = true;

              if (!is_field_null) {
                value = item->restore_sql_text(_sql_statement);
                if (1 < value.size()) {
                  switch (value[0]) {
                    case '\'':
                    case '"':
                      value = value.substr(1, value.size() - 2);
                      break;
                    default:
                      static const std::string func_call_seq = "\\func ";
                      if (value[0] == '\\') {
                        if ((value.size() > func_call_seq.size()) &&
                            (value.compare(0, func_call_seq.size(), func_call_seq) == 0)) {
                          value = '\\' + value;
                        }
                      } else {
                        bool is_expression = false;
                        for (std::string::iterator i = value.begin(), i_end = value.end(); i != i_end; ++i) {
                          if (!std::isdigit(*i) && (*i != '.') && (*i != ',')) {
                            is_expression = true;
                            break;
                          }
                        }
                        if (is_expression) {
                          value = func_call_seq + value;
                        }
                      }
                      break;
                  }
                }
              }

              fields_values.push_back(value);
              null_fields.push_back(is_field_null);
            }
          }

          const std::pair<std::string, std::string> schema_table = make_pair(schema_name, table_name);
          _process_insert(sql_statement(), schema_table, fields_names, fields_values, null_fields);
        }
      }
    }
  }

  return pr_processed;
}
