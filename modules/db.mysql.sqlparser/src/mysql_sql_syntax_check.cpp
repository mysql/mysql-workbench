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

#include "mysql_sql_syntax_check.h"
#include "mysql_sql_parser_fe.h"
#include "grtdb/charset_utils.h"
#include "base/string_utilities.h"
#include <boost/lambda/bind.hpp>

using namespace grt;
using namespace base;
using namespace boost::placeholders;

static size_t MAX_INSERT_SQL_LENGTH = 8 * 1024; // determines max length of passed sql to be checked

Mysql_sql_syntax_check::Null_state_keeper::~Null_state_keeper() {
  boost::function<Parse_result()> f = boost::lambda::constant(pr_irrelevant);
  _sql_parser->_check_sql_statement = boost::bind(f);
}
#define NULL_STATE_KEEPER Null_state_keeper _nsk(this);

Mysql_sql_syntax_check::Mysql_sql_syntax_check() {
  NULL_STATE_KEEPER
}

Mysql_sql_syntax_check::Statement_type Mysql_sql_syntax_check::determine_statement_type(const std::string &sql) {
  NULL_STATE_KEEPER

  typedef std::map<std::string, Statement_type> KnownStatementTypes;
  static KnownStatementTypes known_statement_types;
  struct StaticInitializer {
    StaticInitializer() {
      known_statement_types[""] = sql_empty;
      known_statement_types["CREATE"] = sql_create;
      known_statement_types["ALTER"] = sql_alter;
      known_statement_types["DROP"] = sql_drop;
      known_statement_types["INSERT"] = sql_insert;
      known_statement_types["DELETE"] = sql_delete;
      known_statement_types["UPDATE"] = sql_update;
      known_statement_types["SELECT"] = sql_select;
      known_statement_types["DESC"] = sql_describe;
      known_statement_types["DESCRIBE"] = sql_describe;
      known_statement_types["SHOW"] = sql_show;
      known_statement_types["USE"] = sql_use;
      known_statement_types["LOAD"] = sql_load;
      known_statement_types["SET"] = sql_set;
    }
  };
  static StaticInitializer static_initializer;

  Mysql_sql_parser_fe sql_parser_fe(bec::GRTManager::get()->get_app_option_string("SqlMode"));
  std::string token = sql_parser_fe.get_first_sql_token(sql, "UNKNOWN");
  KnownStatementTypes::iterator statement_type = known_statement_types.find(token);
  return (known_statement_types.end() == statement_type) ? sql_unknown : statement_type->second;
}

int Mysql_sql_syntax_check::check_sql(const char *sql) {
  NULL_STATE_KEEPER
  _messages_enabled = false;
  _use_delimiter = false;

  Check_sql_statement do_check_slot;
  switch (_object_type) {
    case ot_trigger:
      do_check_slot = boost::bind(&Mysql_sql_syntax_check::do_check_trigger, this, _1);
      break;
    case ot_view:
      do_check_slot = boost::bind(&Mysql_sql_syntax_check::do_check_view, this, _1);
      break;
    case ot_routine:
      do_check_slot = boost::bind(&Mysql_sql_syntax_check::do_check_routine, this, _1);
      break;
    default:
      do_check_slot = boost::bind(&Mysql_sql_syntax_check::do_check_sql, this, _1);
      break;
  }

  int err_count = check_sql_statement(sql, do_check_slot, _object_type);
  return (err_count ? 0 : 1);
}

int Mysql_sql_syntax_check::check_trigger(const char *sql) {
  NULL_STATE_KEEPER
  _messages_enabled = false;
  _use_delimiter = true;
  int err_count =
    check_sql_statement(sql, boost::bind(&Mysql_sql_syntax_check::do_check_trigger, this, _1), ot_trigger);
  return (err_count ? 0 : 1);
}

int Mysql_sql_syntax_check::check_view(const char *sql) {
  NULL_STATE_KEEPER
  _messages_enabled = false;
  _use_delimiter = true;
  int err_count = check_sql_statement(sql, boost::bind(&Mysql_sql_syntax_check::do_check_view, this, _1), ot_view);
  return (err_count ? 0 : 1);
}

int Mysql_sql_syntax_check::check_routine(const char *sql) {
  NULL_STATE_KEEPER
  _messages_enabled = false;
  _use_delimiter = true;
  int err_count =
    check_sql_statement(sql, boost::bind(&Mysql_sql_syntax_check::do_check_routine, this, _1), ot_routine);
  return (err_count ? 0 : 1);
}

int Mysql_sql_syntax_check::check_sql_statement(const char *sql, Check_sql_statement check_sql_statement,
                                                ObjectType object_type) {
  _check_sql_statement = check_sql_statement;
  _process_sql_statement = boost::bind(&Mysql_sql_syntax_check::process_sql_statement, this, _1, object_type);

  Mysql_sql_parser_fe sql_parser_fe(bec::GRTManager::get()->get_app_option_string("SqlMode"));
  sql_parser_fe.is_ast_generation_enabled = _is_ast_generation_enabled;
  sql_parser_fe.ignore_dml = false;
  sql_parser_fe.max_insert_statement_size = MAX_INSERT_SQL_LENGTH;
  {
    DictRef options = DictRef::cast_from(grt::GRT::get()->get("/wb/options/options"));
    sql_parser_fe.max_err_count = (int)options.get_int("SqlEditor::SyntaxCheck::MaxErrCount", 100);
  }

  const char *actual_sql = sql;

  // Set delimiter for sql script if needed.
  std::string compound_sql;
  if (_use_delimiter) {
    // We are going to make a duplicate of the passed in SQL, which might lead to massive
    // memory problems if the script is large. The use of _use_delimiter should be prohibited.
    // Pass in the correct text instead.
    compound_sql = "DELIMITER " + _non_std_sql_delimiter + EOL + sql + EOL + _non_std_sql_delimiter;
    actual_sql = compound_sql.c_str();
  }

  return parse_sql_script(sql_parser_fe, actual_sql);
}

int Mysql_sql_syntax_check::process_sql_statement(const SqlAstNode *tree, ObjectType object_type) {
  if (report_sql_statement_border)
    do_report_sql_statement_border(_stmt_begin_lineno, _stmt_begin_line_pos, _stmt_end_lineno, _stmt_end_line_pos);

  if (!_is_ast_generation_enabled && !_err_tok_len)
    return 0;

  if (!tree) {
    report_sql_error(_err_tok_lineno, true, _err_tok_line_pos, _err_tok_len, _err_msg, 2);
    return 1;
  }

  if (tree && (ot_none != object_type))
    tree = tree->subitem(sql::_statement, sql::_create);

  if (!tree)
    return 1;

  if (pr_processed == _check_sql_statement(tree))
    return 0;
  else
    return 1;
}

Mysql_sql_parser_base::Parse_result Mysql_sql_syntax_check::do_check_sql(const SqlAstNode *tree) {
  if (tree)
    return check_sql(tree);
  else
    return pr_invalid;
}

Mysql_sql_parser_base::Parse_result Mysql_sql_syntax_check::do_check_trigger(const SqlAstNode *tree) {
  const SqlAstNode *trigger_tail = NULL;
  {
    static sql::symbol path1[] = {sql::_view_or_trigger_or_sp_or_event, sql::_definer_tail, sql::_};
    static sql::symbol path2[] = {sql::_view_or_trigger_or_sp_or_event, sql::_no_definer_tail, sql::_};
    static sql::symbol *paths[] = {path1, path2};

    trigger_tail = tree->search_by_paths(paths, ARR_CAPACITY(paths));
    if (trigger_tail)
      trigger_tail = trigger_tail->subitem(sql::_trigger_tail);
  }

  if (trigger_tail && trigger_tail->subseq(sql::_TRIGGER_SYM))
    return check_trigger(tree, trigger_tail);
  else
    return pr_irrelevant;
}

Mysql_sql_parser_base::Parse_result Mysql_sql_syntax_check::do_check_view(const SqlAstNode *tree) {
  const SqlAstNode *view_tail = NULL;
  {
    static sql::symbol path1[] = {sql::_view_or_trigger_or_sp_or_event, sql::_definer_tail, sql::_};
    static sql::symbol path2[] = {sql::_view_or_trigger_or_sp_or_event, sql::_no_definer_tail, sql::_};
    static sql::symbol path3[] = {sql::_view_or_trigger_or_sp_or_event, sql::_};
    static sql::symbol *paths[] = {path1, path2, path3};

    view_tail = tree->search_by_paths(paths, ARR_CAPACITY(paths));
    if (view_tail)
      view_tail = view_tail->subitem(sql::_view_tail);
  }

  if (view_tail)
    return check_view(tree, view_tail);
  else
    return pr_irrelevant;
}

Mysql_sql_parser_base::Parse_result Mysql_sql_syntax_check::do_check_routine(const SqlAstNode *tree) {
  const SqlAstNode *routine_tail = NULL;
  {
    static sql::symbol path1[] = {sql::_view_or_trigger_or_sp_or_event, sql::_definer_tail, sql::_};
    static sql::symbol path2[] = {sql::_view_or_trigger_or_sp_or_event, sql::_no_definer_tail, sql::_};
    static sql::symbol *paths[] = {path1, path2};

    routine_tail = tree->search_by_paths(paths, ARR_CAPACITY(paths));
    if (routine_tail) {
      static sql::symbol path1[] = {sql::_sp_tail, sql::_};
      static sql::symbol path2[] = {sql::_sf_tail, sql::_};
      static sql::symbol *paths[] = {path1, path2};

      routine_tail = routine_tail->search_by_paths(paths, ARR_CAPACITY(paths));
    }
  }

  if (routine_tail)
    return check_routine(tree, routine_tail);
  else
    return pr_irrelevant;
}

Mysql_sql_parser_base::Parse_result Mysql_sql_syntax_check::check_sql(const SqlAstNode *tree) {
  return pr_processed;
}

Mysql_sql_parser_base::Parse_result Mysql_sql_syntax_check::check_trigger(const SqlAstNode *tree,
                                                                          const SqlAstNode *trigger_tail) {
  return pr_processed;
}

Mysql_sql_parser_base::Parse_result Mysql_sql_syntax_check::check_view(const SqlAstNode *tree,
                                                                       const SqlAstNode *view_tail) {
  return pr_processed;
}

Mysql_sql_parser_base::Parse_result Mysql_sql_syntax_check::check_routine(const SqlAstNode *tree,
                                                                          const SqlAstNode *routine_tail) {
  return pr_processed;
}
