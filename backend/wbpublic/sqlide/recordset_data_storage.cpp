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

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#include "sqlide_generics_private.h"
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

#include "recordset_data_storage.h"
#include "base/string_utilities.h"
#include "base/boost_smart_ptr_helpers.h"

using namespace bec;
using namespace grt;
using namespace base;

Recordset_data_storage::Recordset_data_storage()
  : _readonly(true),
    _valid(false),
    _limit_rows(false),
    _limit_rows_count(1000),
    _limit_rows_offset(0),
    _limit_rows_applicable(true) {
}

Recordset_data_storage::~Recordset_data_storage() {
}

std::shared_ptr<sqlite::connection> Recordset_data_storage::data_swap_db(const Recordset::Ref &recordset) {
  return recordset->data_swap_db();
}

void Recordset_data_storage::apply_changes(Recordset::Ptr recordset_ptr, bool skip_commit) {
  RETURN_IF_FAIL_TO_RETAIN_WEAK_PTR(Recordset, recordset_ptr, recordset)
  std::shared_ptr<sqlite::connection> data_swap_db = recordset->data_swap_db();
  do_apply_changes(recordset, data_swap_db.get(), skip_commit);
}

void Recordset_data_storage::serialize(Recordset::Ptr recordset_ptr) {
  RETURN_IF_FAIL_TO_RETAIN_WEAK_PTR(Recordset, recordset_ptr, recordset)
  std::shared_ptr<sqlite::connection> data_swap_db = recordset->data_swap_db();
  do_serialize(recordset, data_swap_db.get());
}

void Recordset_data_storage::unserialize(Recordset::Ptr recordset_ptr) {
  RETURN_IF_FAIL_TO_RETAIN_WEAK_PTR(Recordset, recordset_ptr, recordset)
  std::shared_ptr<sqlite::connection> data_swap_db = recordset->data_swap_db();
  do_unserialize(recordset, data_swap_db.get());
  recordset->rebuild_data_index(data_swap_db.get(), false, false);
}

void Recordset_data_storage::fetch_blob_value(Recordset::Ptr recordset_ptr, RowId rowid, ColumnId column,
                                              sqlite::variant_t &blob_value) {
  RETURN_IF_FAIL_TO_RETAIN_WEAK_PTR(Recordset, recordset_ptr, recordset)
  std::shared_ptr<sqlite::connection> data_swap_db = recordset->data_swap_db();
  fetch_blob_value(recordset, data_swap_db.get(), rowid, column, blob_value);
}

void Recordset_data_storage::fetch_blob_value(Recordset *recordset, sqlite::connection *data_swap_db, RowId rowid,
                                              ColumnId column, sqlite::variant_t &blob_value) {
  blob_value = sqlite::null_t();

  do_fetch_blob_value(recordset, data_swap_db, rowid, column, blob_value);

  // cache fetched blob in data swap db, blob shouldn't stay in memory for long
  if (!sqlide::is_var_null(blob_value)) {
    sqlide::Sqlite_transaction_guarder transaction_guarder(data_swap_db);
    update_data_swap_record(data_swap_db, rowid, column, blob_value);
    transaction_guarder.commit();
  }
}

void Recordset_data_storage::create_data_swap_tables(sqlite::connection *data_swap_db,
                                                     Recordset::Column_names &column_names,
                                                     Recordset::Column_types &column_types) {
  // generate sql
  std::list<std::string> data_partitions_creates;
  std::list<std::string> data_partitions_drops;
  std::list<std::string> deleted_rows_partitions_creates;
  std::list<std::string> deleted_rows_partitions_drops;
  {
    sqlide::TypeOfVar type_of_var;
    Recordset::Column_types::iterator column_type_i = column_types.begin();
    for (size_t partition = 0, partition_count = Recordset::data_swap_db_partition_count(column_names.size());
         partition < partition_count; ++partition) {
      std::string partition_suffix = Recordset::data_swap_db_partition_suffix(partition);
      std::ostringstream cr_table_stmt;
      cr_table_stmt << strfmt("create table if not exists `data%s` (", partition_suffix.c_str());
      for (ColumnId col = partition * Recordset::DATA_SWAP_DB_TABLE_MAX_COL_COUNT,
                    col_end = std::min<ColumnId>(column_names.size(),
                                                 (partition + 1) * Recordset::DATA_SWAP_DB_TABLE_MAX_COL_COUNT);
           col < col_end; ++col) {
        std::string column_type = boost::apply_visitor(type_of_var, *column_type_i);
        cr_table_stmt << "`_" << col << "` " << column_type << ", ";
        ++column_type_i;
      }
      cr_table_stmt << "id integer primary key autoincrement)";
      data_partitions_creates.push_back(cr_table_stmt.str());
      data_partitions_drops.push_back(strfmt("drop table if exists `data%s`", partition_suffix.c_str()));
      deleted_rows_partitions_creates.push_back(
        strfmt("create table if not exists `deleted_rows%s` as select * from `data%s`", partition_suffix.c_str(),
               partition_suffix.c_str()));
      deleted_rows_partitions_drops.push_back(
        strfmt("drop table if exists `deleted_rows%s`", partition_suffix.c_str()));
    }
  }

  // execute sql
  for (const std::string &ddl : data_partitions_drops)
    sqlite::execute(*data_swap_db, ddl, true);
  sqlite::execute(*data_swap_db, "drop table if exists `data_index`", true);
  for (const std::string &ddl : deleted_rows_partitions_drops)
    sqlite::execute(*data_swap_db, ddl, true);
  sqlite::execute(*data_swap_db, "drop table if exists `changes`", true);
  for (const std::string &ddl : data_partitions_creates)
    sqlite::execute(*data_swap_db, ddl, true);
  sqlite::execute(*data_swap_db, "create table if not exists `data_index` (`id` integer)", true);
  for (const std::string &ddl : deleted_rows_partitions_creates)
    sqlite::execute(*data_swap_db, ddl, true);
  sqlite::execute(*data_swap_db,
                  "create table if not exists `changes` (`id` integer primary key autoincrement, `record` integer, "
                  "`action` integer, `column` integer)",
                  true);
  sqlite::execute(*data_swap_db,
                  "create index if not exists `changes_idx_1` on `changes` (`record`, `action`, `column`)", true);
}

std::list<std::shared_ptr<sqlite::command> > Recordset_data_storage::prepare_data_swap_record_add_statement(
  sqlite::connection *data_swap_db, Recordset::Column_names &column_names) {
  std::list<std::shared_ptr<sqlite::command> > res;

  for (size_t partition = 0, partition_count = Recordset::data_swap_db_partition_count(column_names.size());
       partition < partition_count; ++partition) {
    std::string partition_suffix = Recordset::data_swap_db_partition_suffix(partition);
    std::ostringstream sql;
    sql << strfmt("insert into `data%s` (", partition_suffix.c_str());
    std::string col_delim;
    for (ColumnId col = partition * Recordset::DATA_SWAP_DB_TABLE_MAX_COL_COUNT,
                  col_end = std::min<ColumnId>(column_names.size(),
                                               (partition + 1) * Recordset::DATA_SWAP_DB_TABLE_MAX_COL_COUNT);
         col < col_end; ++col) {
      sql << col_delim << "`_" << col << "`";
      col_delim = ", ";
    }
    sql << ") values (";
    col_delim.clear();
    for (ColumnId col = partition * Recordset::DATA_SWAP_DB_TABLE_MAX_COL_COUNT,
                  col_count = std::min<ColumnId>(column_names.size(),
                                                 (partition + 1) * Recordset::DATA_SWAP_DB_TABLE_MAX_COL_COUNT);
         col < col_count; ++col) {
      sql << col_delim << "?";
      col_delim = ", ";
    }
    sql << ")";

    res.push_back(std::shared_ptr<sqlite::command>(new sqlite::command(*data_swap_db, sql.str())));
  }

  return res;
}

void Recordset_data_storage::add_data_swap_record(std::list<std::shared_ptr<sqlite::command> > &insert_commands,
                                                  const Var_vector &values) {
  size_t partition = 0;
  for (std::shared_ptr<sqlite::command> &insert_command : insert_commands) {
    insert_command->clear();
    sqlide::BindSqlCommandVar bind_sql_command_var(insert_command.get());
    for (ColumnId
           col = partition * Recordset::DATA_SWAP_DB_TABLE_MAX_COL_COUNT,
           col_end = std::min<ColumnId>(values.size(), (partition + 1) * Recordset::DATA_SWAP_DB_TABLE_MAX_COL_COUNT);
         col < col_end; ++col) {
      const sqlite::variant_t &value = values[col];
      boost::apply_visitor(bind_sql_command_var, value);
    }
    insert_command->emit();
    ++partition;
  }
}

void Recordset_data_storage::update_data_swap_record(sqlite::connection *data_swap_db, RowId rowid, ColumnId column,
                                                     const sqlite::variant_t &value) {
  size_t partition = Recordset::data_swap_db_column_partition(column);
  std::string partition_suffix = Recordset::data_swap_db_partition_suffix(partition);
  std::shared_ptr<sqlite::command> update_command(
    new sqlite::command(*data_swap_db, strfmt("update `data%s` set `_%u`=? where rowid=%u", partition_suffix.c_str(),
                                              (unsigned int)column, (unsigned int)rowid)));
  sqlide::BindSqlCommandVar bind_sql_command_var(update_command.get());
  boost::apply_visitor(bind_sql_command_var, value);
  update_command->emit();
}

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif