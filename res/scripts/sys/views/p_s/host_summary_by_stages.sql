/* Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

/*
 * View: host_summary_by_stages
 *
 * Summarizes stages by host, ordered by host and total latency per stage.
 * 
 * mysql> select * from host_summary_by_stages;
 * +------+--------------------------------+-------+-----------+-----------+
 * | host | event_name                     | total | wait_sum  | wait_avg  |
 * +------+--------------------------------+-------+-----------+-----------+
 * | hal  | stage/sql/Opening tables       |   889 | 1.97 ms   | 2.22 us   |
 * | hal  | stage/sql/Creating sort index  |     4 | 1.79 ms   | 446.30 us |
 * | hal  | stage/sql/init                 |    10 | 312.27 us | 31.23 us  |
 * | hal  | stage/sql/checking permissions |    10 | 300.62 us | 30.06 us  |
 * | hal  | stage/sql/freeing items        |     5 | 85.89 us  | 17.18 us  |
 * | hal  | stage/sql/statistics           |     5 | 79.15 us  | 15.83 us  |
 * | hal  | stage/sql/preparing            |     5 | 69.12 us  | 13.82 us  |
 * | hal  | stage/sql/optimizing           |     5 | 53.11 us  | 10.62 us  |
 * | hal  | stage/sql/Sending data         |     5 | 44.66 us  | 8.93 us   |
 * | hal  | stage/sql/closing tables       |     5 | 37.54 us  | 7.51 us   |
 * | hal  | stage/sql/System lock          |     5 | 34.28 us  | 6.86 us   |
 * | hal  | stage/sql/query end            |     5 | 24.37 us  | 4.87 us   |
 * | hal  | stage/sql/end                  |     5 | 8.60 us   | 1.72 us   |
 * | hal  | stage/sql/Sorting result       |     5 | 8.33 us   | 1.67 us   |
 * | hal  | stage/sql/executing            |     5 | 5.37 us   | 1.07 us   |
 * | hal  | stage/sql/cleaning up          |     5 | 4.60 us   | 919.00 ns |
 * +------+--------------------------------+-------+-----------+-----------+
 *
 */

CREATE OR REPLACE
  ALGORITHM = MERGE
  DEFINER = 'root'@'localhost'
  SQL SECURITY INVOKER 
VIEW host_summary_by_stages (
  host,
  event_name,
  total,
  wait_sum,
  wait_avg
) AS
SELECT host,
       event_name,
       count_star AS total,
       sys.format_time(sum_timer_wait) AS wait_sum, 
       sys.format_time(avg_timer_wait) AS wait_avg 
  FROM performance_schema.events_stages_summary_by_host_by_event_name
 WHERE host IS NOT NULL 
   AND sum_timer_wait != 0 
 ORDER BY host, sum_timer_wait DESC;

/*
 * View: x$host_summary_by_stages
 *
 * Summarizes stages by host, ordered by host and total latency per stage.
 * 
 * mysql> select * from x$host_summary_by_stages;
 * +------+--------------------------------+-------+-------------+-----------+
 * | host | event_name                     | total | wait_sum    | wait_avg  |
 * +------+--------------------------------+-------+-------------+-----------+
 * | hal  | stage/sql/Opening tables       |  1114 | 71919037000 |  64559000 |
 * | hal  | stage/sql/Creating sort index  |     5 |  2245762000 | 449152000 |
 * | hal  | stage/sql/init                 |    13 |   428798000 |  32984000 |
 * | hal  | stage/sql/checking permissions |    13 |   363231000 |  27940000 |
 * | hal  | stage/sql/freeing items        |     7 |   137728000 |  19675000 |
 * | hal  | stage/sql/statistics           |     6 |    93955000 |  15659000 |
 * | hal  | stage/sql/preparing            |     6 |    82571000 |  13761000 |
 * | hal  | stage/sql/optimizing           |     6 |    63338000 |  10556000 |
 * | hal  | stage/sql/Sending data         |     6 |    53400000 |   8900000 |
 * | hal  | stage/sql/closing tables       |     7 |    46922000 |   6703000 |
 * | hal  | stage/sql/System lock          |     6 |    40175000 |   6695000 |
 * | hal  | stage/sql/query end            |     7 |    31723000 |   4531000 |
 * | hal  | stage/sql/Sorting result       |     6 |     9855000 |   1642000 |
 * | hal  | stage/sql/end                  |     6 |     9556000 |   1592000 |
 * | hal  | stage/sql/cleaning up          |     7 |     7312000 |   1044000 |
 * | hal  | stage/sql/executing            |     6 |     6487000 |   1081000 |
 * +------+--------------------------------+-------+-------------+-----------+ *
 */

CREATE OR REPLACE
  ALGORITHM = MERGE
  DEFINER = 'root'@'localhost'
  SQL SECURITY INVOKER 
VIEW x$host_summary_by_stages (
  host,
  event_name,
  total,
  wait_sum,
  wait_avg
) AS
SELECT host,
       event_name,
       count_star AS total,
       sum_timer_wait AS wait_sum, 
       avg_timer_wait AS wait_avg 
  FROM performance_schema.events_stages_summary_by_host_by_event_name
 WHERE host IS NOT NULL 
   AND sum_timer_wait != 0 
 ORDER BY host, sum_timer_wait DESC;
