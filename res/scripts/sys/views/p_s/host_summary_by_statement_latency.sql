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
 * View: host_summary_by_statement_latency
 *
 * Summarizes overall statement statistics by host.
 *
 * mysql> select * from host_summary_by_statement_latency;
 * +------+-------+---------------+-------------+--------------+-----------+---------------+---------------+------------+
 * | host | total | total_latency | max_latency | lock_latency | rows_sent | rows_examined | rows_affected | full_scans |
 * +------+-------+---------------+-------------+--------------+-----------+---------------+---------------+------------+
 * | hal  |  3381 | 00:02:09.13   | 1.48 s      | 1.07 s       |      1151 |         93947 |           150 |         91 |
 * +------+-------+---------------+-------------+--------------+-----------+---------------+---------------+------------+
 *
 */

CREATE OR REPLACE
  ALGORITHM = TEMPTABLE
  DEFINER = 'root'@'localhost'
  SQL SECURITY INVOKER 
VIEW host_summary_by_statement_latency (
  host,
  total,
  total_latency,
  max_latency,
  lock_latency,
  rows_sent,
  rows_examined,
  rows_affected,
  full_scans
) AS
SELECT host,
       SUM(total) AS total,
       sys.format_time(SUM(total_latency)) AS total_latency,
       sys.format_time(SUM(max_latency)) AS max_latency,
       sys.format_time(SUM(lock_latency)) AS lock_latency,
       SUM(rows_sent) AS rows_sent,
       SUM(rows_examined) AS rows_examined,
       SUM(rows_affected) AS rows_affected,
       SUM(full_scans) AS full_scans
  FROM sys.x$host_summary_by_statement_type
 GROUP BY host
 ORDER BY SUM(total_latency) DESC;

/*
 * View: x$host_summary_by_statement_latency
 *
 * Summarizes overall statement statistics by host.
 *
 * mysql> select * from x$host_summary_by_statement_latency;
 * +------+-------+-----------------+---------------+---------------+-----------+---------------+---------------+------------+
 * | host | total | total_latency   | max_latency   | lock_latency  | rows_sent | rows_examined | rows_affected | full_scans |
 * +------+-------+-----------------+---------------+---------------+-----------+---------------+---------------+------------+
 * | hal  |  3382 | 129134039432000 | 1483246743000 | 1069831000000 |      1152 |         94286 |           150 |         92 |
 * +------+-------+-----------------+---------------+---------------+-----------+---------------+---------------+------------+
 *
 */

CREATE OR REPLACE
  ALGORITHM = TEMPTABLE
  DEFINER = 'root'@'localhost'
  SQL SECURITY INVOKER 
VIEW x$host_summary_by_statement_latency (
  host,
  total,
  total_latency,
  max_latency,
  lock_latency,
  rows_sent,
  rows_examined,
  rows_affected,
  full_scans
) AS
SELECT host,
       SUM(total) AS total,
       SUM(total_latency) AS total_latency,
       SUM(max_latency) AS max_latency,
       SUM(lock_latency) AS lock_latency,
       SUM(rows_sent) AS rows_sent,
       SUM(rows_examined) AS rows_examined,
       SUM(rows_affected) AS rows_affected,
       SUM(full_scans) AS full_scans
  FROM sys.x$host_summary_by_statement_type
 GROUP BY host
 ORDER BY SUM(total_latency) DESC;
