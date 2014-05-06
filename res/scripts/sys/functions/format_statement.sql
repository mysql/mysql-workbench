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

DROP FUNCTION IF EXISTS format_statement;

DELIMITER $$

CREATE DEFINER='root'@'localhost' FUNCTION format_statement (
        statement LONGTEXT
    )
    RETURNS VARCHAR(65)
    COMMENT '
             Description
             -----------

             Formats a normalized statement, truncating it if it\'s > 64 characters long.

             Useful for printing statement related data from Performance Schema from 
             the command line.

             Parameters
             -----------

             statement (LONGTEXT): 
               The statement to format.

             Returns
             -----------

             VARCHAR(65)

             Example
             -----------

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
            '
    SQL SECURITY INVOKER
    DETERMINISTIC
    NO SQL
BEGIN
  IF LENGTH(statement) > 64 THEN 
      RETURN REPLACE(CONCAT(LEFT(statement, 30), ' ... ', RIGHT(statement, 30)), '\n', ' ');
  ELSE 
      RETURN REPLACE(statement, '\n', ' ');
  END IF;
END $$

DELIMITER ;
