/*
 * Copyright (c) 2012, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "copytable.h"

#include "converter.h"
#include "base/log.h"
#include "base/string_utilities.h"
#include <string>
#include <cstdlib>

DEFAULT_LOG_DOMAIN("copytable");

void BaseConverter::init_mysql_time(MYSQL_TIME* target) {
  target->year = 0;
  target->month = 0;
  target->day = 0;
  target->hour = 0;
  target->minute = 0;
  target->second = 0;
  target->second_part = 0;
  target->time_type = MYSQL_TIMESTAMP_NONE;
  target->neg = 0;
}

void BaseConverter::convert_date(DATE_STRUCT* source, MYSQL_TIME* target) {
  init_mysql_time(target);

  target->year = source->year;
  target->month = source->month;
  target->day = source->day;

  target->time_type = MYSQL_TIMESTAMP_DATE;
}

void BaseConverter::convert_date(const char* source, MYSQL_TIME* target) {
  init_mysql_time(target);

  // Date could come in the format of YYYY-MM-DD
  //                                  0123456789
  // Additional formats might be added as needed
  std::string time(source);

  // A call to std::string.substr with a start position past the end
  // of the string will yield an out_of_range exception, hence:
  if (time.length() < 10) // 9 is also valid but probably shouldn't be accepted
  {
    logWarning("Invalid date literal detected: '%s'\n", source);
    return;
  }

  target->year = base::atoi<int>(time.substr(0, 4).c_str(), 0);
  target->month = base::atoi<int>(time.substr(5, 2).c_str(), 0);
  target->day = base::atoi<int>(time.substr(8, 2).c_str(), 0);

  target->time_type = MYSQL_TIMESTAMP_DATE;
}

void BaseConverter::convert_time(const char* source, MYSQL_TIME* target) {
  init_mysql_time(target);

  // time comes in the format of
  // HH:MM:SS.mmmmm
  // 01234567890123
  std::string time(source);

  if (time.length() < 8) {
    logWarning("Invalid time literal detected: '%s'\n", source);
    return;
  }

  std::string msec = "0";

  // Get the milliseconds if present
  if (time.length() > 9) {
    msec = time.substr(9, time.length() - 9);
    msec.resize(6, '0');
  }

  target->hour = base::atoi<int>(time.substr(0, 2).c_str(), 0);
  target->minute = base::atoi<int>(time.substr(3, 2).c_str(), 0);
  target->second = base::atoi<int>(time.substr(6, 2).c_str(), 0);
  target->second_part = base::atoi<int>(msec.c_str(), 0);

  target->time_type = MYSQL_TIMESTAMP_TIME;
}

void BaseConverter::convert_timestamp(TIMESTAMP_STRUCT* source, MYSQL_TIME* target) {
  target->year = source->year;
  target->month = source->month;
  target->day = source->day;

  target->hour = source->hour;
  target->minute = source->minute;
  target->second = source->second;
  target->second_part = source->fraction;

  target->time_type = MYSQL_TIMESTAMP_DATETIME;
  target->neg = 0;
}

void BaseConverter::convert_timestamp(const char* source, MYSQL_TIME* target) {
  init_mysql_time(target);

  // Timestamp comes in the format of YYYY-MM-DD HH:MM:SS:mmm...
  //                                  01234567890123456789012...
  // Additional formats might be added as needed
  std::string time(source);

  if (time.length() < 19) {
    logWarning("Invalid timestamp literal detected: '%s'\n", source);
    return;
  }

  std::string msecond = "0";

  // Get the milliseconds if present
  if (time.length() > 20) {
    msecond = time.substr(20, time.length() - 20);
    msecond.resize(6, '0');
  }

  target->year = base::atoi<int>(time.substr(0, 4).c_str(), 0);
  target->month = base::atoi<int>(time.substr(5, 2).c_str(), 0);
  target->day = base::atoi<int>(time.substr(8, 2).c_str(), 0);

  target->hour = base::atoi<int>(time.substr(11, 2).c_str(), 0);
  target->minute = base::atoi<int>(time.substr(14, 2).c_str(), 0);
  target->second = base::atoi<int>(time.substr(17, 2).c_str(), 0);
  target->second_part = base::atoi<int>(msecond.c_str(), 0);

  target->time_type = MYSQL_TIMESTAMP_DATETIME;
}

void BaseConverter::convert_date_time(const char* source, MYSQL_TIME* target, int type) {
  switch (type) {
    case MYSQL_TYPE_DATE:
      convert_date(source, target);
      break;
    case MYSQL_TYPE_TIME:
      convert_time(source, target);
      break;
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_TIMESTAMP:
      convert_timestamp(source, target);
      break;
  }
}
