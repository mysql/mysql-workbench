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

#include "sqlide_generics.h"
#include <sqlite/execute.hpp>
#ifndef _MSC_VER
#include <sys/time.h>
#endif
#include <locale>

namespace sqlide {

  class IsVarTypeEqTo : public boost::static_visitor<bool> {
  public:
    IsVarTypeEqTo() {
    }
    template <typename T>
    result_type operator()(const T &v1, const T &v2) const {
      return true;
    }
    template <typename T1, typename T2>
    result_type operator()(const T1 &v1, const T2 &v2) const {
      return false;
    }
  };
  static const IsVarTypeEqTo is_var_type_eq_to;

  bool is_var_null(const sqlite::variant_t &value) {
    static const sqlite::variant_t null_value = sqlite::null_t();
    return boost::apply_visitor(is_var_type_eq_to, value, null_value);
  }

  bool is_var_unknown(const sqlite::variant_t &value) {
    static const sqlite::variant_t unknown_value = sqlite::unknown_t();
    return boost::apply_visitor(is_var_type_eq_to, value, unknown_value);
  }

  bool is_var_blob(const sqlite::variant_t &value) {
    static const sqlite::variant_t blob_value = sqlite::blob_ref_t();
    return boost::apply_visitor(is_var_type_eq_to, value, blob_value);
  }

  void optimize_sqlite_connection_for_speed(sqlite::connection *conn) {
    //! sqlite::execute(*conn, "pragma locking_mode = exclusive", true);
    sqlite::execute(*conn, "pragma fsync = 0", true);
    sqlite::execute(*conn, "pragma synchronous = off", true);
    sqlite::execute(*conn, "pragma encoding = \"UTF-8\"", true);
    sqlite::execute(*conn, "pragma temp_store = 2", true);
    sqlite::execute(*conn, "pragma auto_vacuum = 0", true);
    sqlite::execute(*conn, "pragma count_changes = 0", true);
    sqlite::execute(*conn, "pragma journal_mode = OFF");
  }

  Sqlite_transaction_guarder::Sqlite_transaction_guarder(sqlite::connection *conn, bool use_immediate)
    : _conn(conn), _in_trans(false) {
    if (_conn) {
      if (use_immediate)
        sqlite::execute(*conn, "begin immediate", true);
      else
        sqlite::execute(*conn, "BEGIN", true);
      _in_trans = true;
    }
  }

  Sqlite_transaction_guarder::~Sqlite_transaction_guarder() {
    if (!_in_trans)
      return;
    const char *action = std::uncaught_exception() ? "rollback" : "commit";
    sqlite::execute(*_conn, action, true);
  }

  void Sqlite_transaction_guarder::commit() {
    sqlite::execute(*_conn, "commit", true);
    _in_trans = false;
  }

  void Sqlite_transaction_guarder::commit_and_start_new_transaction() {
    commit();
    sqlite::execute(*_conn, "begin", true);
    _in_trans = true;
  }

} // namespace sqlide

std::tm local_timestamp() {
  std::time_t ltime;
  time(&ltime);
  std::tm t;
#ifdef _MSC_VER
  localtime_s(&t, &ltime);
#else
  localtime_r(&ltime, &t);
#endif
  return t;
}

std::string format_time(const std::tm &t, const char *format) {
  const size_t BUFFER_SIZE = 256;
  char buf[BUFFER_SIZE];
  strftime(buf, BUFFER_SIZE, format, &t);
  return std::string(buf);
}

std::string current_time(const char *format) {
  return format_time(local_timestamp(), format);
}
