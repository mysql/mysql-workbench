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

#include <sqlite/execute.hpp>
#include <sqlite/query.hpp>

#include "recordset_table_inserts_storage.h"
#include "recordset_be.h"
#include "base/string_utilities.h"

#include <algorithm>
#include <list>
#include <ctype.h>

using namespace bec;
using namespace grt;
using namespace base;

Recordset_table_inserts_storage::Recordset_table_inserts_storage(const std::string &path) : Recordset_sqlite_storage() {
  db_path(path);
}

Recordset_table_inserts_storage::~Recordset_table_inserts_storage() {
}

void Recordset_table_inserts_storage::do_unserialize(Recordset *recordset, sqlite::connection *data_swap_db) {
  Recordset::Column_names &column_names = get_column_names(recordset);
  Recordset::Column_types &column_types = get_column_types(recordset);
  Recordset::Column_types &real_column_types = get_real_column_types(recordset);
  Recordset::Column_flags &column_flags = get_column_flags(recordset);
  Recordset::DBColumn_types &dbColumnTypes = getDbColumnTypes(recordset);

  reinit(_mapped_colnames);
  _mapped_table_name = _table->id();
  table_name(_table->name());

  ListRef<db_Column> columns(_table->columns());
  size_t col_count = columns.count();
  if (0 == col_count)
    throw std::runtime_error(strfmt("Table %s doesn't have any column", full_table_name().c_str()));

  AutoSwap<std::string> table_name_mapper(_table_name, _mapped_table_name);

  // column types
  class Known_types {
  private:
    std::map<std::string, sqlite::variant_t> known_typegroups;
    std::map<std::string, sqlite::variant_t> known_real_typegroups;

  public:
    Known_types() {
      known_typegroups[""] = std::string();
      known_typegroups["com.mysql.rdbms.common.typegroup.numeric"] = std::string();
      known_typegroups["com.mysql.rdbms.common.typegroup.string"] = std::string();
      known_typegroups["com.mysql.rdbms.common.typegroup.text"] = std::string();
      known_typegroups["com.mysql.rdbms.common.typegroup.blob"] = sqlite::blob_ref_t();
      known_typegroups["com.mysql.rdbms.common.typegroup.datetime"] = std::string();
      known_typegroups["com.mysql.rdbms.common.typegroup.gis"] = sqlite::unknown_t();
      known_typegroups["com.mysql.rdbms.common.typegroup.various"] = std::string();
      known_typegroups["com.mysql.rdbms.common.typegroup.userdefined"] = std::string();
      known_typegroups["com.mysql.rdbms.common.typegroup.structured"] = std::string();

      long double ld = 0.0L;
      known_real_typegroups = known_typegroups;
      known_real_typegroups["com.mysql.rdbms.common.typegroup.numeric"] = ld;
    }

    inline sqlite::variant_t map_simple_datatype(db_SimpleDatatypeRef simple_datatype, bool real_type) {
      sqlite::variant_t mapped_type;
      std::string datatype_group_name;
      if (simple_datatype.is_valid()) {
        db_DatatypeGroupRef datatype_group = simple_datatype->group();
        if (datatype_group.is_valid() && datatype_group->id() == "com.mysql.rdbms.common.typegroup.numeric"  &&
            simple_datatype->numericScale() == 0) {
          if (real_type) {
            if (8 < simple_datatype->numericPrecision())
              mapped_type = int();
            else
              mapped_type = std::int64_t();
          } else {
            mapped_type = std::string();
          }
        } else if ("com.mysql.rdbms.mysql.datatype.bit" == simple_datatype->id()) {
          mapped_type = sqlite::unknown_t();
        } else {
          if (datatype_group.is_valid())
            datatype_group_name = datatype_group->id();
          std::map<std::string, sqlite::variant_t> *typegroups = real_type ? &known_real_typegroups : &known_typegroups;
          mapped_type = (*typegroups)[datatype_group_name];
        }
      } else {
        mapped_type = sqlite::unknown_t();
      }
      return mapped_type;
    }
  };
  static Known_types known_types;

  column_types.reserve(col_count);
  // some column types might be defined in derived class. don't redefine types for those columns.
  for (size_t n = column_types.size(); n < col_count; ++n) {
    db_ColumnRef column = columns.get(n);
    db_SimpleDatatypeRef stype(grt::Initialized);

    // handle user defined types
    if (column->simpleType().is_valid())
      stype = column->simpleType();
    else if (column->userType().is_valid())
      stype = column->userType()->actualType();

    if (stype.is_valid())
      dbColumnTypes.push_back(stype->name());
    column_types.push_back(known_types.map_simple_datatype(stype, false));
    real_column_types.push_back(known_types.map_simple_datatype(stype, true));
    int flags = 0;
    if (stype.is_valid())
      flags = *stype->needsQuotes() != 0 ? Recordset::NeedsQuoteFlag : 0;
    if (*column->isNotNull())
      flags = flags | (int)Recordset::NotNullFlag;
    column_flags.push_back(flags);
  }

  // column names
  // mapped column names (indifferent to original names)
  {
    column_names.reserve(col_count);
    _mapped_colnames.reserve(col_count);
    // some column names might be defined in derived class. don't redefine names for those columns.
    for (ColumnId n = column_names.size(), count = col_count; n < count; ++n) {
      db_ColumnRef col(columns[n]);
      column_names.push_back(col->name());
      _mapped_colnames.push_back(col->id());
    }
  }

  // create/adjust underlying table
  {
    sqlite::connection conn(db_path());
    {
      sqlide::Sqlite_transaction_guarder transaction_guarder(&conn);

      // create table if it doesn't exist yet
      {
        sqlite::query q(conn, strfmt("select count(1) from sqlite_master where tbl_name='%s'", _table->id().c_str()));
        sqlite::result_type rs = q.emit_result();
        if (0 == rs->get_int(0)) {
          sqlite::execute(conn, strfmt("create table %s (`_` varchar)", full_table_name().c_str()), true);
        }
      }

      // make a list of missing (newly added) columns
      {
        // calc missing column names
        Recordset::Column_names ncn; // needed column names
        {
          ncn.reserve(col_count);
          std::copy(_mapped_colnames.begin(), _mapped_colnames.end(), std::back_inserter(ncn));
          std::sort(ncn.begin(), ncn.end());
        }
        Recordset::Column_names ecn; // existing column names
        {
          ecn.reserve(col_count);
          sqlite::query q(conn, strfmt("pragma table_info(%s)", full_table_name().c_str()));
          if (q.emit()) {
            sqlite::result_type rs = q.get_result();
            do
              ecn.push_back(rs->get_string(1)); // colindex: 1 -> "name"
            while (rs->next_row());
            std::sort(ecn.begin(), ecn.end());
          }
        }
        Recordset::Column_names mcn; // missing column names
        {
          mcn.reserve(col_count);
          std::set_difference(ncn.begin(), ncn.end(), ecn.begin(), ecn.end(), std::back_inserter(mcn));
        }

        if (!mcn.empty()) {
          // alter table structure
          for (const std::string &cn : mcn) {
            sqlite::execute(
              conn, strfmt("alter table %s add column `%s` varchar", full_table_name().c_str(), cn.c_str()), true);
          }
        }
      }

      // commit is required after ddl statements
      transaction_guarder.commit_and_start_new_transaction();
    }
  }

  {
    AutoSwap<Recordset::Column_names> columns_names_mapper(column_names, _mapped_colnames);
    Recordset_sqlite_storage::do_unserialize(recordset, data_swap_db);
  }

  // copy aux columns into columns
  column_names.reserve(_mapped_colnames.size());
  std::copy(_mapped_colnames.begin() + column_names.size(), _mapped_colnames.end(), std::back_inserter(column_names));
}

void Recordset_table_inserts_storage::do_serialize(const Recordset *recordset, sqlite::connection *data_swap_db) {
  AutoSwap<std::string> table_name_mapper(_table_name, _mapped_table_name);

  Recordset::Column_names *colnames = const_cast<Recordset::Column_names *>(recordset->column_names());
  AutoSwap<Recordset::Column_names> columns_names_mapper(*colnames, _mapped_colnames);

  omit_schema_qualifier(true);
  Recordset_sqlite_storage::do_serialize(recordset, data_swap_db);
}

void Recordset_table_inserts_storage::generate_sql_script(const Recordset *recordset, sqlite::connection *data_swap_db,
                                                          Sql_script &sql_script, bool is_update_script, bool binaryAsString) {
  AutoSwap<std::string> table_name_mapper(_table_name, _mapped_table_name);

  Recordset::Column_names *colnames = const_cast<Recordset::Column_names *>(recordset->column_names());
  AutoSwap<Recordset::Column_names> columns_names_mapper(*colnames, _mapped_colnames);

  Recordset_sqlite_storage::generate_sql_script(recordset, data_swap_db, sql_script, is_update_script, true);
}

void Recordset_table_inserts_storage::do_apply_changes(const Recordset *recordset, sqlite::connection *data_swap_db,
                                                       bool skip_commit) {
  Recordset_sqlite_storage::do_apply_changes(recordset, data_swap_db, skip_commit);
  bec::GRTManager::get()->has_unsaved_changes(true);
}

void Recordset_table_inserts_storage::do_fetch_blob_value(Recordset *recordset, sqlite::connection *data_swap_db,
                                                          RowId rowid, ColumnId column, sqlite::variant_t &blob_value) {
  AutoSwap<std::string> table_name_mapper(_table_name, _mapped_table_name);

  Recordset::Column_names *colnames = const_cast<Recordset::Column_names *>(recordset->column_names());
  AutoSwap<Recordset::Column_names> columns_names_mapper(*colnames, _mapped_colnames);

  Recordset_sqlite_storage::do_fetch_blob_value(recordset, data_swap_db, rowid, column, blob_value);
}
