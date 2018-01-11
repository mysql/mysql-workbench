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

#include "sqlide_generics_private.h"

#include "recordset_sqlite_storage.h"
#include "recordset_be.h"
#include "grtsqlparser/sql_facade.h"
#include "base/string_utilities.h"
#include "base/boost_smart_ptr_helpers.h"
#include <sqlite/execute.hpp>
#include <sqlite/query.hpp>
#include <algorithm>
#include <ctype.h>

using namespace bec;
using namespace grt;
using namespace base;

Recordset_sqlite_storage::Recordset_sqlite_storage() : Recordset_sql_storage() {
}

Recordset_sqlite_storage::~Recordset_sqlite_storage() {
}

std::string Recordset_sqlite_storage::decorated_sql_query(Recordset::Column_names &column_names) {
  std::string sql_query;
  if (_sql_query.empty()) {
    if (column_names.empty())
      sql_query = strfmt("select *, rowid from %s", full_table_name().c_str());
    else {
      sql_query = "select ";
      for (const auto &column_name : column_names)
        sql_query += strfmt("`%s`, ", column_name.c_str());
      sql_query += "rowid from " + full_table_name();
    }
  } else {
    sql_query = _sql_query;
  }
  return sql_query;
}

void Recordset_sqlite_storage::do_unserialize(Recordset *recordset, sqlite::connection *data_swap_db) {
  Recordset_sql_storage::do_unserialize(recordset, data_swap_db);

  Recordset::Column_names &column_names = get_column_names(recordset);
  Recordset::Column_types &column_types = get_column_types(recordset);
  Recordset::Column_types &real_column_types = get_real_column_types(recordset);
  Recordset::Column_flags &column_flags = get_column_flags(recordset);

  std::string sql_query = decorated_sql_query(column_names);

  sqlite::connection conn(_db_path);
  sqlite::query q(conn, sql_query);
  bool rs_contains_rows = q.emit();

  std::shared_ptr<sqlite::result> rs = BoostHelper::convertPointer(q.get_result());

  _valid = (NULL != rs.get());
  if (!_valid)
    return;

  int col_count = rs->get_column_count();

  // column types
  static std::map<std::string, sqlite::variant_t> known_types;
  static std::map<std::string, bool> known_quoted_types;
  {
    struct Known_type_initializer {
      Known_type_initializer() {
        long double ld = 0.0L;
        known_types["BLOB"] = sqlite::blob_ref_t();
        known_quoted_types["BLOB"] = true;
        known_types["CHAR"] = std::string();
        known_quoted_types["CHAR"] = true;
        known_types["MEMO"] = std::string();
        known_quoted_types["MEMO"] = true;
        known_types["DATE"] = std::string();
        known_quoted_types["DATE"] = true;
        known_types["DATETIME"] = std::string();
        known_quoted_types["DATETIME"] = true;
        known_types["FLOAT"] = ld;
        known_quoted_types["FLOAT"] = false;
        known_types["INTEGER"] = int();
        known_quoted_types["INTEGER"] = false;
        known_types["TEXT"] = std::string();
        known_quoted_types["TEXT"] = true;
        known_types["VARCHAR"] = std::string();
        known_quoted_types["VARCHAR"] = true;
        known_types["REAL"] = ld;
        known_quoted_types["REAL"] = false;
      }
    };
    static Known_type_initializer known_type_initializer;
  }

  column_types.reserve(col_count);
  // some column types might be defined in derived class. don't redefine types for those columns.
  for (int n = (int)column_types.size(); n < col_count; ++n) {
    std::string type_name = rs->get_column_decltype(n);
    type_name = base::toupper(type_name);
    std::string::size_type tne = type_name.find('(');
    type_name = type_name.substr(0, tne);
    column_types.push_back(known_types[type_name]);
    real_column_types.push_back(known_types[type_name]);
    int flags = 0;
    if (known_quoted_types[type_name])
      flags = Recordset::NeedsQuoteFlag;
    column_flags.push_back(flags);
  }

  // column names
  column_names.reserve(col_count);
  // some column names might be defined in derived class. don't redefine names for those columns.
  for (int n = (int)column_names.size(); n < col_count; ++n)
    column_names.push_back(rs->get_column_name(n));

  // pkey columns
  // if (!_readonly) // pk fields can be of use to fetch blob values as needed
  _pkey_columns.push_back(col_count - 1);

  // data
  {
    sqlide::Sqlite_transaction_guarder transaction_guarder(data_swap_db);

    create_data_swap_tables(data_swap_db, column_names, column_types);
    if (rs_contains_rows) {
      Var_vector row_values(col_count);
      std::list<std::shared_ptr<sqlite::command> > insert_commands =
        prepare_data_swap_record_add_statement(data_swap_db, column_names);

      do {
        for (int n = 0, count = col_count; n < count; ++n) {
          sqlite::variant_t v;
          v = rs->get_variant(n);
          sqlite::variant_t r = column_types[n];
          v = boost::apply_visitor(_var_cast, r, v);
          row_values[n] = v;
        }
        add_data_swap_record(insert_commands, row_values);
      } while (rs->next_row());
    }

    transaction_guarder.commit();
  }
}

void Recordset_sqlite_storage::do_serialize(const Recordset *recordset, sqlite::connection *data_swap_db) {
  Recordset_sql_storage::do_serialize(recordset, data_swap_db);

  SqlFacade::Ref sql_facade = SqlFacade::instance_for_rdbms_name("Mysql"); //!
  Sql_script sql_script;
  sql_facade->splitSqlScript(this->sql_script(), sql_script.statements);
  run_sql_script(sql_script, false);
}

void Recordset_sqlite_storage::run_sql_script(const Sql_script &sql_script, bool skip_commit) {
  sqlite::connection conn(_db_path);
  sqlide::optimize_sqlite_connection_for_speed(&conn);
  {
    sqlide::Sqlite_transaction_guarder transaction_guarder(&conn);

    Sql_script::Statements_bindings::const_iterator sql_bindings = sql_script.statements_bindings.begin();
    for (const auto &sql : sql_script.statements) {
      sqlite::command sql_command(conn, sql);
      sqlide::BindSqlCommandVar sql_var_binder(&sql_command);
      if (sql_script.statements_bindings.end() != sql_bindings) {
        for (const auto &bind_var : *sql_bindings)
          boost::apply_visitor(sql_var_binder, bind_var);
        ++sql_bindings;
      }
      sql_command.emit();
    }
  }
  //! sqlite::execute(conn, "vacuum", true); //! do it in cleanup proc
}

void Recordset_sqlite_storage::do_fetch_blob_value(Recordset *recordset, sqlite::connection *data_swap_db, RowId rowid,
                                                   ColumnId column, sqlite::variant_t &blob_value) {
  Recordset::Column_names &column_names = get_column_names(recordset);
  // unused  Recordset::Column_types &column_types= get_column_types(recordset);
  // unused  Recordset::Column_types &real_column_types= get_real_column_types(recordset);

  if (column >= column_names.size())
    return;

  std::string sql_query = decorated_sql_query(column_names);
  {
    std::string pkey_predicate;
    get_pkey_predicate_for_data_cache_rowid(recordset, data_swap_db, rowid, pkey_predicate);
    sql_query = strfmt("select `%s` from (%s) t where %s", column_names[column].c_str(), sql_query.c_str(),
                       pkey_predicate.c_str());
  }

  sqlite::connection conn(_db_path);
  sqlite::query q(conn, sql_query);
  bool rs_contains_rows = q.emit();
  std::shared_ptr<sqlite::result> rs = BoostHelper::convertPointer(q.get_result());

  _valid = (NULL != rs.get());
  if (!_valid)
    return;

  if (rs_contains_rows) {
    do
      (blob_value = rs->get_variant(0));
    while (rs->next_row());
  }
}
