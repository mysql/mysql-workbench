/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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
