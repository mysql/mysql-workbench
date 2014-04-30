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
 * View: schema_tables_with_full_table_scans
 *
 * Find tables that are being accessed by full table scans
 * ordering by the number of rows scanned descending.
 *
 * mysql> select * from schema_tables_with_full_table_scans limit 5;
 * +------------------+-------------------+-------------------+
 * | object_schema    | object_name       | rows_full_scanned |
 * +------------------+-------------------+-------------------+
 * | mem              | rule_alarms       |              1210 |
 * | mem30__advisors  | advisor_schedules |              1021 |
 * | mem30__inventory | agent             |               498 |
 * | mem              | dc_p_string       |               449 |
 * | mem30__inventory | mysqlserver       |               294 |
 * +------------------+-------------------+-------------------+
 *
 */

CREATE OR REPLACE
  ALGORITHM = MERGE
  DEFINER = 'root'@'localhost'
  SQL SECURITY INVOKER 
VIEW schema_tables_with_full_table_scans (
  object_schema,
  object_name,
  rows_full_scanned
) AS
SELECT object_schema, 
       object_name,
       count_read AS rows_full_scanned
  FROM performance_schema.table_io_waits_summary_by_index_usage 
 WHERE index_name IS NULL
   AND count_read > 0
 ORDER BY count_read DESC;
