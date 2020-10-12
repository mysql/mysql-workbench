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

#include "mysql_sql_statement_decomposer.h"
#include "mysql_sql_parser_utils.h"
#include "mysql_sql_parser_fe.h"
#include "base/string_utilities.h"
#include <boost/foreach.hpp>
#include <boost/lambda/bind.hpp>

using namespace grt;
using namespace base;
using namespace boost::placeholders;

Mysql_sql_statement_decomposer::Null_state_keeper::~Null_state_keeper() {
  boost::function<Parse_result()> f = boost::lambda::constant(pr_irrelevant);
  _sql_parser->_do_process_sql_statement = boost::bind(f);
}
#define NULL_STATE_KEEPER Null_state_keeper _nsk(this);

void Mysql_sql_statement_decomposer::set_options(const grt::DictRef &opts) {
  if (opts.is_valid())
    Sql_parser_base::case_sensitive_identifiers(opts.get_int("case_sensitive_identifiers", grt::IntegerRef(1)) != 0);
}

Mysql_sql_statement_decomposer::Mysql_sql_statement_decomposer() {
  NULL_STATE_KEEPER
}

int Mysql_sql_statement_decomposer::decompose_query(const std::string &sql, SelectStatement::Ref select_statement) {
  NULL_STATE_KEEPER
  int err_count = process_sql_statement(sql, select_statement,
                                        boost::bind(&Mysql_sql_statement_decomposer::do_decompose_query, this, _1));
  return (err_count ? 0 : 1);
}

int Mysql_sql_statement_decomposer::decompose_view(const std::string &sql, SelectStatement::Ref select_statement) {
  NULL_STATE_KEEPER
  int err_count = process_sql_statement(sql, select_statement,
                                        boost::bind(&Mysql_sql_statement_decomposer::do_decompose_view, this, _1));
  return (err_count ? 0 : 1);
}

int Mysql_sql_statement_decomposer::process_sql_statement(const std::string &sql, SelectStatement::Ref select_statement,
                                                          ProcessSqlStatement do_process_sql_statement_cb) {
  _messages_enabled = false;
  _do_process_sql_statement = do_process_sql_statement_cb;
  _process_sql_statement = boost::bind(&Mysql_sql_statement_decomposer::do_process_sql_statement, this, _1);

  Mysql_sql_parser_fe sql_parser_fe(bec::GRTManager::get()->get_app_option_string("SqlMode"));
  sql_parser_fe.ignore_dml = false;

  return process_sql_statement(sql, select_statement, sql_parser_fe);
}

int Mysql_sql_statement_decomposer::process_sql_statement(const std::string &sql, SelectStatement::Ref select_statement,
                                                          Mysql_sql_parser_fe &sql_parser_fe) {
  _select_statement = select_statement;

  // set delimiter for sql script if needed
  std::string sql_ = "DELIMITER " + _non_std_sql_delimiter + EOL + sql + EOL + _non_std_sql_delimiter;

  int res = parse_sql_script(sql_parser_fe, sql_.c_str());

  if (res != 0)
    return res;

  for (FromItem &from_item : _select_statement->from_items) {
    if (!from_item.subquery.empty()) {
      from_item.statement.reset(new SelectStatement());
      from_item.statement->parent = select_statement;
      res = process_sql_statement(from_item.subquery, from_item.statement, sql_parser_fe);
      if (res != 0)
        return res;
    }
  }

  return res;
}

int Mysql_sql_statement_decomposer::do_process_sql_statement(const SqlAstNode *tree) {
  if (!tree) {
    report_sql_error(_err_tok_lineno, true, _err_tok_line_pos, _err_tok_len, _err_msg, 2);
    return 1;
  }

  if (tree)
    tree = tree->subitem(sql::_statement);
  if (tree) {
    const SqlAstNode *tree_ = tree->subitem(sql::_create);
    if (tree_)
      tree = tree_;
  }

  if (!tree)
    return 1;

  if (pr_processed == _do_process_sql_statement(tree))
    return 0;
  else
    return 1;
}

Mysql_sql_parser_base::Parse_result Mysql_sql_statement_decomposer::decompose_query(const SqlAstNode *select_init) {
  const SqlAstNode *select_part2 = select_init->subitem(sql::_select_init2, sql::_select_part2);

  // Views now have a separate path for their select part.
  if (select_part2 == NULL)
    select_part2 = select_init->subitem(sql::_create_view_select, sql::_select_part2);

  if (select_part2 == NULL)
    return pr_invalid;

  // from clause
  {
    if (const SqlAstNode *dual_sym = select_part2->subitem(sql::_select_into, sql::_select_from, sql::_DUAL_SYM)) {
      FromItem from_item;
      from_item.table = dual_sym->restore_sql_text(_sql_statement);
      from_item.alias = from_item.table;
      _select_statement->from_items.push_back(from_item);
    } else if (const SqlAstNode *derived_table_list = select_part2->subitem(
                 sql::_select_into, sql::_select_from, sql::_join_table_list, sql::_derived_table_list)) {
      // collect table_factor nodes
      struct Helper {
        static void process(const SqlAstNode *table_ref, std::list<const SqlAstNode *> &table_factor_list) {
          if (const SqlAstNode *join_table = table_ref->subitem(sql::_join_table))
            table_ref = join_table;

          if (const SqlAstNode *table_factor = table_ref->subitem(sql::_table_factor)) {
            table_factor_list.push_back(table_factor);
            process(table_factor, table_factor_list); // table_factor may contain table_ref subitem
          }
          const SqlAstNode *table_ref1 = table_ref->subitem(sql::_table_ref);
          if (table_ref1)
            process(table_ref1, table_factor_list);
          const SqlAstNode *table_ref2 = table_ref->find_subseq(table_ref->subitem(1), sql::_table_ref);
          if (table_ref2 && (table_ref1 != table_ref2))
            process(table_ref2, table_factor_list);
        }
      };
      std::list<const SqlAstNode *> table_factor_list;
      for (const SqlAstNode *esc_table_ref : *derived_table_list->subitems())
        if (const SqlAstNode *table_ref = esc_table_ref->subitem(sql::_table_ref))
          Helper::process(table_ref, table_factor_list);

      // process table_factor nodes
      for (const SqlAstNode *table_factor : table_factor_list) {
        FromItem from_item;

        // alias
        const SqlAstNode *alias = table_factor->subitem(sql::_opt_table_alias, sql::_ident);
        if (alias)
          from_item.alias = alias->restore_sql_text(_sql_statement);

        if (const SqlAstNode *table_ident = table_factor->subitem(sql::_table_ident))
        // table ref
        {
          std::list<std::string> idents;
          for (const SqlAstNode *ident : *table_ident->subitems())
            if (ident->name_equals(sql::_ident))
              idents.push_back(ident->restore_sql_text(_sql_statement));
          std::list<std::string>::const_reverse_iterator i = idents.rbegin(), end = idents.rend();
          from_item.table = *i;
          if (++i != end)
            from_item.schema = *i;
        } else
        // subquery
        {
          if (from_item.subquery.empty()) {
            const SqlAstNode *select_derived_init = table_factor->subseq(sql::_select_derived_init);
            if (select_derived_init)
              from_item.subquery = table_factor->restore_sql_text(_sql_statement);
          }

          if (from_item.subquery.empty()) {
            if (const SqlAstNode *parenthesis_open =
                  table_factor->subitem(sql::_40)) // sql::_40 == ASCII('('); sql::_41 == ASCII(')')
            {
              from_item.subquery =
                table_factor->restore_sql_text(_sql_statement, parenthesis_open, table_factor->subitem(sql::_41));
              std::string::size_type pos_begin = from_item.subquery.find_first_not_of("( \t\n\r");
              std::string::size_type pos_end = from_item.subquery.find_last_not_of(") \t\n\r");
              from_item.subquery = from_item.subquery.substr(pos_begin, pos_end - pos_begin + 1);
            }
          }
        }

        if (from_item.alias.empty())
          from_item.alias = from_item.table;

        if (!from_item.table.empty() || !from_item.subquery.empty())
          _select_statement->from_items.push_back(from_item);
      }
    }
  }

  // fields
  {
    const SqlAstNode *select_item_list = select_part2->subitem(sql::_select_item_list);
    if (!select_item_list) {
      const SqlAstNode *select_paren = select_init->subitem(sql::_select_paren);
      while (const SqlAstNode *select_paren_ = select_paren->subitem(sql::_select_paren))
        select_paren = select_paren_;
      select_item_list = select_paren->subitem(sql::_select_part2, sql::_select_item_list);
    } else {
      for (const SqlAstNode *select_item : *select_item_list->subitems()) {
        if (select_item->name_equals(sql::_select_item)) {
          SelectItem select_item_;

          if (const SqlAstNode *table_wild = select_item->subitem(sql::_table_wild))
          // table wildcard
          {
            std::list<std::string> idents;
            for (const SqlAstNode *ident : *table_wild->subitems())
              if (ident->name_equals(sql::_ident))
                idents.push_back(ident->restore_sql_text(_sql_statement));
            std::list<std::string>::const_reverse_iterator i = idents.rbegin(), end = idents.rend();
            select_item_.table = *i;
            if (++i != end)
              select_item_.schema = *i;
            select_item_.wildcard = true;
          } else if (const SqlAstNode *expr = select_item->subitem(sql::_expr))
          // expression
          {
            const SqlAstNode *simple_ident = NULL;
            {
              const SqlAstNode *simple_expr =
                expr->subitem(sql::_bool_pri, sql::_predicate, sql::_bit_expr, sql::_simple_expr);
              while (simple_expr) {
                if ((simple_ident = simple_expr->subitem(sql::_simple_ident))) {
                  break;
                }

                if (simple_expr->find_subseq(sql::_40, sql::_expr,
                                             sql::_41)) // sql::_40 == ASCII('('); sql::_41 == ASCII(')')
                  simple_expr = simple_expr->subitem(sql::_expr, sql::_bool_pri, sql::_predicate, sql::_bit_expr,
                                                     sql::_simple_expr);
                else
                  simple_expr = NULL;
              }
            }

            if (simple_ident)
            // field ref
            {
              const SqlAstNode *simple_ident_q = simple_ident->subitem(sql::_simple_ident_q);
              if (simple_ident_q) {
                std::list<std::string> idents;
                for (const SqlAstNode *ident : *simple_ident_q->subitems())
                  if (ident->name_equals(sql::_ident))
                    idents.push_back(ident->restore_sql_text(_sql_statement));
                std::list<std::string>::const_reverse_iterator i = idents.rbegin(), end = idents.rend();
                select_item_.field = *i;
                if (++i != end)
                  select_item_.table = *i;
                if (++i != end)
                  select_item_.schema = *i;
              } else {
                select_item_.field = simple_ident->restore_sql_text(_sql_statement);
              }
            } else
            // complex expression
            {
              select_item_.expr = expr->restore_sql_text(_sql_statement);
            }
          }

          // alias
          const SqlAstNode *select_alias = select_item->subitem(sql::_select_alias);
          if (select_alias) {
            const SqlAstNode *ident = select_alias->subitem(sql::_ident);
            if (!ident)
              ident = select_alias->subitem(sql::_TEXT_STRING_sys);
            select_item_.alias = ident->restore_sql_text(_sql_statement);
          }

          _select_statement->select_items.push_back(select_item_);
        } else if (select_item->name_equals(sql::_42)) // sql::_42 == ASCII('*')
        {
          SelectItem select_item_;
          select_item_.wildcard = true;
          _select_statement->select_items.push_back(select_item_);
        }
      }
    }
  }

  return pr_processed;
}

Mysql_sql_parser_base::Parse_result Mysql_sql_statement_decomposer::do_decompose_query(const SqlAstNode *tree) {
  const SqlAstNode *select_init = tree->subitem(sql::_select, sql::_select_init);

  if (!select_init)
    return pr_irrelevant;

  return decompose_query(select_init);
}

Mysql_sql_parser_base::Parse_result Mysql_sql_statement_decomposer::do_decompose_view(const SqlAstNode *tree) {
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

  if (!view_tail)
    return pr_irrelevant;

  const SqlAstNode *view_select_aux = view_tail->subitem(sql::_view_select, sql::_view_select_aux);

  // process all subsequent sql as 'select' queries
  _do_process_sql_statement = boost::bind(&Mysql_sql_statement_decomposer::do_decompose_query, this, _1);

  Parse_result res = decompose_query(view_select_aux);

  // save explicit columns names if any
  if (const SqlAstNode *view_list = view_tail->subitem(sql::_view_list_opt, sql::_view_list)) {
    for (const SqlAstNode *ident : *view_list->subitems())
      if (ident->name_equals(sql::_ident))
        _view_columns_names.push_back(ident->restore_sql_text(_sql_statement));
  }

  return res;
}

int Mysql_sql_statement_decomposer::decompose_view(db_ViewRef view, SelectStatement::Ref select_statement) {
  db_SchemaRef schema = db_SchemaRef::cast_from(view->owner());
  grt::ListRef<db_Schema> schemata = db_CatalogRef::cast_from(schema->owner())->schemata();

  std::string ddl = view->sqlDefinition();
  _view_columns_names.clear();
  int res = decompose_view(ddl, select_statement);
  if (res) {
    expand_wildcards(select_statement, schema, schemata);

    // overwrite explicit column names
    if (!_view_columns_names.empty()) {
      std::list<std::string>::iterator alias = _view_columns_names.begin();
      for (SelectItem &select_item : _select_statement->select_items) {
        select_item.alias = *alias;
        ++alias;
      }
      _view_columns_names.clear();
    }
  }
  return res;
}

void Mysql_sql_statement_decomposer::expand_wildcards(SelectStatement::Ref select_statement, db_SchemaRef &db_schema,
                                                      grt::ListRef<db_Schema> &db_schemata) {
  std::list<SelectItems::iterator> wildcard_fields;
  for (SelectItems::iterator i = select_statement->select_items.begin(), i_end = select_statement->select_items.end();
       i != i_end; ++i)
    if ((*i).wildcard)
      wildcard_fields.push_back(i);
  for (SelectItems::iterator &wildcard_field : wildcard_fields) {
    std::string active_schema = normalize_identifier_case(db_schema->name());
    std::string context_schema = normalize_identifier_case(wildcard_field->schema);
    std::string context_table = normalize_identifier_case(wildcard_field->table);

    // find corresponding from_item(s) & expand wildcard
    {
      SelectStatement::Ref context_statement = select_statement;
      while (context_statement) {
        for (FromItem &from_item : context_statement->from_items) {
          std::string alias = normalize_identifier_case(from_item.alias);
          std::string table = normalize_identifier_case(from_item.table);

          if (context_table.empty() || alias == context_table) {
            std::string schema = normalize_identifier_case(from_item.schema);

            if ((context_schema.empty() && schema.empty()) || (context_schema == schema) ||
                (context_schema.empty() && active_schema == schema) ||
                (context_schema == active_schema && schema.empty())) {
              if (from_item.statement) {
                expand_wildcards(from_item.statement, db_schema, db_schemata);
                for (const SelectItem &select_item : from_item.statement->select_items) {
                  SelectItem new_select_item;
                  new_select_item.schema = schema;
                  new_select_item.table = from_item.alias;
                  new_select_item.field = select_item.effective_alias();
                  select_statement->select_items.insert(wildcard_field, new_select_item);
                }
              } else {
                std::string affective_schema =
                  !schema.empty() ? schema : (!context_schema.empty() ? context_schema : active_schema);
                db_SchemaRef db_schema =
                  find_named_object_in_list(db_schemata, affective_schema, _case_sensitive_identifiers);
                db_TableRef db_table =
                  find_named_object_in_list(db_schema->tables(), table, _case_sensitive_identifiers);
                if (db_table.is_valid()) {
                  GRTLIST_FOREACH(db_Column, db_table->columns(), col) {
                    SelectItem new_select_item;
                    new_select_item.schema = schema;
                    new_select_item.table = from_item.alias;
                    new_select_item.field = (*col)->name();
                    select_statement->select_items.insert(wildcard_field, new_select_item);
                  }
                } else {
                  db_ViewRef db_view =
                    find_named_object_in_list(db_schema->views(), table, _case_sensitive_identifiers);
                  if (db_view.is_valid()) {
                    for (grt::StringListRef::const_iterator col = db_view->columns().begin();
                         col != db_view->columns().end(); ++col) {
                      SelectItem new_select_item;
                      new_select_item.schema = schema;
                      new_select_item.table = from_item.alias;
                      new_select_item.field = *col;
                      select_statement->select_items.insert(wildcard_field, new_select_item);
                    }
                  }
                }
              }
            }
          }
        }
        context_statement = context_table.empty() ? SelectStatement::Ref() : context_statement->parent;
      }
    }
  }

  for (SelectItems::iterator &wildcard_field : wildcard_fields)
    select_statement->select_items.erase(wildcard_field);
}
