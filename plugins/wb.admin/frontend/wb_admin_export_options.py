# Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is also distributed with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms, as
# designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have included with MySQL.
# This program is distributed in the hope that it will be useful,  but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

export_options = {
#     "Tables":{
#     "no-create-info":["Do not write CREATE TABLE statements that re-create each dumped table.","FALSE"],
#     "skip-triggers":["Do not dump triggers", "FALSE"]
#     },
#    "Databases":{
#    "add-drop-database":["Add a DROP DATABASE statement before each CREATE DATABASE statement.","FALSE"],
#    "no-create-db":["This option suppresses the CREATE DATABASE statements.","FALSE"]
#    },
    "Inserts":{
    "extended-insert":["Use multiple-row INSERT syntax that include several VALUES lists.","TRUE"],
    "delayed-insert":["Write INSERT DELAYED statements rather than INSERT statements.","FALSE", "BOOL", ("5.0.0", "5.6.6")],
    "add-locks":["Surround each table dump with LOCK TABLES and UNLOCK TABLES statements.","TRUE"],
    "replace":["Write REPLACE statements rather than INSERT statements.","FALSE"],
    "insert-ignore":["Write INSERT IGNORE statements rather than INSERT statements.","FALSE"],
    "complete-insert":["Use complete INSERT statements that include column names.","FALSE"]
    },
    "SQL":{
    "create-options":["Include all MySQL-specific table options in CREATE TABLE statements.","TRUE"],
    "quote-names":["Quote identifiers within backtick characters.","TRUE"],
    "allow-keywords":["Allow creation of column names that are keywords.","FALSE"]
    },
    "Other":{
#    "routines":["Dump stored routines (procedures and functions) from the dumped databases.","FALSE"],
    "compress":["Use compression in server/client protocol.","FALSE"],
    "delete-master-logs":["On a master replication server, delete the binary logs after performing the dump operation.","FALSE"],
    "disable-keys":["For each table, surround the INSERT statements with statements to disable and enable keys.","TRUE"],
    "lock-tables":["Lock tables for read. Disable if user has no LOCK TABLES privilege.", "TRUE"],
#    "events":["Dump events from the dumped databases.","FALSE"],
    "flush-logs":["Flush the MySQL server log files before starting the dump.","FALSE"],
    "flush-privileges":["Emit a FLUSH PRIVILEGES statement after dumping the mysql database.","FALSE"],
    "force":["Continue even if we get an sql-error.","FALSE"],
    "hex-blob":["Dump binary columns using hexadecimal notation (for example, 'abc' becomes 0x616263).","FALSE"],
    #"no-data":["Do not dump table contents.","FALSE"],
    "order-by-primary":["Dump each table's rows sorted by its primary key, or by its first unique index.","FALSE"],
    "dump-date":["Include dump date as \"Dump completed on\" comment if --comments is given.","TRUE"],
    "Show Internal Schemas":["Show internal MySQL schemas (mysql, information_schema, performance_schema) in the export schema list.","FALSE"],
    "tz-utc":["Add SET TIME_ZONE='+00:00' to the dump file.","TRUE"],
#    "xml":["Produce XML output.","FALSE"]
    "set-gtid-purged":["Add 'SET @@GLOBAL.GTID_PURGED' to the output.","AUTO","STR",("5.6.9", None)],
    "column-statistics":["Writing ANALYZE TABLE statements to generate statistics histograms.", "FALSE", "BOOL", ("8.0.2", None)]
    }

    }
