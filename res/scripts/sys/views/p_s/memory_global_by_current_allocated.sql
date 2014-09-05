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
 * View: memory_global_by_current_allocated
 * 
 * Shows the current memory usage within the server globally broken down by allocation type.
 *
 * mysql> select * from memory_global_by_current_allocated;
 * +----------------------------------------+---------------+---------------+-------------------+------------+------------+----------------+
 * | event_name                             | current_count | current_alloc | current_avg_alloc | high_count | high_alloc | high_avg_alloc |
 * +----------------------------------------+---------------+---------------+-------------------+------------+------------+----------------+
 * | memory/sql/TABLE_SHARE::mem_root       |           269 | 568.21 KiB    | 2.11 KiB          |        339 | 706.04 KiB | 2.08 KiB       |
 * | memory/sql/TABLE                       |           214 | 366.56 KiB    | 1.71 KiB          |        245 | 481.13 KiB | 1.96 KiB       |
 * | memory/sql/sp_head::main_mem_root      |            32 | 334.97 KiB    | 10.47 KiB         |        421 | 9.73 MiB   | 23.66 KiB      |
 * | memory/sql/Filesort_buffer::sort_keys  |             1 | 255.89 KiB    | 255.89 KiB        |          1 | 256.00 KiB | 256.00 KiB     |
 * | memory/mysys/array_buffer              |            82 | 121.66 KiB    | 1.48 KiB          |       1124 | 852.55 KiB | 777 bytes      |
 * ...
 * +----------------------------------------+---------------+---------------+-------------------+------------+------------+----------------+
 *
 */

CREATE OR REPLACE
  ALGORITHM = MERGE
  DEFINER = 'root'@'localhost'
  SQL SECURITY INVOKER 
VIEW memory_global_by_current_allocated (
  event_name,
  current_count,
  current_alloc,
  current_avg_alloc,
  high_count,
  high_alloc,
  high_avg_alloc
) AS
SELECT event_name,
       current_count_used AS current_count,
       sys.format_bytes(current_number_of_bytes_used) AS current_alloc,
       sys.format_bytes(IFNULL(current_number_of_bytes_used / NULLIF(current_count_used, 0), 0)) AS current_avg_alloc,
       high_count_used AS high_count,
       sys.format_bytes(high_number_of_bytes_used) AS high_alloc,
       sys.format_bytes(IFNULL(high_number_of_bytes_used / NULLIF(high_count_used, 0), 0)) AS high_avg_alloc
  FROM performance_schema.memory_summary_global_by_event_name
 WHERE current_number_of_bytes_used > 0
 ORDER BY current_number_of_bytes_used DESC;

/* 
 * View: x$memory_global_by_current_allocated
 * 
 * Shows the current memory usage within the server globally broken down by allocation type.
 *
 * mysql> select * from x$memory_global_by_current_allocated;
 * +----------------------------------------+---------------+---------------+-------------------+------------+------------+----------------+
 * | event_name                             | current_count | current_alloc | current_avg_alloc | high_count | high_alloc | high_avg_alloc |
 * +----------------------------------------+---------------+---------------+-------------------+------------+------------+----------------+
 * | memory/sql/TABLE_SHARE::mem_root       |           270 |        582656 |         2157.9852 |        339 |     722984 |      2132.6962 |
 * | memory/sql/TABLE                       |           214 |        375353 |         1753.9860 |        245 |     492672 |      2010.9061 |
 * | memory/sql/sp_head::main_mem_root      |            32 |        343008 |        10719.0000 |        421 |   10200008 |     24228.0475 |
 * | memory/sql/Filesort_buffer::sort_keys  |             1 |        262036 |       262036.0000 |          1 |     262140 |    262140.0000 |
 * | memory/mysys/array_buffer              |            82 |        124576 |         1519.2195 |       1124 |     873008 |       776.6975 |
 * ...
 * +----------------------------------------+---------------+---------------+-------------------+------------+------------+----------------+
 *
 */

CREATE OR REPLACE
  ALGORITHM = MERGE
  DEFINER = 'root'@'localhost'
  SQL SECURITY INVOKER 
VIEW x$memory_global_by_current_allocated (
  event_name,
  current_count,
  current_alloc,
  current_avg_alloc,
  high_count,
  high_alloc,
  high_avg_alloc
) AS
SELECT event_name,
       current_count_used AS current_count,
       current_number_of_bytes_used AS current_alloc,
       IFNULL(current_number_of_bytes_used / NULLIF(current_count_used, 0), 0) AS current_avg_alloc,
       high_count_used AS high_count,
       high_number_of_bytes_used AS high_alloc,
       IFNULL(high_number_of_bytes_used / NULLIF(high_count_used, 0), 0) AS high_avg_alloc
  FROM performance_schema.memory_summary_global_by_event_name
 WHERE current_number_of_bytes_used > 0
 ORDER BY current_number_of_bytes_used DESC;