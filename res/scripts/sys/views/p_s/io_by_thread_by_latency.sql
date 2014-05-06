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
 * View: io_by_thread_by_latency
 *
 * Show the top IO consumers by thread, ordered by total latency
 *
 * mysql> select * from io_by_thread_by_latency;
 * +---------------------+-------+---------------+-------------+-------------+-------------+-----------+----------------+
 * | user                | total | total_latency | min_latency | avg_latency | max_latency | thread_id | processlist_id |
 * +---------------------+-------+---------------+-------------+-------------+-------------+-----------+----------------+
 * | root@localhost      | 11580 | 18.01 s       | 429.78 ns   | 1.12 ms     | 181.07 ms   |        25 |              6 |
 * | main                |  1358 | 1.31 s        | 475.02 ns   | 2.27 ms     | 350.70 ms   |         1 |           NULL |
 * | page_cleaner_thread |   654 | 147.44 ms     | 588.12 ns   | 225.44 us   | 46.41 ms    |        18 |           NULL |
 * | io_write_thread     |   131 | 107.75 ms     | 8.60 us     | 822.55 us   | 27.69 ms    |         8 |           NULL |
 * | io_write_thread     |    46 | 47.07 ms      | 10.64 us    | 1.02 ms     | 16.90 ms    |         9 |           NULL |
 * | io_write_thread     |    71 | 46.99 ms      | 9.11 us     | 661.81 us   | 17.04 ms    |        11 |           NULL |
 * | io_log_thread       |    20 | 21.01 ms      | 14.25 us    | 1.05 ms     | 7.08 ms     |         3 |           NULL |
 * | srv_master_thread   |    13 | 17.60 ms      | 8.49 us     | 1.35 ms     | 9.99 ms     |        16 |           NULL |
 * | srv_purge_thread    |     4 | 1.81 ms       | 34.31 us    | 452.45 us   | 1.02 ms     |        17 |           NULL |
 * | io_write_thread     |    19 | 951.39 us     | 9.75 us     | 50.07 us    | 297.47 us   |        10 |           NULL |
 * | signal_handler      |     3 | 218.03 us     | 21.64 us    | 72.68 us    | 154.84 us   |        19 |           NULL |
 * +---------------------+-------+---------------+-------------+-------------+-------------+-----------+----------------+
 *
 */

CREATE OR REPLACE
  ALGORITHM = TEMPTABLE
  DEFINER = 'root'@'localhost'
  SQL SECURITY INVOKER 
VIEW io_by_thread_by_latency (
  user,
  total,
  total_latency,
  min_latency,
  avg_latency,
  max_latency,
  thread_id,
  processlist_id
)
AS
SELECT IF(processlist_id IS NULL, 
             SUBSTRING_INDEX(name, '/', -1), 
             CONCAT(processlist_user, '@', processlist_host)
          ) user, 
       SUM(count_star) total,
       sys.format_time(SUM(sum_timer_wait)) total_latency,
       sys.format_time(MIN(min_timer_wait)) min_latency,
       sys.format_time(AVG(avg_timer_wait)) avg_latency,
       sys.format_time(MAX(max_timer_wait)) max_latency,
       thread_id,
       processlist_id
  FROM performance_schema.events_waits_summary_by_thread_by_event_name 
  LEFT JOIN performance_schema.threads USING (thread_id)
 WHERE event_name LIKE 'wait/io/file/%'
   AND sum_timer_wait > 0
 GROUP BY thread_id
 ORDER BY SUM(sum_timer_wait) DESC;

/*
 * View: x$io_by_thread_by_latency
 *
 * Show the top IO consumers by thread, ordered by total latency
 *
 * mysql> select * from x$io_by_thread_by_latency;
 * +---------------------+-------+----------------+-------------+-----------------+--------------+-----------+----------------+
 * | user                | total | total_latency  | min_latency | avg_latency     | max_latency  | thread_id | processlist_id |
 * +---------------------+-------+----------------+-------------+-----------------+--------------+-----------+----------------+
 * | root@localhost      | 11587 | 18007539905680 |      429780 | 1120831681.6667 | 181065665560 |        25 |              6 |
 * | main                |  1358 |  1309001741320 |      475020 | 2269581997.8000 | 350700491310 |         1 |           NULL |
 * | page_cleaner_thread |   654 |   147435455960 |      588120 |  225436198.0000 |  46412043990 |        18 |           NULL |
 * | io_write_thread     |   131 |   107754483070 |     8603140 |  822553303.0000 |  27691592500 |         8 |           NULL |
 * | io_write_thread     |    46 |    47074926860 |    10642710 | 1023367631.0000 |  16899745070 |         9 |           NULL |
 * | io_write_thread     |    71 |    46988801210 |     9108320 |  661814075.0000 |  17042760020 |        11 |           NULL |
 * | io_log_thread       |    20 |    21007710490 |    14250600 | 1050385336.0000 |   7081255090 |         3 |           NULL |
 * | srv_master_thread   |    13 |    17601511720 |     8486270 | 1353962324.0000 |   9990100380 |        16 |           NULL |
 * | srv_purge_thread    |     4 |     1809792270 |    34307000 |  452447879.0000 |   1018887740 |        17 |           NULL |
 * | io_write_thread     |    19 |      951385890 |     9745450 |   50072763.0000 |    297468080 |        10 |           NULL |
 * | signal_handler      |     3 |      218026640 |    21639800 |   72675421.0000 |    154841440 |        19 |           NULL |
 * +---------------------+-------+----------------+-------------+-----------------+--------------+-----------+----------------+
 *
 */

CREATE OR REPLACE
  ALGORITHM = TEMPTABLE
  DEFINER = 'root'@'localhost'
  SQL SECURITY INVOKER 
VIEW x$io_by_thread_by_latency (
  user,
  total,
  total_latency,
  min_latency,
  avg_latency,
  max_latency,
  thread_id,
  processlist_id
)
AS
SELECT IF(processlist_id IS NULL, 
             SUBSTRING_INDEX(name, '/', -1), 
             CONCAT(processlist_user, '@', processlist_host)
          ) user, 
       SUM(count_star) total,
       SUM(sum_timer_wait) total_latency,
       MIN(min_timer_wait) min_latency,
       AVG(avg_timer_wait) avg_latency,
       MAX(max_timer_wait) max_latency,
       thread_id,
       processlist_id
  FROM performance_schema.events_waits_summary_by_thread_by_event_name 
  LEFT JOIN performance_schema.threads USING (thread_id)
 WHERE event_name LIKE 'wait/io/file/%'
   AND sum_timer_wait > 0
 GROUP BY thread_id
 ORDER BY SUM(sum_timer_wait) DESC;
