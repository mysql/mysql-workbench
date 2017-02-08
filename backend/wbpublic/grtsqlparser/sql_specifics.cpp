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

#include "sql_specifics.h"

Sql_specifics::Sql_specifics() {
}

std::string Sql_specifics::limit_select_query(const std::string &sql, int *row_count, int *offset) {
  // since there is no standart syntax do-nothing by default
  return sql;
}

void Sql_specifics::get_connection_startup_script(std::list<std::string> &sql_script) {
}

std::string Sql_specifics::query_connection_id() {
  return "";
}

std::string Sql_specifics::query_kill_connection(std::int64_t connection_id) {
  return "";
}

std::string Sql_specifics::query_kill_query(std::int64_t connection_id) {
  return "";
}

std::string Sql_specifics::query_variable(const std::string &name) {
  return "";
}

sqlide::QuoteVar::Escape_sql_string Sql_specifics::escape_sql_string() {
  return &sqlide::QuoteVar::escape_ansi_sql_string;
}

sqlide::QuoteVar::Blob_to_string Sql_specifics::blob_to_string() {
  return sqlide::QuoteVar::Blob_to_string();
}

std::string Sql_specifics::setting_non_std_sql_delimiter() {
  return "";
}

std::string Sql_specifics::non_std_sql_delimiter() {
  return ";";
}

std::string Sql_specifics::setting_ansi_quotes() {
  return "";
}
