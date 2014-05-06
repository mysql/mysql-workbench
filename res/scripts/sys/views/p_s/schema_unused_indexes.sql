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
 * View: schema_unused_indexes
 * 
 * Finds indexes that have had no events against them (and hence, no usage).
 *
 * To trust whether the data from this view is representative of your workload,
 * you should ensure that the server has been up for a representative amount of
 * time before using it.
 *
 * mysql> select * from schema_unused_indexes limit 5;
 * +-------------------------+----------------------------------------+------------+
 * | object_schema           | object_name                            | index_name |
 * +-------------------------+----------------------------------------+------------+
 * | mem30_test__instruments | mysqlavailabilityadvisor$observedstate | PRIMARY    |
 * | mem30_test__test        | compressme                             | PRIMARY    |
 * | mem30_test__test        | compressmekeyblocksize                 | PRIMARY    |
 * | mem30_test__test        | dontcompressme                         | PRIMARY    |
 * | mem30_test__test        | round_robin_test                       | PRIMARY    |
 * +-------------------------+----------------------------------------+------------+
 *
 */

CREATE OR REPLACE
  ALGORITHM = MERGE
  DEFINER = 'root'@'localhost'
  SQL SECURITY INVOKER 
VIEW schema_unused_indexes (
  object_schema,
  object_name,
  index_name
) AS
SELECT object_schema,
       object_name,
       index_name
  FROM performance_schema.table_io_waits_summary_by_index_usage 
 WHERE index_name IS NOT NULL
   AND count_star = 0
   AND object_schema != 'mysql'
 ORDER BY object_schema, object_name;
