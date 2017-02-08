/*
 * Copyright (c) 2012, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _WIN32

#include <mysql.h>
#include <sql.h>

#endif

class BaseConverter {
  static void init_mysql_time(MYSQL_TIME* target);

public:
  static void convert_date(DATE_STRUCT* source, MYSQL_TIME* target);
  static void convert_date(const char* source, MYSQL_TIME* target);
  static void convert_time(const char* source, MYSQL_TIME* target);
  static void convert_timestamp(const char* source, MYSQL_TIME* target);
  static void convert_timestamp(TIMESTAMP_STRUCT* source, MYSQL_TIME* target);
  static void convert_date_time(const char* source, MYSQL_TIME* target, int type);
};
