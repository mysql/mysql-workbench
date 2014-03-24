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
 * View: x$ps_digest_avg_latency_distribution
 *
 * Helper view for x$ps_digest_95th_percentile_by_avg_us
 *
 */

CREATE OR REPLACE
  ALGORITHM = TEMPTABLE
  DEFINER = 'root'@'localhost'
  SQL SECURITY INVOKER 
VIEW x$ps_digest_avg_latency_distribution (
  cnt,
  avg_us
) AS
SELECT COUNT(*) cnt, 
       ROUND(avg_timer_wait/1000000) AS avg_us
  FROM performance_schema.events_statements_summary_by_digest
 GROUP BY avg_us;

/*
 * View: x$ps_digest_95th_percentile_by_avg_us
 *
 * Helper view for statements_with_runtimes_in_95th_percentile.
 * Lists the 95th percentile runtime, for all statements
 *
 * mysql> select * from x$ps_digest_95th_percentile_by_avg_us;
 * +--------+------------+
 * | avg_us | percentile |
 * +--------+------------+
 * |    964 |     0.9525 |
 * +--------+------------+
 *
 */

CREATE OR REPLACE
  ALGORITHM = TEMPTABLE
  DEFINER = 'root'@'localhost'
  SQL SECURITY INVOKER 
VIEW x$ps_digest_95th_percentile_by_avg_us (
  avg_us,
  percentile
) AS
SELECT s2.avg_us avg_us,
       SUM(s1.cnt)/(SELECT COUNT(*) FROM performance_schema.events_statements_summary_by_digest) percentile
  FROM sys.x$ps_digest_avg_latency_distribution AS s1
  JOIN sys.x$ps_digest_avg_latency_distribution AS s2
    ON s1.avg_us <= s2.avg_us
 GROUP BY s2.avg_us
HAVING percentile > 0.95
 ORDER BY percentile
 LIMIT 1;

/*
 * View: statements_with_runtimes_in_95th_percentile
 *
 * List all statements who's average runtime, in microseconds, is in the top 95th percentile.
 * 
 * mysql> select * from statements_with_runtimes_in_95th_percentile limit 5;
 * +-------------------------------------------------------------------+------+-----------+------------+-----------+------------+---------------+-------------+-------------+-----------+---------------+---------------+-------------------+---------------------+---------------------+----------------------------------+
 * | query                                                             | db   | full_scan | exec_count | err_count | warn_count | total_latency | max_latency | avg_latency | rows_sent | rows_sent_avg | rows_examined | rows_examined_avg | FIRST_SEEN          | LAST_SEEN           | digest                           |
 * +-------------------------------------------------------------------+------+-----------+------------+-----------+------------+---------------+-------------+-------------+-----------+---------------+---------------+-------------------+---------------------+---------------------+----------------------------------+
 * | SELECT `e` . `round_robin_bin` ...  `timestamp` = `maxes` . `ts`  | mem  | *         |         14 |         0 |          0 | 43.96 s       | 6.69 s      | 3.14 s      |        11 |             1 |        253170 |             18084 | 2013-12-04 20:05:01 | 2013-12-04 20:06:34 | 29ba002bf039bb6439357a10134407de |
 * | SELECT `e` . `round_robin_bin` ...  `timestamp` = `maxes` . `ts`  | mem  | *         |          8 |         0 |          0 | 17.89 s       | 4.12 s      | 2.24 s      |         7 |             1 |        169534 |             21192 | 2013-12-04 20:04:54 | 2013-12-04 20:05:05 | 0b1c1f91e7e9e0ff91aa49d15f540793 |
 * | SELECT `e` . `round_robin_bin` ...  `timestamp` = `maxes` . `ts`  | mem  | *         |          1 |         0 |          0 | 2.22 s        | 2.22 s      | 2.22 s      |         1 |             1 |         40322 |             40322 | 2013-12-04 20:05:39 | 2013-12-04 20:05:39 | 07b27145c8f8a3779737df5032374833 |
 * | SELECT `e` . `round_robin_bin` ...  `timestamp` = `maxes` . `ts`  | mem  | *         |          1 |         0 |          0 | 1.97 s        | 1.97 s      | 1.97 s      |         1 |             1 |         40322 |             40322 | 2013-12-04 20:05:39 | 2013-12-04 20:05:39 | a07488137ea5c1bccf3e291c50bfd21f |
 * | SELECT `e` . `round_robin_bin` ...  `timestamp` = `maxes` . `ts`  | mem  | *         |          2 |         0 |          0 | 3.91 s        | 3.91 s      | 1.96 s      |         1 |             1 |         13126 |              6563 | 2013-12-04 20:05:04 | 2013-12-04 20:06:34 | b8bddc6566366dafc7e474f67096a93b |
 * +-------------------------------------------------------------------+------+-----------+------------+-----------+------------+---------------+-------------+-------------+-----------+---------------+---------------+-------------------+---------------------+---------------------+----------------------------------+
 *
 */

CREATE OR REPLACE
  ALGORITHM = MERGE
  DEFINER = 'root'@'localhost'
  SQL SECURITY INVOKER 
VIEW statements_with_runtimes_in_95th_percentile (
  query,
  db,
  full_scan,
  exec_count,
  err_count,
  warn_count,
  total_latency,
  max_latency,
  avg_latency,
  rows_sent,
  rows_sent_avg,
  rows_examined,
  rows_examined_avg,
  first_seen,
  last_seen,
  digest
) AS
SELECT sys.format_statement(DIGEST_TEXT) AS query,
       SCHEMA_NAME as db,
       IF(SUM_NO_GOOD_INDEX_USED > 0 OR SUM_NO_INDEX_USED > 0, '*', '') AS full_scan,
       COUNT_STAR AS exec_count,
       SUM_ERRORS AS err_count,
       SUM_WARNINGS AS warn_count,
       sys.format_time(SUM_TIMER_WAIT) AS total_latency,
       sys.format_time(MAX_TIMER_WAIT) AS max_latency,
       sys.format_time(AVG_TIMER_WAIT) AS avg_latency,
       SUM_ROWS_SENT AS rows_sent,
       ROUND(SUM_ROWS_SENT / COUNT_STAR) AS rows_sent_avg,
       SUM_ROWS_EXAMINED AS rows_examined,
       ROUND(SUM_ROWS_EXAMINED / COUNT_STAR) AS rows_examined_avg,
       FIRST_SEEN AS first_seen,
       LAST_SEEN AS last_seen,
       DIGEST AS digest
  FROM performance_schema.events_statements_summary_by_digest stmts
  JOIN sys.x$ps_digest_95th_percentile_by_avg_us AS top_percentile
    ON ROUND(stmts.avg_timer_wait/1000000) >= top_percentile.avg_us
 ORDER BY AVG_TIMER_WAIT DESC;

/*
 * View: x$statements_with_runtimes_in_95th_percentile
 *
 * List all statements who's average runtime, in microseconds, is in the top 95th percentile.
 * 
 * mysql> SELECT * FROM x$statements_with_runtimes_in_95th_percentile LIMIT 1\G
 * *************************** 1. row ***************************
 *             query: SELECT `e` . `round_robin_bin` AS `round1_1706_0_` , `e` . `id` AS `id1706_0_` , `e` . `timestamp` AS `timestamp1706_0_` , `e` . `rxBytes` AS `rxBytes1706_0_` , `e` . `rxPackets` AS `rxPackets1706_0_` , `e` . `rxErrors` AS `rxErrors1706_0_` , `e` . `txBytes` AS `txBytes1706_0_` , `e` . `txPackets` AS `txPackets1706_0_` , `e` . `txErrors` AS `txErrors1706_0_` , `e` . `txCollisions` AS `txColli10_1706_0_` FROM `mem__instruments` . `NetworkTrafficAdvisor_NetworkTraffic` AS `e` JOIN ( SELECT `id` AS `t` , MAX ( TIMESTAMP ) AS `ts` FROM `mem__instruments` . `NetworkTrafficAdvisor_NetworkTraffic` WHERE `id` IN (?) GROUP BY `id` ORDER BY NULL ) `maxes` ON `e` . `id` = `maxes` . `t` AND `e` . `timestamp` = `maxes` . `ts`
 *                db: mem
 *         full_scan: *
 *        exec_count: 14
 *         err_count: 0
 *        warn_count: 0
 *     total_latency: 43961670267000
 *       max_latency: 6686877140000
 *       avg_latency: 3140119304000
 *         rows_sent: 11
 *     rows_sent_avg: 1
 *     rows_examined: 253170
 * rows_examined_avg: 18084
 *        first_seen: 2013-12-04 20:05:01
 *         last_seen: 2013-12-04 20:06:34
 *            digest: 29ba002bf039bb6439357a10134407de
 *
 */

CREATE OR REPLACE
  ALGORITHM = MERGE
  DEFINER = 'root'@'localhost'
  SQL SECURITY INVOKER 
VIEW x$statements_with_runtimes_in_95th_percentile (
  query,
  db,
  full_scan,
  exec_count,
  err_count,
  warn_count,
  total_latency,
  max_latency,
  avg_latency,
  rows_sent,
  rows_sent_avg,
  rows_examined,
  rows_examined_avg,
  first_seen,
  last_seen,
  digest
) AS
SELECT DIGEST_TEXT AS query,
       SCHEMA_NAME AS db,
       IF(SUM_NO_GOOD_INDEX_USED > 0 OR SUM_NO_INDEX_USED > 0, '*', '') AS full_scan,
       COUNT_STAR AS exec_count,
       SUM_ERRORS AS err_count,
       SUM_WARNINGS AS warn_count,
       SUM_TIMER_WAIT AS total_latency,
       MAX_TIMER_WAIT AS max_latency,
       AVG_TIMER_WAIT AS avg_latency,
       SUM_ROWS_SENT AS rows_sent,
       ROUND(SUM_ROWS_SENT / COUNT_STAR) AS rows_sent_avg,
       SUM_ROWS_EXAMINED AS rows_examined,
       ROUND(SUM_ROWS_EXAMINED / COUNT_STAR) AS rows_examined_avg,
       FIRST_SEEN as first_seen,
       LAST_SEEN as last_seen,
       DIGEST AS digest
  FROM performance_schema.events_statements_summary_by_digest stmts
  JOIN sys.x$ps_digest_95th_percentile_by_avg_us AS top_percentile
    ON ROUND(stmts.avg_timer_wait/1000000) >= top_percentile.avg_us
 ORDER BY AVG_TIMER_WAIT DESC;
