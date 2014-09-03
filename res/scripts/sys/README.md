# The MySQL sys schema

A collection of views, functions and procedures to help MySQL administrators get insight in to MySQL Database usage.

There are install files available for 5.6 and 5.7 respectively. To load these, you must position yourself within the directory that you downloaded to, as these top level files SOURCE individual files that are shared across versions in most cases (though not all).

## Installation

The objects should all be created as the root user (but run with the privileges of the invoker).

For instance if you download to /tmp/mysql-sys/, and want to install the 5.6 version you should:

    cd /tmp/mysql-sys/
    mysql -u root -p < ./sys_56.sql

Or if you would like to log in to the client, and install the 5.7 version:

    cd /tmp/mysql-sys/
    mysql -u root -p 
    SOURCE ./sys_57.sql

Alternatively, you could just choose to load individual files based on your needs, but beware, certain objects have dependencies on other objects. You will need to ensure that these are also loaded.

## Overview of objects

### Views

Many of the views in the sys schema have both a command line user friendly format output, as well as tooling friendly versions of any view that contains formatted output duplicated as an x$ table.

The examples below show output for only the formatted views, and note where there is an x$ counterpart available.

#### innodb_buffer_stats_by_schema / x$innodb_buffer_stats_by_schema

##### Description

Summarizes the output of the INFORMATION_SCHEMA.INNODB_BUFFER_PAGE table, aggregating by schema.

##### Example

```SQL
mysql> select * from innodb_buffer_stats_by_schema;
+--------------------------+------------+------------+-------+--------------+-----------+-------------+
| object_schema            | allocated  | data       | pages | pages_hashed | pages_old | rows_cached |
+--------------------------+------------+------------+-------+--------------+-----------+-------------+
| mem30_trunk__instruments | 1.69 MiB   | 510.03 KiB |   108 |          108 |       108 |        3885 |
| InnoDB System            | 688.00 KiB | 351.62 KiB |    43 |           43 |        43 |         862 |
| mem30_trunk__events      | 80.00 KiB  | 21.61 KiB  |     5 |            5 |         5 |         229 |
+--------------------------+------------+------------+-------+--------------+-----------+-------------+
```

#### innodb_buffer_stats_by_table / x$innodb_buffer_stats_by_table

##### Description

Summarizes the output of the INFORMATION_SCHEMA.INNODB_BUFFER_PAGE table, aggregating by schema and table name.

##### Example

```SQL
mysql> select * from innodb_buffer_stats_by_table;
+--------------------------+------------------------------------+------------+-----------+-------+--------------+-----------+-------------+
| object_schema            | object_name                        | allocated  | data      | pages | pages_hashed | pages_old | rows_cached |
+--------------------------+------------------------------------+------------+-----------+-------+--------------+-----------+-------------+
| InnoDB System            | SYS_COLUMNS                        | 128.00 KiB | 98.97 KiB |     8 |            8 |         8 |        1532 |
| InnoDB System            | SYS_FOREIGN                        | 128.00 KiB | 55.48 KiB |     8 |            8 |         8 |         172 |
| InnoDB System            | SYS_TABLES                         | 128.00 KiB | 56.18 KiB |     8 |            8 |         8 |         365 |
| InnoDB System            | SYS_INDEXES                        | 112.00 KiB | 76.16 KiB |     7 |            7 |         7 |        1046 |
| mem30_trunk__instruments | agentlatencytime                   | 96.00 KiB  | 28.83 KiB |     6 |            6 |         6 |         252 |
| mem30_trunk__instruments | binlogspaceusagedata               | 96.00 KiB  | 22.54 KiB |     6 |            6 |         6 |         196 |
| mem30_trunk__instruments | connectionsdata                    | 96.00 KiB  | 36.68 KiB |     6 |            6 |         6 |         276 |
| mem30_trunk__instruments | connectionsmaxdata                 | 96.00 KiB  | 31.88 KiB |     6 |            6 |         6 |         271 |
| mem30_trunk__instruments | cpuaverage                         | 96.00 KiB  | 14.32 KiB |     6 |            6 |         6 |          55 |
| mem30_trunk__instruments | diskiototaldata                    | 96.00 KiB  | 42.71 KiB |     6 |            6 |         6 |         152 |
| mem30_trunk__instruments | innodbopenfilesdata                | 96.00 KiB  | 32.61 KiB |     6 |            6 |         6 |         266 |
| mem30_trunk__instruments | innodbrowlocktimestatisticsdata    | 96.00 KiB  | 32.16 KiB |     6 |            6 |         6 |         261 |
| mem30_trunk__instruments | myisamkeybufferusagedata           | 96.00 KiB  | 25.99 KiB |     6 |            6 |         6 |         232 |
| mem30_trunk__instruments | mysqlprocessactivity               | 96.00 KiB  | 31.99 KiB |     6 |            6 |         6 |         252 |
| mem30_trunk__instruments | querycacheaveragefreeblocksizedata | 96.00 KiB  | 27.00 KiB |     6 |            6 |         6 |         237 |
| mem30_trunk__instruments | querycacheaveragequerysizedata     | 96.00 KiB  | 38.29 KiB |     6 |            6 |         6 |         315 |
| mem30_trunk__instruments | querycachefragmentationdata        | 96.00 KiB  | 27.00 KiB |     6 |            6 |         6 |         237 |
| mem30_trunk__instruments | querycachememorydata               | 96.00 KiB  | 32.58 KiB |     6 |            6 |         6 |         278 |
| mem30_trunk__instruments | querycachequeriesincachedata       | 96.00 KiB  | 27.15 KiB |     6 |            6 |         6 |         238 |
| mem30_trunk__instruments | ramusagedata                       | 96.00 KiB  | 15.02 KiB |     6 |            6 |         6 |          59 |
| mem30_trunk__instruments | slaverelaylogspaceusagedata        | 96.00 KiB  | 28.28 KiB |     6 |            6 |         6 |         249 |
| mem30_trunk__instruments | swapusagedata                      | 96.00 KiB  | 15.02 KiB |     6 |            6 |         6 |          59 |
| InnoDB System            | SYS_FIELDS                         | 80.00 KiB  | 49.78 KiB |     5 |            5 |         5 |        1147 |
| InnoDB System            | SYS_DATAFILES                      | 32.00 KiB  | 3.97 KiB  |     2 |            2 |         2 |          60 |
| InnoDB System            | SYS_FOREIGN_COLS                   | 32.00 KiB  | 7.43 KiB  |     2 |            2 |         2 |          83 |
| InnoDB System            | SYS_TABLESPACES                    | 32.00 KiB  | 3.65 KiB  |     2 |            2 |         2 |          56 |
| InnoDB System            | SYS_IBUF_TABLE                     | 16.00 KiB  | 0 bytes   |     1 |            1 |         1 |           0 |
+--------------------------+------------------------------------+------------+-----------+-------+--------------+-----------+-------------+
```

#### io_by_thread_by_latency / x$io_by_thread_by_latency

##### Description

Shows the top IO consumers by thread, ordered by total latency.

##### Example

```SQL
mysql> select * from io_by_thread_by_latency;
+---------------------+-------+---------------+-------------+-------------+-------------+-----------+----------------+
| user                | total | total_latency | min_latency | avg_latency | max_latency | thread_id | processlist_id |
+---------------------+-------+---------------+-------------+-------------+-------------+-----------+----------------+
| root@localhost      | 11580 | 18.01 s       | 429.78 ns   | 1.12 ms     | 181.07 ms   |        25 |              6 |
| main                |  1358 | 1.31 s        | 475.02 ns   | 2.27 ms     | 350.70 ms   |         1 |           NULL |
| page_cleaner_thread |   654 | 147.44 ms     | 588.12 ns   | 225.44 us   | 46.41 ms    |        18 |           NULL |
| io_write_thread     |   131 | 107.75 ms     | 8.60 us     | 822.55 us   | 27.69 ms    |         8 |           NULL |
| io_write_thread     |    46 | 47.07 ms      | 10.64 us    | 1.02 ms     | 16.90 ms    |         9 |           NULL |
| io_write_thread     |    71 | 46.99 ms      | 9.11 us     | 661.81 us   | 17.04 ms    |        11 |           NULL |
| io_log_thread       |    20 | 21.01 ms      | 14.25 us    | 1.05 ms     | 7.08 ms     |         3 |           NULL |
| srv_master_thread   |    13 | 17.60 ms      | 8.49 us     | 1.35 ms     | 9.99 ms     |        16 |           NULL |
| srv_purge_thread    |     4 | 1.81 ms       | 34.31 us    | 452.45 us   | 1.02 ms     |        17 |           NULL |
| io_write_thread     |    19 | 951.39 us     | 9.75 us     | 50.07 us    | 297.47 us   |        10 |           NULL |
| signal_handler      |     3 | 218.03 us     | 21.64 us    | 72.68 us    | 154.84 us   |        19 |           NULL |
+---------------------+-------+---------------+-------------+-------------+-------------+-----------+----------------+
```

#### io_global_by_file_by_bytes / x$io_global_by_file_by_bytes

##### Description

Shows the top global IO consumers by bytes usage by file.

##### Example

```SQL
mysql> SELECT * FROM io_global_by_file_by_bytes LIMIT 5;
+--------------------------------------------+------------+------------+-----------+-------------+---------------+-----------+------------+-----------+
| file                                       | count_read | total_read | avg_read  | count_write | total_written | avg_write | total      | write_pct |
+--------------------------------------------+------------+------------+-----------+-------------+---------------+-----------+------------+-----------+
| @@datadir/ibdata1                          |        147 | 4.27 MiB   | 29.71 KiB |           3 | 48.00 KiB     | 16.00 KiB | 4.31 MiB   |      1.09 |
| @@datadir/mysql/proc.MYD                   |        347 | 85.35 KiB  | 252 bytes |         111 | 19.08 KiB     | 176 bytes | 104.43 KiB |     18.27 |
| @@datadir/ib_logfile0                      |          6 | 68.00 KiB  | 11.33 KiB |           8 | 4.00 KiB      | 512 bytes | 72.00 KiB  |      5.56 |
| /opt/mysql/5.5.33/share/english/errmsg.sys |          3 | 43.68 KiB  | 14.56 KiB |           0 | 0 bytes       | 0 bytes   | 43.68 KiB  |      0.00 |
| /opt/mysql/5.5.33/share/charsets/Index.xml |          1 | 17.89 KiB  | 17.89 KiB |           0 | 0 bytes       | 0 bytes   | 17.89 KiB  |      0.00 |
+--------------------------------------------+------------+------------+-----------+-------------+---------------+-----------+------------+-----------+
```

#### io_global_by_file_by_latency / x$io_global_by_file_by_latency

##### Description

Shows the top global IO consumers by latency by file.

##### Example

```SQL
mysql> select * from io_global_by_file_by_latency limit 5;
+-----------------------------------------------------------+-------+---------------+------------+--------------+-------------+---------------+------------+--------------+
| file                                                      | total | total_latency | count_read | read_latency | count_write | write_latency | count_misc | misc_latency |
+-----------------------------------------------------------+-------+---------------+------------+--------------+-------------+---------------+------------+--------------+
| @@datadir/sys/wait_classes_global_by_avg_latency_raw.frm~ |    24 | 451.99 ms     |          0 | 0 ps         |           4 | 108.07 us     |         20 | 451.88 ms    |
| @@datadir/sys/innodb_buffer_stats_by_schema_raw.frm~      |    24 | 379.84 ms     |          0 | 0 ps         |           4 | 108.88 us     |         20 | 379.73 ms    |
| @@datadir/sys/io_by_thread_by_latency_raw.frm~            |    24 | 379.46 ms     |          0 | 0 ps         |           4 | 101.37 us     |         20 | 379.36 ms    |
| @@datadir/ibtmp1                                          |    53 | 373.45 ms     |          0 | 0 ps         |          48 | 246.08 ms     |          5 | 127.37 ms    |
| @@datadir/sys/statement_analysis_raw.frm~                 |    24 | 353.14 ms     |          0 | 0 ps         |           4 | 94.96 us      |         20 | 353.04 ms    |
+-----------------------------------------------------------+-------+---------------+------------+--------------+-------------+---------------+------------+--------------+
```

#### io_global_by_wait_by_bytes / x$io_global_by_wait_by_bytes

##### Description

Shows the top global IO consumer classes by bytes usage.

##### Example

```SQL
mysql> select * from io_global_by_wait_by_bytes;
+--------------------+--------+---------------+-------------+-------------+-------------+------------+------------+-----------+-------------+---------------+-------------+-----------------+
| event_name         | total  | total_latency | min_latency | avg_latency | max_latency | count_read | total_read | avg_read  | count_write | total_written | avg_written | total_requested |
+--------------------+--------+---------------+-------------+-------------+-------------+------------+------------+-----------+-------------+---------------+-------------+-----------------+
| myisam/dfile       | 163681 | 983.13 ms     | 379.08 ns   | 6.01 µs     | 22.06 ms    |      68737 | 127.31 MiB | 1.90 KiB  |     1012221 | 121.52 MiB    | 126 bytes   | 248.83 MiB      |
| myisam/kfile       |   1775 | 375.13 ms     | 1.02 µs     | 211.34 µs   | 35.15 ms    |      54066 | 9.97 MiB   | 193 bytes |      428257 | 12.40 MiB     | 30 bytes    | 22.37 MiB       |
| sql/FRM            |  57889 | 8.40 s        | 19.44 ns    | 145.05 µs   | 336.71 ms   |       8009 | 2.60 MiB   | 341 bytes |       14675 | 2.91 MiB      | 208 bytes   | 5.51 MiB        |
| sql/global_ddl_log |    164 | 75.96 ms      | 5.72 µs     | 463.19 µs   | 7.43 ms     |         20 | 80.00 KiB  | 4.00 KiB  |          76 | 304.00 KiB    | 4.00 KiB    | 384.00 KiB      |
| sql/file_parser    |    419 | 601.37 ms     | 1.96 µs     | 1.44 ms     | 37.14 ms    |         66 | 42.01 KiB  | 652 bytes |          64 | 226.98 KiB    | 3.55 KiB    | 268.99 KiB      |
| sql/binlog         |    190 | 6.79 s        | 1.56 µs     | 35.76 ms    | 4.21 s      |         52 | 60.54 KiB  | 1.16 KiB  |           0 | 0 bytes       | 0 bytes     | 60.54 KiB       |
| sql/ERRMSG         |      5 | 2.03 s        | 8.61 µs     | 405.40 ms   | 2.03 s      |          3 | 51.82 KiB  | 17.27 KiB |           0 | 0 bytes       | 0 bytes     | 51.82 KiB       |
| mysys/charset      |      3 | 196.52 µs     | 17.61 µs    | 65.51 µs    | 137.33 µs   |          1 | 17.83 KiB  | 17.83 KiB |           0 | 0 bytes       | 0 bytes     | 17.83 KiB       |
| sql/partition      |     81 | 18.87 ms      | 888.08 ns   | 232.92 µs   | 4.67 ms     |         66 | 2.75 KiB   | 43 bytes  |           8 | 288 bytes     | 36 bytes    | 3.04 KiB        |
| sql/dbopt          | 329166 | 26.95 s       | 2.06 µs     | 81.89 µs    | 178.71 ms   |          0 | 0 bytes    | 0 bytes   |           9 | 585 bytes     | 65 bytes    | 585 bytes       |
| sql/relaylog       |      7 | 1.18 ms       | 838.84 ns   | 168.30 µs   | 892.70 µs   |          0 | 0 bytes    | 0 bytes   |           1 | 120 bytes     | 120 bytes   | 120 bytes       |
| mysys/cnf          |      5 | 171.61 µs     | 303.26 ns   | 34.32 µs    | 115.21 µs   |          3 | 56 bytes   | 19 bytes  |           0 | 0 bytes       | 0 bytes     | 56 bytes        |
| sql/pid            |      3 | 220.55 µs     | 29.29 µs    | 73.52 µs    | 143.11 µs   |          0 | 0 bytes    | 0 bytes   |           1 | 5 bytes       | 5 bytes     | 5 bytes         |
| sql/casetest       |      1 | 121.19 µs     | 121.19 µs   | 121.19 µs   | 121.19 µs   |          0 | 0 bytes    | 0 bytes   |           0 | 0 bytes       | 0 bytes     | 0 bytes         |
| sql/binlog_index   |      5 | 593.47 µs     | 1.07 µs     | 118.69 µs   | 535.90 µs   |          0 | 0 bytes    | 0 bytes   |           0 | 0 bytes       | 0 bytes     | 0 bytes         |
| sql/misc           |     23 | 2.73 ms       | 65.14 µs    | 118.50 µs   | 255.31 µs   |          0 | 0 bytes    | 0 bytes   |           0 | 0 bytes       | 0 bytes     | 0 bytes         |
+--------------------+--------+---------------+-------------+-------------+-------------+------------+------------+-----------+-------------+---------------+-------------+-----------------+
```

#### io_global_by_wait_by_latency / x$io_global_by_wait_by_latency

##### Description

Shows the top global IO consumers by latency.

##### Example

```SQL
mysql> SELECT * FROM io_global_by_wait_by_latency;
+-------------------------+-------+---------------+-------------+-------------+--------------+---------------+--------------+------------+------------+-----------+-------------+---------------+-------------+
| event_name              | total | total_latency | avg_latency | max_latency | read_latency | write_latency | misc_latency | count_read | total_read | avg_read  | count_write | total_written | avg_written |
+-------------------------+-------+---------------+-------------+-------------+--------------+---------------+--------------+------------+------------+-----------+-------------+---------------+-------------+
| sql/file_parser         |  5433 | 30.20 s       | 5.56 ms     | 203.65 ms   | 22.08 ms     | 24.89 ms      | 30.16 s      |         24 | 6.18 KiB   | 264 bytes |         737 | 2.15 MiB      | 2.99 KiB    |
| innodb/innodb_data_file |  1344 | 1.52 s        | 1.13 ms     | 350.70 ms   | 203.82 ms    | 450.96 ms     | 868.21 ms    |        147 | 2.30 MiB   | 16.00 KiB |        1001 | 53.61 MiB     | 54.84 KiB   |
| innodb/innodb_log_file  |   828 | 893.48 ms     | 1.08 ms     | 30.11 ms    | 16.32 ms     | 705.89 ms     | 171.27 ms    |          6 | 68.00 KiB  | 11.33 KiB |         413 | 2.19 MiB      | 5.42 KiB    |
| myisam/kfile            |  7642 | 242.34 ms     | 31.71 us    | 19.27 ms    | 73.60 ms     | 23.48 ms      | 145.26 ms    |        758 | 135.63 KiB | 183 bytes |        4386 | 232.52 KiB    | 54 bytes    |
| myisam/dfile            | 12540 | 223.47 ms     | 17.82 us    | 32.50 ms    | 87.76 ms     | 16.97 ms      | 118.74 ms    |       5390 | 4.49 MiB   | 873 bytes |        1448 | 2.65 MiB      | 1.88 KiB    |
| csv/metadata            |     8 | 28.98 ms      | 3.62 ms     | 20.15 ms    | 399.27 us    | 0 ps          | 28.58 ms     |          2 | 70 bytes   | 35 bytes  |           0 | 0 bytes       | 0 bytes     |
| mysys/charset           |     3 | 24.24 ms      | 8.08 ms     | 24.15 ms    | 24.15 ms     | 0 ps          | 93.18 us     |          1 | 17.31 KiB  | 17.31 KiB |           0 | 0 bytes       | 0 bytes     |
| sql/ERRMSG              |     5 | 20.43 ms      | 4.09 ms     | 19.31 ms    | 20.32 ms     | 0 ps          | 103.20 us    |          3 | 58.97 KiB  | 19.66 KiB |           0 | 0 bytes       | 0 bytes     |
| mysys/cnf               |     5 | 11.37 ms      | 2.27 ms     | 11.28 ms    | 11.29 ms     | 0 ps          | 78.22 us     |          3 | 56 bytes   | 19 bytes  |           0 | 0 bytes       | 0 bytes     |
| sql/dbopt               |    57 | 4.04 ms       | 70.92 us    | 843.70 us   | 0 ps         | 186.43 us     | 3.86 ms      |          0 | 0 bytes    | 0 bytes   |           7 | 431 bytes     | 62 bytes    |
| csv/data                |     4 | 411.55 us     | 102.89 us   | 234.89 us   | 0 ps         | 0 ps          | 411.55 us    |          0 | 0 bytes    | 0 bytes   |           0 | 0 bytes       | 0 bytes     |
| sql/misc                |    22 | 340.38 us     | 15.47 us    | 33.77 us    | 0 ps         | 0 ps          | 340.38 us    |          0 | 0 bytes    | 0 bytes   |           0 | 0 bytes       | 0 bytes     |
| archive/data            |    39 | 277.86 us     | 7.12 us     | 16.18 us    | 0 ps         | 0 ps          | 277.86 us    |          0 | 0 bytes    | 0 bytes   |           0 | 0 bytes       | 0 bytes     |
| sql/pid                 |     3 | 218.03 us     | 72.68 us    | 154.84 us   | 0 ps         | 21.64 us      | 196.39 us    |          0 | 0 bytes    | 0 bytes   |           1 | 6 bytes       | 6 bytes     |
| sql/casetest            |     5 | 197.15 us     | 39.43 us    | 126.31 us   | 0 ps         | 0 ps          | 197.15 us    |          0 | 0 bytes    | 0 bytes   |           0 | 0 bytes       | 0 bytes     |
| sql/global_ddl_log      |     2 | 14.60 us      | 7.30 us     | 12.12 us    | 0 ps         | 0 ps          | 14.60 us     |          0 | 0 bytes    | 0 bytes   |           0 | 0 bytes       | 0 bytes     |
+-------------------------+-------+---------------+-------------+-------------+--------------+---------------+--------------+------------+------------+-----------+-------------+---------------+-------------+
```

#### latest_file_io / x$latest_file_io

##### Description

Shows the latest file IO, by file / thread.

##### Example

```SQL
mysql> select * from latest_file_io limit 5;
+----------------------+----------------------------------------+------------+-----------+-----------+
| thread               | file                                   | latency    | operation | requested |
+----------------------+----------------------------------------+------------+-----------+-----------+
| msandbox@localhost:1 | @@tmpdir/#sqlcf28_1_4e.MYI             | 9.26 µs    | write     | 124 bytes |
| msandbox@localhost:1 | @@tmpdir/#sqlcf28_1_4e.MYI             | 4.00 µs    | write     | 2 bytes   |
| msandbox@localhost:1 | @@tmpdir/#sqlcf28_1_4e.MYI             | 56.34 µs   | close     | NULL      |
| msandbox@localhost:1 | @@tmpdir/#sqlcf28_1_4e.MYD             | 53.93 µs   | close     | NULL      |
| msandbox@localhost:1 | @@tmpdir/#sqlcf28_1_4e.MYI             | 104.05 ms  | delete    | NULL      |
+----------------------+----------------------------------------+------------+-----------+-----------+
```

#### memory_by_user_by_current_bytes / x$memory_by_user_by_current_bytes

##### Description

Summarizes memory use by user using the 5.7 Performance Schema instrumentation.

##### Example

```SQL
mysql> select * from memory_by_user_by_current_bytes WHERE user IS NOT NULL;
+------+--------------------+-------------------+-------------------+-------------------+-----------------+
| user | current_count_used | current_allocated | current_avg_alloc | current_max_alloc | total_allocated |
+------+--------------------+-------------------+-------------------+-------------------+-----------------+
| root |               1401 | 1.09 MiB          | 815 bytes         | 334.97 KiB        | 42.73 MiB       |
| mark |                201 | 496.08 KiB        | 2.47 KiB          | 334.97 KiB        | 5.50 MiB        |
+------+--------------------+-------------------+-------------------+-------------------+-----------------+
```

#### memory_global_by_current_allocated / x$memory_global_by_current_allocated

##### Description

Shows the current memory usage within the server globally broken down by allocation type.

##### Example

```SQL
mysql> select * from memory_global_by_current_allocated;
+----------------------------------------+---------------+---------------+-------------------+------------+------------+----------------+
| event_name                             | current_count | current_alloc | current_avg_alloc | high_count | high_alloc | high_avg_alloc |
+----------------------------------------+---------------+---------------+-------------------+------------+------------+----------------+
| memory/sql/TABLE_SHARE::mem_root       |           269 | 568.21 KiB    | 2.11 KiB          |        339 | 706.04 KiB | 2.08 KiB       |
| memory/sql/TABLE                       |           214 | 366.56 KiB    | 1.71 KiB          |        245 | 481.13 KiB | 1.96 KiB       |
| memory/sql/sp_head::main_mem_root      |            32 | 334.97 KiB    | 10.47 KiB         |        421 | 9.73 MiB   | 23.66 KiB      |
| memory/sql/Filesort_buffer::sort_keys  |             1 | 255.89 KiB    | 255.89 KiB        |          1 | 256.00 KiB | 256.00 KiB     |
| memory/mysys/array_buffer              |            82 | 121.66 KiB    | 1.48 KiB          |       1124 | 852.55 KiB | 777 bytes      |
...
+----------------------------------------+---------------+---------------+-------------------+------------+------------+----------------+
```

#### memory_global_total

##### Description

Shows the total memory usage within the server globally.

##### Example

```SQL
mysql> select * from memory_global_total;
+-----------------+
| total_allocated |
+-----------------+
| 458.44 MiB      |
+-----------------+
```

#### processlist / x$processlist

##### Description

A detailed non-blocking processlist view to replace [INFORMATION_SCHEMA. | SHOW FULL] PROCESSLIST.

Performs less locking than the legacy sources, whilst giving extra information.

##### Example

```SQL
mysql> select * from processlist where conn_id is not null\G
*************************** 1. row ***************************
                thd_id: 31
               conn_id: 12
                  user: root@localhost
                    db: information_schema
               command: Query
                 state: Sending data
                  time: 0
     current_statement: select * from processlist limit 5
          lock_latency: 684.00 us
         rows_examined: 0
             rows_sent: 0
         rows_affected: 0
            tmp_tables: 2
       tmp_disk_tables: 0
             full_scan: YES
        current_memory: 1.29 MiB
        last_statement: NULL
last_statement_latency: NULL
             last_wait: wait/synch/mutex/sql/THD::LOCK_query_plan
     last_wait_latency: 260.13 ns
                source: sql_optimizer.cc:1075
```

#### ps_check_lost_instrumentation

##### Description

Used to check whether Performance Schema is not able to monitor all runtime data - only returns variables that have lost instruments

##### Example

```SQL
mysql> select * from ps_check_lost_instrumentation;
+----------------------------------------+----------------+
| variable_name                          | variable_value |
+----------------------------------------+----------------+
| Performance_schema_file_handles_lost   | 101223         |
| Performance_schema_file_instances_lost | 1231           |
+----------------------------------------+----------------+
```

#### schema_index_statistics / x$schema_index_statistics

##### Description

Statistics around indexes.

Ordered by the total wait time descending - top indexes are most contended.

##### Example

```SQL
mysql> select * from schema_index_statistics limit 5;
+------------------+-------------+------------+---------------+----------------+---------------+----------------+--------------+----------------+--------------+----------------+
| table_schema     | table_name  | index_name | rows_selected | select_latency | rows_inserted | insert_latency | rows_updated | update_latency | rows_deleted | delete_latency |
+------------------+-------------+------------+---------------+----------------+---------------+----------------+--------------+----------------+--------------+----------------+
| mem              | mysqlserver | PRIMARY    |          6208 | 108.27 ms      |             0 | 0 ps           |         5470 | 1.47 s         |            0 | 0 ps           |
| mem              | innodb      | PRIMARY    |          4666 | 76.27 ms       |             0 | 0 ps           |         4454 | 571.47 ms      |            0 | 0 ps           |
| mem              | connection  | PRIMARY    |          1064 | 20.98 ms       |             0 | 0 ps           |         1064 | 457.30 ms      |            0 | 0 ps           |
| mem              | environment | PRIMARY    |          5566 | 151.17 ms      |             0 | 0 ps           |          694 | 252.57 ms      |            0 | 0 ps           |
| mem              | querycache  | PRIMARY    |          1698 | 27.99 ms       |             0 | 0 ps           |         1698 | 371.72 ms      |            0 | 0 ps           |
+------------------+-------------+------------+---------------+----------------+---------------+----------------+--------------+----------------+--------------+----------------+
```

#### schema_object_overview

##### Description

Shows an overview of the types of objects within each schema

Note: On instances with a large numbers of objects, this could take some time to execute, and may not be recommended.

##### Example

```SQL
mysql> select * from schema_object_overview;
+---------------------------------+---------------+-------+
| db                              | object_type   | count |
+---------------------------------+---------------+-------+
| admin                           | PROCEDURE     |     1 |
| information_schema              | SYSTEM VIEW   |    59 |
| mem30_test__instruments         | BASE TABLE    |     1 |
| mem30_test__instruments         | INDEX (BTREE) |     2 |
| mem30_trunk__advisors           | BASE TABLE    |     2 |
| mem30_trunk__advisors           | INDEX (BTREE) |     4 |
| mem30_trunk__advisor_text       | BASE TABLE    |     2 |
| mem30_trunk__advisor_text       | INDEX (BTREE) |     5 |
| mem30_trunk__bean_config        | BASE TABLE    |     4 |
| mem30_trunk__bean_config        | INDEX (BTREE) |     6 |
| mem30_trunk__config             | BASE TABLE    |    12 |
| mem30_trunk__config             | INDEX (BTREE) |    21 |
| mem30_trunk__enterprise         | BASE TABLE    |     2 |
| mem30_trunk__enterprise         | INDEX (BTREE) |     3 |
| mem30_trunk__events             | BASE TABLE    |    32 |
| mem30_trunk__events             | INDEX (BTREE) |    69 |
| mem30_trunk__instruments        | BASE TABLE    |   118 |
| mem30_trunk__instruments        | INDEX (BTREE) |   587 |
| mem30_trunk__instruments_config | BASE TABLE    |     1 |
| mem30_trunk__instruments_config | INDEX (BTREE) |     1 |
| mem30_trunk__inventory          | BASE TABLE    |    77 |
| mem30_trunk__inventory          | INDEX (BTREE) |   277 |
| mem30_trunk__quan               | BASE TABLE    |     8 |
| mem30_trunk__quan               | INDEX (BTREE) |    35 |
| mysql                           | BASE TABLE    |    29 |
| mysql                           | INDEX (BTREE) |    54 |
| performance_schema              | BASE TABLE    |    52 |
| sys                             | FUNCTION      |     8 |
| sys                             | PROCEDURE     |    16 |
| sys                             | VIEW          |    59 |
+---------------------------------+---------------+-------+
```

#### schema_table_statistics / x$schema_table_statistics

##### Description

Statistics around tables.

Ordered by the total wait time descending - top tables are most contended.

##### Example

```SQL
mysql> select * from schema_table_statistics limit 1\G
*************************** 1. row ***************************
                 table_schema: mem
                   table_name: mysqlserver
                 rows_fetched: 27087
                fetch_latency: 442.72 ms
                rows_inserted: 2
               insert_latency: 185.04 µs 
                 rows_updated: 5096
               update_latency: 1.39 s
                 rows_deleted: 0
               delete_latency: 0 ps
             io_read_requests: 2565
                io_read_bytes: 1121627
              io_read_latency: 10.07 ms
            io_write_requests: 1691
               io_write_bytes: 128383
             io_write_latency: 14.17 ms
             io_misc_requests: 2698
              io_misc_latency: 433.66 ms
```

#### schema_table_statistics_with_buffer / x$schema_table_statistics_with_buffer

##### Description

Statistics around tables.

Ordered by the total wait time descending - top tables are most contended.

More statistics such as caching stats for the InnoDB buffer pool with InnoDB tables

##### Example

```SQL
mysql> select * from schema_table_statistics_with_buffer limit 1\G
*************************** 1. row ***************************
                 table_schema: mem
                   table_name: mysqlserver
                 rows_fetched: 27087
                fetch_latency: 442.72 ms
                rows_inserted: 2
               insert_latency: 185.04 µs 
                 rows_updated: 5096
               update_latency: 1.39 s
                 rows_deleted: 0
               delete_latency: 0 ps
             io_read_requests: 2565
                io_read_bytes: 1121627
              io_read_latency: 10.07 ms
            io_write_requests: 1691
               io_write_bytes: 128383
             io_write_latency: 14.17 ms
             io_misc_requests: 2698
              io_misc_latency: 433.66 ms
          innodb_buffer_pages: 19
   innodb_buffer_pages_hashed: 19
      innodb_buffer_pages_old: 19
innodb_buffer_bytes_allocated: 311296
     innodb_buffer_bytes_data: 1924
    innodb_buffer_rows_cached: 2
```

#### schema_tables_with_full_table_scans

##### Description

Finds tables that are being accessed by full table scans ordering by the number of rows scanned descending.

##### Example

```SQL
mysql> select * from schema_tables_with_full_table_scans limit 5;
+------------------+-------------------+-------------------+
| object_schema    | object_name       | rows_full_scanned |
+------------------+-------------------+-------------------+
| mem              | rule_alarms       |              1210 |
| mem30__advisors  | advisor_schedules |              1021 |
| mem30__inventory | agent             |               498 |
| mem              | dc_p_string       |               449 |
| mem30__inventory | mysqlserver       |               294 |
+------------------+-------------------+-------------------+
```

#### schema_unused_indexes

##### Description

Finds indexes that have had no events against them (and hence, no usage).

To trust whether the data from this view is representative of your workload, you should ensure that the server has been up for a representative amount of time before using it.

##### Example

```SQL
mysql> select * from schema_unused_indexes limit 10;
+-------------------------+----------------------------------------+-----------------+
| object_schema           | object_name                            | index_name      |
+-------------------------+----------------------------------------+-----------------+
| mem30_test__instruments | mysqlavailabilityadvisor$observedstate | PRIMARY         |
| mem30_test__test        | compressme                             | PRIMARY         |
| mem30_test__test        | compressmekeyblocksize                 | PRIMARY         |
| mem30_test__test        | dontcompressme                         | PRIMARY         |
| mem30_test__test        | round_robin_test                       | PRIMARY         |
| mem30_test__test        | round_robin_test                       | round_robin_bin |
| mem30_test__test        | testprovider$dummy1                    | PRIMARY         |
| mem30_test__test        | testprovider$dummy1                    | ts              |
| mem30_test__test        | testprovider$dummy2                    | PRIMARY         |
| mem30_test__test        | testprovider$dummy2                    | ts              |
+-------------------------+----------------------------------------+-----------------+
```

#### statement_analysis / x$statement_analysis

##### Description

Lists a normalized statement view with aggregated statistics, mimics the MySQL Enterprise Monitor Query Analysis view, ordered by the total execution time per normalized statement

##### Example

```SQL
mysql> select * from statement_analysis limit 1\G
*************************** 1. row ***************************
            query: SELECT * FROM `schema_object_o ... MA` , `information_schema` ...
               db: sys
        full_scan: *
       exec_count: 2
        err_count: 0
       warn_count: 0
    total_latency: 16.75 s
      max_latency: 16.57 s
      avg_latency: 8.38 s
     lock_latency: 16.69 s
        rows_sent: 84
    rows_sent_avg: 42
    rows_examined: 20012
rows_examined_avg: 10006
       tmp_tables: 378
  tmp_disk_tables: 66
      rows_sorted: 168
sort_merge_passes: 0
           digest: 54f9bd520f0bbf15db0c2ed93386bec9
       first_seen: 2014-03-07 13:13:41
        last_seen: 2014-03-07 13:13:48
```

#### statements_with_errors_or_warnings / x$statements_with_errors_or_warnings

##### Description

Lists all normalized statements that have raised errors or warnings.

##### Example

```SQL
mysql> select * from statements_with_errors_or_warnings LIMIT 1\G
*************************** 1. row ***************************
      query: CREATE OR REPLACE ALGORITHM =  ... _delete` AS `rows_deleted` ...
         db: sys
 exec_count: 2
     errors: 1
  error_pct: 50.0000
   warnings: 0
warning_pct: 0.0000
 first_seen: 2014-03-07 12:56:54
  last_seen: 2014-03-07 13:01:01
     digest: 943a788859e623d5f7798ba0ae0fd8a9
```

#### statements_with_full_table_scans / x$statements_with_full_table_scans

##### Description

Lists all normalized statements that use have done a full table scan ordered by number the percentage of times a full scan was done, then by the statement latency.

##### Example

```SQL
mysql> select * from statements_with_full_table_scans limit 1\G
*************************** 1. row ***************************
                   query: SELECT * FROM `schema_tables_w ... ex_usage` . `COUNT_READ` DESC
                      db: sys
              exec_count: 1
           total_latency: 88.20 ms
     no_index_used_count: 1
no_good_index_used_count: 0
       no_index_used_pct: 100
               rows_sent: 0
           rows_examined: 1501
           rows_sent_avg: 0
       rows_examined_avg: 1501
              first_seen: 2014-03-07 13:58:20
               last_seen: 2014-03-07 13:58:20
                  digest: 64baecd5c1e1e1651a6b92e55442a288
```

#### statements_with_runtimes_in_95th_percentile / x$statements_with_runtimes_in_95th_percentile

##### Description

Lists all statements who's average runtime, in microseconds, is in the top 95th percentile.

Also includes two helper views:

* x$ps_digest_avg_latency_distribution
* x$ps_digest_95th_percentile_by_avg_us

##### Example

```SQL
mysql> select * from statements_with_runtimes_in_95th_percentile\G
*************************** 1. row ***************************
            query: SELECT * FROM `schema_object_o ... MA` , `information_schema` ...
               db: sys
        full_scan: *
       exec_count: 2
        err_count: 0
       warn_count: 0
    total_latency: 16.75 s
      max_latency: 16.57 s
      avg_latency: 8.38 s
        rows_sent: 84
    rows_sent_avg: 42
    rows_examined: 20012
rows_examined_avg: 10006
       first_seen: 2014-03-07 13:13:41
        last_seen: 2014-03-07 13:13:48
           digest: 54f9bd520f0bbf15db0c2ed93386bec9
```

#### statements_with_sorting / x$statements_with_sorting

##### Description

Lists all normalized statements that have done sorts, ordered by total_latency descending.

##### Example

```SQL
mysql> select * from statements_with_sorting limit 1\G
*************************** 1. row ***************************
            query: SELECT * FROM `schema_object_o ... MA` , `information_schema` ...
               db: sys
       exec_count: 2
    total_latency: 16.75 s
sort_merge_passes: 0
  avg_sort_merges: 0
sorts_using_scans: 12
 sort_using_range: 0
      rows_sorted: 168
  avg_rows_sorted: 84
       first_seen: 2014-03-07 13:13:41
        last_seen: 2014-03-07 13:13:48
           digest: 54f9bd520f0bbf15db0c2ed93386bec9
```

#### statements_with_temp_tables / x$statements_with_temp_tables

##### Description

Lists all normalized statements that use temporary tables ordered by number of on disk temporary tables descending first, then by the number of memory tables.

##### Example

```SQL
mysql> select * from statements_with_temp_tables limit 1\G
*************************** 1. row ***************************
                   query: SELECT * FROM `schema_object_o ... MA` , `information_schema` ...
                      db: sys
              exec_count: 2
           total_latency: 16.75 s
       memory_tmp_tables: 378
         disk_tmp_tables: 66
avg_tmp_tables_per_query: 189
  tmp_tables_to_disk_pct: 17
              first_seen: 2014-03-07 13:13:41
               last_seen: 2014-03-07 13:13:48
                  digest: 54f9bd520f0bbf15db0c2ed93386bec9
```

#### user_summary / x$user_summary

##### Description

Summarizes statement activity, file IO and connections by user.

##### Example

```SQL
mysql> select * from user_summary;
+------+------------+-------------------+-----------------------+-------------+----------+-----------------+---------------------+-------------------+--------------+
| user | statements | statement_latency | statement_avg_latency | table_scans | file_ios | file_io_latency | current_connections | total_connections | unique_hosts |
+------+------------+-------------------+-----------------------+-------------+----------+-----------------+---------------------+-------------------+--------------+
| root |       2924 | 00:03:59.53       | 81.92 ms              |          82 |    54702 | 55.61 s         |                   1 |                 1 |            1 |
+------+------------+-------------------+-----------------------+-------------+----------+-----------------+---------------------+-------------------+--------------+
```

#### user_summary_by_file_io_type / x$user_summary_by_file_io_type

##### Description

Summarizes file IO by event type per user.

When the user found is NULL, it is assumed to be a "background" thread.

##### Example

```SQL
mysql> select * from user_summary_by_file_io_type;
+------------+--------------------------------------+-------+-----------+-------------+
| user       | event_name                           | total | latency   | max_latency |
+------------+--------------------------------------+-------+-----------+-------------+
| background | wait/io/file/sql/FRM                 |   871 | 168.15 ms | 18.48 ms    |
| background | wait/io/file/innodb/innodb_data_file |   173 | 129.56 ms | 34.09 ms    |
| background | wait/io/file/innodb/innodb_log_file  |    20 | 77.53 ms  | 60.66 ms    |
| background | wait/io/file/myisam/dfile            |    40 | 6.54 ms   | 4.58 ms     |
| background | wait/io/file/mysys/charset           |     3 | 4.79 ms   | 4.71 ms     |
| background | wait/io/file/myisam/kfile            |    67 | 4.38 ms   | 300.04 us   |
| background | wait/io/file/sql/ERRMSG              |     5 | 2.72 ms   | 1.69 ms     |
| background | wait/io/file/sql/pid                 |     3 | 266.30 us | 185.47 us   |
| background | wait/io/file/sql/casetest            |     5 | 246.81 us | 150.19 us   |
| background | wait/io/file/sql/global_ddl_log      |     2 | 21.24 us  | 18.59 us    |
| root       | wait/io/file/sql/file_parser         |  1422 | 4.80 s    | 135.14 ms   |
| root       | wait/io/file/sql/FRM                 |   865 | 85.82 ms  | 9.81 ms     |
| root       | wait/io/file/myisam/kfile            |  1073 | 37.14 ms  | 15.79 ms    |
| root       | wait/io/file/myisam/dfile            |  2991 | 25.53 ms  | 5.25 ms     |
| root       | wait/io/file/sql/dbopt               |    20 | 1.07 ms   | 153.07 us   |
| root       | wait/io/file/sql/misc                |     4 | 59.71 us  | 33.75 us    |
| root       | wait/io/file/archive/data            |     1 | 13.91 us  | 13.91 us    |
+------------+--------------------------------------+-------+-----------+-------------+
 ```

#### user_summary_by_file_io / x$user_summary_by_file_io

##### Description

Summarizes file IO totals per user.

When the user found is NULL, it is assumed to be a "background" thread.

##### Example

```SQL
mysql> select * from user_summary_by_file_io;
+------------+-------+------------+
| user       | ios   | io_latency |
+------------+-------+------------+
| root       | 26457 | 21.58 s    |
| background |  1189 | 394.21 ms  |
+------------+-------+------------+
```

#### user_summary_by_file_io_type / x$user_summary_by_file_io_type

##### Description

Summarizes file IO by event type per user.

When the user found is NULL, it is assumed to be a "background" thread.

##### Example

```SQL
mysql> select * from user_summary_by_file_io_type;
+------------+--------------------------------------+-------+-----------+-------------+
| user       | event_name                           | total | latency   | max_latency |
+------------+--------------------------------------+-------+-----------+-------------+
| background | wait/io/file/innodb/innodb_data_file |  1434 | 3.29 s    | 147.56 ms   |
| background | wait/io/file/sql/FRM                 |   910 | 286.61 ms | 32.92 ms    |
| background | wait/io/file/sql/relaylog            |     9 | 252.28 ms | 144.17 ms   |
| background | wait/io/file/sql/binlog              |    56 | 193.73 ms | 153.72 ms   |
| background | wait/io/file/sql/binlog_index        |    22 | 183.02 ms | 81.83 ms    |
| background | wait/io/file/innodb/innodb_log_file  |    20 | 117.17 ms | 36.53 ms    |
| background | wait/io/file/sql/relaylog_index      |     9 | 50.15 ms  | 48.04 ms    |
| background | wait/io/file/sql/ERRMSG              |     5 | 35.41 ms  | 31.78 ms    |
| background | wait/io/file/myisam/kfile            |    67 | 18.14 ms  | 9.00 ms     |
| background | wait/io/file/mysys/charset           |     3 | 7.46 ms   | 4.13 ms     |
| background | wait/io/file/sql/casetest            |     5 | 6.01 ms   | 5.86 ms     |
| background | wait/io/file/sql/pid                 |     3 | 5.96 ms   | 3.06 ms     |
| background | wait/io/file/myisam/dfile            |    43 | 980.38 us | 152.46 us   |
| background | wait/io/file/mysys/cnf               |     5 | 154.97 us | 58.87 us    |
| background | wait/io/file/sql/global_ddl_log      |     2 | 18.64 us  | 16.40 us    |
| root       | wait/io/file/sql/file_parser         | 11048 | 48.79 s   | 201.11 ms   |
| root       | wait/io/file/innodb/innodb_data_file |  4699 | 3.02 s    | 46.93 ms    |
| root       | wait/io/file/sql/FRM                 | 10403 | 2.38 s    | 61.72 ms    |
| root       | wait/io/file/myisam/dfile            | 22143 | 726.77 ms | 308.79 ms   |
| root       | wait/io/file/myisam/kfile            |  6213 | 435.35 ms | 88.76 ms    |
| root       | wait/io/file/sql/dbopt               |   159 | 130.86 ms | 15.46 ms    |
| root       | wait/io/file/csv/metadata            |     8 | 86.60 ms  | 50.32 ms    |
| root       | wait/io/file/sql/binlog              |    15 | 38.79 ms  | 9.40 ms     |
| root       | wait/io/file/sql/misc                |    21 | 22.33 ms  | 15.30 ms    |
| root       | wait/io/file/csv/data                |     4 | 297.46 us | 111.93 us   |
| root       | wait/io/file/archive/data            |     3 | 54.10 us  | 40.74 us    |
+------------+--------------------------------------+-------+-----------+-------------+
```

#### user_summary_by_stages / x$user_summary_by_stages

##### Description

Summarizes stages by user, ordered by user and total latency per stage.

##### Example

```SQL
mysql> select * from user_summary_by_stages;
+------+--------------------------------+-------+-----------+-----------+
| user | event_name                     | total | wait_sum  | wait_avg  |
+------+--------------------------------+-------+-----------+-----------+
| root | stage/sql/Opening tables       |   889 | 1.97 ms   | 2.22 us   |
| root | stage/sql/Creating sort index  |     4 | 1.79 ms   | 446.30 us |
| root | stage/sql/init                 |    10 | 312.27 us | 31.23 us  |
| root | stage/sql/checking permissions |    10 | 300.62 us | 30.06 us  |
| root | stage/sql/freeing items        |     5 | 85.89 us  | 17.18 us  |
| root | stage/sql/statistics           |     5 | 79.15 us  | 15.83 us  |
| root | stage/sql/preparing            |     5 | 69.12 us  | 13.82 us  |
| root | stage/sql/optimizing           |     5 | 53.11 us  | 10.62 us  |
| root | stage/sql/Sending data         |     5 | 44.66 us  | 8.93 us   |
| root | stage/sql/closing tables       |     5 | 37.54 us  | 7.51 us   |
| root | stage/sql/System lock          |     5 | 34.28 us  | 6.86 us   |
| root | stage/sql/query end            |     5 | 24.37 us  | 4.87 us   |
| root | stage/sql/end                  |     5 | 8.60 us   | 1.72 us   |
| root | stage/sql/Sorting result       |     5 | 8.33 us   | 1.67 us   |
| root | stage/sql/executing            |     5 | 5.37 us   | 1.07 us   |
| root | stage/sql/cleaning up          |     5 | 4.60 us   | 919.00 ns |
+------+--------------------------------+-------+-----------+-----------+
```

#### user_summary_by_statement_latency / x$user_summary_by_statement_latency

##### Description

Summarizes overall statement statistics by user.

##### Example

```SQL
mysql> select * from user_summary_by_statement_latency;
+------+-------+---------------+-------------+--------------+-----------+---------------+---------------+------------+
| user | total | total_latency | max_latency | lock_latency | rows_sent | rows_examined | rows_affected | full_scans |
+------+-------+---------------+-------------+--------------+-----------+---------------+---------------+------------+
| root |  3381 | 00:02:09.13   | 1.48 s      | 1.07 s       |      1151 |         93947 |           150 |         91 |
+------+-------+---------------+-------------+--------------+-----------+---------------+---------------+------------+
```

#### user_summary_by_statement_type / x$user_summary_by_statement_type

##### Description

Summarizes the types of statements executed by each user.

##### Example

```SQL
mysql> select * from user_summary_by_statement_type;
+------+------------------+-------+---------------+-------------+--------------+-----------+---------------+---------------+------------+
| user | statement        | total | total_latency | max_latency | lock_latency | rows_sent | rows_examined | rows_affected | full_scans |
+------+------------------+-------+---------------+-------------+--------------+-----------+---------------+---------------+------------+
| root | create_view      |  1332 | 00:03:39.08   | 677.76 ms   | 494.56 ms    |         0 |             0 |             0 |          0 |
| root | select           |    88 | 20.13 s       | 16.57 s     | 17.40 s      |      1804 |         77285 |             0 |         48 |
| root | drop_db          |    16 | 6.83 s        | 1.14 s      | 5.73 s       |         0 |             0 |           953 |          0 |
| root | drop_view        |   392 | 1.70 s        | 739.49 ms   | 0 ps         |         0 |             0 |             0 |          0 |
| root | show_databases   |    16 | 1.37 s        | 587.44 ms   | 1.31 ms      |       400 |           400 |             0 |         16 |
| root | show_tables      |    34 | 676.78 ms     | 167.04 ms   | 3.46 ms      |      1087 |          1087 |             0 |         34 |
| root | create_db        |    22 | 334.90 ms     | 38.93 ms    | 0 ps         |         0 |             0 |            22 |          0 |
| root | create_procedure |   352 | 250.02 ms     | 21.90 ms    | 165.17 ms    |         0 |             0 |             0 |          0 |
| root | drop_function    |   176 | 122.44 ms     | 69.18 ms    | 87.24 ms     |         0 |             0 |             0 |          0 |
| root | create_function  |   176 | 76.12 ms      | 1.36 ms     | 49.50 ms     |         0 |             0 |             0 |          0 |
| root | drop_procedure   |   352 | 67.41 ms      | 1.57 ms     | 36.22 ms     |         0 |             0 |             0 |          0 |
| root | update           |     2 | 41.75 ms      | 35.96 ms    | 35.52 ms     |         0 |           557 |           338 |          0 |
| root | error            |     3 | 17.22 ms      | 17.05 ms    | 0 ps         |         0 |             0 |             0 |          0 |
| root | set_option       |    88 | 8.02 ms       | 1.63 ms     | 0 ps         |         0 |             0 |             0 |          0 |
| root | call_procedure   |     2 | 2.98 ms       | 2.29 ms     | 95.00 us     |         0 |             0 |             0 |          0 |
| root | Init DB          |    22 | 1.07 ms       | 117.65 us   | 0 ps         |         0 |             0 |             0 |          0 |
| root | show_status      |     1 | 408.69 us     | 408.69 us   | 102.00 us    |        23 |            23 |             0 |          1 |
+------+------------------+-------+---------------+-------------+--------------+-----------+---------------+---------------+------------+
```

#### wait_classes_global_by_avg_latency / x$wait_classes_global_by_avg_latency

##### Description

Lists the top wait classes by average latency, ignoring idle (this may be very large).

##### Example

```SQL
mysql> select * from wait_classes_global_by_avg_latency where event_class != 'idle';
+-------------------+--------+---------------+-------------+-------------+-------------+
| event_class       | total  | total_latency | min_latency | avg_latency | max_latency |
+-------------------+--------+---------------+-------------+-------------+-------------+
| wait/io/file      | 543123 | 44.60 s       | 19.44 ns    | 82.11 µs    | 4.21 s      |
| wait/io/table     |  22002 | 766.60 ms     | 148.72 ns   | 34.84 µs    | 44.97 ms    |
| wait/io/socket    |  79613 | 967.17 ms     | 0 ps        | 12.15 µs    | 27.10 ms    |
| wait/lock/table   |  35409 | 18.68 ms      | 65.45 ns    | 527.51 ns   | 969.88 µs   |
| wait/synch/rwlock |  37935 | 4.61 ms       | 21.38 ns    | 121.61 ns   | 34.65 µs    |
| wait/synch/mutex  | 390622 | 18.60 ms      | 19.44 ns    | 47.61 ns    | 10.32 µs    |
+-------------------+--------+---------------+-------------+-------------+-------------+
```

#### wait_classes_global_by_latency / x$wait_classes_global_by_latency

##### Description

Lists the top wait classes by total latency, ignoring idle (this may be very large).

##### Example

```SQL
mysql> select * from wait_classes_global_by_latency;
+-------------------+--------+---------------+-------------+-------------+-------------+
| event_class       | total  | total_latency | min_latency | avg_latency | max_latency |
+-------------------+--------+---------------+-------------+-------------+-------------+
| wait/io/file      | 550470 | 46.01 s       | 19.44 ns    | 83.58 µs    | 4.21 s      |
| wait/io/socket    | 228833 | 2.71 s        | 0 ps        | 11.86 µs    | 29.93 ms    |
| wait/io/table     |  64063 | 1.89 s        | 99.79 ns    | 29.43 µs    | 68.07 ms    |
| wait/lock/table   |  76029 | 47.19 ms      | 65.45 ns    | 620.74 ns   | 969.88 µs   |
| wait/synch/mutex  | 635925 | 34.93 ms      | 19.44 ns    | 54.93 ns    | 107.70 µs   |
| wait/synch/rwlock |  61287 | 7.62 ms       | 21.38 ns    | 124.37 ns   | 34.65 µs    |
+-------------------+--------+---------------+-------------+-------------+-------------+
```

#### waits_by_user_by_latency / x$waits_by_user_by_latency

##### Description

Lists the top wait events by their total latency, ignoring idle (this may be very large).

##### Example

```SQL
mysql> select * from waits_by_user_by_latency;
+------+-----------------------------------------------------+--------+---------------+-------------+-------------+
| user | event                                               | total  | total_latency | avg_latency | max_latency |
+------+-----------------------------------------------------+--------+---------------+-------------+-------------+
| root | wait/io/file/sql/file_parser                        |  13743 | 00:01:00.46   | 4.40 ms     | 231.88 ms   |
| root | wait/io/file/innodb/innodb_data_file                |   4699 | 3.02 s        | 643.38 us   | 46.93 ms    |
| root | wait/io/file/sql/FRM                                |  11462 | 2.60 s        | 226.83 us   | 61.72 ms    |
| root | wait/io/file/myisam/dfile                           |  26776 | 746.70 ms     | 27.89 us    | 308.79 ms   |
| root | wait/io/file/myisam/kfile                           |   7126 | 462.66 ms     | 64.93 us    | 88.76 ms    |
| root | wait/io/file/sql/dbopt                              |    179 | 137.58 ms     | 768.59 us   | 15.46 ms    |
| root | wait/io/file/csv/metadata                           |      8 | 86.60 ms      | 10.82 ms    | 50.32 ms    |
| root | wait/synch/mutex/mysys/IO_CACHE::append_buffer_lock | 798080 | 66.46 ms      | 82.94 ns    | 161.03 us   |
| root | wait/io/file/sql/binlog                             |     19 | 49.11 ms      | 2.58 ms     | 9.40 ms     |
| root | wait/io/file/sql/misc                               |     26 | 22.38 ms      | 860.80 us   | 15.30 ms    |
| root | wait/io/file/csv/data                               |      4 | 297.46 us     | 74.37 us    | 111.93 us   |
| root | wait/synch/rwlock/sql/MDL_lock::rwlock              |    944 | 287.86 us     | 304.62 ns   | 874.64 ns   |
| root | wait/io/file/archive/data                           |      4 | 82.71 us      | 20.68 us    | 40.74 us    |
| root | wait/synch/mutex/myisam/MYISAM_SHARE::intern_lock   |     60 | 12.21 us      | 203.20 ns   | 512.72 ns   |
| root | wait/synch/mutex/innodb/trx_mutex                   |     81 | 5.93 us       | 73.14 ns    | 252.59 ns   |
+------+-----------------------------------------------------+--------+---------------+-------------+-------------+
```

#### waits_global_by_latency / x$waits_global_by_latency

##### Description

Lists the top wait events by their total latency, ignoring idle (this may be very large).

##### Example

```SQL
mysql> select * from waits_global_by_latency;
+-----------------------------------------------------+---------+---------------+-------------+-------------+
| events                                              | total   | total_latency | avg_latency | max_latency |
+-----------------------------------------------------+---------+---------------+-------------+-------------+
| wait/io/file/sql/file_parser                        | 14936   | 00:01:06.64   | 4.46 ms     | 231.88 ms   |
| wait/io/file/innodb/innodb_data_file                |    6133 | 6.31 s        | 1.03 ms     | 147.56 ms   |
| wait/io/file/sql/FRM                                |   12677 | 2.83 s        | 223.37 us   | 40.86 ms    |
| wait/io/file/myisam/dfile                           |   28446 | 754.40 ms     | 26.52 us    | 308.79 ms   |
| wait/io/file/myisam/kfile                           |    7572 | 491.17 ms     | 64.87 us    | 88.76 ms    |
| wait/io/file/sql/relaylog                           |       9 | 252.28 ms     | 28.03 ms    | 144.17 ms   |
| wait/io/file/sql/binlog                             |      76 | 242.87 ms     | 3.20 ms     | 153.72 ms   |
| wait/io/file/sql/binlog_index                       |      21 | 173.07 ms     | 8.24 ms     | 81.83 ms    |
| wait/io/file/sql/dbopt                              |     184 | 149.52 ms     | 812.62 us   | 15.46 ms    |
| wait/io/file/innodb/innodb_log_file                 |      20 | 117.17 ms     | 5.86 ms     | 36.53 ms    |
| wait/synch/mutex/mysys/IO_CACHE::append_buffer_lock | 1197128 | 99.27 ms      | 82.56 ns    | 161.03 us   |
| wait/io/file/csv/metadata                           |       8 | 86.60 ms      | 10.82 ms    | 50.32 ms    |
| wait/io/file/sql/relaylog_index                     |      10 | 60.10 ms      | 6.01 ms     | 48.04 ms    |
| wait/io/file/sql/ERRMSG                             |       5 | 35.41 ms      | 7.08 ms     | 31.78 ms    |
| wait/io/file/sql/misc                               |      28 | 22.40 ms      | 800.06 us   | 15.30 ms    |
| wait/io/file/mysys/charset                          |       3 | 7.46 ms       | 2.49 ms     | 4.13 ms     |
| wait/io/file/sql/casetest                           |       5 | 6.01 ms       | 1.20 ms     | 5.86 ms     |
| wait/io/file/sql/pid                                |       3 | 5.96 ms       | 1.99 ms     | 3.06 ms     |
| wait/synch/rwlock/sql/MDL_lock::rwlock              |    1396 | 420.58 us     | 301.22 ns   | 874.64 ns   |
| wait/io/file/csv/data                               |       4 | 297.46 us     | 74.37 us    | 111.93 us   |
| wait/io/file/mysys/cnf                              |       5 | 154.97 us     | 30.99 us    | 58.87 us    |
| wait/io/file/archive/data                           |       4 | 82.71 us      | 20.68 us    | 40.74 us    |
| wait/synch/mutex/myisam/MYISAM_SHARE::intern_lock   |      90 | 19.23 us      | 213.38 ns   | 576.81 ns   |
| wait/io/file/sql/global_ddl_log                     |       2 | 18.64 us      | 9.32 us     | 16.40 us    |
| wait/synch/mutex/innodb/trx_mutex                   |     108 | 8.23 us       | 76.15 ns    | 365.69 ns   |
+-----------------------------------------------------+---------+---------------+-------------+-------------+
```

### Functions

#### extract_schema_from_file_name

##### Description

Takes a raw file path, and attempts to extract the schema name from it.

Useful for when interacting with Performance Schema data concerning IO statistics, for example.

Currently relies on the fact that a table data file will be within a specified database directory (will not work with partitions or tables that specify an individual DATA_DIRECTORY).

##### Parameters

* path (VARCHAR(512)): The full file path to a data file to extract the schema name from.

##### Returns

VARCHAR(512)

##### Example
```SQL
mysql> SELECT sys.extract_schema_from_file_name('/var/lib/mysql/employees/employee.ibd');
+----------------------------------------------------------------------------+
| sys.extract_schema_from_file_name('/var/lib/mysql/employees/employee.ibd') |
+----------------------------------------------------------------------------+
| employees                                                                  |
+----------------------------------------------------------------------------+
1 row in set (0.00 sec)
```

#### extract_table_from_file_name

##### Description

Takes a raw file path, and extracts the table name from it.

Useful for when interacting with Performance Schema data concerning IO statistics, for example.

##### Parameters

* path (VARCHAR(512)): The full file path to a data file to extract the table name from.

##### Returns

VARCHAR(512)

##### Example
```SQL
mysql> SELECT sys.extract_table_from_file_name('/var/lib/mysql/employees/employee.ibd');
+---------------------------------------------------------------------------+
| sys.extract_table_from_file_name('/var/lib/mysql/employees/employee.ibd') |
+---------------------------------------------------------------------------+
| employee                                                                  |
+---------------------------------------------------------------------------+
1 row in set (0.02 sec)
```         

#### format_bytes

##### Description

Takes a raw bytes value, and converts it to a human readable format.

##### Parameters

* bytes (BIGINT): A raw bytes value.

##### Returns

VARCHAR(16)

##### Example
```SQL
mysql> SELECT sys.format_bytes(2348723492723746) AS size;
+----------+
| size     |
+----------+
| 2.09 PiB |
+----------+
1 row in set (0.00 sec)

mysql> SELECT sys.format_bytes(2348723492723) AS size;
+----------+
| size     |
+----------+
| 2.14 TiB |
+----------+
1 row in set (0.00 sec)

mysql> SELECT sys.format_bytes(23487234) AS size;
+-----------+
| size      |
+-----------+
| 22.40 MiB |
+-----------+
1 row in set (0.00 sec)
```

#### format_path

##### Description

Takes a raw path value, and strips out the datadir or tmpdir replacing with @@datadir and @@tmpdir respectively. 

Also normalizes the paths across operating systems, so backslashes on Windows are converted to forward slashes.

##### Parameters

* path (VARCHAR(260)): The raw file path value to format.

##### Returns

VARCHAR(260) CHARSET UTF8

##### Example
```SQL
mysql> select @@datadir;
+-----------------------------------------------+
| @@datadir                                     |
+-----------------------------------------------+
| /Users/mark/sandboxes/SmallTree/AMaster/data/ |
+-----------------------------------------------+
1 row in set (0.06 sec)

mysql> select format_path('/Users/mark/sandboxes/SmallTree/AMaster/data/mysql/proc.MYD') AS path;
+--------------------------+
| path                     |
+--------------------------+
| @@datadir/mysql/proc.MYD |
+--------------------------+
1 row in set (0.03 sec)
```

#### format_statement

##### Description

Formats a normalized statement, truncating it if it's > 64 characters long.

Useful for printing statement related data from Performance Schema from the command line.

##### Parameters

* statement (LONGTEXT): The statement to format.

##### Returns

VARCHAR(65)

##### Example
```SQL
mysql> SELECT sys.format_statement(digest_text)
    ->   FROM performance_schema.events_statements_summary_by_digest
    ->  ORDER by sum_timer_wait DESC limit 5;
+-------------------------------------------------------------------+
| sys.format_statement(digest_text)                                 |
+-------------------------------------------------------------------+
| CREATE SQL SECURITY INVOKER VI ... KE ? AND `variable_value` > ?  |
| CREATE SQL SECURITY INVOKER VI ... ait` IS NOT NULL , `esc` . ... |
| CREATE SQL SECURITY INVOKER VI ... ait` IS NOT NULL , `sys` . ... |
| CREATE SQL SECURITY INVOKER VI ...  , `compressed_size` ) ) DESC  |
| CREATE SQL SECURITY INVOKER VI ... LIKE ? ORDER BY `timer_start`  |
+-------------------------------------------------------------------+
5 rows in set (0.00 sec)
```

#### format_time

##### Description

Takes a raw picoseconds value, and converts it to a human readable form.
             
Picoseconds are the precision that all latency values are printed in within Performance Schema, however are not user friendly when wanting to scan output from the command line.

##### Parameters

* picoseconds (BIGINT UNSIGNED): The raw picoseconds value to convert.

##### Returns

VARCHAR(16) CHARSET UTF8

##### Example
```SQL
mysql> select format_time(342342342342345);
+------------------------------+
| format_time(342342342342345) |
+------------------------------+
| 00:05:42                     |
+------------------------------+
1 row in set (0.00 sec)

mysql> select format_time(342342342);
+------------------------+
| format_time(342342342) |
+------------------------+
| 342.34 µs              |
+------------------------+
1 row in set (0.00 sec)

mysql> select format_time(34234);
 +--------------------+
| format_time(34234) |
+--------------------+
| 34.23 ns           |
+--------------------+
1 row in set (0.00 sec)
```

#### ps_is_account_enabled

##### Description

Determines whether instrumentation of an account is enabled within Performance Schema.

##### Parameters

* in_host VARCHAR(60): The hostname of the account to check.
* in_user (VARCHAR(16)): The username of the account to check.

##### Returns

ENUM('YES', 'NO', 'PARTIAL')

##### Example
```SQL
mysql> SELECT sys.ps_is_account_enabled('localhost', 'root');
+------------------------------------------------+
| sys.ps_is_account_enabled('localhost', 'root') |
+------------------------------------------------+
| YES                                            |
+------------------------------------------------+
1 row in set (0.01 sec)
```

#### ps_thread_stack

##### Description

Outputs a JSON formatted stack of all statements, stages and events within Performance Schema for the specified thread.

##### Parameters

* thd_id (BIGINT): The id of the thread to trace. This should match the thread_id column from the performance_schema.threads table.

##### Example

(line separation added for output)

```SQL
 mysql> SELECT sys.ps_thread_stack(37, FALSE) AS thread_stack\G
*************************** 1. row ***************************
thread_stack: {"rankdir": "LR","nodesep": "0.10","stack_created": "2014-02-19 13:39:03",
"mysql_version": "5.7.3-m13","mysql_user": "root@localhost","events": 
[{"nesting_event_id": "0", "event_id": "10", "timer_wait": 256.35, "event_info": 
"sql/select", "wait_info": "select @@version_comment limit 1\nerrors: 0\nwarnings: 0\nlock time:
...
```


### Procedures

#### create_synonym_db

##### Description

Takes a source database name and synonym name, and then creates the synonym database with views that point to all of the tables within the source database.

Useful for creating a "ps" synonym for "performance_schema", or "is" instead of "information_schema", for example.

##### Parameters

* in_db_name (VARCHAR(64)):
** The database name that you would like to create a synonym for.
* in_synonym (VARCHAR(64)):
** The database synonym name.

##### Example
```SQL
mysql> SHOW DATABASES;
+--------------------+
| Database           |
+--------------------+
| information_schema |
| mysql              |
| performance_schema |
| sys                |
| test               |
+--------------------+
5 rows in set (0.00 sec)

mysql> CALL sys.create_synonym_db('performance_schema', 'ps');
+-------------------------------------+
| summary                             |
+-------------------------------------+
| Created 74 views in the ps database |
+-------------------------------------+
1 row in set (8.57 sec)

Query OK, 0 rows affected (8.57 sec)

mysql> SHOW DATABASES;
+--------------------+
| Database           |
+--------------------+
| information_schema |
| mysql              |
| performance_schema |
| ps                 |
| sys                |
| test               |
+--------------------+
6 rows in set (0.00 sec)

mysql> SHOW FULL TABLES FROM ps;
+-----------------------------------------+------------+
| Tables_in_ps                            | Table_type |
+-----------------------------------------+------------+
| accounts                                | VIEW       |
| cond_instances                          | VIEW       |
| events_stages_current                   | VIEW       |
| events_stages_history                   | VIEW       |
...
```

#### ps_setup_disable_background_threads

##### Description

Disable all background thread instrumentation within Performance Schema.

Requires the SUPER privilege for "SET sql_log_bin = 0;".

##### Parameters

None.

##### Example
```SQL
mysql> CALL sys.ps_setup_disable_background_threads();
+--------------------------------+
| summary                        |
+--------------------------------+
| Disabled 18 background threads |
+--------------------------------+
1 row in set (0.00 sec)
```

#### ps_setup_disable_instrument

##### Description

Disables instruments within Performance Schema  matching the input pattern.

Requires the SUPER privilege for "SET sql_log_bin = 0;".

##### Parameters

* in_pattern (VARCHAR(128)): A LIKE pattern match (using "%in_pattern%") of events to disable

##### Example

To disable all mutex instruments:
```SQL
mysql> CALL sys.ps_setup_disable_instrument('wait/synch/mutex');
+--------------------------+
| summary                  |
+--------------------------+
| Disabled 155 instruments |
+--------------------------+
1 row in set (0.02 sec)
```
To disable just a the scpecific TCP/IP based network IO instrument:
```SQL
mysql> CALL sys.ps_setup_disable_instrument('wait/io/socket/sql/server_tcpip_socket');
+------------------------+
| summary                |
+------------------------+
| Disabled 1 instruments |
+------------------------+
1 row in set (0.00 sec)
```
To enable all instruments:
```SQL
mysql> CALL sys.ps_setup_disable_instrument('');
+--------------------------+
| summary                  |
+--------------------------+
| Disabled 547 instruments |
+--------------------------+
1 row in set (0.01 sec)
```

#### ps_setup_disable_consumers

##### Description

Disables consumers within Performance Schema matching the input pattern.

Requires the SUPER privilege for "SET sql_log_bin = 0;".

##### Parameters

* consumer (VARCHAR(128)): A LIKE pattern match (using "%consumer%") of consumers to disable

##### Example

To disable all consumers:
```SQL
mysql> CALL sys.ps_setup_disable_consumers('');
+--------------------------+
| summary                  |
+--------------------------+
| Disabled 15 consumers    |
+--------------------------+
1 row in set (0.02 sec)
```

To disable just the event_stage consumers:
```SQL
mysql> CALL sys.ps_setup_disable_consumers('stage');
+------------------------+
| summary                |
+------------------------+
| Disabled 3 consumers   |
+------------------------+
1 row in set (0.00 sec)
```

#### ps_setup_disable_thread

##### Description

Disable the given connection/thread in Performance Schema.

Requires the SUPER privilege for "SET sql_log_bin = 0;".

##### Parameters

* in_connection_id (BIGINT): The connection ID (PROCESSLIST_ID from performance_schema.threads or the ID shown within SHOW PROCESSLIST)

##### Example
```SQL
mysql> CALL sys.ps_setup_disable_thread(3);
+-------------------+
| summary           |
+-------------------+
| Disabled 1 thread |
+-------------------+
1 row in set (0.01 sec)
```
To disable the current connection:
```SQL
mysql> CALL sys.ps_setup_disable_thread(CONNECTION_ID());
+-------------------+
| summary           |
+-------------------+
| Disabled 1 thread |
+-------------------+
1 row in set (0.00 sec)
```

#### ps_setup_enable_background_threads

##### Description

Enable all background thread instrumentation within Performance Schema.

Requires the SUPER privilege for "SET sql_log_bin = 0;".

##### Parameters

None.

##### Example
```SQL
mysql> CALL sys.ps_setup_enable_background_threads();
+-------------------------------+
| summary                       |
+-------------------------------+
| Enabled 18 background threads |
+-------------------------------+
1 row in set (0.00 sec)
```

#### ps_setup_enable_consumers

##### Description

Enables consumers within Performance Schema matching the input pattern.

Requires the SUPER privilege for "SET sql_log_bin = 0;".

##### Parameters

* consumer (VARCHAR(128)): A LIKE pattern match (using "%consumer%") of consumers to enable

##### Example

To enable all consumers:
```SQL
mysql> CALL sys.ps_setup_enable_consumers('');
+-------------------------+
| summary                 |
+-------------------------+
| Enabled 10 consumers    |
+-------------------------+
1 row in set (0.02 sec)
```

To enable just "waits" consumers:
```SQL
mysql> CALL sys.ps_setup_enable_consumers('waits');
+-----------------------+
| summary               |
+-----------------------+
| Enabled 3 consumers   |
+-----------------------+
1 row in set (0.00 sec)
```

#### ps_setup_enable_instrument

##### Description

Enables instruments within Performance Schema matching the input pattern.

Requires the SUPER privilege for "SET sql_log_bin = 0;".

##### Parameters


* in_pattern (VARCHAR(128)): A LIKE pattern match (using "%in_pattern%") of events to enable

##### Example

To enable all mutex instruments:
```SQL
mysql> CALL sys.ps_setup_enable_instrument('wait/synch/mutex');
+-------------------------+
| summary                 |
+-------------------------+
| Enabled 155 instruments |
+-------------------------+
1 row in set (0.02 sec)
```
To enable just a the scpecific TCP/IP based network IO instrument:
```SQL
mysql> CALL sys.ps_setup_enable_instrument('wait/io/socket/sql/server_tcpip_socket');
+-----------------------+
| summary               |
+-----------------------+
| Enabled 1 instruments |
+-----------------------+
1 row in set (0.00 sec)
```
To enable all instruments:
```SQL
mysql> CALL sys.ps_setup_enable_instrument('');
+-------------------------+
| summary                 |
+-------------------------+
| Enabled 547 instruments |
+-------------------------+
1 row in set (0.01 sec)
```

#### ps_setup_enable_thread

##### Description

Enable the given connection/thread in Performance Schema.

Requires the SUPER privilege for "SET sql_log_bin = 0;".

##### Parameters


* in_connection_id (BIGINT): The connection ID (PROCESSLIST_ID from performance_schema.threads or the ID shown within SHOW PROCESSLIST)

##### Example
```SQL
mysql> CALL sys.ps_setup_enable_thread(3);
+------------------+
| summary          |
+------------------+
| Enabled 1 thread |
+------------------+
1 row in set (0.01 sec)
```
To enable the current connection:
```SQL
mysql> CALL sys.ps_setup_enable_thread(CONNECTION_ID());
+------------------+
| summary          |
+------------------+
| Enabled 1 thread |
+------------------+
1 row in set (0.00 sec)
```

#### ps_setup_reload_saved

##### Description

Reloads a saved Performance Schema configuration, so that you can alter the setup for debugging purposes, but restore it to a previous state.
             
Use the companion procedure - ps_setup_save(), to save a configuration.

Requires the SUPER privilege for "SET sql_log_bin = 0;".

##### Parameters

None.

##### Example
```SQL
mysql> CALL sys.ps_setup_save();
Query OK, 0 rows affected (0.08 sec)

mysql> UPDATE performance_schema.setup_instruments SET enabled = 'YES', timed = 'YES';
Query OK, 547 rows affected (0.40 sec)
Rows matched: 784  Changed: 547  Warnings: 0

/* Run some tests that need more detailed instrumentation here */

mysql> CALL sys.ps_setup_reload_saved();
Query OK, 0 rows affected (0.32 sec)
```

#### ps_setup_reset_to_default

##### Description

Resets the Performance Schema setup to the default settings.

##### Parameters

* in_verbose (BOOLEAN): Whether to print each setup stage (including the SQL) whilst running.

##### Example
```SQL
mysql> CALL sys.ps_setup_reset_to_default(true)\G
*************************** 1. row ***************************
status: Resetting: setup_actors
DELETE
FROM performance_schema.setup_actors
WHERE NOT (HOST = '%' AND USER = '%' AND ROLE = '%')
1 row in set (0.00 sec)

*************************** 1. row ***************************
status: Resetting: setup_actors
INSERT IGNORE INTO performance_schema.setup_actors
VALUES ('%', '%', '%')
1 row in set (0.00 sec)
...

mysql> CALL sys.ps_setup_reset_to_default(false)G
Query OK, 0 rows affected (0.00 sec)
```

#### ps_setup_save

##### Description

Saves the current configuration of Performance Schema, so that you can alter the setup for debugging purposes, but restore it to a previous state.

Use the companion procedure - ps_setup_reload_saved(), to restore the saved config.

Requires the SUPER privilege for "SET sql_log_bin = 0;".

##### Parameters

None.

##### Example
```SQL
mysql> CALL sys.ps_setup_save();
Query OK, 0 rows affected (0.08 sec)

mysql> UPDATE performance_schema.setup_instruments 
    ->    SET enabled = 'YES', timed = 'YES';
Query OK, 547 rows affected (0.40 sec)
Rows matched: 784  Changed: 547  Warnings: 0

/* Run some tests that need more detailed instrumentation here */

mysql> CALL sys.ps_setup_reload_saved();
Query OK, 0 rows affected (0.32 sec)
```

#### ps_setup_show_disabled

##### Description

Shows all currently disable Performance Schema configuration.

##### Parameters

* in_in_show_instruments (BOOLEAN): Whether to print disabled instruments (can print many items)
* in_in_show_threads (BOOLEAN): Whether to print disabled threads

##### Example
```SQL
mysql> CALL sys.ps_setup_show_disabled(TRUE, TRUE);
+----------------------------+
| performance_schema_enabled |
+----------------------------+
|                          1 |
+----------------------------+
1 row in set (0.00 sec)

+--------------------+
| enabled_users      |
+--------------------+
| 'mark'@'localhost' |
+--------------------+
1 row in set (0.00 sec)

+-------------+----------------------+---------+-------+
| object_type | objects              | enabled | timed |
+-------------+----------------------+---------+-------+
| EVENT       | mysql.%              | NO      | NO    |
| EVENT       | performance_schema.% | NO      | NO    |
| EVENT       | information_schema.% | NO      | NO    |
| FUNCTION    | mysql.%              | NO      | NO    |
| FUNCTION    | performance_schema.% | NO      | NO    |
| FUNCTION    | information_schema.% | NO      | NO    |
| PROCEDURE   | mysql.%              | NO      | NO    |
| PROCEDURE   | performance_schema.% | NO      | NO    |
| PROCEDURE   | information_schema.% | NO      | NO    |
| TABLE       | mysql.%              | NO      | NO    |
| TABLE       | performance_schema.% | NO      | NO    |
| TABLE       | information_schema.% | NO      | NO    |
| TRIGGER     | mysql.%              | NO      | NO    |
| TRIGGER     | performance_schema.% | NO      | NO    |
| TRIGGER     | information_schema.% | NO      | NO    |
+-------------+----------------------+---------+-------+
15 rows in set (0.00 sec)

+----------------------------------+
| disabled_consumers               |
+----------------------------------+
| events_stages_current            |
| events_stages_history            |
| events_stages_history_long       |
| events_statements_history        |
| events_statements_history_long   |
| events_transactions_history      |
| events_transactions_history_long |
| events_waits_current             |
| events_waits_history             |
| events_waits_history_long        |
+----------------------------------+
10 rows in set (0.00 sec)

Empty set (0.00 sec)
             
+---------------------------------------------------------------------------------------+-------+
| disabled_instruments                                                                  | timed |
+---------------------------------------------------------------------------------------+-------+
| wait/synch/mutex/sql/TC_LOG_MMAP::LOCK_tc                                             | NO    |
| wait/synch/mutex/sql/LOCK_des_key_file                                                | NO    |
| wait/synch/mutex/sql/MYSQL_BIN_LOG::LOCK_commit                                       | NO    |
...
| memory/sql/servers_cache                                                              | NO    |
| memory/sql/udf_mem                                                                    | NO    |
| wait/lock/metadata/sql/mdl                                                            | NO    |
+---------------------------------------------------------------------------------------+-------+
547 rows in set (0.00 sec)

Query OK, 0 rows affected (0.01 sec)
```

#### ps_setup_show_disabled_consumers

##### Description

Shows all currently disabled consumers.

##### Parameters

None
 
##### Example

```SQL
mysql> CALL sys.ps_setup_show_disabled_consumers();

+---------------------------+
| disabled_consumers        |
+---------------------------+
| events_statements_current |
| global_instrumentation    |
| thread_instrumentation    |
| statements_digest         |
+---------------------------+
4 rows in set (0.05 sec)
```

#### ps_setup_show_disabled_instruments

##### Description

Shows all currently disabled instruments.

##### Parameters

None
			 
##### Example

```SQL
mysql> CALL sys.ps_setup_show_disabled_instruments();
```

#### ps_setup_show_enabled

##### Description

Shows all currently enabled Performance Schema configuration.

##### Parameters

* in_show_instruments (BOOLEAN): Whether to print enabled instruments (can print many items)
* in_show_threads (BOOLEAN): Whether to print enabled threads

##### Example
```SQL
mysql> CALL sys.ps_setup_show_enabled(TRUE, TRUE);
+----------------------------+
| performance_schema_enabled |
+----------------------------+
|                          1 |
+----------------------------+
1 row in set (0.00 sec)

+---------------+
| enabled_users |
+---------------+
| '%'@'%'       |
+---------------+
1 row in set (0.01 sec)

+----------------------+---------+-------+
| objects              | enabled | timed |
+----------------------+---------+-------+
| mysql.%              | NO      | NO    |
| performance_schema.% | NO      | NO    |
| information_schema.% | NO      | NO    |
| %.%                  | YES     | YES   |
+----------------------+---------+-------+
4 rows in set (0.01 sec)

+---------------------------+
| enabled_consumers         |
+---------------------------+
| events_statements_current |
| global_instrumentation    |
| thread_instrumentation    |
| statements_digest         |
+---------------------------+
4 rows in set (0.05 sec)

+--------------------------+-------------+
| enabled_threads          | thread_type |
+--------------------------+-------------+
| innodb/srv_master_thread | BACKGROUND  |
| root@localhost           | FOREGROUND  |
| root@localhost           | FOREGROUND  |
| root@localhost           | FOREGROUND  |
| root@localhost           | FOREGROUND  |
+--------------------------+-------------+
5 rows in set (0.03 sec)

+-------------------------------------+-------+
| enabled_instruments                 | timed |
+-------------------------------------+-------+
| wait/io/file/sql/map                | YES   |
| wait/io/file/sql/binlog             | YES   |
...
| statement/com/Error                 | YES   |
| statement/com/                      | YES   |
| idle                                | YES   |
+-------------------------------------+-------+
210 rows in set (0.08 sec)

Query OK, 0 rows affected (0.89 sec)
```

#### ps_setup_show_enabled_consumers

##### Description

Shows all currently enabled consumers.

##### Parameters

None

##### Example

```SQL
mysql> CALL sys.ps_setup_show_enabled_consumers();

+---------------------------+
| enabled_consumers         |
+---------------------------+
| events_statements_current |
| global_instrumentation    |
| thread_instrumentation    |
| statements_digest         |
+---------------------------+
4 rows in set (0.05 sec)
```

#### ps_setup_show_enabled_instruments

##### Description

Shows all currently enabled instruments.

##### Parameters

None

##### Example

```SQL
mysql> CALL sys.ps_setup_show_enabled_instruments();
```

#### ps_statement_avg_latency_histogram

##### Description

Outputs a textual histogram graph of the average latency values across all normalized queries tracked within the Performance Schema events_statements_summary_by_digest table.

Can be used to show a very high level picture of what kind of latency distribution statements running within this instance have.

##### Parameters

None.

##### Example
```SQL
mysql> CALL sys.ps_statement_avg_latency_histogram()G
*************************** 1. row ***************************
Performance Schema Statement Digest Average Latency Histogram:

  . = 1 unit
  * = 2 units
  # = 3 units

(0 - 38ms)     240 | ################################################################################
(38 - 77ms)    38  | ......................................
(77 - 115ms)   3   | ...
(115 - 154ms)  62  | *******************************
(154 - 192ms)  3   | ...
(192 - 231ms)  0   |
(231 - 269ms)  0   |
(269 - 307ms)  0   |
(307 - 346ms)  0   |
(346 - 384ms)  1   | .
(384 - 423ms)  1   | .
(423 - 461ms)  0   |
(461 - 499ms)  0   |
(499 - 538ms)  0   |
(538 - 576ms)  0   |
(576 - 615ms)  1   | .

  Total Statements: 350; Buckets: 16; Bucket Size: 38 ms;
```

#### ps_trace_statement_digest

##### Description

Traces all instrumentation within Performance Schema for a specific Statement Digest. 

When finding a statement of interest within the performance_schema.events_statements_summary_by_digest table, feed the DIGEST MD5 value in to this procedure, set how long to poll for, and at what interval to poll, and it will generate a report of all statistics tracked within Performance Schema for that digest for the interval.

It will also attempt to generate an EXPLAIN for the longest running example of the digest during the interval.

Note this may fail, as Performance Schema truncates long SQL_TEXT values (and hence the EXPLAIN will fail due to parse errors).

##### Parameters

* in_digest VARCHAR(32): The statement digest identifier you would like to analyze
* in_runtime (INT): The number of seconds to run analysis for (defaults to a minute)
* in_interval (DECIMAL(2,2)): The interval (in seconds, may be fractional) at which to try and take snapshots (defaults to a second)
* in_start_fresh (BOOLEAN): Whether to TRUNCATE the events_statements_history_long and events_stages_history_long tables before starting (default false)
* in_auto_enable (BOOLEAN): Whether to automatically turn on required consumers (default false)

##### Example
```SQL
mysql> call ps_analyze_statement_digest('891ec6860f98ba46d89dd20b0c03652c', 10, 0.1, true, true);
+--------------------+
| SUMMARY STATISTICS |
+--------------------+
| SUMMARY STATISTICS |
+--------------------+
1 row in set (9.11 sec)

+------------+-----------+-----------+-----------+---------------+------------+------------+
| executions | exec_time | lock_time | rows_sent | rows_examined | tmp_tables | full_scans |
+------------+-----------+-----------+-----------+---------------+------------+------------+
|         21 | 4.11 ms   | 2.00 ms   |         0 |            21 |          0 |          0 |
+------------+-----------+-----------+-----------+---------------+------------+------------+
1 row in set (9.11 sec)

+------------------------------------------+-------+-----------+
| event_name                               | count | latency   |
+------------------------------------------+-------+-----------+
| stage/sql/checking query cache for query |    16 | 724.37 µs |
| stage/sql/statistics                     |    16 | 546.92 µs |
| stage/sql/freeing items                  |    18 | 520.11 µs |
| stage/sql/init                           |    51 | 466.80 µs |
...
| stage/sql/cleaning up                    |    18 | 11.92 µs  |
| stage/sql/executing                      |    16 | 6.95 µs   |
+------------------------------------------+-------+-----------+
17 rows in set (9.12 sec)

+---------------------------+
| LONGEST RUNNING STATEMENT |
+---------------------------+
| LONGEST RUNNING STATEMENT |
+---------------------------+
1 row in set (9.16 sec)
             
+-----------+-----------+-----------+-----------+---------------+------------+-----------+
| thread_id | exec_time | lock_time | rows_sent | rows_examined | tmp_tables | full_scan |
+-----------+-----------+-----------+-----------+---------------+------------+-----------+
|    166646 | 618.43 µs | 1.00 ms   |         0 |             1 |          0 |         0 |
+-----------+-----------+-----------+-----------+---------------+------------+-----------+
1 row in set (9.16 sec)

// Truncated for clarity...
+-----------------------------------------------------------------+
| sql_text                                                        |
+-----------------------------------------------------------------+
| select hibeventhe0_.id as id1382_, hibeventhe0_.createdTime ... |
+-----------------------------------------------------------------+
1 row in set (9.17 sec)

+------------------------------------------+-----------+
| event_name                               | latency   |
+------------------------------------------+-----------+
| stage/sql/init                           | 8.61 µs   |
| stage/sql/Waiting for query cache lock   | 453.23 µs |
| stage/sql/init                           | 331.07 ns |
| stage/sql/checking query cache for query | 43.04 µs  |
...
| stage/sql/freeing items                  | 30.46 µs  |
| stage/sql/cleaning up                    | 662.13 ns |
+------------------------------------------+-----------+
             18 rows in set (9.23 sec)

+----+-------------+--------------+-------+---------------+-----------+---------+-------------+------+-------+
| id | select_type | table        | type  | possible_keys | key       | key_len | ref         | rows | Extra |
+----+-------------+--------------+-------+---------------+-----------+---------+-------------+------+-------+
|  1 | SIMPLE      | hibeventhe0_ | const | fixedTime     | fixedTime | 775     | const,const |    1 | NULL  |
+----+-------------+--------------+-------+---------------+-----------+---------+-------------+------+-------+
1 row in set (9.27 sec)

Query OK, 0 rows affected (9.28 sec)
```

#### ps_trace_thread

##### Description

Dumps all data within Performance Schema for an instrumented thread, to create a DOT formatted graph file. 

Each resultset returned from the procedure should be used for a complete graph

##### Parameters

* in_thread_id (INT): The thread that you would like a stack trace for
* in_outfile  (VARCHAR(255)): The filename the dot file will be written to
* in_max_runtime (DECIMAL(20,2)): The maximum time to keep collecting data. Use NULL to get the default which is 60 seconds.
* in_interval (DECIMAL(20,2)): How long to sleep between data collections. Use NULL to get the default which is 1 second.
* in_start_fresh (BOOLEAN): Whether to reset all Performance Schema data before tracing.
* in_auto_setup (BOOLEAN): Whether to disable all other threads and enable all consumers/instruments. This will also reset the settings at the end of the run.
* in_debug (BOOLEAN): Whether you would like to include file:lineno in the graph

##### Example
```SQL
mysql> CALL sys.ps_dump_thread_stack(25, CONCAT('/tmp/stack-', REPLACE(NOW(), ' ', '-'), '.dot'), NULL, NULL, TRUE, TRUE, TRUE);
+-------------------+
| summary           |
+-------------------+
| Disabled 1 thread |
+-------------------+
1 row in set (0.00 sec)

+---------------------------------------------+
| Info                                        |
+---------------------------------------------+
| Data collection starting for THREAD_ID = 25 |
+---------------------------------------------+
1 row in set (0.03 sec)

+-----------------------------------------------------------+
| Info                                                      |
+-----------------------------------------------------------+
| Stack trace written to /tmp/stack-2014-02-16-21:18:41.dot |
+-----------------------------------------------------------+
1 row in set (60.07 sec)

+-------------------------------------------------------------------+
| Convert to PDF                                                    |
+-------------------------------------------------------------------+
| dot -Tpdf -o /tmp/stack_25.pdf /tmp/stack-2014-02-16-21:18:41.dot |
+-------------------------------------------------------------------+
1 row in set (60.07 sec)

+-------------------------------------------------------------------+
| Convert to PNG                                                    |
+-------------------------------------------------------------------+
| dot -Tpng -o /tmp/stack_25.png /tmp/stack-2014-02-16-21:18:41.dot |
+-------------------------------------------------------------------+
1 row in set (60.07 sec)

+------------------+
| summary          |
+------------------+
| Enabled 1 thread |
+------------------+
1 row in set (60.32 sec)
```

#### ps_truncate_all_tables

##### Description

Truncates all summary tables within Performance Schema, resetting all aggregated instrumentation as a snapshot.

Requires the SUPER privilege for "SET sql_log_bin = 0;".

##### Parameters

* in_verbose (BOOLEAN): Whether to print each TRUNCATE statement before running

##### Example
```SQL
mysql> CALL sys.ps_truncate_all_tables(false);
+---------------------+
| summary             |
+---------------------+
| Truncated 44 tables |
+---------------------+
1 row in set (0.10 sec)
```
