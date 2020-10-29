/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mysql_sql_specifics.h"
#include "mysql_sql_parser_fe.h"
#include "grtdb/charset_utils.h"
#include "base/string_utilities.h"
#include <boost/scoped_array.hpp>
#include <sstream>

using namespace grt;
using namespace base;

#define NULL_STATE_KEEPER Null_state_keeper _nsk(this);

class MYSQL_SQL_PARSER_PUBLIC_FUNC Mysql_sql_statement_info : protected Mysql_sql_parser_base {
public:
  Mysql_sql_statement_info()
    : _row_count(nullptr),
      _row_offset(nullptr),
      _contains_limit_clause(nullptr),
      _limit_ins_pos(nullptr),
      _statement_valid(false) {
    NULL_STATE_KEEPER
  }
  virtual ~Mysql_sql_statement_info() {
  }

private:
  int *_row_count;
  int *_row_offset;
  bool *_contains_limit_clause;
  size_t *_limit_ins_pos;
  bool _statement_valid;

public:
  bool get_limit_clause_params(const std::string &sql, int *row_count, int *row_row_offset, bool *contains_limit_clause,
                               size_t *limit_ins_pos) {
    NULL_STATE_KEEPER

    _row_count = row_count;
    _row_offset = row_row_offset;
    _contains_limit_clause = contains_limit_clause;
    _limit_ins_pos = limit_ins_pos;
    _statement_valid = false;

    _process_sql_statement = boost::bind(&Mysql_sql_statement_info::process_sql_statement, this, boost::placeholders::_1);

    Mysql_sql_parser_fe sql_parser_fe(bec::GRTManager::get()->get_app_option_string("SqlMode"));
    sql_parser_fe.ignore_dml = false;
    Mysql_sql_parser_base::parse_sql_script(sql_parser_fe, sql.c_str());

    return _statement_valid;
  }

protected:
  int process_sql_statement(const SqlAstNode *tree) {
    if (tree) {
      _statement_valid = true;

      if (const SqlAstNode *item = tree->subitem(sql::_statement, sql::_select))
        process_select_statement(item);
    }

    return 0; // error count
  }

  Parse_result process_select_statement(const SqlAstNode *tree) {
    // Look for an existing limit clause in a normal SELECT.
    {
      static sql::symbol path1[] = {sql::_select_init, sql::_select_init2,     sql::_select_part2, sql::_select_into,
                                    sql::_select_from, sql::_opt_limit_clause, sql::_limit_clause, sql::_};
      static sql::symbol path2[] = {sql::_select_init,
                                    sql::_select_init2,
                                    sql::_select_part2,
                                    sql::_select_into,
                                    sql::_opt_limit_clause,
                                    sql::_limit_clause,
                                    sql::_};
      static sql::symbol *paths[] = {path1, path2};

      const SqlAstNode *limit_clause = tree->search_by_paths(paths, ARR_CAPACITY(paths));
      if (limit_clause) {
        const SqlAstNode *limit_options = limit_clause->subitem(sql::_limit_options);

        const SqlAstNode *limit_row_offset = *limit_options->subitems()->begin();
        const SqlAstNode *limit_rowcount = *limit_options->subitems()->rbegin();
        if (limit_rowcount == limit_row_offset)
          limit_row_offset = NULL;
        else if (limit_clause->subitem(sql::_OFFSET_SYM))
          std::swap(limit_rowcount, limit_row_offset);

        if (limit_row_offset) {
          std::stringstream ss;
          ss << limit_row_offset->restore_sql_text(_sql_statement);
          ss >> *_row_offset;
        } else
          *_row_offset = 0;

        {
          std::stringstream ss;
          ss << limit_rowcount->restore_sql_text(_sql_statement);
          ss >> *_row_count;
        }
      }

      *_contains_limit_clause = (limit_clause != NULL);
    }

    if (*_contains_limit_clause)
      return pr_processed;

    // Look for an INTO clause, which signals us not to add a LIMIT clause either.
    {
      static sql::symbol path1[] = {sql::_select_init, sql::_select_init2, sql::_select_part2,
                                    sql::_select_into, sql::_into,         sql::_};
      static sql::symbol *paths[] = {path1};

      const SqlAstNode *into_clause = tree->search_by_paths(paths, ARR_CAPACITY(paths));
      if (into_clause != NULL) {
        *_contains_limit_clause = true; // Not entirely true but does the job.
        return pr_processed;
      }
    }

    // Now look if this is a UNION statement (UNION at top level only, not sure if nested UNIONS are even possible).
    // We don't add a LIMIT clause for such queries as we would have to handle each individual select
    // statement then.
    {
      static sql::symbol path1[] = {sql::_select_init, sql::_select_init2, sql::_union_clause, sql::_};
      static sql::symbol *paths[] = {path1};

      const SqlAstNode *union_clause = tree->search_by_paths(paths, ARR_CAPACITY(paths));
      if (union_clause != NULL) {
        *_contains_limit_clause = true;
        return pr_processed;
      }
    }

    // look for clauses that go after limit clause
    {
      static sql::symbol path1[] = {sql::_select_init, sql::_select_init2, sql::_select_part2,
                                    sql::_select_into, sql::_into,         sql::_};
      static sql::symbol path2[] = {sql::_select_init, sql::_select_init2, sql::_select_part2, sql::_select_lock_type,
                                    sql::_};
      // TODO: looks _procedure_analyse_clause, _procedure_analyse_param should be used now instead of _procedure_clause
      static sql::symbol path3[] = {sql::_select_init,
                                    sql::_select_init2,
                                    sql::_select_part2,
                                    sql::_select_into,
                                    sql::_select_from,
                                    sql::_procedure_analyse_clause,
                                    sql::_};
      static sql::symbol *paths[] = {path1, path2, path3};

      const SqlAstNode *afterlimit_clause = tree->search_by_paths(paths, ARR_CAPACITY(paths));
      if (afterlimit_clause) {
        *_limit_ins_pos = (size_t)afterlimit_clause->stmt_boffset();
      } else {
        *_limit_ins_pos = _sql_statement.empty() ? 0U : _sql_statement.size();
      }
    }

    return pr_processed;
  }
};

Mysql_sql_specifics::Mysql_sql_specifics() {
}

std::string Mysql_sql_specifics::limit_select_query(const std::string &sql, int *row_count, int *row_row_offset) {
  Mysql_sql_statement_info statement_info;
  bool contains_limit_clause = false;
  size_t limit_ins_pos = sql.length();

  if (statement_info.get_limit_clause_params(sql, row_count, row_row_offset, &contains_limit_clause, &limit_ins_pos)) {
    if (!contains_limit_clause) {
      std::string limit_clause = strfmt("\nLIMIT %i, %i\n", *row_row_offset, *row_count);
      std::string res;
      res.reserve(sql.size() + limit_clause.size());
      res = sql;
      res.insert(limit_ins_pos, limit_clause);
      return res;
    }
  }
  return sql;
}

void Mysql_sql_specifics::get_connection_startup_script(std::list<std::string> &sql_script) {
  sql_script.push_back("SET CHARACTER SET utf8");
  sql_script.push_back("SET NAMES utf8");
}

std::string Mysql_sql_specifics::query_connection_id() {
  return "SELECT CONNECTION_ID()";
}

std::string Mysql_sql_specifics::query_kill_connection(std::int64_t connection_id) {
#ifdef __GNUC__
  return strfmt("KILL CONNECTION %lli", (long long int)connection_id);
#else
  return strfmt("KILL CONNECTION %i", connection_id);
#endif
}

std::string Mysql_sql_specifics::query_kill_query(std::int64_t connection_id) {
#ifdef __GNUC__
  return strfmt("KILL QUERY %lli", (long long int)connection_id);
#else
  return strfmt("KILL QUERY %i", connection_id);
#endif
}

std::string Mysql_sql_specifics::query_variable(const std::string &name) {
  return strfmt("SHOW SESSION VARIABLES LIKE '%s'", name.c_str());
}

std::string escape_c_string_(const std::string &text) {
  std::string res;
  Mysql_sql_parser_fe::escape_string(text, res);
  return res;
}

sqlide::QuoteVar::Escape_sql_string Mysql_sql_specifics::escape_sql_string() {
  bool ansi_sql_strings = false;

  grt::ValueRef sql_mode_value = bec::GRTManager::get()->get_app_option("SqlMode");
  if (sql_mode_value.is_valid() && grt::StringRef::can_wrap(sql_mode_value)) {
    std::string sql_mode_string = toupper(grt::StringRef::cast_from(sql_mode_value));
    std::istringstream iss(sql_mode_string);
    std::string mode;
    while (std::getline(iss, mode, ',')) {
      if (mode == "NO_BACKSLASH_ESCAPES") {
        ansi_sql_strings = true;
        break;
      }
    }
  }

  return (ansi_sql_strings) ? &sqlide::QuoteVar::escape_ansi_sql_string : &escape_c_string_;
}

std::string blob_to_string_(const unsigned char *data, size_t size) {
  boost::scoped_array<char> out(new char[size * 2 + 1]);
  Mysql_sql_parser_fe::escape_string(out.get(), 0, (const char *)data, (unsigned long)size);
  return std::string(out.get());
}

sqlide::QuoteVar::Blob_to_string Mysql_sql_specifics::blob_to_string() {
  return blob_to_string_;
}

std::string Mysql_sql_specifics::setting_non_std_sql_delimiter() {
  return "DELIMITER " + non_std_sql_delimiter() + EolHelpers::eol();
}

std::string Mysql_sql_specifics::non_std_sql_delimiter() {
  return bec::GRTManager::get()->get_app_option_string("SqlDelimiter", "$$");
}

std::string Mysql_sql_specifics::setting_ansi_quotes() {
  return "SET @@sql_mode=concat(@@sql_mode, ',ANSI_QUOTES')";
}
