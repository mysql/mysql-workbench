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

#pragma once

#include "wbpublic_public_interface.h"
#include "sqlide/recordset_be.h"

namespace sqlite {
  struct command;
}

class WBPUBLICBACKEND_PUBLIC_FUNC Recordset_data_storage {
public:
  typedef std::shared_ptr<Recordset_data_storage> Ref;
  typedef std::weak_ptr<Recordset_data_storage> Ptr;
  virtual ~Recordset_data_storage();

protected:
  Recordset_data_storage();

  friend class Recordset;

public:
  typedef std::list<sqlite::variant_t> Var_list;
  typedef std::vector<sqlite::variant_t> Var_vector;

protected:
  std::shared_ptr<sqlite::connection> data_swap_db(const Recordset::Ref &recordset);

public:
  void apply_changes(Recordset::Ptr recordset, bool skip_commit);
  void serialize(Recordset::Ptr recordset);
  void unserialize(Recordset::Ptr recordset);
  void fetch_blob_value(Recordset::Ptr recordset, RowId rowid, ColumnId column, sqlite::variant_t &blob_value);

protected:
  virtual void fetch_blob_value(Recordset *recordset, sqlite::connection *data_swap_db, RowId rowid, ColumnId column,
                                sqlite::variant_t &blob_value);

protected:
  virtual void do_apply_changes(const Recordset *recordset, sqlite::connection *data_swap_db, bool skip_commit) = 0;
  virtual void do_serialize(const Recordset *recordset, sqlite::connection *data_swap_db) = 0;
  virtual void do_unserialize(Recordset *recordset, sqlite::connection *data_swap_db) = 0;
  virtual void do_fetch_blob_value(Recordset *recordset, sqlite::connection *data_swap_db, RowId rowid, ColumnId column,
                                   sqlite::variant_t &blob_value) = 0;

public:
  bool valid() {
    return _valid;
  }
  bool readonly() {
    return _readonly;
  }
  std::string readonly_reason() {
    return _readonly_reason;
  }
  void readonly_reason(const std::string &reason) {
    _readonly_reason = reason;
  }
  virtual ColumnId aux_column_count() = 0;

protected:
  bool _readonly;
  std::string _readonly_reason;
  bool _valid;

public:
  virtual bool reloadable() const {
    return true;
  }

public:
  static void create_data_swap_tables(sqlite::connection *data_swap_db, Recordset::Column_names &column_names,
                                      Recordset::Column_types &column_types);

protected:
  std::list<std::shared_ptr<sqlite::command> > prepare_data_swap_record_add_statement(
    sqlite::connection *data_swap_db, Recordset::Column_names &column_names);
  void add_data_swap_record(std::list<std::shared_ptr<sqlite::command> > &insert_commands, const Var_vector &values);
  void update_data_swap_record(sqlite::connection *data_swap_db, RowId rowid, ColumnId column,
                               const sqlite::variant_t &value);

protected:
  static Recordset::Column_names &get_column_names(Recordset *recordset) {
    return recordset->_column_names;
  }
  static Recordset::Column_types &get_column_types(Recordset *recordset) {
    return recordset->_column_types;
  }
  static Recordset::Column_types &get_real_column_types(Recordset *recordset) {
    return recordset->_real_column_types;
  }
  static Recordset::Column_flags &get_column_flags(Recordset *recordset) {
    return recordset->_column_flags;
  }
  static Recordset::DBColumn_types &getDbColumnTypes(Recordset *recordset) {
    return recordset->_dbColumnTypes; 
  }
  static const Recordset::Column_names &get_column_names(const Recordset *recordset) {
    return recordset->_column_names;
  }
  static const Recordset::Column_types &get_column_types(const Recordset *recordset) {
    return recordset->_column_types;
  }
  static const Recordset::Column_types &get_real_column_types(const Recordset *recordset) {
    return recordset->_real_column_types;
  }
  static const Recordset::Column_flags &get_column_flags(const Recordset *recordset) {
    return recordset->_column_flags;
  }
  static const Recordset::DBColumn_types &getDbColumnTypes(const Recordset *recordset) {
    return recordset->_dbColumnTypes;
  }

public:
  bool limit_rows() {
    return _limit_rows;
  }
  void limit_rows(bool value) {
    _limit_rows = value;
  }
  int limit_rows_count() {
    return _limit_rows_count;
  }
  void limit_rows_count(RowId value) {
    _limit_rows_count = (int)value;
  }
  bool limit_rows_applicable() {
    return _limit_rows_applicable;
  }
  void limit_rows_applicable(bool val) {
    _limit_rows_applicable = val;
  }
  int limit_rows_offset() {
    return _limit_rows_offset;
  }
  void scroll_rows_frame_forward() {
    _limit_rows_offset += _limit_rows_count;
  }
  void scroll_rows_frame_backward() {
    _limit_rows_offset = std::max<int>(0, (_limit_rows_offset - _limit_rows_count));
  }

protected:
  bool _limit_rows;
  int _limit_rows_count;
  int _limit_rows_offset;
  bool _limit_rows_applicable;
};
