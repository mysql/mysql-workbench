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
 * View: io_global_by_wait_by_bytes
 *
 * Shows the top global IO consumer classes by bytes usage.
 *
 * mysql> select * from io_global_by_wait_by_bytes;
 * +--------------------+--------+---------------+-------------+-------------+-------------+------------+------------+-----------+-------------+---------------+-------------+-----------------+
 * | event_name         | total  | total_latency | min_latency | avg_latency | max_latency | count_read | total_read | avg_read  | count_write | total_written | avg_written | total_requested |
 * +--------------------+--------+---------------+-------------+-------------+-------------+------------+------------+-----------+-------------+---------------+-------------+-----------------+
 * | myisam/dfile       | 163681 | 983.13 ms     | 379.08 ns   | 6.01 µs     | 22.06 ms    |      68737 | 127.31 MiB | 1.90 KiB  |     1012221 | 121.52 MiB    | 126 bytes   | 248.83 MiB      |
 * | myisam/kfile       |   1775 | 375.13 ms     | 1.02 µs     | 211.34 µs   | 35.15 ms    |      54066 | 9.97 MiB   | 193 bytes |      428257 | 12.40 MiB     | 30 bytes    | 22.37 MiB       |
 * | sql/FRM            |  57889 | 8.40 s        | 19.44 ns    | 145.05 µs   | 336.71 ms   |       8009 | 2.60 MiB   | 341 bytes |       14675 | 2.91 MiB      | 208 bytes   | 5.51 MiB        |
 * | sql/global_ddl_log |    164 | 75.96 ms      | 5.72 µs     | 463.19 µs   | 7.43 ms     |         20 | 80.00 KiB  | 4.00 KiB  |          76 | 304.00 KiB    | 4.00 KiB    | 384.00 KiB      |
 * | sql/file_parser    |    419 | 601.37 ms     | 1.96 µs     | 1.44 ms     | 37.14 ms    |         66 | 42.01 KiB  | 652 bytes |          64 | 226.98 KiB    | 3.55 KiB    | 268.99 KiB      |
 * | sql/binlog         |    190 | 6.79 s        | 1.56 µs     | 35.76 ms    | 4.21 s      |         52 | 60.54 KiB  | 1.16 KiB  |           0 | 0 bytes       | 0 bytes     | 60.54 KiB       |
 * | sql/ERRMSG         |      5 | 2.03 s        | 8.61 µs     | 405.40 ms   | 2.03 s      |          3 | 51.82 KiB  | 17.27 KiB |           0 | 0 bytes       | 0 bytes     | 51.82 KiB       |
 * | mysys/charset      |      3 | 196.52 µs     | 17.61 µs    | 65.51 µs    | 137.33 µs   |          1 | 17.83 KiB  | 17.83 KiB |           0 | 0 bytes       | 0 bytes     | 17.83 KiB       |
 * | sql/partition      |     81 | 18.87 ms      | 888.08 ns   | 232.92 µs   | 4.67 ms     |         66 | 2.75 KiB   | 43 bytes  |           8 | 288 bytes     | 36 bytes    | 3.04 KiB        |
 * | sql/dbopt          | 329166 | 26.95 s       | 2.06 µs     | 81.89 µs    | 178.71 ms   |          0 | 0 bytes    | 0 bytes   |           9 | 585 bytes     | 65 bytes    | 585 bytes       |
 * | sql/relaylog       |      7 | 1.18 ms       | 838.84 ns   | 168.30 µs   | 892.70 µs   |          0 | 0 bytes    | 0 bytes   |           1 | 120 bytes     | 120 bytes   | 120 bytes       |
 * | mysys/cnf          |      5 | 171.61 µs     | 303.26 ns   | 34.32 µs    | 115.21 µs   |          3 | 56 bytes   | 19 bytes  |           0 | 0 bytes       | 0 bytes     | 56 bytes        |
 * | sql/pid            |      3 | 220.55 µs     | 29.29 µs    | 73.52 µs    | 143.11 µs   |          0 | 0 bytes    | 0 bytes   |           1 | 5 bytes       | 5 bytes     | 5 bytes         |
 * | sql/casetest       |      1 | 121.19 µs     | 121.19 µs   | 121.19 µs   | 121.19 µs   |          0 | 0 bytes    | 0 bytes   |           0 | 0 bytes       | 0 bytes     | 0 bytes         |
 * | sql/binlog_index   |      5 | 593.47 µs     | 1.07 µs     | 118.69 µs   | 535.90 µs   |          0 | 0 bytes    | 0 bytes   |           0 | 0 bytes       | 0 bytes     | 0 bytes         |
 * | sql/misc           |     23 | 2.73 ms       | 65.14 µs    | 118.50 µs   | 255.31 µs   |          0 | 0 bytes    | 0 bytes   |           0 | 0 bytes       | 0 bytes     | 0 bytes         |
 * +--------------------+--------+---------------+-------------+-------------+-------------+------------+------------+-----------+-------------+---------------+-------------+-----------------+
 *
 */

CREATE OR REPLACE
  ALGORITHM = MERGE
  DEFINER = 'root'@'localhost'
  SQL SECURITY INVOKER 
VIEW io_global_by_wait_by_bytes (
  event_name,
  total,
  total_latency,
  min_latency,
  avg_latency,
  max_latency,
  count_read,
  total_read,
  avg_read,
  count_write,
  total_written,
  avg_written,
  total_requested
) AS
SELECT SUBSTRING_INDEX(event_name, '/', -2) event_name,
       count_star AS total,
       sys.format_time(sum_timer_wait) AS total_latency,
       sys.format_time(min_timer_wait) AS min_latency,
       sys.format_time(avg_timer_wait) AS avg_latency,
       sys.format_time(max_timer_wait) AS max_latency,
       count_read,
       sys.format_bytes(sum_number_of_bytes_read) AS total_read,
       sys.format_bytes(IFNULL(sum_number_of_bytes_read / count_read, 0)) AS avg_read,
       count_write,
       sys.format_bytes(sum_number_of_bytes_write) AS total_written,
       sys.format_bytes(IFNULL(sum_number_of_bytes_write / count_write, 0)) AS avg_written,
       sys.format_bytes(sum_number_of_bytes_write + sum_number_of_bytes_read) AS total_requested
  FROM performance_schema.file_summary_by_event_name
 WHERE event_name LIKE 'wait/io/file/%' 
   AND count_star > 0
 ORDER BY sum_number_of_bytes_write + sum_number_of_bytes_read DESC;

/*
 * View: x$io_global_by_wait_by_bytes
 *
 * Shows the top global IO consumer classes by bytes usage.
 *
 * mysql> select * from x$io_global_by_wait_by_bytes;
 * +-------------------------+-------+---------------+-------------+-------------+--------------+------------+------------+------------+-------------+---------------+-------------+-----------------+
 * | event_name              | total | total_latency | min_latency | avg_latency | max_latency  | count_read | total_read | avg_read   | count_write | total_written | avg_written | total_requested |
 * +-------------------------+-------+---------------+-------------+-------------+--------------+------------+------------+------------+-------------+---------------+-------------+-----------------+
 * | innodb/innodb_data_file |   151 |  334405721910 |     8399560 |  2214607429 | 107444600380 |        147 |    4472832 | 30427.4286 |           0 |             0 |      0.0000 |         4472832 |
 * | sql/FRM                 |   555 |  147752034170 |      674830 |   266219881 |  57705900850 |        270 |     112174 |   415.4593 |           0 |             0 |      0.0000 |          112174 |
 * | innodb/innodb_log_file  |    22 |   56776429970 |     2476890 |  2580746816 |  18883021430 |          6 |      69632 | 11605.3333 |           5 |          2560 |    512.0000 |           72192 |
 * | sql/ERRMSG              |     5 |   11862056180 |    14883960 |  2372411236 |  11109473700 |          3 |      44724 | 14908.0000 |           0 |             0 |      0.0000 |           44724 |
 * | mysys/charset           |     3 |    7256869230 |    19796270 |  2418956410 |   7198498320 |          1 |      18317 | 18317.0000 |           0 |             0 |      0.0000 |           18317 |
 * | myisam/kfile            |   135 |   10194698280 |      784160 |    75516283 |   2593514950 |         40 |       9216 |   230.4000 |          33 |          1017 |     30.8182 |           10233 |
 * | myisam/dfile            |    68 |   10527909730 |      772850 |   154822201 |   7600014630 |          9 |       6667 |   740.7778 |           0 |             0 |      0.0000 |            6667 |
 * | sql/pid                 |     3 |     216507330 |    41296580 |    72169110 |    100617530 |          0 |          0 |     0.0000 |           1 |             6 |      6.0000 |               6 |
 * | sql/casetest            |     5 |     185261570 |     4105530 |    37052314 |    113488310 |          0 |          0 |     0.0000 |           0 |             0 |      0.0000 |               0 |
 * | sql/global_ddl_log      |     2 |      21538010 |     3121560 |    10769005 |     18416450 |          0 |          0 |     0.0000 |           0 |             0 |      0.0000 |               0 |
 * | sql/dbopt               |    10 |    1004267680 |     1164930 |   100426768 |    939894930 |          0 |          0 |     0.0000 |           0 |             0 |      0.0000 |               0 |
 * +-------------------------+-------+---------------+-------------+-------------+--------------+------------+------------+------------+-------------+---------------+-------------+-----------------+
 *
 */

CREATE OR REPLACE
  ALGORITHM = MERGE
  DEFINER = 'root'@'localhost'
  SQL SECURITY INVOKER 
VIEW x$io_global_by_wait_by_bytes (
  event_name,
  total,
  total_latency,
  min_latency,
  avg_latency,
  max_latency,
  count_read,
  total_read,
  avg_read,
  count_write,
  total_written,
  avg_written,
  total_requested
) AS
SELECT SUBSTRING_INDEX(event_name, '/', -2) AS event_name,
       count_star AS total,
       sum_timer_wait AS total_latency,
       min_timer_wait AS min_latency,
       avg_timer_wait AS avg_latency,
       max_timer_wait AS max_latency,
       count_read,
       sum_number_of_bytes_read AS total_read,
       IFNULL(sum_number_of_bytes_read / count_read, 0) AS avg_read,
       count_write,
       sum_number_of_bytes_write AS total_written,
       IFNULL(sum_number_of_bytes_write / count_write, 0) AS avg_written,
       sum_number_of_bytes_write + sum_number_of_bytes_read AS total_requested
  FROM performance_schema.file_summary_by_event_name
 WHERE event_name LIKE 'wait/io/file/%' 
   AND count_star > 0
 ORDER BY sum_number_of_bytes_write + sum_number_of_bytes_read DESC;
