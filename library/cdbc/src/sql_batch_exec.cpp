/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "sql_batch_exec.h"
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <memory>

namespace sql {

  SqlBatchExec::SqlBatchExec()
    : _batch_exec_success_count(0),
      _batch_exec_err_count(0),
      _batch_exec_progress_state(0),
      _batch_exec_progress_inc(0),
      _stop_on_error(true) {
  }

  long SqlBatchExec::operator()(sql::Statement *stmt, std::list<std::string> &statements) {
    _batch_exec_success_count = 0;
    _batch_exec_err_count = 0;
    _sql_log.clear();

    exec_sql_script(stmt, statements, _batch_exec_err_count);
    if (_batch_exec_err_count && !_failback_statements.empty()) {
      long failback_script_exec_err_count = 0;
      exec_sql_script(stmt, _failback_statements, failback_script_exec_err_count);
      _batch_exec_err_count += failback_script_exec_err_count;
    }

    if (_batch_exec_stat_cb)
      _batch_exec_stat_cb(_batch_exec_success_count, _batch_exec_err_count);

    return _batch_exec_err_count;
  }

  void SqlBatchExec::exec_sql_script(sql::Statement *stmt, std::list<std::string> &statements,
                                     long &batch_exec_err_count) {
    _batch_exec_progress_state = 0;
    _batch_exec_progress_inc = 1.f / statements.size();

    for (std::list<std::string>::const_iterator i = statements.begin(), i_end = statements.end(); i != i_end; ++i) {
      try {
        _sql_log.push_back(*i);
        if (stmt->execute(*i))
          std::unique_ptr<sql::ResultSet> rs(stmt->getResultSet());
        ++_batch_exec_success_count;
      } catch (SQLException &e) {
        ++batch_exec_err_count;
        if (!_error_cb)
          throw;
        else {
          if (&_batch_exec_err_count != &batch_exec_err_count) // applies only to failback scripts
            _error_cb(-1, "Error when running failback script. Details follow.", "");
          _error_cb(e.getErrorCode(), e.what(), *i);
        }
      }
      _batch_exec_progress_state += _batch_exec_progress_inc;
      if (_batch_exec_progress_cb)
        _batch_exec_progress_cb(_batch_exec_progress_state);

      if (batch_exec_err_count && _stop_on_error)
        break;
    }
  }

} // namespace sql
