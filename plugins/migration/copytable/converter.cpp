/*
 * Copyright (c) 2012, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "converter.h"
#include "base/log.h"

#include <string>
#include <cstdlib>

DEFAULT_LOG_DOMAIN("copytable");

void BaseConverter::init_mysql_time(MYSQL_TIME* target)
{
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

void BaseConverter::convert_date(DATE_STRUCT* source, MYSQL_TIME* target)
{
  init_mysql_time(target);

  target->year = source->year;
  target->month = source->month;
  target->day = source->day;

  target->time_type = MYSQL_TIMESTAMP_DATE;
}

void BaseConverter::convert_date(const char* source, MYSQL_TIME* target)
{
  init_mysql_time(target);

  // Date could come in the format of YYYY-MM-DD
  //                                  0123456789
  // Additional formats might be added as needed
  std::string time(source);

  // A call to std::string.substr with a start position past the end
  // of the string will yield an out_of_range exception, hence:
  if (time.length() < 10)  // 9 is also valid but probably shouldn't be accepted
  {
    log_warning("Invalid date literal detected: '%s'\n", source);
    return;
  }

  std::string year  = time.substr(0, 4);
  std::string month = time.substr(5, 2);
  std::string day   = time.substr(8, 2);

  target->year  = atoi(year.data());
  target->month = atoi(month.data());
  target->day   = atoi(day.data());

  target->time_type = MYSQL_TIMESTAMP_DATE;
}

void BaseConverter::convert_time(const char* source, MYSQL_TIME* target)
{
  init_mysql_time(target);

  // time comes in the format of
  // HH:MM:SS.mmmmm
  // 01234567890123
  std::string time(source);

  if (time.length() < 8)
  {
    log_warning("Invalid time literal detected: '%s'\n", source);
    return;
  }

  std::string hour = time.substr(0, 2);
  std::string min  = time.substr(3, 2);
  std::string sec  = time.substr(6, 2);
  std::string msec = "0";

  // Get the milliseconds if present
  if (time.length() > 9)
    msec = time.substr(9, time.length()-9);

  target->hour = atoi(hour.data());
  target->minute = atoi(min.data());
  target->second = atoi(sec.data());
  target->second_part = atoi(msec.data());

  target->time_type = MYSQL_TIMESTAMP_TIME;
}

void BaseConverter::convert_timestamp(TIMESTAMP_STRUCT* source, MYSQL_TIME* target)
{
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

void BaseConverter::convert_timestamp(const char* source, MYSQL_TIME* target)
{
  init_mysql_time(target);

  // Timestamp comes in the format of YYYY-MM-DD HH:MM:SS:mmm...
  //                                  01234567890123456789012...
  // Additional formats might be added as needed
  std::string time(source);

  if (time.length() < 19)
  {
    log_warning("Invalid timestamp literal detected: '%s'\n", source);
    return;
  }

  std::string year  = time.substr(0, 4);
  std::string month = time.substr(5, 2);
  std::string day   = time.substr(8, 2);

  std::string hour    = time.substr(11, 2);
  std::string minute  = time.substr(14, 2);
  std::string second  = time.substr(17, 2);
  std::string msecond = "0";

  // Get the milliseconds if present
  if (time.length() > 20)
    msecond = time.substr(20, time.length()-20);

  target->year  = atoi(year.data());
  target->month = atoi(month.data());
  target->day   = atoi(day.data());

  target->hour        = atoi(hour.data());
  target->minute      = atoi(minute.data());
  target->second      = atoi(second.data());
  target->second_part = atoi(msecond.data());

  target->time_type = MYSQL_TIMESTAMP_DATETIME;
}

void BaseConverter::convert_date_time(const char* source, MYSQL_TIME* target, int type)
{
  switch(type)
  {
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
