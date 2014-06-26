/*
 * Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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

#include <sqlite/execute.hpp>
#include <sqlite/query.hpp>
#include <sqlite/database_exception.hpp>
#include <glib.h>

#include "base/string_utilities.h"
#include "base/log.h"
#include "base/file_utilities.h"
#include "base/sqlstring.h"
#include "grt/common.h"

using namespace bec;


#include "column_width_cache.h"

DEFAULT_LOG_DOMAIN("column_widths");

ColumnWidthCache::ColumnWidthCache(const std::string &connection_id, const std::string &cache_dir)
: _connection_id(connection_id)
{
  _sqconn = new sqlite::connection(make_path(cache_dir, connection_id)+".column_widths");
  sqlite::execute(*_sqconn, "PRAGMA temp_store=MEMORY", true);
  sqlite::execute(*_sqconn, "PRAGMA synchronous=NORMAL", true);

  log_debug2("Using column width cache file %s\n", (make_path(cache_dir, connection_id)+".column_widths").c_str());

  // check if the DB is already initialized
  sqlite::query q(*_sqconn, "select name from sqlite_master where type='table'");
  int found = 0;
  if (q.emit())
  {
    boost::shared_ptr<sqlite::result> res(q.get_result());
    do
    {
      std::string name = res->get_string(0);
      if (name == "widths")
        found++;
    }
    while (res->next_row());
  }
  if (found == 0)
  {
    log_debug3("Initializing cache\n");
    init_db();
  }
}


void ColumnWidthCache::init_db()
{
  std::string code = "create table widths (column_id varchar(100) primary key, width int)";

  log_info("Initializing column width cache for %s\n", _connection_id.c_str());
  try
  {
    sqlite::execute(*_sqconn, code, true);
  }
  catch (std::exception &exc)
  {
    log_error("Error creating cache %s: %s\n", code.c_str(), exc.what());
  }
}


void ColumnWidthCache::save_column_width(const std::string &column_id, int width)
{
  sqlite::query q(*_sqconn, "update widths set width=? where column_id=?");
  q.bind(1, width);
  q.bind(2, column_id);
  try
  {
    q.emit();
    if (q.get_result()->get_row_count() == 0)
    {
      // try inserting if update didn't do anything
      sqlite::query q(*_sqconn, "insert into widths values (?, ?)");
      q.bind(1, column_id);
      q.bind(2, width);
      q.emit();
    }
  }
  catch (std::exception &exc)
  {
    log_error("Error storing column width to cache %s: %s\n", column_id.c_str(), exc.what());
  }
}


int ColumnWidthCache::get_column_width(const std::string &column_id)
{
  sqlite::query q(*_sqconn, "select width from widths where column_id = ?");
  q.bind(1, column_id);
  try
  {
    if (q.emit())
    {
      boost::shared_ptr<sqlite::result> res(q.get_result());
      return res->get_int(0);
    }
  }
  catch (std::exception &exc)
  {
    log_error("Error storing column width to cache %s: %s\n", column_id.c_str(), exc.what());
  }
  return -1;
}
