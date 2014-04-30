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

DROP FUNCTION IF EXISTS format_time;

DELIMITER $$

CREATE DEFINER='root'@'localhost' FUNCTION format_time (
        picoseconds BIGINT UNSIGNED
    )
    RETURNS VARCHAR(16) CHARSET UTF8
    COMMENT '
             Description
             -----------

             Takes a raw picoseconds value, and converts it to a human readable form.
             
             Picoseconds are the precision that all latency values are printed in 
             within Performance Schema, however are not user friendly when wanting
             to scan output from the command line.

             Parameters
             -----------

             picoseconds (BIGINT UNSIGNED): 
               The raw picoseconds value to convert.

             Returns
             -----------

             VARCHAR(16) CHARSET UTF8

             Example
             -----------

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
            '
    SQL SECURITY INVOKER
    DETERMINISTIC
    NO SQL
BEGIN
  IF picoseconds IS NULL THEN RETURN NULL;
  ELSEIF picoseconds >= 3600000000000000 THEN RETURN CONCAT(ROUND(picoseconds / 3600000000000000, 2), 'h');
  ELSEIF picoseconds >= 60000000000000 THEN RETURN SEC_TO_TIME(ROUND(picoseconds / 1000000000000, 2));
  ELSEIF picoseconds >= 1000000000000 THEN RETURN CONCAT(ROUND(picoseconds / 1000000000000, 2), ' s');
  ELSEIF picoseconds >= 1000000000 THEN RETURN CONCAT(ROUND(picoseconds / 1000000000, 2), ' ms');
  ELSEIF picoseconds >= 1000000 THEN RETURN CONCAT(ROUND(picoseconds / 1000000, 2), ' us');
  ELSEIF picoseconds >= 1000 THEN RETURN CONCAT(ROUND(picoseconds / 1000, 2), ' ns');
  ELSE RETURN CONCAT(picoseconds, ' ps');
  END IF;
END $$

DELIMITER ;
