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

#include <sqlite/execute.hpp>
#include <sqlite/query.hpp>
#include <sqlite/database_exception.hpp>
#include <glib.h>

#include "base/string_utilities.h"
#include "base/log.h"
#include "base/file_utilities.h"
#include "base/sqlstring.h"
#include "base/boost_smart_ptr_helpers.h"
#include "grt/common.h"
#include "sqlide_generics.h"

using namespace bec;

#include "column_width_cache.h"

DEFAULT_LOG_DOMAIN("column_widths");

ColumnWidthCache::ColumnWidthCache(const std::string &connection_id, const std::string &cache_dir)
  : _connection_id(connection_id) {
  _sqconn = new sqlite::connection(base::makePath(cache_dir, connection_id) + ".column_widths");
  sqlite::execute(*_sqconn, "PRAGMA temp_store=MEMORY", true);
  sqlite::execute(*_sqconn, "PRAGMA synchronous=NORMAL", true);

  logDebug2("Using column width cache file %s\n",
            (base::makePath(cache_dir, connection_id) + ".column_widths").c_str());

  // check if the DB is already initialized
  sqlite::query q(*_sqconn, "select name from sqlite_master where type='table'");
  int found = 0;
  if (q.emit()) {
    std::shared_ptr<sqlite::result> res(BoostHelper::convertPointer(q.get_result()));
    do {
      std::string name = res->get_string(0);
      if (name == "widths")
        found++;
    } while (res->next_row());
  }
  if (found == 0) {
    logDebug3("Initializing cache\n");
    init_db();
  }
}

ColumnWidthCache::~ColumnWidthCache() {
  delete _sqconn;
}

void ColumnWidthCache::init_db() {
  std::string code = "create table widths (column_id varchar(100) primary key, width int)";

  logInfo("Initializing column width cache for %s\n", _connection_id.c_str());
  try {
    sqlite::execute(*_sqconn, code, true);
  } catch (std::exception &exc) {
    logError("Error creating cache %s: %s\n", code.c_str(), exc.what());
  }
}

void ColumnWidthCache::save_column_width(const std::string &column_id, int width) {
  try {
    sqlite::query q(*_sqconn, "insert or replace into widths values (?, ?)");
    q.bind(1, column_id);
    q.bind(2, width);
    q.emit();
  } catch (std::exception &exc) {
    logError("Error storing column width to cache %s: %s\n", column_id.c_str(), exc.what());
  }
}

void ColumnWidthCache::save_columns_width(const std::map<std::string, int> &columns) {
  std::map<std::string, int>::const_iterator it;
  try {
    sqlide::Sqlite_transaction_guarder transaction(_sqconn);
    sqlite::query q(*_sqconn, "insert or replace into widths values (?, ?)");
    for (it = columns.begin(); it != columns.end(); ++it) {
      q.bind(1, it->first);
      q.bind(2, it->second);
      q.emit();
      q.clear();
    }
  } catch (std::exception &exc) {
    logError("Error storing column width to cache %s: %s\n", it->first.c_str(), exc.what());
  }
}

int ColumnWidthCache::get_column_width(const std::string &column_id) {
  sqlite::query q(*_sqconn, "select width from widths where column_id = ?");
  q.bind(1, column_id);
  try {
    if (q.emit()) {
      std::shared_ptr<sqlite::result> res(BoostHelper::convertPointer(q.get_result()));
      return res->get_int(0);
    }
  } catch (std::exception &exc) {
    logError("Error storing column width to cache %s: %s\n", column_id.c_str(), exc.what());
  }
  return -1;
}

void ColumnWidthCache::delete_column_width(const std::string &column_id) {
  sqlite::query q(*_sqconn, "delete from widths where column_id = ?");
  q.bind(1, column_id);
  try {
    q.emit();
  } catch (std::exception &exc) {
    logDebug("Error deleting column width to cache %s: %s\n", column_id.c_str(), exc.what());
  }
}
