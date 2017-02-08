/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#pragma once

#include "wbpublic_public_interface.h"
#include "sqlide/recordset_sql_storage.h"
#include "cppdbc.h"

class WBPUBLICBACKEND_PUBLIC_FUNC Recordset_cdbc_storage : public Recordset_sql_storage {
public:
  struct FieldInfo {
    std::string catalog;
    std::string schema;
    std::string table;
    std::string field;
    std::string type;
    std::string charset;
    int display_size;
    int precision;
    int scale;
  };

  typedef std::shared_ptr<Recordset_cdbc_storage> Ref;
  static Ref create() {
    return Ref(new Recordset_cdbc_storage());
  }
  virtual ~Recordset_cdbc_storage();

protected:
  Recordset_cdbc_storage();

protected:
  virtual void do_unserialize(Recordset *recordset, sqlite::connection *data_swap_db);
  virtual void do_fetch_blob_value(Recordset *recordset, sqlite::connection *data_swap_db, RowId rowid, ColumnId column,
                                   sqlite::variant_t &blob_value);

protected:
  virtual void run_sql_script(const Sql_script &sql_script, bool skip_transaction);

public:
  std::string decorated_sql_query(); // adds limit clause if defined by options

public:
  void setAuxConnectionGetter(
    std::function<base::RecMutexLock(sql::Dbc_connection_handler::Ref &, bool)> getConnection) {
    _getAuxConnection = getConnection;
  };
  void setUserConnectionGetter(
    std::function<base::RecMutexLock(sql::Dbc_connection_handler::Ref &, bool)> getConnection) {
    _getUserConnection = getConnection;
  };

  void dbc_resultset(std::shared_ptr<sql::ResultSet> &value) {
    _dbc_resultset = value;
  }
  void dbc_statement(std::shared_ptr<sql::Statement> &value) {
    _dbc_statement = value;
  }
  bool reloadable() const {
    return _reloadable;
  }
  void reloadable(bool val) {
    _reloadable = val;
  }

  void set_gather_field_info(bool flag) {
    _gather_field_info = flag;
  }
  std::vector<FieldInfo> &field_info() {
    return _field_info;
  }

private:
  std::function<base::RecMutexLock(sql::Dbc_connection_handler::Ref &, bool)> _getAuxConnection;
  std::function<base::RecMutexLock(sql::Dbc_connection_handler::Ref &, bool)> _getUserConnection;

  std::shared_ptr<sql::ResultSet> _dbc_resultset; // for 1-time unserialization
  std::shared_ptr<sql::Statement> _dbc_statement; // for 1-time unserialization
  std::vector<FieldInfo> _field_info;
  bool _reloadable; // whether can be reloaded using stored sql query
  bool _gather_field_info;

  size_t determine_pkey_columns(Recordset::Column_names &column_names, Recordset::Column_types &column_types,
                                Recordset::Column_types &real_column_types);
  size_t determine_pkey_columns_alt(Recordset::Column_names &column_names, Recordset::Column_types &column_types,
                                    Recordset::Column_types &real_column_types);
};
