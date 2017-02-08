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
#include "sqlide/sqlide_generics.h"
#include "grt.h"
#include <list>
#include <string>

/**
 * Defines interface to generalize above DBMS SQL specifics.
 *
 * @ingroup sqlparser
 */
class WBPUBLICBACKEND_PUBLIC_FUNC Sql_specifics {
public:
  typedef std::shared_ptr<Sql_specifics> Ref;
  virtual ~Sql_specifics() {
  }

protected:
  Sql_specifics();

public:
  virtual std::string limit_select_query(const std::string &sql, int *row_count, int *offset);
  virtual void get_connection_startup_script(std::list<std::string> &sql_script);
  virtual std::string query_connection_id();
  virtual std::string query_kill_connection(std::int64_t connection_id);
  virtual std::string query_kill_query(std::int64_t connection_id);
  virtual std::string query_variable(const std::string &name);
  virtual sqlide::QuoteVar::Escape_sql_string escape_sql_string();
  virtual sqlide::QuoteVar::Blob_to_string blob_to_string();
  virtual std::string setting_non_std_sql_delimiter();
  virtual std::string non_std_sql_delimiter();
  virtual std::string setting_ansi_quotes();
};
