/*
 * Copyright (c) 2015, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <sqlite/connection.hpp>

#include <string>

class WBPUBLICBACKEND_PUBLIC_FUNC ColumnWidthCache {
  std::string _connection_id;
  sqlite::connection *_sqconn;

  void init_db();

public:
  ColumnWidthCache(const std::string &connection_id, const std::string &cache_dir);
  virtual ~ColumnWidthCache();

  void save_column_width(const std::string &column_id, int width);
  void save_columns_width(const std::map<std::string, int> &columns);
  int get_column_width(const std::string &column_id);
  void delete_column_width(const std::string &column_id);
};
