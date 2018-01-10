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

#include <boost/lambda/bind.hpp>

#include "mysql_sql_parser_base.h"
#include "mysql_sql_parser_fe.h"
#include "myx_statement_parser.h"
#include "mysql_sql_specifics.h"

using namespace grt;
using namespace bec;

Mysql_sql_parser_base::Null_state_keeper::~Null_state_keeper() {
  _sql_parser->_err_msg = std::string();
  _sql_parser->_stmt_begin_lineno = 0;
  _sql_parser->_stmt_begin_line_pos = 0;
  _sql_parser->_stmt_end_lineno = 0;
  _sql_parser->_stmt_end_line_pos = 0;
  _sql_parser->_err_tok_lineno = 0;
  _sql_parser->_err_tok_line_pos = 0;
  _sql_parser->_err_tok_len = 0;
  _sql_parser->_active_schema = db_mysql_SchemaRef();
  _sql_parser->_catalog = db_mysql_CatalogRef();
  _sql_parser->_splitter = NULL;
  boost::function<bool()> f = boost::lambda::constant(false);
  _sql_parser->_process_sql_statement = boost::bind(f);
}
#define NULL_STATE_KEEPER Null_state_keeper _nsk(this);

Mysql_sql_parser_base::Mysql_sql_parser_base() : _override_sql_mode(false) {
  NULL_STATE_KEEPER // reset all members to null-values

    Sql_specifics::Ref sql_specifics = Mysql_sql_specifics::create();
  _non_std_sql_delimiter = sql_specifics->non_std_sql_delimiter();
}

void Mysql_sql_parser_base::set_options(const grt::DictRef &options) {
  Sql_parser_base::set_options(options);

  if (!options.is_valid())
    return;

  if (options.has_key("sql_mode"))
    sql_mode(grt::StringRef::cast_from(options.get("sql_mode")));
}

void Mysql_sql_parser_base::sql_mode(const std::string &value) {
  _override_sql_mode = true;
  _sql_mode = value;
}

int Mysql_sql_parser_base::parse_sql_script(Mysql_sql_parser_fe &sql_parser_fe, const char *sql) {
  SlotAutoDisconnector<boost::function<bool()> > on_stop_slot_disconnector(_stop_cb);
  _stop_cb = boost::bind(&Mysql_sql_parser_base::on_stop, this, &sql_parser_fe);
  if (_override_sql_mode)
    sql_parser_fe.parse_sql_mode(_sql_mode);
  return sql_parser_fe.parse_sql_script(sql, &process_sql_statement, this);
}

int Mysql_sql_parser_base::parse_sql_script_file(Mysql_sql_parser_fe &sql_parser_fe, const std::string &filename) {
  SlotAutoDisconnector<boost::function<bool()> > on_stop_slot_disconnector(_stop_cb);
  _stop_cb = boost::bind(&Mysql_sql_parser_base::on_stop, this, &sql_parser_fe);
  if (_override_sql_mode)
    sql_parser_fe.parse_sql_mode(_sql_mode);
  return sql_parser_fe.parse_sql_script_file(filename, &process_sql_statement, this);
}

bool Mysql_sql_parser_base::on_stop(Mysql_sql_parser_fe *sql_parser_fe) {
  return (0 != sql_parser_fe->stop());
}

int Mysql_sql_parser_base::process_sql_statement(void *sql_parser_ptr, const MyxStatementParser *splitter,
                                                 const char *sql_statement, const SqlAstNode *tree,
                                                 int stmt_begin_lineno, int stmt_begin_line_pos, int stmt_end_lineno,
                                                 int stmt_end_line_pos, int err_tok_lineno, int err_tok_line_pos,
                                                 int err_tok_len, const std::string &err_msg) {
  Mysql_sql_parser_base *sql_parser = reinterpret_cast<Mysql_sql_parser_base *>(sql_parser_ptr);

  sql_parser->_splitter = splitter;
  sql_parser->_sql_statement = sql_statement;
  sql_parser->_stmt_begin_lineno = stmt_begin_lineno;
  sql_parser->_stmt_begin_line_pos = stmt_begin_line_pos;
  sql_parser->_stmt_end_lineno = stmt_end_lineno;
  sql_parser->_stmt_end_line_pos = stmt_end_line_pos;
  sql_parser->_err_tok_lineno = err_tok_lineno;
  sql_parser->_err_tok_line_pos = err_tok_line_pos;
  sql_parser->_err_tok_len = err_tok_len;
  sql_parser->_err_msg = err_msg;

  return sql_parser->_process_sql_statement(tree);
}

int Mysql_sql_parser_base::total_line_count() {
  return _splitter->total_line_count();
}

void Mysql_sql_parser_base::report_semantic_error(const SqlAstNode *item, const std::string &err_msg, int entry_type) {
  int lineno = -1;
  int token_line_pos = 0;
  int token_len = 0;
  if (item)
    Mysql_sql_parser_fe::determine_token_position(item, _splitter, _sql_statement.c_str(), lineno, token_line_pos,
                                                  token_len);
  report_sql_error(lineno, true, token_line_pos, token_len, err_msg, entry_type, "");
}

void Mysql_sql_parser_base::process_obj_full_name_item(const SqlAstNode *item, std::string &schema_name,
                                                       std::string &obj_name) {
  if (!item)
    return;

  if (3 == item->subitems()->size()) // ident.ident
    schema_name = (*item->subitems()->begin())->value();
  obj_name = (*item->subitems()->rbegin())->value();
}
