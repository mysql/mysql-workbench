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

#ifndef _SQLIDE_GENERICS_PRIVATE_H_
#define _SQLIDE_GENERICS_PRIVATE_H_

#include "wbpublic_public_interface.h"
#include <sqlite/database_exception.hpp>
#include <sqlite/result.hpp>
#include <sqlite/query.hpp>
#include <sqlite/execute.hpp>
#include <ctime>

namespace sqlide {

  using namespace sqlite;

  class WBPUBLICBACKEND_PUBLIC_FUNC BindSqlCommandVar : public boost::static_visitor<void> {
  public:
    BindSqlCommandVar() : _sql_command(NULL) {
    }
    BindSqlCommandVar(sqlite::command *sql_command) : _sql_command(sql_command) {
    }
    void sql_command(sqlite::command *sql_command) {
      _sql_command = sql_command;
    }

  protected:
    sqlite::command *_sql_command;

  public:
    result_type operator()(const long double &v) const {
      (*_sql_command) % (double)v;
    }
    result_type operator()(const int &v) const {
      (*_sql_command) % v;
    }
    result_type operator()(const std::int64_t &v) const {
      (*_sql_command) % v;
    }
    result_type operator()(const std::string &v) const {
      (*_sql_command) % v;
    }
    result_type operator()(const null_t &v) const {
      (*_sql_command) % sqlite::nil;
    }
    result_type operator()(const unknown_t &v) const {
      (*_sql_command) % sqlite::nil;
    }
    result_type operator()(const blob_ref_t &v) const {
      //    This code looks wrong since the beginning, but looks wrong and causes bug #70817 where '' stored in a
      //    binary/blob column
      //    is shown as NULL. '' is not the same as NULL even as a BLOB

      if (v->empty())
        (*_sql_command) % "";
      //      (*_sql_command) % sqlite::nil;
      else
        (*_sql_command) % *v;
    }

    template <typename T>
    result_type operator()(const T &v) const {
      throw std::runtime_error(std::string("BindSqlCommandVar: not implemented: ") + typeid(T).name());
    }
  };

} // namespace sqlide

#endif /* _SQLIDE_GENERICS_H_ */
