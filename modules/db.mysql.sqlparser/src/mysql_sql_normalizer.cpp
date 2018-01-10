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

#include "mysql_sql_normalizer.h"
#include "mysql_sql_parser_utils.h"

using namespace grt;

#define NULL_STATE_KEEPER Null_state_keeper _nsk(this);

Mysql_sql_normalizer::Mysql_sql_normalizer() : _cut_sym_count(0) {
  NULL_STATE_KEEPER
}

std::string Mysql_sql_normalizer::normalize(const std::string &sql, const std::string &schema_name) {
  NULL_STATE_KEEPER

  _schema_name = schema_name;
  _process_sql_statement = boost::bind(&Mysql_sql_normalizer::process_sql_statement, this, _1);
  _norm_stmt = strip_sql_statement(sql, true);
  std::string sql_ = "DELIMITER " + _delimiter + EOL + _norm_stmt + _delimiter;

  Mysql_sql_parser_fe sql_parser_fe(bec::GRTManager::get()->get_app_option_string("SqlMode"));
  sql_parser_fe.ignore_dml = false;
  Mysql_sql_parser_base::parse_sql_script(sql_parser_fe, sql_.c_str());

  return _norm_script;
}

int Mysql_sql_normalizer::process_sql_statement(const SqlAstNode *tree) {
  _cut_sym_count = 0;

  if (tree) {
    if (const SqlAstNode *item = tree->subitem(sql::_statement, sql::_create))
      process_create_statement(item);
    else if (const SqlAstNode *item = tree->subitem(sql::_statement, sql::_insert))
      process_insert_statement(item);
  }

  append_stmt_to_script(_norm_stmt);

  return 0; // error count
}

void Mysql_sql_normalizer::append_stmt_to_script(const std::string &stmt) {
  if (stmt.empty())
    return;
  if (!_norm_script.empty())
    _norm_script += EOL;
  _norm_script += stmt;
}

void Mysql_sql_normalizer::qualify_obj_ident(const SqlAstNode *sp_name) {
  if (sp_name) {
    const SqlAstNode *schema_ident = NULL;
    const SqlAstNode *obj_ident = NULL;
    if (1 < sp_name->subitems()->size()) {
      schema_ident = sp_name->subitem(sql::_ident);
      obj_ident = sp_name->find_subseq(sql::_46, sql::_ident); // 46 == ascii('.')
    } else
      obj_ident = sp_name->subitem(sql::_ident);

    int stmt_boffset = -1;
    int stmt_eoffset = -1;

    if (schema_ident)
      stmt_boffset = schema_ident->stmt_boffset();
    else
      stmt_boffset = obj_ident->stmt_boffset();
    stmt_eoffset = obj_ident->stmt_eoffset();

    stmt_boffset -= _cut_sym_count;
    stmt_eoffset -= _cut_sym_count;

    // "`" symbol is not part of ident in lexer, so some tricky code here to guess if it was quoted or not.
    if (stmt_boffset > 0 && '`' == _norm_stmt[stmt_boffset - 1])
      --stmt_boffset;
    if (stmt_eoffset > 0 && stmt_eoffset < (int)_norm_stmt.size() && '`' == _norm_stmt[stmt_eoffset])
      ++stmt_eoffset;

    std::string obj_name_ = obj_ident->value();
    std::string schema_name_ = (schema_ident ? schema_ident->value() : _schema_name);

    std::string qual_obj_name = qualify_obj_name(obj_name_, schema_name_);
    _norm_stmt.replace(stmt_boffset, stmt_eoffset - stmt_boffset, qual_obj_name);

    _cut_sym_count += stmt_eoffset - stmt_boffset - (int)qual_obj_name.size();
  }
}

Mysql_sql_normalizer::Parse_result Mysql_sql_normalizer::process_create_statement(const SqlAstNode *tree) {
  typedef Parse_result (Mysql_sql_normalizer::*statement_processor)(const SqlAstNode *);
  static statement_processor proc_arr[] = {
    &Mysql_sql_normalizer::process_create_table_statement,
    &Mysql_sql_normalizer::process_create_index_statement,
    &Mysql_sql_normalizer::process_create_view_statement,
    &Mysql_sql_normalizer::process_create_routine_statement,
    &Mysql_sql_normalizer::process_create_trigger_statement,
    &Mysql_sql_normalizer::process_create_server_link_statement,
    &Mysql_sql_normalizer::process_create_tablespace_statement,
    &Mysql_sql_normalizer::process_create_logfile_group_statement,
    &Mysql_sql_normalizer::process_create_schema_statement,
  };

  for (size_t n = 0; n < ARR_CAPACITY(proc_arr); ++n) {
    statement_processor proc = proc_arr[n];
    Parse_result result = (this->*proc)(tree);
    if (pr_irrelevant != result)
      return result;
  }

  return pr_irrelevant;
}

Mysql_sql_normalizer::Parse_result Mysql_sql_normalizer::process_create_table_statement(const SqlAstNode *tree) {
  const SqlAstNode *create2_item = tree->subitem(sql::_create2);

  // check if statement is relevant
  if (!create2_item)
    return pr_irrelevant;

  return pr_processed;
}

Mysql_sql_normalizer::Parse_result Mysql_sql_normalizer::process_create_view_statement(const SqlAstNode *tree) {
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

  // check if statement is relevant
  if (!view_tail)
    return pr_irrelevant;

  return pr_processed;
}

Mysql_sql_normalizer::Parse_result Mysql_sql_normalizer::process_create_routine_statement(const SqlAstNode *tree) {
  const SqlAstNode *routine_tail = NULL;
  {
    static sql::symbol path1[] = {sql::_view_or_trigger_or_sp_or_event, sql::_definer_tail, sql::_};
    static sql::symbol path2[] = {sql::_view_or_trigger_or_sp_or_event, sql::_no_definer_tail, sql::_};
    static sql::symbol *paths[] = {path1, path2};

    routine_tail = tree->search_by_paths(paths, ARR_CAPACITY(paths));
  }
  if (routine_tail) {
    static sql::symbol path1[] = {sql::_sp_tail, sql::_};
    static sql::symbol path2[] = {sql::_sf_tail, sql::_};
    static sql::symbol *paths[] = {path1, path2};

    routine_tail = routine_tail->search_by_paths(paths, ARR_CAPACITY(paths));
  }

  // check if statement is relevant
  if (!routine_tail)
    return pr_irrelevant;

  // solution for alter/diff requirement: inject schema name into routine name & wrap both names with '`'
  qualify_obj_ident(routine_tail->subitem(sql::_sp_name));

  return pr_processed;
}

Mysql_sql_normalizer::Parse_result Mysql_sql_normalizer::process_create_index_statement(const SqlAstNode *tree) {
  // check if statement is relevant
  if (!tree->find_subseq(sql::_INDEX_SYM, sql::_ident))
    return pr_irrelevant;
  return pr_processed;
}

Mysql_sql_normalizer::Parse_result Mysql_sql_normalizer::process_create_logfile_group_statement(
  const SqlAstNode *tree) {
  // check if statement is relevant
  if (!tree->subseq(sql::_CREATE, sql::_LOGFILE_SYM, sql::_GROUP_SYM))
    return pr_irrelevant;
  return pr_processed;
}

Mysql_sql_normalizer::Parse_result Mysql_sql_normalizer::process_create_tablespace_statement(const SqlAstNode *tree) {
  // check if statement is relevant
  if (!tree->subseq(sql::_CREATE, sql::_TABLESPACE))
    return pr_irrelevant;
  return pr_processed;
}

Mysql_sql_normalizer::Parse_result Mysql_sql_normalizer::process_create_server_link_statement(const SqlAstNode *tree) {
  // check if statement is relevant
  if (!tree->subseq(sql::_CREATE, sql::_server_def))
    return pr_irrelevant;
  return pr_processed;
}

Mysql_sql_normalizer::Parse_result Mysql_sql_normalizer::process_create_trigger_statement(const SqlAstNode *tree) {
  const SqlAstNode *trigger_tail = NULL;
  {
    static sql::symbol path1[] = {sql::_view_or_trigger_or_sp_or_event, sql::_definer_tail, sql::_};
    static sql::symbol path2[] = {sql::_view_or_trigger_or_sp_or_event, sql::_no_definer_tail, sql::_};
    static sql::symbol *paths[] = {path1, path2};

    trigger_tail = tree->search_by_paths(paths, ARR_CAPACITY(paths));
    if (trigger_tail)
      trigger_tail = trigger_tail->subitem(sql::_trigger_tail);
  }

  // check if statement is relevant
  if (!trigger_tail || !trigger_tail->subseq(sql::_TRIGGER_SYM))
    return pr_irrelevant;

  /*
    `show create` specifies definer clause indifferently to whether it was specified in user input.
    that makes problem for differ, since it has to compare original SQL text as well (sqlDefinition member).
    workaround is to ignore definer clause in diff mode.
    definer clause (and nothing but it) is always located between `create` & `trigger` tokens.
  */
  {
    const SqlAstNode *create_item = tree->subseq(sql::_CREATE);
    const SqlAstNode *trigger_item = trigger_tail->subseq(sql::_TRIGGER_SYM);

    int stmt_boffset = create_item->stmt_eoffset();
    int stmt_eoffset = trigger_item->stmt_boffset();

    stmt_boffset -= _cut_sym_count;
    stmt_eoffset -= _cut_sym_count;

    _norm_stmt.replace(stmt_boffset, stmt_eoffset - stmt_boffset, " ");
    _cut_sym_count += stmt_eoffset - stmt_boffset - 1 /*length of space sym*/;
  }

  // qualify trigger name & table name
  qualify_obj_ident(trigger_tail->subitem(sql::_sp_name));
  qualify_obj_ident(trigger_tail->subitem(sql::_table_ident));

  return pr_processed;
}

Mysql_sql_normalizer::Parse_result Mysql_sql_normalizer::process_create_schema_statement(const SqlAstNode *tree) {
  // check if statement is relevant
  if (!tree->subseq(sql::_CREATE, sql::_DATABASE))
    return pr_irrelevant;
  return pr_processed;
}

Mysql_sql_normalizer::Parse_result Mysql_sql_normalizer::process_insert_statement(const SqlAstNode *tree) {
  _norm_stmt.clear();
  _common_sql = "INSERT INTO ";

  const SqlAstNode *insert_field_spec = tree->subitem(sql::_insert_field_spec);
  if (insert_field_spec) {
    {
      const SqlAstNode *insert_table = tree->subitem(sql::_insert2, sql::_insert_table);
      if (insert_table) {
        std::string tablename = insert_table->restore_sql_text(_sql_statement);
        if (0 != tablename.find('`')) {
          tablename.insert(0, "`");
          tablename.push_back('`');
        }
        _common_sql.append(tablename);
      }
    }

    std::string fields_clause;

    if ((insert_field_spec->subitem(sql::_fields))) {
      _common_sql += " ";
      if (fields_clause.empty())
        _common_sql += insert_field_spec->restore_sql_text(
          _sql_statement, insert_field_spec->subitem(sql::_40),
          insert_field_spec->subitem(sql::_41)); // 40 == ascii('('), 41 == ascii(')')
      else
        _common_sql += "(" + fields_clause + ")";
      _common_sql += " VALUES ";
    }

    {
      const SqlAstNode *insert_values = insert_field_spec->subitem(sql::_insert_values, sql::_values_list);
      for (SqlAstNode::SubItemList::const_iterator it = insert_values->subitems()->begin(),
                                                   it_end = insert_values->subitems()->end();
           it != it_end; ++it) {
        const SqlAstNode *item = *it;
        if (item->name_equals(sql::_no_braces)) {
          std::string stmt = _common_sql + item->restore_sql_text(_sql_statement) + ";";
          stmt = strip_sql_statement(stmt, true);
          append_stmt_to_script(stmt);
        }
      }
    }
  }

  return pr_processed;
}
